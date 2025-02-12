// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#include "ReaderPSD2Trace.h"

#include <byteswap.h>

using DAQMW::FatalType::DATAPATH_DISCONNECTED;
using DAQMW::FatalType::OUTPORT_ERROR;
using DAQMW::FatalType::USER_DEFINED_ERROR1;

// Module specification
// Change following items to suit your component's spec.
static const char *reader_spec[] = {"implementation_id",
                                    "ReaderPSD2Trace",
                                    "type_name",
                                    "ReaderPSD2Trace",
                                    "description",
                                    "ReaderPSD2Trace component",
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

ReaderPSD2Trace::ReaderPSD2Trace(RTC::Manager *manager)
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

ReaderPSD2Trace::~ReaderPSD2Trace() {}

RTC::ReturnCode_t ReaderPSD2Trace::onInitialize()
{
  if (m_debug) {
    std::cerr << "ReaderPSD2Trace::onInitialize()" << std::endl;
  }

  return RTC::RTC_OK;
}

RTC::ReturnCode_t ReaderPSD2Trace::onExecute(RTC::UniqueId ec_id)
{
  daq_do();

  return RTC::RTC_OK;
}

int ReaderPSD2Trace::daq_dummy() { return 0; }

int ReaderPSD2Trace::daq_configure()
{
  std::cerr << "*** ReaderPSD2Trace::configure" << std::endl;

  ::NVList *paramList;
  paramList = m_daq_service0.getCompParams();
  parse_params(paramList);

  // fDigitizer.reset(new PSD2);
  // fDigitizer->LoadConfig(fConfigFile);
  // fDigitizer->Initialize();
  // fDigitizer->Configure();
  fDigitizer.clear();
  for (auto &config : fConfigFile) {
    fDigitizer.push_back(std::make_unique<PSD2>());
    fDigitizer.back()->LoadConfig(config);
    fDigitizer.back()->Initialize();
    fDigitizer.back()->Configure();
  }

  return 0;
}

int ReaderPSD2Trace::parse_params(::NVList *list)
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

  return 0;
}

int ReaderPSD2Trace::daq_unconfigure()
{
  std::cerr << "*** ReaderPSD2Trace::unconfigure" << std::endl;

  return 0;
}

int ReaderPSD2Trace::daq_start()
{
  std::cerr << "*** ReaderPSD2Trace::start" << std::endl;

  m_out_status = BUF_SUCCESS;

  fDataBuffer.reset(new std::vector<char>);
  StartThreads();
  // fDigitizer->StartAcquisition();
  for (auto &digitizer : fDigitizer) {
    digitizer->StartAcquisition();
  }

  return 0;
}

int ReaderPSD2Trace::daq_stop()
{
  std::cerr << "*** ReaderPSD2Trace::stop" << std::endl;

  // fDigitizer->StopAcquisition();
  for (auto &digitizer : fDigitizer) {
    digitizer->StopAcquisition();
  }
  StopThreads();
  return 0;
}

int ReaderPSD2Trace::daq_pause()
{
  std::cerr << "*** ReaderPSD2Trace::pause" << std::endl;

  return 0;
}

int ReaderPSD2Trace::daq_resume()
{
  std::cerr << "*** ReaderPSD2Trace::resume" << std::endl;

  return 0;
}

void ReaderPSD2Trace::StartThreads()
{
  fDataReadThreadFlag = true;
  fDataReadThread = std::thread(&ReaderPSD2Trace::DataReadThread, this);

  fDataProcessThreadFlag = true;
  fDataProcessThread = std::thread(&ReaderPSD2Trace::DataProcessThread, this);
}

void ReaderPSD2Trace::StopThreads()
{
  fDataReadThreadFlag = false;
  fDataProcessThreadFlag = false;

  fDataReadThread.join();
  fDataProcessThread.join();
}

void ReaderPSD2Trace::DataReadThread()
{
  while (fDataProcessThreadFlag) {
    // auto data = fDigitizer->GetData();
    // if (data.size() > 0) {
    //   std::lock_guard<std::mutex> lock(fRawDataMutex);
    //   fDataVec.insert(fDataVec.end(), std::make_move_iterator(data.begin()),
    //                   std::make_move_iterator(data.end()));
    // }
    for (auto &digitizer : fDigitizer) {
      auto data = digitizer->GetData();
      if (data.size() > 0) {
        std::lock_guard<std::mutex> lock(fRawDataMutex);
        fDataVec.insert(fDataVec.end(), std::make_move_iterator(data.begin()),
                        std::make_move_iterator(data.end()));
      }
    }
    usleep(10);
  }
}

void ReaderPSD2Trace::DataProcessThread()
{
  constexpr auto sizeMod = sizeof(TraceData::Mod);
  constexpr auto sizeCh = sizeof(TraceData::Ch);
  constexpr auto sizeFineTS = sizeof(TraceData::FineTS);
  constexpr auto sizeADC = sizeof(TraceData::ADC);
  constexpr auto sizeRL = sizeof(TraceData::RecordLength);

  while (fDataProcessThreadFlag) {
    auto dataSize = 0;
    {
      std::lock_guard<std::mutex> lock(fRawDataMutex);
      dataSize = fDataVec.size();
    }
    if (dataSize > 0) {
      std::vector<std::unique_ptr<PSD2Data>> dataVec;
      {
        std::lock_guard<std::mutex> lock(fRawDataMutex);
        dataVec = std::move(fDataVec);
        fDataVec.clear();
      }

      std::vector<char> threadBuffer;
      for (auto i = 0; i < dataVec.size(); i++) {
        auto data = std::move(dataVec.at(i));
        const auto sizeTrace =
            sizeof(TraceData::Trace1[0]) * data->waveformSize;
        const auto oneHitSize =
            sizeMod + sizeCh + sizeFineTS + sizeADC + sizeRL + sizeTrace;

        auto index = 0;
        std::vector<uint8_t> hit;
        hit.resize(oneHitSize);

        TraceData dummy;
        dummy.Mod = data->module + fStartModNo;
        memcpy(&hit[index], &(dummy.Mod), sizeMod);
        index += sizeMod;

        memcpy(&hit[index], &(data->channel), sizeCh);
        index += sizeCh;

        double fineTS = data->timeStampNs * 1000 + data->fineTimeStamp;
        memcpy(&hit[index], &fineTS, sizeFineTS);
        index += sizeFineTS;

        dummy.ADC = 0;
        memcpy(&hit[index], &(dummy.ADC), sizeADC);
        index += sizeADC;

        // This is from size_t(64 bits) to uint16_t. Dangerous
        dummy.RecordLength = data->waveformSize;
        // std::cout << "RecordLength: " << dummy.RecordLength << "\t"
        //           << data->waveformSize << std::endl;
        memcpy(&hit[index], &(dummy.RecordLength), sizeRL);
        index += sizeRL;

        dummy.Trace1.resize(data->waveformSize);
        for (int i = 0; i < data->waveformSize; i++) {
          // std::cout << data->analogProbe1[i] << std::endl;
          dummy.Trace1[i] = data->analogProbe1[i];  //Crazy
        }
        memcpy(&hit[index], &(dummy.Trace1[0]), sizeTrace);

        threadBuffer.insert(threadBuffer.end(), hit.begin(), hit.end());
      }

      fFinalDataMutex.lock();
      fDataBuffer->insert(fDataBuffer->end(), threadBuffer.begin(),
                          threadBuffer.end());
      fFinalDataMutex.unlock();
    }
    usleep(1000);
  }
}

int ReaderPSD2Trace::set_data()
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

int ReaderPSD2Trace::write_OutPort()
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

int ReaderPSD2Trace::daq_run()
{
  if (m_debug) {
    std::cerr << "*** ReaderPSD2Trace::run" << std::endl;
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
void ReaderPSD2TraceInit(RTC::Manager *manager)
{
  RTC::Properties profile(reader_spec);
  manager->registerFactory(profile, RTC::Create<ReaderPSD2Trace>,
                           RTC::Delete<ReaderPSD2Trace>);
}
};
