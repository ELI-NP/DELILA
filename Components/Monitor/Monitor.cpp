// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#include <sys/stat.h>
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>

#include <TSystem.h>
#include <TStyle.h>
#include <TBufferJSON.h>
#include <TCanvas.h>

#include "Monitor.h"

using DAQMW::FatalType::DATAPATH_DISCONNECTED;
using DAQMW::FatalType::INPORT_ERROR;
using DAQMW::FatalType::HEADER_DATA_MISMATCH;
using DAQMW::FatalType::FOOTER_DATA_MISMATCH;
using DAQMW::FatalType::USER_DEFINED_ERROR1;

// Module specification
// Change following items to suit your component's spec.
static const char* monitor_spec[] =
  {
    "implementation_id", "Monitor",
    "type_name",         "Monitor",
    "description",       "Monitor component",
    "version",           "1.0",
    "vendor",            "Kazuo Nakayoshi, KEK",
    "category",          "example",
    "activity_type",     "DataFlowComponent",
    "max_instance",      "1",
    "language",          "C++",
    "lang_type",         "compile",
    ""
  };


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
  if(ptr != nullptr) stream->assign(ptr, dataLength);
  return dataLength;
}

Monitor::Monitor(RTC::Manager* manager)
  : DAQMW::DaqComponentBase(manager),
  m_InPort("monitor_in",   m_in_data),
  m_in_status(BUF_SUCCESS),
  m_debug(false)
{
  // Registration: InPort/OutPort/Service

  // Set InPort buffers
  registerInPort ("monitor_in",  m_InPort);
   
  init_command_port();
  init_state_table();
  set_comp_name("MONITOR");

  gStyle->SetOptStat(1111);
  gStyle->SetOptFit(1111);
  fServ.reset(new THttpServer("http:8080?monitoring=5000;rw;noglobal"));

  for(auto iBrd = 0; iBrd < kgBrds; iBrd++){
    TString regDirectory = Form("/Brd%02d", iBrd);
    for(auto iCh = 0; iCh < kgChs; iCh++){
      TString histName = Form("hist%02d_%02d", iBrd, iCh);
      TString histTitle = Form("Brd%02d ch%02d", iBrd, iCh);
      fHist[iBrd][iCh].reset(new TH1D(histName, histTitle, 30000, 0.5, 30000.5));

      TString grName = Form("signal%02d_%02d", iBrd, iCh);
      fSignal[iBrd][iCh].reset(new TGraph());
      fSignal[iBrd][iCh]->SetNameTitle(grName, histTitle);
      fSignal[iBrd][iCh]->SetMinimum(0);
      fSignal[iBrd][iCh]->SetMaximum(18000);
      
      fServ->Register(regDirectory, fHist[iBrd][iCh].get());
      fServ->Register(regDirectory, fSignal[iBrd][iCh].get());
    }
  }

  fCounter = 0;
  fDumpAPI = "";
  fDumpState = "";
  fEveRateAPI = "";
}

Monitor::~Monitor()
{
  curl_easy_cleanup(fCurl);
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

  ::NVList* paramList;
  paramList = m_daq_service0.getCompParams();
  parse_params(paramList);

   
  // fServ->Register("/", fHistRaw.get());

  gStyle->SetOptStat(1111);
  gStyle->SetOptFit(1111);

  fCurl = curl_easy_init();
  if (fCurl == nullptr) {
    std::cerr << "Failed to initializing curl" << std::endl;
    return 1;
  }
  if(fDumpAPI != ""){
    curl_easy_setopt(fCurl, CURLOPT_URL, fDumpAPI.c_str());
    curl_easy_setopt(fCurl, CURLOPT_WRITEFUNCTION, CallbackFunc);
    curl_easy_setopt(fCurl, CURLOPT_WRITEDATA, &fDumpState);
  }
  
  // Using name of histogram "hist", NOT the variable "fHist"
  // fServ->RegisterCommand("/Reset","/hist/->Reset()", "button;rootsys/icons/ed_delete.png");
  // fServ->RegisterCommand("/ResetRaw","/raw/->Reset()", "button;rootsys/icons/ed_delete.png");

  return 0;
}

int Monitor::parse_params(::NVList* list)
{

  std::cerr << "param list length:" << (*list).length() << std::endl;

  int len = (*list).length();
  for (int i = 0; i < len; i+=2) {
    std::string sname  = (std::string)(*list)[i].value;
    std::string svalue = (std::string)(*list)[i+1].value;

    std::cerr << "sname: " << sname << "  ";
    std::cerr << "value: " << svalue << std::endl;

    if(sname == "DumpAPI"){
      fDumpAPI = svalue;
    } else if (sname == "EveRateAPI") {
      fEveRateAPI = svalue;
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
  m_in_status  = BUF_SUCCESS;

  fLastCountTime = time(0);
  for(auto &&brd: fEventCounter) {
    for(auto &&ch: brd) {
      ch = 0;
    }
  }
  for(auto &&histVec: fHist){
    for(auto &&hist: histVec){
      hist->Reset();
    }
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
  while(ret == true) {
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
  if (ret == false) { // false: TIMEOUT or FATAL
    m_in_status = check_inPort_status(m_InPort);
    if (m_in_status == BUF_TIMEOUT) { // Buffer empty.
      if (check_trans_lock()) {     // Check if stop command has come.
	set_trans_unlock();       // Transit to CONFIGURE state.
      }
    }
    else if (m_in_status == BUF_FATAL) { // Fatal error
      fatal_error_report(INPORT_ERROR);
    }
  }
  else {
    recv_byte_size = m_in_data.data.length();
  }

  if (m_debug) {
    std::cerr << "m_in_data.data.length():" << recv_byte_size
	      << std::endl;
  }

  return recv_byte_size;
}

int Monitor::daq_run()
{
  if (m_debug) {
    std::cerr << "*** Monitor::run" << std::endl;
  }
  
  if(fDumpAPI != "") {
    // fDumpState = "";
    auto curlCode = curl_easy_perform(fCurl);
    // std::cout << curlCode <<"\t"<< fDumpState << std::endl;
    if(curlCode == 0 && fDumpState == "true")
      DumpHists();
  }

  // constexpr auto uploadInterval = 60;
  constexpr auto uploadInterval = 10;
  auto now = time(0);
  auto timeDiff = now - fLastCountTime;
  if(timeDiff > uploadInterval) {
    fLastCountTime = now;
    UploadEventRate(timeDiff);
  }
  
  unsigned int recv_byte_size = read_InPort();
  if (recv_byte_size == 0) { // Timeout
    return 0;
  }

  check_header_footer(m_in_data, recv_byte_size); // check header and footer
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

  inc_sequence_num();                       // increase sequence num.
  inc_total_data_size(event_byte_size);     // increase total data byte size

  return 0;
}

void Monitor::FillHist(int size)
{
  constexpr auto sizeMod = sizeof(PHAData::ModNumber);
  constexpr auto sizeCh = sizeof(PHAData::ChNumber);
  constexpr auto sizeTS = sizeof(PHAData::TimeStamp);
  constexpr auto sizeEne = sizeof(PHAData::Energy);
  constexpr auto sizeRL =  sizeof(PHAData::RecordLength);
  constexpr auto sizeEle = sizeof(*(PHAData::Trace1));
  
  PHAData data(5000000); // 5000000 = 10ms, enough big for waveform???

  constexpr int headerSize = 8;
  for(unsigned int i = headerSize; i < size;) {
    // The order of data should be the same as Reader
    memcpy(&data.ModNumber, &m_in_data.data[i], sizeMod);
    i += sizeMod;

    memcpy(&data.ChNumber, &m_in_data.data[i], sizeCh);
    i += sizeCh;

    memcpy(&data.TimeStamp, &m_in_data.data[i], sizeTS);
    i += sizeTS;

    memcpy(&data.Energy, &m_in_data.data[i], sizeEne);
    i += sizeEne;

    memcpy(&data.RecordLength, &m_in_data.data[i], sizeRL);
    i += sizeRL;

    auto sizeTrace = sizeof(*(PHAData::Trace1)) * data.RecordLength;
    memcpy(data.Trace1, &m_in_data.data[i], sizeTrace);
    i += sizeTrace;

    // Reject the overflow events
    if(data.ModNumber >= 0 && data.ModNumber < kgBrds &&
       data.ChNumber >= 0 && data.ChNumber < kgChs &&
       data.Energy < (1 << 15)){
      fHist[data.ModNumber][data.ChNumber]->Fill(data.Energy);
      fEventCounter[data.ModNumber][data.ChNumber]++;
      
      for(auto iPoint = 0; iPoint < data.RecordLength; iPoint++)
	fSignal[data.ModNumber][data.ChNumber]->SetPoint(iPoint, iPoint, data.Trace1[iPoint]);
    }
  }
  
}

void Monitor::DumpHists()
{
  std::cout << "Dump ASCII files" << std::endl;
  auto now = time(nullptr);
  auto runNo = get_run_number();
  std::string dirName = "/tmp/daqmw/run" + std::to_string(runNo) + "_" + std::to_string(now);
  mkdir(dirName.c_str(), 0777);

  for(auto iBrd = 0; iBrd < kgBrds; iBrd++) {
    for(auto iCh = 0; iCh < kgChs; iCh++) {
      auto fileName = dirName + "/" + Form("Brd%02dCh%02d.txt", iBrd, iCh);
      std::cout << fileName << std::endl;
      std::ofstream fout(fileName);

      const auto nBins = fHist[iBrd][iCh]->GetNbinsX();
      for(auto iBin = 1; iBin <= nBins; iBin++) {
	fout << fHist[iBrd][iCh]->GetBinCenter(iBin) <<"\t"
	     << fHist[iBrd][iCh]->GetBinContent(iBin) <<"\n";
      }
      // fout << std::endl;
      fout.close();
    }
  }

}

void Monitor::UploadEventRate(int timeDuration)
{
  for(auto &&brd: fEventCounter) {
    for(auto &&ch: brd) {
      ch /= timeDuration;
    }
  }

  nlohmann::json result;
  nlohmann::json j_array(fEventCounter);
  result["eveRate"] = j_array;
  std::string postData = result.dump();
  // std::cout << postData << std::endl;
  
  
  if(fEveRateAPI != "") {
    auto curl = curl_easy_init();
    if(curl){
      curl_easy_setopt(curl, CURLOPT_URL, fEveRateAPI.c_str());
      curl_easy_setopt(curl, CURLOPT_POST, true);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(postData.c_str()));
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CallbackFunc);
      std::string buf;
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
      auto res = curl_easy_perform(curl);
      curl_easy_cleanup(curl);

      if (res != CURLE_OK) {
	std::cerr << "Communication error in UploadEventRate: " << res << std::endl;
	std::exit (1);
      }
    }
  }
  
  fLastCountTime = time(0);
  for(auto &&brd: fEventCounter) {
    for(auto &&ch: brd) {
      ch = 0;
    }
  }
  
}

extern "C"
{
  void MonitorInit(RTC::Manager* manager)
  {
    RTC::Properties profile(monitor_spec);
    manager->registerFactory(profile,
			     RTC::Create<Monitor>,
			     RTC::Delete<Monitor>);
  }
};
