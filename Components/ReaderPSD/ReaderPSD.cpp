// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#include "ReaderPSD.h"

using DAQMW::FatalType::DATAPATH_DISCONNECTED;
using DAQMW::FatalType::OUTPORT_ERROR;
using DAQMW::FatalType::USER_DEFINED_ERROR1;

// Module specification
// Change following items to suit your component's spec.
static const char *reader_spec[] = {"implementation_id",
                                    "ReaderPSD",
                                    "type_name",
                                    "ReaderPSD",
                                    "description",
                                    "ReaderPSD component",
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

ReaderPSD::ReaderPSD(RTC::Manager *manager)
    : DAQMW::DaqComponentBase(manager),
      m_OutPort("reader_out", m_out_data),
      m_recv_byte_size(0),
      m_out_status(BUF_SUCCESS),

      m_debug(false)
{
  // Registration: InPort/OutPort/Service

  // Set OutPort buffers
  registerOutPort("reader_out", m_OutPort);

  init_command_port();
  init_state_table();
  set_comp_name("READER");

  fData = new unsigned char[1024 * 1024 * 16];

  fConfigFile = "/DAQ/PSD.conf";

  fFlagSWTrg = false;
  fNSWTrg = 0;
}

ReaderPSD::~ReaderPSD() {}

RTC::ReturnCode_t ReaderPSD::onInitialize()
{
  if (m_debug) {
    std::cerr << "ReaderPSD::onInitialize()" << std::endl;
  }

  return RTC::RTC_OK;
}

RTC::ReturnCode_t ReaderPSD::onExecute(RTC::UniqueId ec_id)
{
  daq_do();

  return RTC::RTC_OK;
}

int ReaderPSD::daq_dummy() { return 0; }

int ReaderPSD::daq_configure()
{
  std::cerr << "*** ReaderPSD::configure" << std::endl;

  ::NVList *paramList;
  paramList = m_daq_service0.getCompParams();
  parse_params(paramList);

  fDigitizer.reset(new TPSD);
  fDigitizer->LoadParameters(fConfigFile);
  fDigitizer->OpenDigitizers();
  fDigitizer->InitDigitizers();
  fDigitizer->UseFineTS();
  if (fFlagTrgCounter) fDigitizer->UseTrgCounter(fTrgCounterMod, fTrgCounterCh);
  fDigitizer->AllocateMemory();

  return 0;
}

int ReaderPSD::parse_params(::NVList *list)
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
    } else if (sname == "TrgCounter") {
      auto posMod = svalue.find("mod");
      auto posCh = svalue.find("ch");
      char modNumber[10];
      svalue.copy(modNumber, posCh - (posMod + 3), posMod + 3);
      char chNumber[10];
      svalue.copy(chNumber, 3, posCh + 2);

      fTrgCounterCh = atoi(chNumber);
      fTrgCounterMod = atoi(modNumber);
      std::cout << "Mod: " << fTrgCounterMod << " Ch: " << fTrgCounterCh
                << " uses TrgCounter mode." << std::endl;

      fFlagTrgCounter = true;
    } else if (sname == "SWTrigger") {
      fFlagSWTrg = true;
      fNSWTrg = std::stoi(svalue);
    }
  }

  return 0;
}

int ReaderPSD::daq_unconfigure()
{
  std::cerr << "*** ReaderPSD::unconfigure" << std::endl;

  fDigitizer->FreeMemory();
  fDigitizer->CloseDigitizers();

  return 0;
}

int ReaderPSD::daq_start()
{
  std::cerr << "*** ReaderPSD::start" << std::endl;

  m_out_status = BUF_SUCCESS;

  fDataContainer = TDataContainer();

  // usleep(1000);
  fDigitizer->Start();

  return 0;
}

int ReaderPSD::daq_stop()
{
  std::cerr << "*** ReaderPSD::stop" << std::endl;

  fDigitizer->Stop();

  return 0;
}

int ReaderPSD::daq_pause()
{
  std::cerr << "*** ReaderPSD::pause" << std::endl;

  return 0;
}

int ReaderPSD::daq_resume()
{
  std::cerr << "*** ReaderPSD::resume" << std::endl;

  return 0;
}

int ReaderPSD::read_data_from_detectors()
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

  if (data->size() > 0) {
    const auto nData = data->size();

    for (auto i = 0; i < nData; i++) {
      const auto oneHitSize =
          sizeMod + sizeCh + sizeTS + sizeFineTS + sizeEne + sizeShort +
          sizeRL + (sizeof(*(PSDData::Trace1)) * data->at(i)->RecordLength);

      std::vector<char> hit;
      hit.resize(oneHitSize);
      auto index = 0;

      unsigned char mod = data->at(i)->ModNumber + fStartModNo;
      memcpy(&hit[index], &(mod), sizeMod);
      index += sizeMod;
      received_data_size += sizeMod;

      memcpy(&hit[index], &(data->at(i)->ChNumber), sizeCh);
      index += sizeCh;
      received_data_size += sizeCh;

      memcpy(&hit[index], &(data->at(i)->TimeStamp), sizeTS);
      index += sizeTS;
      received_data_size += sizeTS;

      memcpy(&hit[index], &(data->at(i)->FineTS), sizeFineTS);
      index += sizeFineTS;
      received_data_size += sizeFineTS;

      memcpy(&hit[index], &(data->at(i)->ChargeLong), sizeEne);
      index += sizeEne;
      received_data_size += sizeEne;

      memcpy(&hit[index], &(data->at(i)->ChargeShort), sizeShort);
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

  return received_data_size;
}

int ReaderPSD::set_data()
{
  unsigned char header[8];
  unsigned char footer[8];

  auto packet = fDataContainer.GetPacket();

  set_header(&header[0], packet.size());
  set_footer(&footer[0]);

  /// set OutPort buffer length
  m_out_data.data.length(packet.size() + HEADER_BYTE_SIZE + FOOTER_BYTE_SIZE);
  memcpy(&(m_out_data.data[0]), &header[0], HEADER_BYTE_SIZE);
  memcpy(&(m_out_data.data[HEADER_BYTE_SIZE]), &packet[0], packet.size());
  memcpy(&(m_out_data.data[HEADER_BYTE_SIZE + packet.size()]), &footer[0],
         FOOTER_BYTE_SIZE);

  return packet.size();
}

int ReaderPSD::write_OutPort()
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

int ReaderPSD::daq_run()
{
  if (m_debug) {
    std::cerr << "*** ReaderPSD::run" << std::endl;
  }

  if (check_trans_lock()) {  // check if stop command has come
    set_trans_unlock();      // transit to CONFIGURED state
    return 0;
  }

  if (fFlagSWTrg) {
    for (auto i = 0; i < fNSWTrg; i++) fDigitizer->SendSWTrigger();
  }

  int sentDataSize = 0;
  if (m_out_status ==
      BUF_SUCCESS) {  // previous OutPort.write() successfully done
    if (++fCounter > 50 || fDataContainer.GetSize() == 0) {
      fCounter = 0;
      read_data_from_detectors();
    }
    sentDataSize = set_data();  // set data to OutPort Buffer
  }

  if (write_OutPort() < 0) {
    ;                                   // Timeout. do nothing.
  } else {                              // OutPort write successfully done
    inc_sequence_num();                 // increase sequence num.
    inc_total_data_size(sentDataSize);  // increase total data byte size
  }

  return 0;
}

extern "C" {
void ReaderPSDInit(RTC::Manager *manager)
{
  RTC::Properties profile(reader_spec);
  manager->registerFactory(profile, RTC::Create<ReaderPSD>,
                           RTC::Delete<ReaderPSD>);
}
};
