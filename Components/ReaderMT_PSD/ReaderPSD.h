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

#include "../../TDigiTES/include/TDigiTes.hpp"
#include "../../TDigiTES/include/TPHA.hpp"
#include "../../TDigiTES/include/TPSD.hpp"
#include "../include/TDataContainer.hpp"
#include "../include/TreeData.h"
#include "DaqComponentBase.h"

using namespace RTC;

class ReaderPSD : public DAQMW::DaqComponentBase
{
 public:
  ReaderPSD(RTC::Manager *manager);
  ~ReaderPSD();

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
  int read_data_from_detectors();
  int set_data();
  int write_OutPort();

  static const int SEND_BUFFER_SIZE = 0;
  unsigned char m_data[SEND_BUFFER_SIZE];
  unsigned int m_recv_byte_size;

  BufferStatus m_out_status;
  bool m_debug;

  // Digitizer
  std::unique_ptr<TPSD> fDigitizer;
  unsigned char *fData;
  std::deque<TreeData_t> fQue;
  std::string fConfigFile;
  int fStartModNo = 0;
  unsigned int fCounter = 0;

  int fTrgCounterMod = -1;
  int fTrgCounterCh = -1;
  bool fFlagTrgCounter = false;

  bool fFlagSWTrg;
  int fNSWTrg;

  TDataContainer fDataContainer;

  // For MT
  std::unique_ptr<std::vector<char>> fDataBuffer;
  std::vector<std::unique_ptr<std::vector<std::unique_ptr<TreeData_t>>>>
      fDataVec;
  std::mutex fDataMutex;
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
void ReaderPSDInit(RTC::Manager *manager);
};

#endif  // READER_H
