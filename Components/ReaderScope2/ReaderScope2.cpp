// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#include "ReaderScope2.h"

#include <byteswap.h>

using DAQMW::FatalType::DATAPATH_DISCONNECTED;
using DAQMW::FatalType::OUTPORT_ERROR;
using DAQMW::FatalType::USER_DEFINED_ERROR1;

// Module specification
// Change following items to suit your component's spec.
static const char *reader_spec[] = {"implementation_id",
                                    "ReaderScope2",
                                    "type_name",
                                    "ReaderScope2",
                                    "description",
                                    "ReaderScope2 component",
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

ReaderScope2::ReaderScope2(RTC::Manager *manager)
    : DAQMW::DaqComponentBase(manager),
      m_OutPort("reader_out", m_out_data),
      m_recv_byte_size(0),
      m_out_status(BUF_SUCCESS),

      m_debug(false)
{
  // Registration: InPort/OutPort/Service

  // Set OutPort buffers
  registerOutPort("reader_out", m_OutPort);

  init_command_port();
  init_state_table();
  set_comp_name("READER");

  fData = new unsigned char[1024 * 1024 * 1024];

  fConfigFile.clear();
}

ReaderScope2::~ReaderScope2() {}

RTC::ReturnCode_t ReaderScope2::onInitialize()
{
  if (m_debug) {
    std::cerr << "ReaderScope2::onInitialize()" << std::endl;
  }

  return RTC::RTC_OK;
}

RTC::ReturnCode_t ReaderScope2::onExecute(RTC::UniqueId ec_id)
{
  daq_do();

  return RTC::RTC_OK;
}

int ReaderScope2::daq_dummy() { return 0; }

int ReaderScope2::daq_configure()
{
  std::cerr << "*** ReaderScope2::configure" << std::endl;

  ::NVList *paramList;
  paramList = m_daq_service0.getCompParams();
  parse_params(paramList);

  const int nMod = fConfigFile.size();

  fDigitizer.clear();
  fDigitizer.resize(nMod);
  fRawData.resize(nMod);
  fRawDataMutex = std::vector<std::mutex>(nMod);
  for (int i = 0; i < nMod; i++) {
    fDigitizer[i].reset(new Scope2);
    fDigitizer[i]->LoadConfig(fConfigFile[i]);
    fDigitizer[i]->Initialize();
    fDigitizer[i]->Configure();
  }

  return 0;
}

int ReaderScope2::parse_params(::NVList *list)
{
  std::cerr << "param list length:" << (*list).length() << std::endl;
  fConfigFile.clear();

  int len = (*list).length();
  for (int i = 0; i < len; i += 2) {
    std::string sname = (std::string)(*list)[i].value;
    std::string svalue = (std::string)(*list)[i + 1].value;

    std::cerr << "sname: " << sname << "  ";
    std::cerr << "value: " << svalue << std::endl;

    if (sname == "ConfigFile") {
      fConfigFile.push_back(svalue);
    } else if (sname == "StartModNo") {
      fStartModNo = std::stoi(svalue);
    }
  }

  for (auto &config : fConfigFile) {
    std::cerr << "ConfigFile: " << config << std::endl;
  }

  return 0;
}

int ReaderScope2::daq_unconfigure()
{
  std::cerr << "*** ReaderScope2::unconfigure" << std::endl;

  return 0;
}

int ReaderScope2::daq_start()
{
  std::cerr << "*** ReaderScope2::start" << std::endl;

  m_out_status = BUF_SUCCESS;

  fDataBuffer.reset(new std::vector<char>);
  StartThreads();

  const int nMod = fConfigFile.size();
  for (int i = nMod - 1; i >= 0; i--) {
    fDigitizer[i]->StartAcquisition();
  }

  return 0;
}

int ReaderScope2::daq_stop()
{
  std::cerr << "*** ReaderScope2::stop" << std::endl;

  const int nMod = fConfigFile.size();
  for (int i = nMod - 1; i >= 0; i--) {
    fDigitizer[i]->StopAcquisition();
  }

  StopThreads();
  return 0;
}

int ReaderScope2::daq_pause()
{
  std::cerr << "*** ReaderScope2::pause" << std::endl;

  return 0;
}

int ReaderScope2::daq_resume()
{
  std::cerr << "*** ReaderScope2::resume" << std::endl;

  return 0;
}

void ReaderScope2::StartThreads()
{
  fDataReadThreadFlag = true;
  fDataReadThread = std::thread(&ReaderScope2::DataReadThread, this);

  fDataProcessThreadFlag = true;
  fDataProcessThread = std::thread(&ReaderScope2::DataProcessThread, this);
}

void ReaderScope2::StopThreads()
{
  fDataReadThreadFlag = false;
  fDataProcessThreadFlag = false;

  fDataReadThread.join();
  fDataProcessThread.join();
}

void ReaderScope2::DataReadThread()
{
  while (fDataProcessThreadFlag) {
    for (auto i = 0; i < fDigitizer.size(); i++) {
      auto data = fDigitizer[i]->GetData();
      if (data.size() > 0) {
        std::lock_guard<std::mutex> lock(fRawDataMutex[i]);
        fRawData[i].insert(fRawData[i].end(),
                           std::make_move_iterator(data.begin()),
                           std::make_move_iterator(data.end()));
      }
    }
    usleep(10);
  }
}

void ReaderScope2::DataProcessThread()
{
  constexpr auto sizeMod = sizeof(TraceData::Mod);
  constexpr auto sizeCh = sizeof(TraceData::Ch);
  constexpr auto sizeFineTS = sizeof(TraceData::FineTS);
  constexpr auto sizeADC = sizeof(TraceData::ADC);
  constexpr auto sizeRL = sizeof(TraceData::RecordLength);
  std::vector<char> threadBuffer;

  while (fDataProcessThreadFlag) {
    for (auto iMod = 0; iMod < fRawData.size(); iMod++) {
      auto dataSize = 0;
      {
        std::lock_guard<std::mutex> lock(fRawDataMutex[iMod]);
        dataSize = fRawData[iMod].size();
      }
      if (dataSize > 0) {
        std::vector<std::unique_ptr<Scope2Data>> dataVec;
        {
          std::lock_guard<std::mutex> lock(fRawDataMutex[iMod]);
          dataVec = std::move(fRawData[iMod]);
          fRawData[iMod].clear();
        }

        threadBuffer.clear();
        for (auto i = 0; i < dataVec.size(); i++) {
          auto data = std::move(dataVec[i]);
          for (auto iCh = 0; iCh < 32; iCh++) {
            if (data->waveform[iCh][0] == 0) continue;
            auto sizeTrace =
                sizeof(TraceData::Trace1[0]) * data->waveformSize[iCh];
            if (sizeTrace == 0) continue;
            auto sizeData = sizeMod + sizeCh + sizeFineTS + sizeRL + sizeTrace;

            auto index = 0;
            std::vector<uint8_t> hit;
            hit.resize(sizeData);

            TraceData dummy;
            dummy.Mod = iMod;
            dummy.Ch = iCh;
            dummy.FineTS = data->timeStampNs;
            dummy.ADC = 0;
            dummy.RecordLength = data->waveformSize[iCh];

            memcpy(&hit[index], &dummy.Mod, sizeMod);
            index += sizeMod;
            memcpy(&hit[index], &dummy.Ch, sizeCh);
            index += sizeCh;
            memcpy(&hit[index], &dummy.FineTS, sizeFineTS);
            index += sizeFineTS;
            memcpy(&hit[index], &dummy.ADC, sizeADC);
            index += sizeADC;
            memcpy(&hit[index], &dummy.RecordLength, sizeRL);
            index += sizeRL;
            memcpy(&hit[index], &(data->waveform[iCh][0]), sizeTrace);
            index += sizeTrace;

            threadBuffer.insert(threadBuffer.end(), hit.begin(), hit.end());
          }
        }
        if (threadBuffer.size() > 0) {
          std::lock_guard<std::mutex> lock(fFinalDataMutex);
          fDataBuffer->insert(fDataBuffer->end(), threadBuffer.begin(),
                              threadBuffer.end());
        }
      }
    }
    usleep(10);
  }
}

int ReaderScope2::set_data()
{
  if (m_debug) {
    std::cerr << "*** ReaderPHA::set_data" << std::endl;
  }

  unsigned char header[8];
  unsigned char footer[8];

  std::vector<char> *dataBuffer = nullptr;
  {
    std::lock_guard<std::mutex> lock(fFinalDataMutex);
    if (fDataBuffer->size() > 0) {
      dataBuffer = fDataBuffer.release();
      fDataBuffer.reset(new std::vector<char>);
    }
  }
  if (dataBuffer == nullptr) return 0;
  auto size = dataBuffer->size();

  if (size > 0) {
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

int ReaderScope2::write_OutPort()
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

int ReaderScope2::daq_run()
{
  if (m_debug) {
    std::cerr << "*** ReaderScope2::run" << std::endl;
  }

  if (check_trans_lock()) {  // check if stop command has come
    set_trans_unlock();      // transit to CONFIGURED state
    return 0;
  }

  int sentDataSize = 0;
  if (m_out_status ==
      BUF_SUCCESS) {            // previous OutPort.write() successfully done
    sentDataSize = set_data();  // set data to OutPort Buffer
  }

  if (sentDataSize > 0) {
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
void ReaderScope2Init(RTC::Manager *manager)
{
  RTC::Properties profile(reader_spec);
  manager->registerFactory(profile, RTC::Create<ReaderScope2>,
                           RTC::Delete<ReaderScope2>);
}
};
