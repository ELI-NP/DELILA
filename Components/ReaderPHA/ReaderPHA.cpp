// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#include <curl/curl.h>

#include <fstream>
#include <nlohmann/json.hpp>

#include "ReaderPHA.h"
#include "../../TDigiTES/include/TPSDData.hpp"

using DAQMW::FatalType::DATAPATH_DISCONNECTED;
using DAQMW::FatalType::OUTPORT_ERROR;
using DAQMW::FatalType::USER_DEFINED_ERROR1;

// Module specification
// Change following items to suit your component's spec.
static const char *reader_spec[] = {"implementation_id",
                                    "ReaderPHA",
                                    "type_name",
                                    "ReaderPHA",
                                    "description",
                                    "ReaderPHA component",
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

// For curl
size_t CallbackWrite(char *ptr, size_t size, size_t nmemb, std::string *stream)
{
  int dataLength = size * nmemb;
  stream->append(ptr, dataLength);
  return dataLength;
}

ReaderPHA::ReaderPHA(RTC::Manager *manager)
    : DAQMW::DaqComponentBase(manager),
      m_OutPort("reader_out", m_out_data),
      m_recv_byte_size(0),
      m_out_status(BUF_SUCCESS),

      m_debug(false)
{
  // Registration: InPort/OutPort/Service

  // Set OutPort buffers
  registerOutPort("reader_es1_out", m_OutPort);

  init_command_port();
  init_state_table();
  set_comp_name("READER");

  // fDigitizer.reset(new TPHA);

  fData = new unsigned char[1024 * 1024 * 16];

  fConfigFile = "/DAQ/PHA.conf";
  fParameterAPI = "";
}

ReaderPHA::~ReaderPHA() {}

RTC::ReturnCode_t ReaderPHA::onInitialize()
{
  if (m_debug) {
    std::cerr << "ReaderPHA::onInitialize()" << std::endl;
  }

  return RTC::RTC_OK;
}

RTC::ReturnCode_t ReaderPHA::onExecute(RTC::UniqueId ec_id)
{
  daq_do();

  return RTC::RTC_OK;
}

int ReaderPHA::daq_dummy() { return 0; }

int ReaderPHA::daq_configure()
{
  std::cerr << "*** ReaderPHA::configure" << std::endl;

  ::NVList *paramList;
  paramList = m_daq_service0.getCompParams();
  parse_params(paramList);

  fDigitizer.reset(new TPHA);
  fDigitizer->LoadParameters(fConfigFile);
  fDigitizer->OpenDigitizers();
  fDigitizer->InitDigitizers();
  fDigitizer->UseFineTS();
  fDigitizer->AllocateMemory();

  return 0;
}

int ReaderPHA::parse_params(::NVList *list)
{
  std::cerr << "param list length:" << (*list).length() << std::endl;

  int len = (*list).length();
  for (int i = 0; i < len; i += 2) {
    std::string sname = (std::string)(*list)[i].value;
    std::string svalue = (std::string)(*list)[i + 1].value;

    std::cerr << "sname: " << sname << "  ";
    std::cerr << "value: " << svalue << std::endl;

    if (sname == "ConfigFile") {
      fConfigFile = svalue;
    } else if (sname == "StartModNo") {
      fStartModNo = std::stoi(svalue);
    } else if (sname == "ParameterAPI") {
      fParameterAPI = svalue;
    }
  }

  return 0;
}

int ReaderPHA::daq_unconfigure()
{
  std::cerr << "*** ReaderPHA::unconfigure" << std::endl;
  fDigitizer->FreeMemory();
  fDigitizer->CloseDigitizers();

  return 0;
}

int ReaderPHA::daq_start()
{
  std::cerr << "*** ReaderPHA::start" << std::endl;

  m_out_status = BUF_SUCCESS;

  if (fParameterAPI != "") {
    auto curl = curl_easy_init();
    std::string buf;
    curl_easy_setopt(curl, CURLOPT_URL, fParameterAPI.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CallbackWrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    auto ret = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    // code=404 is only from my web server.  For others, using 404 as key is crazy...
    if (ret == CURLE_OK && (buf.find("code=404") == std::string::npos)) {
      auto json = nlohmann::json::parse(buf);
      auto par = fDigitizer->GetParameters();
      const auto nBrds = par.NumBrd;
      const auto nChs = par.NumPhyCh;

      for (auto iBrd = 0 + fStartModNo; iBrd < nBrds + fStartModNo; iBrd++) {
        for (auto iCh = 0; iCh < nChs; iCh++) {
          par.TrapPoleZero[iBrd][iCh] =
              json["PHAPar"][iBrd]["trapPoleZero"][iCh];
          par.TrapFlatTop[iBrd][iCh] = json["PHAPar"][iBrd]["trapFlatTop"][iCh];
          par.TrapRiseTime[iBrd][iCh] =
              json["PHAPar"][iBrd]["trapRiseTime"][iCh];
          par.PeakingTime[iBrd][iCh] = json["PHAPar"][iBrd]["peakingTime"][iCh];
          par.TTFsmoothing[iBrd][iCh] =
              json["PHAPar"][iBrd]["TTFSmoothing"][iCh];
          par.TTFdelay[iBrd][iCh] = json["PHAPar"][iBrd]["signalRiseTime"][iCh];
          par.TrgThreshold[iBrd][iCh] =
              json["PHAPar"][iBrd]["trgThreshold"][iCh];
          par.NsBaseline[iBrd][iCh] = json["PHAPar"][iBrd]["NSBaseline"][iCh];
          par.NSPeak[iBrd][iCh] = json["PHAPar"][iBrd]["NSPeak"][iCh];
          par.PeakHoldOff[iBrd][iCh] = json["PHAPar"][iBrd]["peakHoldOff"][iCh];
          // par.BaseLineHoldOff[iBrd][iCh] = json["PHAPar"][iBrd]["baselineHoldOff"][iCh];
          par.TrgHoldOff = json["PHAPar"][iBrd]["trgHoldOff"][iCh];
          // par.RTDWindow[iBrd][iCh] = json["PHAPar"][iBrd]["RTDWindow"][iCh];
          par.CoincWindow = json["PHAPar"][iBrd]["trgAccWindow"][iCh];
          // par.DigitalGain[iBrd][iCh] = json["PHAPar"][iBrd]["digitalGain"][iCh];
          par.EnergyFineGain[iBrd][iCh] =
              json["PHAPar"][iBrd]["eneFineGain"][iCh];
          par.Decimation[iBrd][iCh] = json["PHAPar"][iBrd]["decimation"][iCh];
        }
      }

      fDigitizer->SetParameters(par);
      fDigitizer->SetPHAPar();
    }
  }

  fDigitizer->Start();
  return 0;
}

int ReaderPHA::daq_stop()
{
  std::cerr << "*** ReaderPHA::stop" << std::endl;

  fDigitizer->Stop();
  /*
  fDigitizer->FreeMemory();
  fDigitizer->CloseDigitizers();
  */
  return 0;
}

int ReaderPHA::daq_pause()
{
  std::cerr << "*** ReaderPHA::pause" << std::endl;

  return 0;
}

int ReaderPHA::daq_resume()
{
  std::cerr << "*** ReaderPHA::resume" << std::endl;

  return 0;
}

int ReaderPHA::read_data_from_detectors()
{
  int received_data_size = 0;
  /// write your logic here

  constexpr auto maxSize = 2000000;  // < 2MB(2 * 1024 * 1024)

  constexpr auto sizeMod = sizeof(PSDData::ModNumber);
  constexpr auto sizeCh = sizeof(PSDData::ChNumber);
  constexpr auto sizeTS = sizeof(PSDData::TimeStamp);
  constexpr auto sizeFineTS = sizeof(PSDData::FineTS);
  constexpr auto sizeEne = sizeof(PSDData::ChargeLong);
  constexpr auto sizeShort = sizeof(PSDData::ChargeShort);
  constexpr auto sizeRL = sizeof(PSDData::RecordLength);
  
  fDigitizer->ReadEvents();
  auto data = fDigitizer->GetData();
  // std::cout << data->size() << std::endl;
  if (data->size() > 0) {
    const auto nData = data->size();

    for (auto i = 0; i < nData; i++) {
      const auto oneHitSize =
        sizeMod + sizeCh + sizeTS + sizeFineTS + sizeEne + sizeShort + sizeRL +
        (sizeof(*(PSDData::Trace1)) * data->at(i)->RecordLength);

      if (data->at(i)->Energy > 0 && data->at(i)->Energy < 32767) {
        auto index = 0;
        std::vector<char> hit;
        hit.resize(oneHitSize);

	PSDData dummy;
	
	dummy.ModNumber = data->at(i)->ModNumber + fStartModNo;
	memcpy(&hit[index], &(dummy.ModNumber), sizeMod);
	index += sizeMod;
	received_data_size += sizeMod;
      
	memcpy(&hit[index], &(data->at(i)->ChNumber), sizeCh);
	index += sizeCh;
	received_data_size += sizeCh;
	
	memcpy(&hit[index], &(data->at(i)->TimeStamp), sizeTS);
	index += sizeTS;
	received_data_size += sizeTS;

	dummy.FineTS = 1000 * data->at(i)->TimeStamp + data->at(i)->FineTS;
	memcpy(&hit[index], &(dummy.FineTS), sizeFineTS);
	index += sizeFineTS;
	received_data_size += sizeFineTS;
	
	memcpy(&hit[index], &(data->at(i)->Energy), sizeEne);
	index += sizeEne;
	received_data_size += sizeEne;

	dummy.ChargeShort = 0;
	memcpy(&hit[index], &(dummy.ChargeShort), sizeShort);
	index += sizeShort;
	received_data_size += sizeShort;
	
	memcpy(&hit[index], &(data->at(i)->RecordLength), sizeRL);
	index += sizeRL;
	received_data_size += sizeRL;
	
	const auto sizeTrace =
          sizeof(*(PSDData::Trace1)) * data->at(i)->RecordLength;
	memcpy(&hit[index], data->at(i)->Trace1, sizeTrace);
	index += sizeTrace;
	received_data_size += sizeTrace;
	
	fDataContainer.AddData(hit);
      }
    }
  }

  return received_data_size;
}

int ReaderPHA::set_data()
{
  unsigned char header[8];
  unsigned char footer[8];

  auto packet = fDataContainer.GetPacket();

  set_header(&header[0], packet.size());
  set_footer(&footer[0]);

  ///set OutPort buffer length
  m_out_data.data.length(packet.size() + HEADER_BYTE_SIZE + FOOTER_BYTE_SIZE);
  memcpy(&(m_out_data.data[0]), &header[0], HEADER_BYTE_SIZE);
  memcpy(&(m_out_data.data[HEADER_BYTE_SIZE]), &packet[0], packet.size());
  memcpy(&(m_out_data.data[HEADER_BYTE_SIZE + packet.size()]), &footer[0],
         FOOTER_BYTE_SIZE);

  return packet.size();
}

int ReaderPHA::write_OutPort()
{
  ////////////////// send data from OutPort  //////////////////
  bool ret = m_OutPort.write();

  //////////////////// check write status /////////////////////
  if (ret == false) {  // TIMEOUT or FATAL
    m_out_status = check_outPort_status(m_OutPort);
    if (m_out_status == BUF_FATAL) {  // Fatal error
      fatal_error_report(OUTPORT_ERROR);
    }
    if (m_out_status == BUF_TIMEOUT) {  // Timeout
      return -1;
    }
  } else {
    m_out_status = BUF_SUCCESS;  // successfully done
  }

  return 0;
}

int ReaderPHA::daq_run()
{
  if (m_debug) {
    std::cerr << "*** ReaderPHA::run" << std::endl;
  }

  if (check_trans_lock()) {  // check if stop command has come
    set_trans_unlock();      // transit to CONFIGURED state
    return 0;
  }

  int sentDataSize = 0;
  if (m_out_status ==
      BUF_SUCCESS) {  // previous OutPort.write() successfully done
    read_data_from_detectors();
    if (fDataContainer.GetSize() > 0) {
      sentDataSize = set_data();  // set data to OutPort Buffer
    } else {
      return 0;
    }
  }

  if (write_OutPort() < 0) {
    ;                                   // Timeout. do nothing.
  } else if (sentDataSize > 0) {        // OutPort write successfully done
    inc_sequence_num();                 // increase sequence num.
    inc_total_data_size(sentDataSize);  // increase total data byte size
  }

  return 0;
}

extern "C" {
void ReaderPHAInit(RTC::Manager *manager)
{
  RTC::Properties profile(reader_spec);
  manager->registerFactory(profile, RTC::Create<ReaderPHA>,
                           RTC::Delete<ReaderPHA>);
}
};
