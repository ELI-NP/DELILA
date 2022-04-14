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
#include <TStyle.h>
#include <TSystem.h>
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
  // Registration: InPort/OutPort/Service

  // Set InPort buffers
  registerInPort("monitor_in", m_InPort);

  init_command_port();
  init_state_table();
  set_comp_name("MONITOR");

  gStyle->SetOptStat(1111);
  gStyle->SetOptFit(1111);
  fServ.reset(new THttpServer("http:8080?monitoring=5000;rw;noglobal"));

  fResetFlag = kFALSE;
  fServ->RegisterCommand("/ResetHists", "fResetFlag=kTRUE",
                         "button;rootsys/icons/refresh.png");
  // fServ->Hide("/Resethists");

  fCounter = 0;
  fDumpAPI = "";
  fDumpState = "";
  fEveRateServer = "";

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
}

Monitor::~Monitor() { curl_easy_cleanup(fCurl); }

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

  fCurl = curl_easy_init();
  if (fCurl == nullptr) {
    std::cerr << "Failed to initializing curl" << std::endl;
    return 1;
  }
  if (fDumpAPI != "") {
    curl_easy_setopt(fCurl, CURLOPT_URL, fDumpAPI.c_str());
    curl_easy_setopt(fCurl, CURLOPT_WRITEFUNCTION, CallbackFunc);
    curl_easy_setopt(fCurl, CURLOPT_WRITEDATA, &fDumpState);
  }

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
      const double nBins = int(30000 / binWidth) + 1;
      const double min = minBinWidth / 2. + fCalFnc[iBrd][iCh]->GetParameter(0);
      const double max = min + nBins * binWidth;
      fHist[iBrd][iCh].reset(new TH1D(histName, histTitle, nBins, min, max));
      fHist[iBrd][iCh]->SetXTitle("[keV]");

      histName = Form("ADC%02d_%02d", iBrd, iCh);
      fHistADC[iBrd][iCh].reset(
          new TH1D(histName, histTitle, 30000, 0.5, 30000.5));
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
  fServ->Register("/", fGrEveRate.get());

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
      fServ->Register(regDirectory, fHist[iBrd][iCh].get());
      fServ->Register(regDirectory, fHistADC[iBrd][iCh].get());
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

    if (sname == "DumpAPI") {
      fDumpAPI = svalue;
    } else if (sname == "EveRateServer") {
      fEveRateServer = svalue;
    } else if (sname == "Calibration") {
      fCalibrationFile = svalue;
    } else if (sname == "SignalList") {
      fSignalListFile = svalue;
    } else if (sname == "BGOList") {
      fBGOListFile = svalue;
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
  // std::cout <<"Flag: " << fResetFlag << std::endl;
  if (fResetFlag) {
    ResetHists();
    fResetFlag = kFALSE;
  }
  gSystem->ProcessEvents();

  if (fDumpAPI != "") {
    // fDumpState = "";
    auto curlCode = curl_easy_perform(fCurl);
    // std::cout << curlCode <<"\t"<< fDumpState << std::endl;
    if (curlCode == 0 && fDumpState == "true") DumpHists();
  }

  // constexpr auto uploadInterval = 60;
  constexpr auto uploadInterval = 10;
  auto now = time(0);
  auto timeDiff = now - fLastCountTime;
  if (timeDiff >= uploadInterval) {
    fLastCountTime = now;
    UploadEventRate(timeDiff);
  }

  unsigned int recv_byte_size = read_InPort();
  if (recv_byte_size == 0) {  // Timeout
    return 0;
  }

  check_header_footer(m_in_data, recv_byte_size);  // check header and footer
  unsigned int event_byte_size = get_event_size(recv_byte_size);

  /////////////  Write component main logic here. /////////////
  // online_analyze();
  /////////////////////////////////////////////////////////////

  FillHist(event_byte_size);
  gSystem->ProcessEvents();

  // constexpr long updateInterval = 1000;
  // if((fCounter++ % updateInterval) == 0){
  // gSystem->ProcessEvents();
  // }

  inc_sequence_num();                    // increase sequence num.
  inc_total_data_size(event_byte_size);  // increase total data byte size

  return 0;
}

void Monitor::FillHist(int size)
{
  constexpr auto sizeMod = sizeof(TreeData::Mod);
  constexpr auto sizeCh = sizeof(TreeData::Ch);
  constexpr auto sizeTS = sizeof(TreeData::TimeStamp);
  constexpr auto sizeFineTS = sizeof(TreeData::FineTS);
  constexpr auto sizeEne = sizeof(TreeData::ChargeLong);
  constexpr auto sizeShort = sizeof(TreeData::ChargeShort);
  constexpr auto sizeRL = sizeof(TreeData::RecordLength);

  TreeData data(5000000);  // 5000000 = 10ms, enough big for waveform???

  constexpr int headerSize = 8;
  for (unsigned int i = headerSize; i < size;) {
    // The order of data should be the same as Reader
    memcpy(&data.Mod, &m_in_data.data[i], sizeMod);
    i += sizeMod;

    memcpy(&data.Ch, &m_in_data.data[i], sizeCh);
    i += sizeCh;

    memcpy(&data.TimeStamp, &m_in_data.data[i], sizeTS);
    i += sizeTS;

    memcpy(&data.FineTS, &m_in_data.data[i], sizeFineTS);
    i += sizeFineTS;

    memcpy(&data.ChargeLong, &m_in_data.data[i], sizeEne);
    i += sizeEne;

    memcpy(&data.ChargeShort, &m_in_data.data[i], sizeShort);
    i += sizeShort;

    memcpy(&data.RecordLength, &m_in_data.data[i], sizeRL);
    i += sizeRL;

    auto sizeTrace = sizeof(TreeData::Trace1[0]) * data.RecordLength;
    memcpy(&data.Trace1[0], &m_in_data.data[i], sizeTrace);
    i += sizeTrace;

    // Reject the overflow events
    if (data.Mod >= 0 && data.Mod < kgMods && data.Ch >= 0 && data.Ch < kgChs &&
        data.ChargeLong < (1 << 15)) {
      auto ene = fCalFnc[data.Mod][data.Ch]->Eval(data.ChargeLong);
      fHist[data.Mod][data.Ch]->Fill(ene);
      fHistADC[data.Mod][data.Ch]->Fill(data.ChargeLong);
      fEventCounter[data.Mod][data.Ch]++;

      for (auto iPoint = 0; iPoint < data.RecordLength; iPoint++)
        fWaveform[data.Mod][data.Ch]->SetPoint(iPoint, iPoint,
                                               data.Trace1[iPoint]);
      fWaveform[data.Mod][data.Ch]->GetXaxis()->SetRange();
      // fWaveform[data.Mod][data.Ch]->GetYaxis()->SetRange(0, 18000);
    }
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

void Monitor::DumpHists()
{
  std::cout << "Dump ASCII files" << std::endl;
  auto now = time(nullptr);
  auto runNo = get_run_number();
  std::string dirName =
      "/tmp/daqmw/run" + std::to_string(runNo) + "_" + std::to_string(now);
  mkdir(dirName.c_str(), 0777);

  for (auto iBrd = 0; iBrd < kgMods; iBrd++) {
    for (auto iCh = 0; iCh < kgChs; iCh++) {
      auto fileName = dirName + "/" + Form("Brd%02dCh%02d.txt", iBrd, iCh);
      std::cout << fileName << std::endl;
      std::ofstream fout(fileName);

      const auto nBins = fHist[iBrd][iCh]->GetNbinsX();
      for (auto iBin = 1; iBin <= nBins; iBin++) {
        fout << fHist[iBrd][iCh]->GetBinCenter(iBin) << "\t"
             << fHist[iBrd][iCh]->GetBinContent(iBin) << "\n";
      }
      // fout << std::endl;
      fout.close();
    }
  }
}

void Monitor::UploadEventRate(int timeDuration)
{
  int totalCountRate = 0;
  for (auto &&brd : fEventCounter) {
    for (auto &&ch : brd) {
      ch /= timeDuration;
      totalCountRate += ch;
    }
  }

  fGrEveRate->AddPoint(time(nullptr), totalCountRate);
  while (fGrEveRate->GetN() > 100) {
    fGrEveRate->RemovePoint(0);
  }
  fGrEveRate->GetXaxis()->SetRange();

  if (fEveRateServer != "") {
    auto server = influxdb_cpp::server_info(fEveRateServer, 8086, "event_rate");

    std::string resp;
    auto now = time(nullptr);
    constexpr int nMods = kgMods;
    constexpr int nChs = kgChs;
    for (auto mod = 0; mod < nMods; mod++) {
      for (auto ch = 0; ch < nChs; ch++) {
        auto eventRate = fEventCounter[mod][ch];

        try {
          influxdb_cpp::builder()
              .meas("ifin")
              .tag("ch", std::to_string(ch))
              .tag("mod", std::to_string(mod))
              .field("rate", eventRate)
              .timestamp(now * 1000000000)
              .post_http(server, &resp);
        } catch (const std::exception &e) {
          std::cout << e.what() << "\n" << resp << std::endl;
        }
      }
    }
  }

  for (auto &&brd : fEventCounter) {
    for (auto &&ch : brd) {
      ch = 0;
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
