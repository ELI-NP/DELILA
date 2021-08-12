// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#include <algorithm>
#include <thread>
// #include <filesystem>

#include <TSystem.h>
#include <TFile.h>

#include "../../TDigiTES/include/TPHAData.hpp"

#include "Recorder.h"

using DAQMW::FatalType::DATAPATH_DISCONNECTED;
using DAQMW::FatalType::INPORT_ERROR;
using DAQMW::FatalType::HEADER_DATA_MISMATCH;
using DAQMW::FatalType::FOOTER_DATA_MISMATCH;
using DAQMW::FatalType::USER_DEFINED_ERROR1;

// Module specification
// Change following items to suit your component's spec.
static const char* recorder_spec[] =
  {
   "implementation_id", "Recorder",
   "type_name",         "Recorder",
   "description",       "Recorder component",
   "version",           "1.0",
   "vendor",            "Kazuo Nakayoshi, KEK",
   "category",          "example",
   "activity_type",     "DataFlowComponent",
   "max_instance",      "1",
   "language",          "C++",
   "lang_type",         "compile",
   ""
  };

Recorder::Recorder(RTC::Manager* manager)
  : DAQMW::DaqComponentBase(manager),
    m_InPort("recorder_in",   m_in_data),
    m_in_status(BUF_SUCCESS),

    m_debug(false)
{
  // Registration: InPort/OutPort/Service

  // Set InPort buffers
  registerInPort ("recorder_in",  m_InPort);

  init_command_port();
  init_state_table();
  set_comp_name("RECORDER");

  fLastSave = time(nullptr);
  // fSaveInterval = 2 * 60 * 60; // 2 hours
  fSaveInterval = 1 * 60 * 60; // 1 hours
  // fSaveInterval = 1 * 60; // 1 min
  fSubRunNumber = 0;

  fDataSize = 0.;
  fDataLimit = 1024. * 1024. * 1024. * 1.; // 1 GB
  // fDataLimit = 1024. * 1024. * 1024. * 2.; // 2GB
  // fDataLimit = 1024. * 1024. * 2.; // 2MB

  fDataVec.reserve(5000000);
  
  fOutputDir = "/DAQ/Output";
}

Recorder::~Recorder()
{
}

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

int Recorder::daq_dummy()
{
  return 0;
}

int Recorder::daq_configure()
{
  std::cerr << "*** Recorder::configure" << std::endl;

  ::NVList* paramList;
  paramList = m_daq_service0.getCompParams();
  parse_params(paramList);

  ResetVec();

  // Unfortunately, this can not use C++17 with filesystem
  // std::filesystem::exists(fOutputDir);

  // Using ROOT lib
  if(gSystem->AccessPathName(fOutputDir)) {
    std::cerr << "There are no directory " << fOutputDir
	      << "\nCheck the configulation." << std::endl;
    abort();
  }
  
  return 0;
}

int Recorder::parse_params(::NVList* list)
{

  std::cerr << "param list length:" << (*list).length() << std::endl;

  int len = (*list).length();
  for (int i = 0; i < len; i+=2) {
    std::string sname  = (std::string)(*list)[i].value;
    std::string svalue = (std::string)(*list)[i+1].value;

    std::cerr << "sname: " << sname << "  ";
    std::cerr << "value: " << svalue << std::endl;

    if(sname == "OutputDir") {
      fOutputDir = svalue;
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

  m_in_status  = BUF_SUCCESS;
  fSubRunNumber = 0;
  fLastSave = time(nullptr);
  fDataSize = 0.;
  
  return 0;
}

int Recorder::daq_stop()
{
  std::cerr << "*** Recorder::stop" << std::endl;
  reset_InPort();

  WriteData();
  ResetVec();

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
  while(ret == true) {
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
  if (ret == false) { // false: TIMEOUT or FATAL
    m_in_status = check_inPort_status(m_InPort);
    if (m_in_status == BUF_TIMEOUT) { // Buffer empty.
      if (check_trans_lock()) {     // Check if stop command has come.
	set_trans_unlock();       // Transit to CONFIGURE state.
      }
    }
    else if (m_in_status == BUF_FATAL) { // Fatal error
      fatal_error_report(INPORT_ERROR);
    }
  }
  else {
    recv_byte_size = m_in_data.data.length();
  }

  if (m_debug) {
    std::cerr << "m_in_data.data.length():" << recv_byte_size
	      << std::endl;
  }

  return recv_byte_size;
}

int Recorder::daq_run()
{
  if (m_debug) {
    std::cerr << "*** Recorder::run" << std::endl;
  }

  unsigned int recv_byte_size = read_InPort();
  if (recv_byte_size == 0) { // Timeout
    return 0;
  }

  check_header_footer(m_in_data, recv_byte_size); // check header and footer
  unsigned int event_byte_size = get_event_size(recv_byte_size);

  /////////////  Write component main logic here. /////////////
  // online_analyze();
  /////////////////////////////////////////////////////////////
  FillData(event_byte_size);
  
  inc_sequence_num();                       // increase sequence num.
  inc_total_data_size(event_byte_size);     // increase total data byte size
  fDataSize += event_byte_size;
  
  auto now = time(nullptr);
  if((now - fLastSave > fSaveInterval) ||
     (fDataSize > fDataLimit)){
    WriteData();
    ResetVec();    
    fLastSave = now;
    fDataSize = 0.;
  }
  
  return 0;
}

void Recorder::ResetVec()
{
  for(auto &&data: fDataVec) delete data;
  fDataVec.clear();
  /*  
  fTree.reset(new TTree("ELIADE_Tree", "ELIADE"));
  fTree->Branch("Mod", &fMod, "fMod/b");
  fTree->Branch("Ch", &fCh, "fCh/b");
  fTree->Branch("TimeStamp", &fTimeStamp, "fTimeStamp/l");
  fTree->Branch("Energy", &fEnergy, "fEnergy/s");
  fTree->Branch("RecordLength", &fRecordLength, "fRecordLength/i");
  fTree->Branch("Signal", fTrace1, "fTrace1[fRecordLength]/s");
  */
}

int Recorder::FillData(unsigned int dataSize)
{
  // std::cout << dataSize << std::endl;
  constexpr auto sizeMod = sizeof(TreeData::Mod);
  constexpr auto sizeCh = sizeof(TreeData::Ch);
  constexpr auto sizeTS = sizeof(TreeData::TimeStamp);
  constexpr auto sizeEne = sizeof(TreeData::Energy);
  constexpr auto sizeRL =  sizeof(TreeData::RecordLength);
  constexpr auto sizeEle = sizeof(*(TreeData::Trace1));

  constexpr unsigned int headerSize = 8;
  
  int nHits = 0;
  for(unsigned int i = headerSize; i < dataSize + headerSize;) {
    auto data = new TreeData;
    
    // The order of data should be the same as Reader
    memcpy(&(data->Mod), &m_in_data.data[i], sizeMod);
    i += sizeMod;

    memcpy(&(data->Ch), &m_in_data.data[i], sizeCh);
    i += sizeCh;

    memcpy(&(data->TimeStamp), &m_in_data.data[i], sizeTS);
    i += sizeTS;

    memcpy(&(data->Energy), &m_in_data.data[i], sizeEne);
    i += sizeEne;

    memcpy(&(data->RecordLength), &m_in_data.data[i], sizeRL);
    i += sizeRL;

    if(data->RecordLength > 0){
      auto sizeTrace = sizeof(*(PHAData::Trace1)) * data->RecordLength;
      data->Trace1 = new uint16_t[data->RecordLength];
      memcpy(data->Trace1, &m_in_data.data[i], sizeTrace);
      i += sizeTrace;
    }

    fDataVec.push_back(data);
    nHits++;
  }

  return nHits;
}

void Recorder::WriteData()
{
  std::thread t(&Recorder::WriteTree, this, fDataVec);
  t.detach();
  
  //WriteTree(fDataVec);
  
  fDataVec.clear();
}

void Recorder::WriteTree(std::vector<TreeData *> dataVec)
{
  std::sort(dataVec.begin(), dataVec.end(), [](const TreeData *a, const TreeData *b){
					      return a->TimeStamp < b->TimeStamp;
					    });

  auto runNumber = get_run_number();
  auto fileName = fOutputDir + Form("/run%d_%d.root", runNumber, fSubRunNumber);
  if(!gSystem->AccessPathName(fileName)){
    // In the case of file existing, adding UNIX time.
    fileName = fOutputDir + Form("/run%d_%d_%ld.root", runNumber, fSubRunNumber, time(nullptr));
  }
  
  auto file = new TFile(fileName, "NEW");
  auto tree = new TTree("ELIADE_Tree", "ELIADE data");

  UChar_t Mod, Ch;
  ULong64_t TimeStamp;
  UShort_t Energy;
  UInt_t RecordLength;
  UShort_t Signal[10000];
  tree->Branch("Mod", &Mod, "Mod/b");
  tree->Branch("Ch", &Ch, "Ch/b");
  tree->Branch("TimeStamp", &TimeStamp, "TimeStamp/l");
  tree->Branch("Energy", &Energy, "Energy/s");
  tree->Branch("RecordLength", &RecordLength, "RecordLength/i");
  tree->Branch("Signal", Signal, "Signal[RecordLength]/s");
   
  for(auto iEve = 0; iEve < dataVec.size(); iEve++) {
    auto data = dataVec[iEve];
    
    Mod = data->Mod;
    Ch = data->Ch;
    TimeStamp = data->TimeStamp;
    Energy = data->Energy;
    RecordLength = data->RecordLength;
    if(RecordLength > 0) std::copy(data->Trace1, data->Trace1 + RecordLength, Signal);
    
    tree->Fill();
  }
  
  tree->Write();
  file->Close();
  delete file;

  fSubRunNumber++;

  for(auto &&data: dataVec) delete data;
}


extern "C"
{
  void RecorderInit(RTC::Manager* manager)
  {
    RTC::Properties profile(recorder_spec);
    manager->registerFactory(profile,
			     RTC::Create<Recorder>,
			     RTC::Delete<Recorder>);
  }
};
