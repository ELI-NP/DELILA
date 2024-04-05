// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#include "ReaderPHA.h"

#include <curl/curl.h>

#include <fstream>
#include <nlohmann/json.hpp>

using DAQMW::FatalType::DATAPATH_DISCONNECTED;
using DAQMW::FatalType::OUTPORT_ERROR;
using DAQMW::FatalType::USER_DEFINED_ERROR1;

// Module specification
// Change following items to suit your component's spec.
static const char *reader_spec[] = {"implementation_id",
                                    "ReaderPHA",
                                    "type_name",
                                    "ReaderPHA",
                                    "description",
                                    "ReaderPHA component",
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

// For curl
size_t CallbackWrite(char *ptr, size_t size, size_t nmemb, std::string *stream)
{
  int dataLength = size * nmemb;
  stream->append(ptr, dataLength);
  return dataLength;
}

ReaderPHA::ReaderPHA(RTC::Manager *manager)
    : DAQMW::DaqComponentBase(manager),
      m_OutPort("reader_out", m_out_data),
      m_recv_byte_size(0),
      m_out_status(BUF_SUCCESS),

      m_debug(false)
{
  // Registration: InPort/OutPort/Service

  // Set OutPort buffers
  registerOutPort("reader_es1_out", m_OutPort);

  init_command_port();
  init_state_table();
  set_comp_name("READER");

  // fDigitizer.reset(new TPHA);

  fData = new unsigned char[1024 * 1024 * 16];

  fConfigFile = "/DAQ/PHA.conf";
  fParameterAPI = "";

  fFlagSWTrg = false;
  fNSWTrg = 0;
}

ReaderPHA::~ReaderPHA() {}

RTC::ReturnCode_t ReaderPHA::onInitialize()
{
  if (m_debug) {
    std::cerr << "ReaderPHA::onInitialize()" << std::endl;
  }

  return RTC::RTC_OK;
}

RTC::ReturnCode_t ReaderPHA::onExecute(RTC::UniqueId ec_id)
{
  daq_do();

  return RTC::RTC_OK;
}

int ReaderPHA::daq_dummy() { return 0; }

int ReaderPHA::daq_configure()
{
  std::cerr << "*** ReaderPHA::configure" << std::endl;

  ::NVList *paramList;
  paramList = m_daq_service0.getCompParams();
  parse_params(paramList);

  fDigitizer.reset(new TPHA);
  fDigitizer->LoadParameters(fConfigFile);
  fDigitizer->OpenDigitizers();
  fDigitizer->InitDigitizers();
  fDigitizer->UseHWFineTS();
  fDigitizer->AllocateMemory();

  return 0;
}

int ReaderPHA::parse_params(::NVList *list)
{
  std::cerr << "param list length:" << (*list).length() << std::endl;

  int len = (*list).length();
  for (int i = 0; i < len; i += 2) {
    std::string sname = (std::string)(*list)[i].value;
    std::string svalue = (std::string)(*list)[i + 1].value;

    std::cerr << "sname: " << sname << "  ";
    std::cerr << "value: " << svalue << std::endl;

    if (sname == "ConfigFile") {
      fConfigFile = svalue;
    } else if (sname == "StartModNo") {
      fStartModNo = std::stoi(svalue);
    } else if (sname == "ParameterAPI") {
      fParameterAPI = svalue;
    } else if (sname == "SWTrigger") {
      fFlagSWTrg = true;
      fNSWTrg = std::stoi(svalue);
    }
  }

  return 0;
}

int ReaderPHA::daq_unconfigure()
{
  std::cerr << "*** ReaderPHA::unconfigure" << std::endl;
  fDigitizer->FreeMemory();
  fDigitizer->CloseDigitizers();

  return 0;
}

int ReaderPHA::daq_start()
{
  std::cerr << "*** ReaderPHA::start" << std::endl;

  m_out_status = BUF_SUCCESS;

  if (fParameterAPI != "") {
    auto curl = curl_easy_init();
    std::string buf;
    curl_easy_setopt(curl, CURLOPT_URL, fParameterAPI.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CallbackWrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    auto ret = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    // code=404 is only from my web server.  For others, using 404 as key is crazy...
    if (ret == CURLE_OK && (buf.find("code=404") == std::string::npos)) {
      auto json = nlohmann::json::parse(buf);
      auto par = fDigitizer->GetParameters();
      const auto nBrds = par.NumBrd;
      const auto nChs = par.NumPhyCh;

      for (auto iBrd = 0 + fStartModNo; iBrd < nBrds + fStartModNo; iBrd++) {
        for (auto iCh = 0; iCh < nChs; iCh++) {
          par.TrapPoleZero[iBrd][iCh] =
              json["PHAPar"][iBrd]["trapPoleZero"][iCh];
          par.TrapFlatTop[iBrd][iCh] = json["PHAPar"][iBrd]["trapFlatTop"][iCh];
          par.TrapRiseTime[iBrd][iCh] =
              json["PHAPar"][iBrd]["trapRiseTime"][iCh];
          par.PeakingTime[iBrd][iCh] = json["PHAPar"][iBrd]["peakingTime"][iCh];
          par.TTFsmoothing[iBrd][iCh] =
              json["PHAPar"][iBrd]["TTFSmoothing"][iCh];
          par.TTFdelay[iBrd][iCh] = json["PHAPar"][iBrd]["signalRiseTime"][iCh];
          par.TrgThreshold[iBrd][iCh] =
              json["PHAPar"][iBrd]["trgThreshold"][iCh];
          par.NsBaseline[iBrd][iCh] = json["PHAPar"][iBrd]["NSBaseline"][iCh];
          par.NSPeak[iBrd][iCh] = json["PHAPar"][iBrd]["NSPeak"][iCh];
          par.PeakHoldOff[iBrd][iCh] = json["PHAPar"][iBrd]["peakHoldOff"][iCh];
          // par.BaseLineHoldOff[iBrd][iCh] = json["PHAPar"][iBrd]["baselineHoldOff"][iCh];
          par.TrgHoldOff = json["PHAPar"][iBrd]["trgHoldOff"][iCh];
          // par.RTDWindow[iBrd][iCh] = json["PHAPar"][iBrd]["RTDWindow"][iCh];
          par.CoincWindow = json["PHAPar"][iBrd]["trgAccWindow"][iCh];
          // par.DigitalGain[iBrd][iCh] = json["PHAPar"][iBrd]["digitalGain"][iCh];
          par.EnergyFineGain[iBrd][iCh] =
              json["PHAPar"][iBrd]["eneFineGain"][iCh];
          par.Decimation[iBrd][iCh] = json["PHAPar"][iBrd]["decimation"][iCh];
        }
      }

      fDigitizer->SetParameters(par);
      fDigitizer->SetPHAPar();
    }
  }

  fDataBuffer.reset(new std::vector<char>);
  StartThreads();
  fDigitizer->Start();

  return 0;
}

int ReaderPHA::daq_stop()
{
  std::cerr << "*** ReaderPHA::stop" << std::endl;

  fDigitizer->Stop();
  StopThreads();

  return 0;
}

int ReaderPHA::daq_pause()
{
  std::cerr << "*** ReaderPHA::pause" << std::endl;

  return 0;
}

int ReaderPHA::daq_resume()
{
  std::cerr << "*** ReaderPHA::resume" << std::endl;

  return 0;
}

void ReaderPHA::StartThreads()
{
  fDataReadThreadFlag = true;
  fDataReadThread = std::thread(&ReaderPHA::DataReadThread, this);

  fDataProcessThreadFlag = true;
  fDataProcessThread = std::thread(&ReaderPHA::DataProcessThread, this);
}

void ReaderPHA::StopThreads()
{
  fDataReadThreadFlag = false;
  fDataProcessThreadFlag = false;

  fDataReadThread.join();
  fDataProcessThread.join();
}

void ReaderPHA::DataReadThread()
{
  while (fDataProcessThreadFlag) {
    fDigitizer->ReadEvents();
    auto data = fDigitizer->GetData();
    if (data->size() > 0) {
      std::lock_guard<std::mutex> lock(fDataMutex);
      fDataVec.push_back(std::move(data));
    }
    usleep(10);
  }
}

void ReaderPHA::DataProcessThread()
{
  constexpr auto sizeMod = sizeof(TreeData::Mod);
  constexpr auto sizeCh = sizeof(TreeData::Ch);
  constexpr auto sizeTS = sizeof(TreeData::TimeStamp);
  constexpr auto sizeFineTS = sizeof(TreeData::FineTS);
  constexpr auto sizeEne = sizeof(TreeData::ChargeLong);
  constexpr auto sizeShort = sizeof(TreeData::ChargeShort);
  constexpr auto sizeRL = sizeof(TreeData::RecordLength);

  while (fDataProcessThreadFlag) {
    auto dataSize = 0;
    {
      std::lock_guard<std::mutex> lock(fDataMutex);
      dataSize = fDataVec.size();
    }
    if (dataSize > 0) {
      std::vector<std::unique_ptr<std::vector<std::unique_ptr<TreeData_t>>>>
          dataVec;
      {
        std::lock_guard<std::mutex> lock(fDataMutex);
        dataVec = std::move(fDataVec);
        fDataVec.clear();
      }

      for (auto i = 0; i < dataVec.size(); i++) {
        for (auto j = 0; j < dataVec[i]->size(); j++) {
          auto data = std::move(dataVec[i]->at(j));
          const auto oneHitSize =
              sizeMod + sizeCh + sizeTS + sizeFineTS + sizeEne + sizeShort +
              sizeRL + (sizeof(TreeData::Trace1[0]) * data->RecordLength);
          auto index = 0;
          std::vector<char> hit;
          hit.resize(oneHitSize);

          TreeData dummy;
          dummy.Mod = data->Mod + fStartModNo;
          memcpy(&hit[index], &(dummy.Mod), sizeMod);
          index += sizeMod;

          memcpy(&hit[index], &(data->Ch), sizeCh);
          index += sizeCh;

          memcpy(&hit[index], &(data->TimeStamp), sizeTS);
          index += sizeTS;

          memcpy(&hit[index], &(data->FineTS), sizeFineTS);
          index += sizeFineTS;

          memcpy(&hit[index], &(data->ChargeLong), sizeEne);
          index += sizeEne;

          dummy.ChargeShort = 0;
          memcpy(&hit[index], &(dummy.ChargeShort), sizeShort);
          index += sizeShort;

          memcpy(&hit[index], &(data->RecordLength), sizeRL);
          index += sizeRL;

          const auto sizeTrace =
              sizeof(TreeData::Trace1[0]) * data->RecordLength;
          memcpy(&hit[index], &(data->Trace1[0]), sizeTrace);

          fDataMutex.lock();
          fDataBuffer->insert(fDataBuffer->end(), hit.begin(), hit.end());
          fDataMutex.unlock();
        }
      }
    } else {
      // std::cout <<"no data"<< std::endl;
    }
    usleep(10);
  }
}

int ReaderPHA::set_data()
{
  if (m_debug) {
    std::cerr << "*** ReaderPHA::set_data" << std::endl;
  }
    
  unsigned char header[8];
  unsigned char footer[8];

  std::vector<char> *dataBuffer = nullptr;
  {
    std::lock_guard<std::mutex> lock(fDataMutex);
    if(fDataBuffer->size() > 0) {
      dataBuffer = fDataBuffer.release();
      fDataBuffer.reset(new std::vector<char>);
    }
  }
  if(dataBuffer == nullptr) return 0;
  auto size = dataBuffer->size();

  if(size > 0) {
  set_header(&header[0], size);
  set_footer(&footer[0]);

  /// set OutPort buffer length
  m_out_data.data.length(size + HEADER_BYTE_SIZE + FOOTER_BYTE_SIZE);
  memcpy(&(m_out_data.data[0]), &header[0], HEADER_BYTE_SIZE);
  memcpy(&(m_out_data.data[HEADER_BYTE_SIZE]), &(dataBuffer->at(0)), size);
  memcpy(&(m_out_data.data[HEADER_BYTE_SIZE + size]), &footer[0],
         FOOTER_BYTE_SIZE);
  }
  delete dataBuffer;
  return size;
}

int ReaderPHA::write_OutPort()
{
  ////////////////// send data from OutPort  //////////////////
  bool ret = m_OutPort.write();

  //////////////////// check write status /////////////////////
  if (ret == false) {  // TIMEOUT or FATAL
    m_out_status = check_outPort_status(m_OutPort);
    if (m_out_status == BUF_FATAL) {  // Fatal error
      fatal_error_report(OUTPORT_ERROR);
    }
    if (m_out_status == BUF_TIMEOUT) {  // Timeout
      return -1;
    }
  } else {
    m_out_status = BUF_SUCCESS;  // successfully done
  }

  return 0;
}

int ReaderPHA::daq_run()
{
  if (m_debug) {
    std::cerr << "*** ReaderPHA::run" << std::endl;
  }

  if (check_trans_lock()) {  // check if stop command has come
    set_trans_unlock();      // transit to CONFIGURED state
    return 0;
  }

  if (fFlagSWTrg) {
    for (auto i = 0; i < fNSWTrg; i++) fDigitizer->SendSWTrigger();
  }

  int sentDataSize = 0;
  if (m_out_status ==
      BUF_SUCCESS) {            // previous OutPort.write() successfully done
    sentDataSize = set_data();  // set data to OutPort Buffer
  }

  if(sentDataSize > 0) {
    // std::cout <<"Write port"<< std::endl;
    if (write_OutPort() < 0) {
      ;                                   // Timeout. do nothing.
    } else {                              // OutPort write successfully done
      inc_sequence_num();                 // increase sequence num.
      inc_total_data_size(sentDataSize);  // increase total data byte size
    }
  }

  return 0;
}

extern "C" {
void ReaderPHAInit(RTC::Manager *manager)
{
  RTC::Properties profile(reader_spec);
  manager->registerFactory(profile, RTC::Create<ReaderPHA>,
                           RTC::Delete<ReaderPHA>);
}
};
