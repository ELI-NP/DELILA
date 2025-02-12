// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#include "Monitor.h"

#include <TBufferJSON.h>
#include <TCanvas.h>
#include <TROOT.h>
#include <TStyle.h>
#include <TSystem.h>
#include <rtm/Manager.h>
#include <sys/stat.h>

#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>

#include "influxdb.hpp"

using DAQMW::FatalType::DATAPATH_DISCONNECTED;
using DAQMW::FatalType::FOOTER_DATA_MISMATCH;
using DAQMW::FatalType::HEADER_DATA_MISMATCH;
using DAQMW::FatalType::INPORT_ERROR;
using DAQMW::FatalType::USER_DEFINED_ERROR1;

// Module specification
// Change following items to suit your component's spec.
static const char *monitor_spec[] = {"implementation_id",
                                     "Monitor",
                                     "type_name",
                                     "Monitor",
                                     "description",
                                     "Monitor component",
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

// This factor is for fitting
constexpr auto kBGRange = 2.5;
constexpr auto kFitRange = 5.;
Double_t FitFnc(Double_t *pos, Double_t *par)
{  // This should be class not function.
  const auto x = pos[0];
  const auto mean = par[1];
  const auto sigma = par[2];

  const auto limitHigh = mean + kBGRange * sigma;
  const auto limitLow = mean - kBGRange * sigma;

  auto val = par[0] * TMath::Gaus(x, mean, sigma);

  auto backGround = 0.;
  if (x < limitLow)
    backGround = par[3] + par[4] * x;
  else if (x > limitHigh)
    backGround = par[5] + par[6] * x;
  else {
    auto xInc = limitHigh - limitLow;
    auto yInc = (par[5] + par[6] * limitHigh) - (par[3] + par[4] * limitLow);
    auto slope = yInc / xInc;

    backGround = (par[3] + par[4] * limitLow) + slope * (x - limitLow);
  }

  if (backGround < 0.) backGround = 0.;
  val += backGround;

  return val;
}

// For CURL
size_t CallbackFunc(char *ptr, size_t size, size_t nmemb, std::string *stream)
{
  int dataLength = size * nmemb;
  if (ptr != nullptr) stream->assign(ptr, dataLength);
  return dataLength;
}

// To use THttpServer::RegisterCommand, variable should be global?
// Probably, make Monitor class as ROOT object class is solution.
// I have no time to do (Making dictionary and library, and link) now.
// And the first click make nothing.  After second click, working well.
// Need to check
Bool_t fResetFlag;

Monitor::Monitor(RTC::Manager *manager)
    : DAQMW::DaqComponentBase(manager),
      m_InPort("monitor_in", m_in_data),
      m_in_status(BUF_SUCCESS),
      m_debug(false)
{
  ROOT::EnableImplicitMT();
  // Registration: InPort/OutPort/Service

  // Set InPort buffers
  registerInPort("monitor_in", m_InPort);

  init_command_port();
  init_state_table();
  set_comp_name("MONITOR");

  gStyle->SetOptStat(1111);
  gStyle->SetOptFit(1111);
  fServ.reset(new THttpServer("http:8080?monitoring=5000;rw;noglobal"));
  fServ->SetCors();

  // fServ->Hide("/Resethists");

  fEveRateServer = "";
  fMeasurement = "";

  fCalibrationFile = "";

  fSignalListFile = "";
  fBGOListFile = "";

  fBinWidth = 1.;

  for (auto iBrd = 0; iBrd < kgMods; iBrd++) {
    for (auto iCh = 0; iCh < kgChs; iCh++) {
      TString fncName = Form("fnc%02d_%02d", iBrd, iCh);
      fCalFnc[iBrd][iCh].reset(new TF1(fncName, "pol1"));
      fCalFnc[iBrd][iCh]->SetParameters(0.0, 1.0);
    }
  }
  fSiHist = nullptr;

  fDecodeFlag = true;
  fFillingFlag = true;
  fDecodeThread = std::thread(&Monitor::DecodeThread, this);
  for (auto i = 0; i < knThreads; i++) {
    std::thread t(&Monitor::FillingThread, this);
    fThreadPool.push_back(std::move(t));
  }
}

Monitor::~Monitor()
{
  fDecodeFlag = false;
  fFillingFlag = false;
  for (auto &&t : fThreadPool) {
    t.join();
  }
  fDecodeThread.join();
}

RTC::ReturnCode_t Monitor::onInitialize()
{
  if (m_debug) {
    std::cerr << "Monitor::onInitialize()" << std::endl;
  }

  return RTC::RTC_OK;
}

RTC::ReturnCode_t Monitor::onExecute(RTC::UniqueId ec_id)
{
  daq_do();

  return RTC::RTC_OK;
}

int Monitor::daq_dummy()
{
  gSystem->ProcessEvents();
  return 0;
}

int Monitor::daq_configure()
{
  std::cerr << "*** Monitor::configure" << std::endl;

  ::NVList *paramList;
  paramList = m_daq_service0.getCompParams();
  parse_params(paramList);

  gStyle->SetOptStat(1111);
  gStyle->SetOptFit(1111);

  if (fCalibrationFile == "") {
    for (auto iBrd = 0; iBrd < kgMods; iBrd++) {
      for (auto iCh = 0; iCh < kgChs; iCh++) {
        TString fncName = Form("fnc%02d_%02d", iBrd, iCh);
        fCalFnc[iBrd][iCh].reset(new TF1(fncName, "pol1"));
        fCalFnc[iBrd][iCh]->SetParameters(0.0, 1.0);
      }
    }
  } else {
    std::ifstream fin(fCalibrationFile);

    int mod, ch;
    double p0, p1;

    if (fin.is_open()) {
      while (true) {
        fin >> mod >> ch >> p0 >> p1;
        if (fin.eof()) break;

        std::cout << mod << " " << ch << " " << p0 << " " << p1 << std::endl;
        if (mod >= 0 && mod < kgMods && ch >= 0 && ch < kgChs) {
          TString fncName = Form("fnc%02d_%02d", mod, ch);
          fCalFnc[mod][ch].reset(new TF1(fncName, "pol1"));
          fCalFnc[mod][ch]->SetParameters(p0, p1);
        }
      }
    }

    fin.close();
  }

  for (auto iBrd = 0; iBrd < kgMods; iBrd++) {
    for (auto iCh = 0; iCh < kgChs; iCh++) {
      TString histName = Form("hist%02d_%02d", iBrd, iCh);
      TString histTitle = Form("Brd%02d ch%02d", iBrd, iCh);

      const double minBinWidth = fCalFnc[iBrd][iCh]->GetParameter(1);
      const double binWidth =
          (int(fBinWidth / minBinWidth) + 1) * minBinWidth;  // in keV
      const double nBins = int(32000 / binWidth) + 1;
      const double min = minBinWidth / 2. + fCalFnc[iBrd][iCh]->GetParameter(0);
      const double max = min + nBins * binWidth;
      fHist[iBrd][iCh].reset(new TH1D(histName, histTitle, nBins, min, max));
      fHist[iBrd][iCh]->SetXTitle("[keV]");

      histName = Form("ADC%02d_%02d", iBrd, iCh);
      fHistADC[iBrd][iCh].reset(
          new TH1D(histName, histTitle, 32000, 0.5, 32000.5));
      fHistADC[iBrd][iCh]->SetXTitle("ADC channel");

      TString grName = Form("signal%02d_%02d", iBrd, iCh);
      fWaveform[iBrd][iCh].reset(new TGraph());
      fWaveform[iBrd][iCh]->SetNameTitle(grName, histTitle);
      fWaveform[iBrd][iCh]->SetMinimum(0);
      fWaveform[iBrd][iCh]->SetMaximum(18000);
    }
  }

  RegisterHists();

  fGrEveRate.reset(new TGraph());
  fGrEveRate->SetNameTitle("GrEveRate", "Total trigger count rate on monitor");
  fGrEveRate->GetYaxis()->SetTitle("[cps]");
  // fServ->Register("/", fGrEveRate.get());

  if (fSiHist == nullptr && fSiConf.size() > 0 && fSiMap.size() > 0) {
    //double fInnerDiameter = 25.92;
    //double fOuterDiameter = 70.09;
    //int fNRings = 45;
    //int fNSectorsRear = 16;
    //int fNSectorsFront = 1;
    //fSiHist = new SiDetector::TSiHist("SiHist", fInnerDiameter, fOuterDiameter,
    //                                fNRings, fNSectorsRear, fNSectorsFront);

    fSiHist = new SiDetector::TSiHist("SiHist", fSiConf);
    fSiHist->LoadConfig(fSiMap);
    fServ->Register("/", fSiHist->GetHistFront());
    fServ->Register("/", fSiHist->GetHistRear());
    fServ->Register("/", fSiHist->GetHistMatrix());
  }

  return 0;
}

void Monitor::RegisterHists()
{
  if (fSignalListFile != "")
    RegisterDetectors(fSignalListFile, "/CalibratedSignal", "/RawSignal");
  if (fBGOListFile != "")
    RegisterDetectors(fBGOListFile, "/CalibratedBGO", "/RawBGO");

  for (auto iBrd = 0; iBrd < kgMods; iBrd++) {
    TString regDirectory = Form("/Brd%02d", iBrd);
    for (auto iCh = 0; iCh < kgChs; iCh++) {
      // fServ->Register(regDirectory, fHist[iBrd][iCh].get());
      // fServ->Register(regDirectory, fHistADC[iBrd][iCh].get());
      fServ->Register(regDirectory, fWaveform[iBrd][iCh].get());
    }
  }
}

void Monitor::RegisterDetectors(std::string fileName, std::string calDirName,
                                std::string rawDirName)
{
  if (fileName != "") {
    std::ifstream fin(fileName);
    if (fin.is_open()) {
      unsigned int mod, ch;
      std::string detName;

      TString calDirectory = calDirName;
      TString rawDirectory = rawDirName;

      while (true) {
        fin >> mod >> ch >> detName;
        if (fin.eof()) break;

        std::cout << mod << " " << ch << " " << detName << std::endl;

        if (mod < 0 || mod >= kgMods || ch < 0 || ch >= kgChs) {
          std::cerr << "Config file: " << fileName
                    << " indicates unavailable ch or mod.\n"
                    << "Check it again!" << std::endl;
        } else {
          std::string title = fHist[mod][ch]->GetTitle();
          title = detName + ": " + title;
          fHist[mod][ch]->SetTitle(title.c_str());

          title = fHistADC[mod][ch]->GetTitle();
          title = detName + ": " + title;
          fHistADC[mod][ch]->SetTitle(title.c_str());

          fServ->Register(calDirectory, fHist[mod][ch].get());
          fServ->Register(rawDirectory, fHistADC[mod][ch].get());
        }
      }
      fin.close();
    }
  } else {
    std::cerr << "No such the file: " << fileName << std::endl;
  }
}

int Monitor::parse_params(::NVList *list)
{
  std::cerr << "param list length:" << (*list).length() << std::endl;

  int len = (*list).length();
  for (int i = 0; i < len; i += 2) {
    std::string sname = (std::string)(*list)[i].value;
    std::string svalue = (std::string)(*list)[i + 1].value;

    std::cerr << "sname: " << sname << "  ";
    std::cerr << "value: " << svalue << std::endl;

    if (sname == "EveRateServer") {
      fEveRateServer = svalue;
    } else if (sname == "Measurement") {
      fMeasurement = svalue;
    } else if (sname == "Calibration") {
      fCalibrationFile = svalue;
    } else if (sname == "SignalList") {
      fSignalListFile = svalue;
    } else if (sname == "BGOList") {
      fBGOListFile = svalue;
    } else if (sname == "SiConf") {
      fSiConf = svalue;
    } else if (sname == "SiMap") {
      fSiMap = svalue;
    } else if (sname == "BinWidth") {
      fBinWidth = std::stod(svalue);
      if (fBinWidth <= 0.) fBinWidth = 1.;
    }
  }

  return 0;
}

int Monitor::daq_unconfigure()
{
  std::cerr << "*** Monitor::unconfigure" << std::endl;

  return 0;
}

int Monitor::daq_start()
{
  std::cerr << "*** Monitor::start" << std::endl;
  m_in_status = BUF_SUCCESS;

  fLastCountTime = time(0);
  for (auto &&brd : fEventCounter) {
    for (auto &&ch : brd) {
      ch = 0;
    }
  }

  ResetHists();

  if (fSiHist != nullptr) {
    fSiHist->GetHistFront()->Reset("");
    fSiHist->GetHistFront()->SetDrawOption("COLZ");
    fSiHist->GetHistRear()->Reset("");
    fSiHist->GetHistRear()->SetDrawOption("COLZ");
    fSiHist->GetHistMatrix()->Reset("");
    fSiHist->GetHistMatrix()->SetDrawOption("COLZ");
  }

  return 0;
}

int Monitor::daq_stop()
{
  std::cerr << "*** Monitor::stop" << std::endl;
  reset_InPort();

  return 0;
}

int Monitor::daq_pause()
{
  std::cerr << "*** Monitor::pause" << std::endl;

  return 0;
}

int Monitor::daq_resume()
{
  std::cerr << "*** Monitor::resume" << std::endl;

  return 0;
}

int Monitor::reset_InPort()
{
  int ret = true;
  while (ret == true) {
    ret = m_InPort.read();
  }

  return 0;
}

unsigned int Monitor::read_InPort()
{
  /////////////// read data from InPort Buffer ///////////////
  unsigned int recv_byte_size = 0;
  bool ret = m_InPort.read();

  //////////////////// check read status /////////////////////
  if (ret == false) {  // false: TIMEOUT or FATAL
    m_in_status = check_inPort_status(m_InPort);
    if (m_in_status == BUF_TIMEOUT) {  // Buffer empty.
      if (check_trans_lock()) {        // Check if stop command has come.
        set_trans_unlock();            // Transit to CONFIGURE state.
      }
    } else if (m_in_status == BUF_FATAL) {  // Fatal error
      fatal_error_report(INPORT_ERROR);
    }
  } else {
    recv_byte_size = m_in_data.data.length();
  }

  if (m_debug) {
    std::cerr << "m_in_data.data.length():" << recv_byte_size << std::endl;
  }

  return recv_byte_size;
}

int Monitor::daq_run()
{
  if (m_debug) {
    std::cerr << "*** Monitor::run" << std::endl;
  }

  if (check_trans_lock()) {  // check if stop command has come
    set_trans_unlock();      // transit to CONFIGURED state
    return 0;
  }

  // std::cout <<"Flag: " << fResetFlag << std::endl;
  if (fResetFlag) {
    ResetHists();
    fResetFlag = kFALSE;
  }
  gSystem->ProcessEvents();

  // constexpr auto uploadInterval = 60;
  constexpr auto uploadInterval = 10;
  auto now = time(0);
  auto timeDiff = now - fLastCountTime;
  if (timeDiff >= uploadInterval) {
    fLastCountTime = now;
    if (fEveRateServer.size() > 0) UploadEventRate(timeDiff);
  }

  unsigned int recv_byte_size = read_InPort();
  if (recv_byte_size == 0) {  // Timeout
    return 0;
  }

  check_header_footer(m_in_data, recv_byte_size);  // check header and footer
  unsigned int event_byte_size = get_event_size(recv_byte_size);
  inc_sequence_num();                    // increase sequence num.
  inc_total_data_size(event_byte_size);  // increase total data byte size

  /////////////  Write component main logic here. /////////////
  // online_analyze();
  /////////////////////////////////////////////////////////////

  std::unique_ptr<std::vector<uint8_t>> vecData(
      new std::vector<uint8_t>(event_byte_size));
  constexpr int headerSize = 8;
  memcpy(&(*vecData)[0], m_in_data.data.get_buffer() + headerSize,
         event_byte_size);
  {
    std::lock_guard<std::mutex> lock(fRawDataQueueMutex);
    fRawDataQueue.push_back(std::move(vecData));
  }
  gSystem->ProcessEvents();

  return 0;
}

void Monitor::DecodeThread()
{
  std::unique_ptr<std::vector<uint8_t>> vecData;
  while (fDecodeFlag) {
    vecData.reset(nullptr);
    {
      std::lock_guard<std::mutex> lock(fRawDataQueueMutex);
      if (fRawDataQueue.size() > 0) {
        vecData = std::move(fRawDataQueue.front());
        fRawDataQueue.pop_front();
      }
    }

    if (vecData) {
      constexpr auto sizeMod = sizeof(TraceData::Mod);
      constexpr auto sizeCh = sizeof(TraceData::Ch);
      constexpr auto sizeFineTS = sizeof(TraceData::FineTS);
      constexpr auto sizeADC = sizeof(TraceData::ADC);
      constexpr auto sizeRL = sizeof(TraceData::RecordLength);

      std::vector<std::unique_ptr<TraceData>> localData;
      for (uint64_t i = 0; i < vecData->size();) {
        std::unique_ptr<TraceData> data(new TraceData);
        memcpy(&data->Mod, &(*vecData)[i], sizeMod);
        i += sizeMod;

        memcpy(&data->Ch, &(*vecData)[i], sizeCh);
        i += sizeCh;

        memcpy(&data->FineTS, &(*vecData)[i], sizeFineTS);
        i += sizeFineTS;

        memcpy(&data->ADC, &(*vecData)[i], sizeADC);
        i += sizeADC;

        memcpy(&data->RecordLength, &(*vecData)[i], sizeRL);
        i += sizeRL;

        auto sizeTrace = sizeof(TraceData::Trace1[0]) * data->RecordLength;
        data->Trace1.resize(data->RecordLength);
        memcpy(&data->Trace1[0], &(*vecData)[i], sizeTrace);
        i += sizeTrace;

        localData.push_back(std::move(data));
      }

      {
        std::lock_guard<std::mutex> lock(fDataMutex);
        fDataQueue.insert(fDataQueue.end(),
                          std::make_move_iterator(localData.begin()),
                          std::make_move_iterator(localData.end()));
      }

    } else {
      std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
  }
}

void Monitor::FillingThread()
{
  while (fFillingFlag) {
    std::vector<std::unique_ptr<TraceData>> localData;
    {
      std::lock_guard<std::mutex> lock(fDataMutex);
      if (fDataQueue.size() > 0) {
        localData.insert(localData.end(),
                         std::make_move_iterator(fDataQueue.begin()),
                         std::make_move_iterator(fDataQueue.end()));
        fDataQueue.clear();
      }
    }

    for (auto &&data : localData) {
      auto mod = data->Mod;
      auto ch = data->Ch;
      auto fineTS = data->FineTS;
      auto adc = data->ADC;
      auto recordLength = data->RecordLength;
      auto &trace = data->Trace1;

      if (mod < 0 || mod >= kgMods || ch < 0 || ch >= kgChs) {
        std::cerr << "Invalid mod or ch: " << mod << " " << ch << std::endl;
        continue;
      }

      constexpr auto deltaT = 2;  // Take this form Digitizer! Stupid me.
      std::vector<Double_t> x(recordLength);
      std::vector<Double_t> y(recordLength);
      for (auto i = 0; i < recordLength; i++) {
        x[i] = i * deltaT;
        y[i] = trace[i];
      }
      {
        std::lock_guard<std::mutex> lock(fWaveformMutex[mod][ch]);
        fWaveform[mod][ch]->Set(recordLength);
        auto graphX = fWaveform[mod][ch]->GetX();
        auto graphY = fWaveform[mod][ch]->GetY();
        std::copy(x.begin(), x.end(), graphX);
        std::copy(y.begin(), y.end(), graphY);
      }

      fEveRateMutex.lock();
      fEventCounter[mod][ch]++;
      fEveRateMutex.unlock();
    }

    std::this_thread::sleep_for(std::chrono::microseconds(1000));
  }
}

void Monitor::ResetHists()
{
  for (auto &&brd : fHist) {
    for (auto &&ch : brd) {
      ch->Reset();
    }
  }
  for (auto &&brd : fHistADC) {
    for (auto &&ch : brd) {
      ch->Reset();
    }
  }
}

void Monitor::UploadEventRate(int timeDuration)
{
  fEveRateMutex.lock();
  for (auto &&brd : fEventCounter) {
    for (auto &&ch : brd) {
      ch /= timeDuration;
    }
  }

  auto buf = fEventCounter;

  for (auto &&brd : fEventCounter) {
    for (auto &&ch : brd) {
      ch = 0;
    }
  }
  fEveRateMutex.unlock();

  auto server = influxdb_cpp::server_info(fEveRateServer, 8086, "event_rate");

  std::string resp;
  auto now = time(nullptr);
  influxdb_cpp::builder builder;
  influxdb_cpp::detail::ts_caller *caller = nullptr;
  constexpr int nMods = kgMods;
  for (auto mod = 0; mod < nMods; mod++) {
    int nChs = kgChs;
    // if (mod > 1) nChs = 16;
    for (auto ch = 0; ch < nChs; ch++) {
      auto eventRate = buf[mod][ch];
      if (caller) {
        caller = &caller->meas(fMeasurement)
                      .tag("ch", std::to_string(ch))
                      .tag("mod", std::to_string(mod))
                      .field("rate", eventRate)
                      .timestamp(now * 1000000000);
      } else {
        caller = &builder.meas(fMeasurement)
                      .tag("ch", std::to_string(ch))
                      .tag("mod", std::to_string(mod))
                      .field("rate", eventRate)
                      .timestamp(now * 1000000000);
      }
    }
  }
  if (caller) {
    auto result = caller->post_http(server, &resp);
    if (result != 0) {
      std::cout << result << "\t" << resp << std::endl;
    }
  }
}

extern "C" {
void MonitorInit(RTC::Manager *manager)
{
  RTC::Properties profile(monitor_spec);
  manager->registerFactory(profile, RTC::Create<Monitor>, RTC::Delete<Monitor>);
}
};
