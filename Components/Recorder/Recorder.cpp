// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#include <unistd.h>

#include <algorithm>
#include <thread>
// #include <filesystem>

#include <Compression.h>
#include <TFile.h>
#include <TROOT.h>
#include <TSystem.h>

#include "../../TDigiTES/include/TreeData.h"
#include "Recorder.h"

using DAQMW::FatalType::DATAPATH_DISCONNECTED;
using DAQMW::FatalType::FOOTER_DATA_MISMATCH;
using DAQMW::FatalType::HEADER_DATA_MISMATCH;
using DAQMW::FatalType::INPORT_ERROR;
using DAQMW::FatalType::USER_DEFINED_ERROR1;

// Module specification
// Change following items to suit your component's spec.
static const char *recorder_spec[] = {"implementation_id",
                                      "Recorder",
                                      "type_name",
                                      "Recorder",
                                      "description",
                                      "Recorder component",
                                      "version",
                                      "1.0",
                                      "vendor",
                                      "Kazuo Nakayoshi, KEK",
                                      "category",
                                      "example",
                                      "activity_type",
                                      "DataFlowComponent",
                                      "max_instance",
                                      "1",
                                      "language",
                                      "C++",
                                      "lang_type",
                                      "compile",
                                      ""};

Recorder::Recorder(RTC::Manager *manager)
    : DAQMW::DaqComponentBase(manager),
      m_InPort("recorder_in", m_in_data),
      m_in_status(BUF_SUCCESS),

      m_debug(false)
{
  ROOT::EnableImplicitMT();

  // Registration: InPort/OutPort/Service

  // Set InPort buffers
  registerInPort("recorder_in", m_InPort);

  init_command_port();
  init_state_table();
  set_comp_name("RECORDER");

  fLastSave = time(nullptr);
  // fSaveInterval = 2 * 60 * 60; // 2 hours
  fSaveInterval = 1 * 60 * 60;  // 1 hours
  // fSaveInterval = 1 * 60; // 1 min
  fSubRunNumber = 0;

  fDataSize = 0.;
  // fDataLimit = 1024. * 1024. * 1024. * 0.9;  // 900 MiB
  fDataLimit = 1024. * 1024. * 1024. * 1.;  // 1 GiB
  // fDataLimit = 1024. * 1024. * 1024. * 2.; // 2GiB
  // fDataLimit = 1024. * 1024. * 2.; // 2MiB

  ResetVec();

  fOutputDir = "/DAQ/Output";

  char hostName[128];
  gethostname(hostName, sizeof(hostName));
  fHostName = hostName;
}

Recorder::~Recorder() {}

RTC::ReturnCode_t Recorder::onInitialize()
{
  if (m_debug) {
    std::cerr << "Recorder::onInitialize()" << std::endl;
  }

  return RTC::RTC_OK;
}

RTC::ReturnCode_t Recorder::onExecute(RTC::UniqueId ec_id)
{
  daq_do();

  return RTC::RTC_OK;
}

int Recorder::daq_dummy() { return 0; }

int Recorder::daq_configure()
{
  std::cerr << "*** Recorder::configure" << std::endl;

  ::NVList *paramList;
  paramList = m_daq_service0.getCompParams();
  parse_params(paramList);

  ResetVec();

  // Unfortunately, this can not use filesystem class of C++17.
  // std::filesystem::exists(fOutputDir);

  // Using ROOT lib
  std::cout << fOutputDir << std::endl;
  if (gSystem->AccessPathName(fOutputDir)) {
    std::cerr << "There are no directory " << fOutputDir
              << "\nCheck the configulation." << std::endl;
    abort();
  }

  return 0;
}

int Recorder::parse_params(::NVList *list)
{
  std::cerr << "param list length:" << (*list).length() << std::endl;

  int len = (*list).length();
  for (int i = 0; i < len; i += 2) {
    std::string sname = (std::string)(*list)[i].value;
    std::string svalue = (std::string)(*list)[i + 1].value;

    std::cerr << "sname: " << sname << "  ";
    std::cerr << "value: " << svalue << std::endl;

    if (sname == "OutputDir") {
      fOutputDir = svalue;
    } else if (sname == "SaveInterval") {
      auto min = std::stoi(svalue);
      fSaveInterval = min * 60;
    }
  }

  return 0;
}

int Recorder::daq_unconfigure()
{
  std::cerr << "*** Recorder::unconfigure" << std::endl;

  return 0;
}

int Recorder::daq_start()
{
  std::cerr << "*** Recorder::start" << std::endl;

  m_in_status = BUF_SUCCESS;
  fRunNumber = get_run_number();
  fDataWriteFlag = false;
  if (fRunNumber >= 0 && fRunNumber < INT_MAX) fDataWriteFlag = true;
  fSubRunNumber = 0;
  fLastSave = time(nullptr);
  fDataSize = 0.;

  return 0;
}

int Recorder::daq_stop()
{
  std::cerr << "*** Recorder::stop" << std::endl;
  reset_InPort();

  EnqueueData();
  ResetVec();
  for (auto &th : fThreadVec) th.join();
  fThreadVec.clear();

  return 0;
}

int Recorder::daq_pause()
{
  std::cerr << "*** Recorder::pause" << std::endl;

  return 0;
}

int Recorder::daq_resume()
{
  std::cerr << "*** Recorder::resume" << std::endl;

  return 0;
}

int Recorder::reset_InPort()
{
  int ret = true;
  while (ret == true) {
    ret = m_InPort.read();
  }

  return 0;
}

unsigned int Recorder::read_InPort()
{
  /////////////// read data from InPort Buffer ///////////////
  unsigned int recv_byte_size = 0;
  bool ret = m_InPort.read();

  //////////////////// check read status /////////////////////
  if (ret == false) {  // false: TIMEOUT or FATAL
    m_in_status = check_inPort_status(m_InPort);
    if (m_in_status == BUF_TIMEOUT) {  // Buffer empty.
      if (check_trans_lock()) {        // Check if stop command has come.
        set_trans_unlock();            // Transit to CONFIGURE state.
      }
    } else if (m_in_status == BUF_FATAL) {  // Fatal error
      fatal_error_report(INPORT_ERROR);
    }
  } else {
    recv_byte_size = m_in_data.data.length();
  }

  if (m_debug) {
    std::cerr << "m_in_data.data.length():" << recv_byte_size << std::endl;
  }

  return recv_byte_size;
}

int Recorder::daq_run()
{
  if (m_debug) {
    std::cerr << "*** Recorder::run" << std::endl;
  }

  unsigned int recv_byte_size = read_InPort();
  if (recv_byte_size == 0) {  // Timeout
    return 0;
  }

  unsigned int event_byte_size = get_event_size(recv_byte_size);
  if (m_debug) {
    std::cout << "Size: " << event_byte_size << "\t"
              << "Sequence: " << get_sequence_num() << std::endl;
  }
  check_header_footer(m_in_data, recv_byte_size);  // check header and footer
  /////////////  Write component main logic here. /////////////
  // online_analyze();
  /////////////////////////////////////////////////////////////

  inc_sequence_num();                    // increase sequence num.
  inc_total_data_size(event_byte_size);  // increase total data byte size
  fDataSize += event_byte_size;

  if (fDataWriteFlag) {
    FillData(event_byte_size);
    auto now = time(nullptr);
    if ((now - fLastSave > fSaveInterval) || (fDataSize > fDataLimit)) {
      EnqueueData();
      ResetVec();
      fLastSave = now;
      fDataSize = 0.;
    }
  }

  return 0;
}

void Recorder::ResetVec()
{
  fpDataVec.reset(new std::vector<TreeData>);
  fpDataVec->reserve(2 * fDataLimit / sizeof(TreeData));
}

int Recorder::FillData(unsigned int dataSize)
{
  // std::cout << dataSize << std::endl;
  constexpr auto sizeMod = sizeof(TreeData::Mod);
  constexpr auto sizeCh = sizeof(TreeData::Ch);
  constexpr auto sizeTS = sizeof(TreeData::TimeStamp);
  constexpr auto sizeFineTS = sizeof(TreeData::FineTS);
  constexpr auto sizeChargeLong = sizeof(TreeData::ChargeLong);
  constexpr auto sizeChargeShort = sizeof(TreeData::ChargeShort);
  constexpr auto sizeRL = sizeof(TreeData::RecordLength);

  constexpr unsigned int headerSize = 8;

  int nHits = 0;
  for (unsigned int i = headerSize; i < dataSize + headerSize;) {
    TreeData data;

    // The order of data should be the same as Reader
    memcpy(&(data.Mod), &m_in_data.data[i], sizeMod);
    i += sizeMod;

    memcpy(&(data.Ch), &m_in_data.data[i], sizeCh);
    i += sizeCh;

    memcpy(&(data.TimeStamp), &m_in_data.data[i], sizeTS);
    i += sizeTS;

    memcpy(&(data.FineTS), &m_in_data.data[i], sizeFineTS);
    i += sizeFineTS;

    memcpy(&(data.ChargeLong), &m_in_data.data[i], sizeChargeLong);
    i += sizeChargeLong;

    memcpy(&(data.ChargeShort), &m_in_data.data[i], sizeChargeShort);
    i += sizeChargeShort;

    memcpy(&(data.RecordLength), &m_in_data.data[i], sizeRL);
    i += sizeRL;

    if (data.RecordLength > 0) {
      auto sizeTrace = sizeof(TreeData::Trace1[0]) * data.RecordLength;
      data.Trace1.resize(data.RecordLength);
      memcpy(&data.Trace1[0], &m_in_data.data[i], sizeTrace);
      i += sizeTrace;
    }

    fpDataVec->push_back(data);
    nHits++;
  }

  return nHits;
}

void Recorder::EnqueueData()
{
  // fThreadVec.push_back(
  // std::thread(&Recorder::MakeTreeAndFile, this, fpDataVec.release()));
  std::thread(&Recorder::MakeTreeAndFile, this, fpDataVec.release()).detach();
  ResetVec();
}

#include <parallel/algorithm>
void Recorder::MakeTreeAndFile(std::vector<TreeData> *data)
{
  if (fDataWriteFlag) {
    fMutex.lock();
    auto extention = "_" + fHostName + ".root";
    auto fileName =
        fOutputDir + Form("/run%d_%d", fRunNumber, fSubRunNumber) + extention;
    if (!gSystem->AccessPathName(fileName)) {
      // In the case of file already existing, adding UNIX time.
      fileName =
          fOutputDir +
          Form("/run%d_%d_%ld", fRunNumber, fSubRunNumber, time(nullptr)) +
          extention;
    }
    fSubRunNumber++;
    fMutex.unlock();

    auto file = new TFile(fileName, "NEW");
    file->SetCompressionLevel(ROOT::RCompressionSetting::ELevel::kUncompressed);

    auto tree = new TTree("DELILA_Tree", "DELILA data");
    tree->SetDirectory(file);
    UChar_t Mod, Ch;
    ULong64_t TimeStamp;
    Double_t FineTS;
    UShort_t ChargeLong;
    UShort_t ChargeShort;
    UInt_t RecordLength;
    UShort_t Signal[100000]{0};
    tree->Branch("Mod", &Mod, "Mod/b");
    tree->Branch("Ch", &Ch, "Ch/b");
    tree->Branch("TimeStamp", &TimeStamp, "TimeStamp/l");
    tree->Branch("FineTS", &FineTS, "Finets/D");
    tree->Branch("ChargeLong", &ChargeLong, "ChargeLong/s");
    tree->Branch("ChargeShort", &ChargeShort, "ChargeShort/s");
    tree->Branch("RecordLength", &RecordLength, "RecordLength/i");
    tree->Branch("Signal", Signal, "Signal[RecordLength]/s");

    __gnu_parallel::sort(data->begin(), data->end(),
                         [](const TreeData &a, const TreeData &b) {
                           return a.FineTS < b.FineTS;
                         });

    for (auto iEve = 0; iEve < data->size(); iEve++) {
      Mod = data->at(iEve).Mod;
      Ch = data->at(iEve).Ch;
      TimeStamp = data->at(iEve).TimeStamp;
      FineTS = data->at(iEve).FineTS;
      ChargeLong = data->at(iEve).ChargeLong;
      ChargeShort = data->at(iEve).ChargeShort;
      RecordLength = data->at(iEve).RecordLength;
      if (RecordLength > 0)
        std::copy(&data->at(iEve).Trace1[0],
                  &data->at(iEve).Trace1[RecordLength], Signal);

      tree->Fill();
    }

    file->Write();
    file->Close();
    delete file;
  }

  delete data;
}

extern "C" {
void RecorderInit(RTC::Manager *manager)
{
  RTC::Properties profile(recorder_spec);
  manager->registerFactory(profile, RTC::Create<Recorder>,
                           RTC::Delete<Recorder>);
}
};
