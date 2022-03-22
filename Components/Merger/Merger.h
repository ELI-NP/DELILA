// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <chrono>
#include <iostream>
#include <memory>
#include <vector>

#include "DaqComponentBase.h"

using namespace RTC;

class Merger : public DAQMW::DaqComponentBase
{
 public:
  Merger(RTC::Manager *manager);
  ~Merger();

  // The initialize action (on CREATED->ALIVE transition)
  // former rtc_init_entry()
  virtual RTC::ReturnCode_t onInitialize();

  // The execution action that is invoked periodically
  // former rtc_active_do()
  virtual RTC::ReturnCode_t onExecute(RTC::UniqueId ec_id);

 private:
  void ReadInOutSettings();
  int fNIn;
  int fNOut;
  TimedOctetSeq fInDataTotal;
  std::vector<TimedOctetSeq> fInDataVec;
  std::vector<std::unique_ptr<InPort<TimedOctetSeq>>> fInPortVec;
  std::vector<BufferStatus> fInStatusVec;

  TimedOctetSeq m_out1_data;
  OutPort<TimedOctetSeq> m_OutPort1;

  TimedOctetSeq m_out2_data;
  OutPort<TimedOctetSeq> m_OutPort2;

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
  int set_data_OutPort1(unsigned int data_byte_size);
  int set_data_OutPort2(unsigned int data_byte_size);
  int reset_InPort();
  unsigned int read_InPort();
  unsigned int ReadInPorts();
  unsigned int MergeInData(unsigned int dataSize);
  int write_OutPort1();
  int write_OutPort2();

  bool CheckOutPorts();
  bool CheckInPorts();

  BufferStatus m_out1_status;
  BufferStatus m_out2_status;

  bool m_debug;

  std::chrono::time_point<std::chrono::high_resolution_clock> fStart;
};

extern "C" {
void MergerInit(RTC::Manager *manager);
};

#endif  // DISPATCHER_H
