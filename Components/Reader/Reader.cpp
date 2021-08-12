// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#include "Reader.h"

using DAQMW::FatalType::DATAPATH_DISCONNECTED;
using DAQMW::FatalType::OUTPORT_ERROR;
using DAQMW::FatalType::USER_DEFINED_ERROR1;

// Module specification
// Change following items to suit your component's spec.
static const char* reader_spec[] =
  {
   "implementation_id", "Reader",
   "type_name",         "Reader",
   "description",       "Reader component",
   "version",           "1.0",
   "vendor",            "Kazuo Nakayoshi, KEK",
   "category",          "example",
   "activity_type",     "DataFlowComponent",
   "max_instance",      "1",
   "language",          "C++",
   "lang_type",         "compile",
   ""
  };

Reader::Reader(RTC::Manager* manager)
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

  // fDigitizer.reset(new TPHA);

  fData = new unsigned char[1024 * 1024 * 16];

  fConfigFile = "/DAQ/PHA.conf";
}

Reader::~Reader()
{
}

RTC::ReturnCode_t Reader::onInitialize()
{
  if (m_debug) {
    std::cerr << "Reader::onInitialize()" << std::endl;
  }

  return RTC::RTC_OK;
}

RTC::ReturnCode_t Reader::onExecute(RTC::UniqueId ec_id)
{
  daq_do();

  return RTC::RTC_OK;
}

int Reader::daq_dummy()
{
  return 0;
}

int Reader::daq_configure()
{
  std::cerr << "*** Reader::configure" << std::endl;

  ::NVList* paramList;
  paramList = m_daq_service0.getCompParams();
  parse_params(paramList);

  // fDigitizer->LoadParameters(fConfigFile);
  // fDigitizer->OpenDigitizers();
  // fDigitizer->InitDigitizers();
  // fDigitizer->AllocateMemory();
    
  return 0;
}

int Reader::parse_params(::NVList* list)
{
  std::cerr << "param list length:" << (*list).length() << std::endl;

  int len = (*list).length();
  for (int i = 0; i < len; i+=2) {
    std::string sname  = (std::string)(*list)[i].value;
    std::string svalue = (std::string)(*list)[i+1].value;

    std::cerr << "sname: " << sname << "  ";
    std::cerr << "value: " << svalue << std::endl;

    if(sname == "ConfigFile") {
      fConfigFile = svalue;
    } else if (sname == "StartModNo") {
      fStartModNo = std::stoi(svalue);
    }
  }

  return 0;
}

int Reader::daq_unconfigure()
{
  std::cerr << "*** Reader::unconfigure" << std::endl;
  // fDigitizer->FreeMemory();
  // fDigitizer->CloseDigitizers();
   
  return 0;
}

int Reader::daq_start()
{
  std::cerr << "*** Reader::start" << std::endl;

  m_out_status = BUF_SUCCESS;

  fDigitizer.reset(new TPHA);
  fDigitizer->LoadParameters(fConfigFile);
  fDigitizer->OpenDigitizers();
  fDigitizer->InitDigitizers();
  fDigitizer->AllocateMemory();

  fDigitizer->Start();
  
  return 0;
}

int Reader::daq_stop()
{
  std::cerr << "*** Reader::stop" << std::endl;

  fDigitizer->Stop();

  fDigitizer->FreeMemory();
  fDigitizer->CloseDigitizers();
 
  return 0;
}

int Reader::daq_pause()
{
  std::cerr << "*** Reader::pause" << std::endl;

  return 0;
}

int Reader::daq_resume()
{
  std::cerr << "*** Reader::resume" << std::endl;

  return 0;
}

int Reader::read_data_from_detectors()
{
  int received_data_size = 0;
  /// write your logic here

  constexpr auto maxSize = 2000000; // < 2MB(2 * 1024 * 1024)
  
  constexpr auto sizeMod = sizeof(PHAData::ModNumber);
  constexpr auto sizeCh = sizeof(PHAData::ChNumber);
  constexpr auto sizeTS = sizeof(PHAData::TimeStamp);
  constexpr auto sizeEne = sizeof(PHAData::Energy);
  constexpr auto sizeRL = sizeof(PHAData::RecordLength);
  
  fDigitizer->ReadEvents();
  auto data = fDigitizer->GetData();
  // std::cout << data->size() << std::endl;
  if(data->size() > 0) {
    const auto oneHitSize = sizeMod + sizeCh + sizeTS + sizeEne + sizeRL
      + (sizeof(*(PHAData::Trace1)) * data->at(0)->RecordLength);
    
    const auto nData = data->size();
    auto index = 0;
    for(auto i = 0; i < nData; i++) {
      if(received_data_size + oneHitSize > maxSize) break;

      unsigned char mod = data->at(i)->ModNumber + fStartModNo;
      memcpy(&fData[index], &(mod), sizeMod);
      index += sizeMod;
      received_data_size += sizeMod;

      memcpy(&fData[index], &(data->at(i)->ChNumber), sizeCh);
      index += sizeCh;
      received_data_size += sizeCh;

      memcpy(&fData[index], &(data->at(i)->TimeStamp), sizeTS);
      index += sizeTS;
      received_data_size += sizeTS;

      memcpy(&fData[index], &(data->at(i)->Energy), sizeEne);
      index += sizeEne;
      received_data_size += sizeEne;

      memcpy(&fData[index], &(data->at(i)->RecordLength), sizeRL);
      index += sizeRL;
      received_data_size += sizeRL;
      
      const auto sizeTrace = sizeof(*(PHAData::Trace1)) * data->at(i)->RecordLength;
      memcpy(&fData[index], data->at(i)->Trace1, sizeTrace);
      index += sizeTrace;
      received_data_size += sizeTrace;    
    }
  }
  
  return received_data_size;
}

int Reader::set_data(unsigned int data_byte_size)
{
  unsigned char header[8];
  unsigned char footer[8];

  set_header(&header[0], data_byte_size);
  set_footer(&footer[0]);

  ///set OutPort buffer length
  m_out_data.data.length(data_byte_size + HEADER_BYTE_SIZE + FOOTER_BYTE_SIZE);
  memcpy(&(m_out_data.data[0]), &header[0], HEADER_BYTE_SIZE);
  memcpy(&(m_out_data.data[HEADER_BYTE_SIZE]), &fData[0], data_byte_size);
  memcpy(&(m_out_data.data[HEADER_BYTE_SIZE + data_byte_size]), &footer[0],
	 FOOTER_BYTE_SIZE);

  return 0;
}

int Reader::write_OutPort()
{
  ////////////////// send data from OutPort  //////////////////
  bool ret = m_OutPort.write();

  //////////////////// check write status /////////////////////
  if (ret == false) {  // TIMEOUT or FATAL
    m_out_status  = check_outPort_status(m_OutPort);
    if (m_out_status == BUF_FATAL) {   // Fatal error
      fatal_error_report(OUTPORT_ERROR);
    }
    if (m_out_status == BUF_TIMEOUT) { // Timeout
      return -1;
    }
  }
  else {
    m_out_status = BUF_SUCCESS; // successfully done
  }

  return 0;
}

int Reader::daq_run()
{
  if (m_debug) {
    std::cerr << "*** Reader::run" << std::endl;
  }

  if (check_trans_lock()) {  // check if stop command has come
    set_trans_unlock();    // transit to CONFIGURED state
    return 0;
  }

  if (m_out_status == BUF_SUCCESS) {   // previous OutPort.write() successfully done
    m_recv_byte_size = read_data_from_detectors();
    // std::cout << m_recv_byte_size << std::endl;
    if (m_recv_byte_size > 0) {
      set_data(m_recv_byte_size); // set data to OutPort Buffer
    } else {
      return 0;
    }
  }

  if (write_OutPort() < 0) {
    ;     // Timeout. do nothing.
  }
  else if(m_recv_byte_size > 0) {    // OutPort write successfully done
    inc_sequence_num();                    // increase sequence num.
    inc_total_data_size(m_recv_byte_size);  // increase total data byte size
  }

  return 0;
}

extern "C"
{
  void ReaderInit(RTC::Manager* manager)
  {
    RTC::Properties profile(reader_spec);
    manager->registerFactory(profile,
			     RTC::Create<Reader>,
			     RTC::Delete<Reader>);
  }
};
