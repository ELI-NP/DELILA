// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#ifndef READER_H
#define READER_H

#include <deque>
#include <memory>
#include <string>

#include "../include/TraceData.hpp"
#include "./PSD2.hpp"
#include "DaqComponentBase.h"

using namespace RTC;

class ReaderPSD2Trace : public DAQMW::DaqComponentBase
{
 public:
  ReaderPSD2Trace(RTC::Manager *manager);
  ~ReaderPSD2Trace();

  // The initialize action (on CREATED->ALIVE transition)
  // former rtc_init_entry()
  virtual RTC::ReturnCode_t onInitialize();

  // The execution action that is invoked periodically
  // former rtc_active_do()
  virtual RTC::ReturnCode_t onExecute(RTC::UniqueId ec_id);

 private:
  TimedOctetSeq m_out_data;
  OutPort<TimedOctetSeq> m_OutPort;

 private:
  int daq_dummy();
  int daq_configure();
  int daq_unconfigure();
  int daq_start();
  int daq_run();
  int daq_stop();
  int daq_pause();
  int daq_resume();

  int parse_params(::NVList *list);
  int set_data();
  int write_OutPort();

  static const int SEND_BUFFER_SIZE = 0;
  unsigned char m_data[SEND_BUFFER_SIZE];
  unsigned int m_recv_byte_size;

  BufferStatus m_out_status;
  bool m_debug;

  // Digitizer
  std::vector<std::unique_ptr<PSD2>> fDigitizer;
  // std::unique_ptr<PSD2> fDigitizer;
  std::vector<std::string> fConfigFile;
  unsigned char *fData;
  std::deque<TraceData> fQue;
  int fStartModNo = 0;
  unsigned int fCounter = 0;

  // For MT
  std::unique_ptr<std::vector<char>> fDataBuffer;
  std::mutex fFinalDataMutex;
  std::vector<std::unique_ptr<PSD2Data>> fDataVec;
  std::mutex fRawDataMutex;
  void StartThreads();
  void StopThreads();

  std::thread fDataProcessThread;
  bool fDataProcessThreadFlag;
  void DataProcessThread();

  std::thread fDataReadThread;
  bool fDataReadThreadFlag;
  void DataReadThread();
};

extern "C" {
void ReaderPSD2TraceInit(RTC::Manager *manager);
};

#endif  // READER_H
