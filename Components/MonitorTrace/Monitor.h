// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#ifndef MONITOR_H
#define MONITOR_H

#include <TCanvas.h>
#include <TF1.h>
#include <TGraph.h>
#include <TH1.h>
#include <THttpServer.h>
#include <TPolyLine.h>
#include <TSpectrum.h>
#include <TStyle.h>
#include <curl/curl.h>

#include <array>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "../include/TraceData.hpp"
#include "./TSiHist.hpp"
#include "DaqComponentBase.h"

using namespace RTC;

// Number of peaks for TSpectrum
// constexpr int knPeaks = 8;

class Monitor : public DAQMW::DaqComponentBase
{
 public:
  Monitor(RTC::Manager *manager);
  ~Monitor();

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
  // int online_analyze();

  BufferStatus m_in_status;
  bool m_debug;

  static constexpr int kgMods = 3;
  static constexpr int kgChs = 32;
  std::array<std::array<std::unique_ptr<TH1D>, kgChs>, kgMods> fHist;
  std::array<std::array<std::unique_ptr<TH1D>, kgChs>, kgMods> fHistADC;
  std::array<std::array<std::unique_ptr<TGraph>, kgChs>, kgMods> fWaveform;
  std::array<std::array<std::mutex, kgChs>, kgMods> fWaveformMutex;
  std::unique_ptr<THttpServer> fServ;
  void ResetHists();

  // Si detector
  SiDetector::TSiHist *fSiHist = nullptr;
  std::string fSiConf = "";
  std::string fSiMap = "";

  void RegisterHists();
  void RegisterDetectors(std::string fileName, std::string calDirName,
                         std::string rawDirName);
  std::string fSignalListFile;
  std::string fBGOListFile;

  // Event rate uploading
  void UploadEventRate(int timeDuration);
  std::mutex fEveRateMutex;
  std::array<std::array<int, kgChs>, kgMods> fEventCounter;
  std::unique_ptr<TGraph> fGrEveRate;
  long fLastCountTime;
  std::string fEveRateServer;
  std::string fMeasurement;

  // Calibration
  void ReadPar();
  std::string fCalibrationFile;
  std::array<std::array<std::array<double, 2>, kgChs>, kgMods> fCalPar;
  std::array<std::array<std::unique_ptr<TF1>, kgChs>, kgMods> fCalFnc;
  double fBinWidth;

  std::deque<std::unique_ptr<std::vector<u_int8_t>>> fRawDataQueue;
  std::mutex fRawDataQueueMutex;
  bool fDecodeFlag = false;
  std::thread fDecodeThread;
  void DecodeThread();

  // Thread pool for filling histograms and graphs
  // 10 threads
  std::vector<std::unique_ptr<TraceData>> fDataQueue;
  std::mutex fDataMutex;
  static constexpr int knThreads = 10;
  std::vector<std::thread> fThreadPool;
  bool fFillingFlag = false;
  void FillingThread();
};

extern "C" {
void MonitorInit(RTC::Manager *manager);
};

#endif  // MONITOR_H
