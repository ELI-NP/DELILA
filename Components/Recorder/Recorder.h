// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#ifndef RECORDER_H
#define RECORDER_H

#include <TString.h>
#include <TTree.h>

#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "../../TDigiTES/include/TreeData.h"
#include "DaqComponentBase.h"

using namespace RTC;

class Recorder : public DAQMW::DaqComponentBase
{
 public:
  Recorder(RTC::Manager *manager);
  ~Recorder();

  // The initialize action (on CREATED->ALIVE transition)
  // former rtc_init_entry()
  virtual RTC::ReturnCode_t onInitialize();

  // The execution action that is invoked periodically
  // former rtc_active_do()
  virtual RTC::ReturnCode_t onExecute(RTC::UniqueId ec_id);

 private:
  TimedOctetSeq m_in_data;
  InPort<TimedOctetSeq> m_InPort;

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
  int reset_InPort();

  unsigned int read_InPort();
  //int online_analyze();

  BufferStatus m_in_status;
  bool m_debug;

  int FillData(unsigned int dataSize);

  TString fOutputDir;
  TString fHostName;

  double fDataSize;
  double fDataLimit;

  unsigned long fLastSave;
  unsigned long fSaveInterval;
  unsigned int fSubRunNumber;
  int fRunNumber;  // m_runNumber is uint.  But get_run_number return int.
  bool fDataWriteFlag = false;

  std::unique_ptr<std::vector<TreeData>> fpDataVec;
  void ResetVec();

  void EnqueueData();
  std::vector<std::thread> fThreadVec;
  void MakeTreeAndFile(std::vector<TreeData> *data);

  std::deque<std::vector<TreeData> *> fRawDataQueue;
  std::mutex fMutex;
};

extern "C" {
void RecorderInit(RTC::Manager *manager);
};

#endif  // RECORDER_H
