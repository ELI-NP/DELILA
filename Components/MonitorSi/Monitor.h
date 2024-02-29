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

#include "../../TDigiTES/include/TreeData.h"
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

  // Fitting and monitoring
  void FillHistsThread(std::vector<char> dataVec);
  std::vector<std::thread> fThreads;
  std::mutex fMutex;

  long fCounter;

  static constexpr int kgMods = 10;
  static constexpr int kgChs = 64;
  std::array<std::array<std::unique_ptr<TH1D>, kgChs>, kgMods> fHist;
  std::array<std::array<std::unique_ptr<TH1D>, kgChs>, kgMods> fHistADC;
  std::array<std::array<std::unique_ptr<TGraph>, kgChs>, kgMods> fWaveform;
  std::unique_ptr<THttpServer> fServ;

  void RegisterHists();
  void RegisterDetectors(std::string fileName, std::string calDirName,
                         std::string rawDirName);
  std::string fSignalListFile;
  std::string fBGOListFile;

  // Event rate uploading
  void UploadEventRate(int timeDuration);
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

  // ASCII Dump
  void DumpHists();
  // std::unique_ptr<CURL> fCurl;
  CURL *fCurl;
  std::string fDumpAPI;
  std::string fDumpState;

  // Reset Histograms
  void ResetHists();

  SiDetector::TSiHist *fSiHist = nullptr;
  std::string fSiConf = "";
  std::string fSiMap = "";
};

extern "C" {
void MonitorInit(RTC::Manager *manager);
};

#endif  // MONITOR_H
