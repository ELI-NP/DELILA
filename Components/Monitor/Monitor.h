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

#include <memory>
#include <array>

#include <TF1.h>
#include <TH1.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <THttpServer.h>
#include <TStyle.h>
#include <TPolyLine.h>
#include <TSpectrum.h>

#include "../../TDigiTES/include/TPHAData.hpp"

#include "DaqComponentBase.h"

using namespace RTC;


// Number of peaks for TSpectrum
// constexpr int knPeaks = 8;


class Monitor
   : public DAQMW::DaqComponentBase
{
public:
   Monitor(RTC::Manager* manager);
   ~Monitor();

   // The initialize action (on CREATED->ALIVE transition)
   // former rtc_init_entry()
   virtual RTC::ReturnCode_t onInitialize();

   // The execution action that is invoked periodically
   // former rtc_active_do()
   virtual RTC::ReturnCode_t onExecute(RTC::UniqueId ec_id);

private:
   TimedOctetSeq          m_in_data;
   InPort<TimedOctetSeq>  m_InPort;

private:
   int daq_dummy();
   int daq_configure();
   int daq_unconfigure();
   int daq_start();
   int daq_run();
   int daq_stop();
   int daq_pause();
   int daq_resume();

   int parse_params(::NVList* list);
   int reset_InPort();

   unsigned int read_InPort();
   //int online_analyze();

   BufferStatus m_in_status;
   bool m_debug;

   // Fitting and monitoring
   void FillHist(int size);
   long fCounter;
  
  static constexpr int kgBrds = 8;
  static constexpr int kgChs = 16;
  std::array<std::array<std::unique_ptr<TH1D>, kgChs>, kgBrds> fHist;
  std::array<std::array<std::unique_ptr<TGraph>, kgChs>, kgBrds> fSignal;
  std::unique_ptr<THttpServer> fServ;
};


extern "C"
{
   void MonitorInit(RTC::Manager* manager);
};

#endif // MONITOR_H
