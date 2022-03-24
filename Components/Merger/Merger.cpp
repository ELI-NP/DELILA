// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#include "Merger.h"

using DAQMW::FatalType::DATAPATH_DISCONNECTED;
using DAQMW::FatalType::INPORT_ERROR;
using DAQMW::FatalType::OUTPORT_ERROR;
using DAQMW::FatalType::USER_DEFINED_ERROR1;

// Module specification
// Change following items to suit your component's spec.
static const char *dispatcher_spec[] = {"implementation_id",
                                        "Merger",
                                        "type_name",
                                        "Merger",
                                        "description",
                                        "Merger component",
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

Merger::Merger(RTC::Manager *manager)
    : DAQMW::DaqComponentBase(manager),
      m_OutPort1("merger_out1", m_out1_data),
      m_OutPort2("merger_out2", m_out2_data),
      m_out1_status(BUF_SUCCESS),
      m_out2_status(BUF_SUCCESS),
      m_debug(true)
{
  // Registration: InPort/OutPort/Service
  ReadInOutSettings();

  for (auto i = 0; i < fNIn; i++) {
    auto name = "merger_in" + std::to_string(i);
    fInStatusVec.emplace_back(BUF_SUCCESS);
    fInDataVec.emplace_back();
    fInPortVec.emplace_back(
        std::make_unique<InPort<TimedOctetSeq>>(name.c_str(), fInDataVec[i]));
    registerInPort(name.c_str(), *fInPortVec[i]);
  }
  // Set OutPort buffers
  registerOutPort("merger_out1", m_OutPort1);
  registerOutPort("merger_out2", m_OutPort2);

  init_command_port();
  init_state_table();
  set_comp_name("MERGER");
}

Merger::~Merger() {}

void Merger::ReadInOutSettings()
{
  // Something to set these, arguments, read text file, access to DB or etc
  fNIn = 1;
  fNOut = 2;
}

RTC::ReturnCode_t Merger::onInitialize()
{
  if (m_debug) {
    std::cerr << "Merger::onInitialize()" << std::endl;
  }

  return RTC::RTC_OK;
}

RTC::ReturnCode_t Merger::onExecute(RTC::UniqueId ec_id)
{
  daq_do();

  return RTC::RTC_OK;
}

int Merger::daq_dummy() { return 0; }

int Merger::daq_configure()
{
  std::cerr << "*** Merger::configure" << std::endl;

  ::NVList *paramList;
  paramList = m_daq_service0.getCompParams();
  parse_params(paramList);

  return 0;
}

int Merger::parse_params(::NVList *list)
{
  std::cerr << "param list length:" << (*list).length() << std::endl;

  int len = (*list).length();
  for (int i = 0; i < len; i += 2) {
    std::string sname = (std::string)(*list)[i].value;
    std::string svalue = (std::string)(*list)[i + 1].value;

    std::cerr << "sname: " << sname << "  ";
    std::cerr << "value: " << svalue << std::endl;
  }

  return 0;
}

int Merger::daq_unconfigure()
{
  std::cerr << "*** Merger::unconfigure" << std::endl;

  return 0;
}

int Merger::daq_start()
{
  std::cerr << "*** Merger::start" << std::endl;
  for (auto &&status : fInStatusVec) status = BUF_SUCCESS;
  m_out1_status = BUF_SUCCESS;
  m_out2_status = BUF_SUCCESS;

  return 0;
}

int Merger::daq_stop()
{
  std::cerr << "*** Merger::stop" << std::endl;
  reset_InPort();
  return 0;
}

int Merger::daq_pause()
{
  std::cerr << "*** Merger::pause" << std::endl;

  return 0;
}

int Merger::daq_resume()
{
  std::cerr << "*** Merger::resume" << std::endl;

  return 0;
}

int Merger::read_data_from_detectors()
{
  int received_data_size = 0;
  /// write your logic here
  return received_data_size;
}

int Merger::set_data_OutPort1(unsigned int data_byte_size)
{
  /// set OutPort buffer length
  m_out1_data.data.length(data_byte_size);
  memcpy(&(m_out1_data.data[0]), &fInDataTotal.data[0], data_byte_size);
  return 0;
}

int Merger::set_data_OutPort2(unsigned int data_byte_size)
{
  /// set OutPort buffer length
  m_out2_data.data.length(data_byte_size);
  memcpy(&(m_out2_data.data[0]), &(fInDataTotal.data[0]), data_byte_size);

  return 0;
}

int Merger::reset_InPort()
{
  TimedOctetSeq dummy_data;

  while (fInPortVec[0]->isEmpty() == false) {
    *fInPortVec[0] >> dummy_data;
  }
  // std::cerr << "*** Merger::InPort flushed\n";
  return 0;
}

unsigned int Merger::ReadInPorts()
{
  unsigned int dataSize = 0;

  const int nPorts = fInPortVec.size();
  for (auto iPort = 1; iPort < nPorts; iPort++) {
    std::cout << fInPortVec[iPort]->getName() << std::endl;

    auto ret = fInPortVec[iPort]->read();

    if (ret == false) {  // false: TIMEOUT or FATAL
      fInStatusVec[iPort] = check_inPort_status(*fInPortVec[iPort]);
      if (fInStatusVec[iPort] == BUF_TIMEOUT) {  // Buffer empty.
        if (check_trans_lock()) {  // Check if stop command has come.
          set_trans_unlock();      // Transit to CONFIGURE state.
        }
      } else if (fInStatusVec[iPort] == BUF_FATAL) {  // Fatal error
        fatal_error_report(INPORT_ERROR);
      }
    } else {  // success
      auto recvSize = fInDataVec[iPort].data.length();
      check_header_footer(fInDataVec[iPort], recvSize);
      dataSize += get_event_size(recvSize);
      fInStatusVec[iPort] = BUF_SUCCESS;
    }
    if (m_debug) {
      std::cerr << "fInDataVec[" << iPort << "].data.length():" << dataSize
                << std::endl;
    }
  }

  auto totalSize = MergeInData(dataSize);
  return totalSize;
}

unsigned int Merger::MergeInData(unsigned int dataSize)
{
  auto totalSize = dataSize + HEADER_BYTE_SIZE + FOOTER_BYTE_SIZE;

  unsigned char header[HEADER_BYTE_SIZE];
  unsigned char footer[FOOTER_BYTE_SIZE];
  set_header(&header[0], dataSize);
  set_footer(&footer[0]);

  const int nPorts = fInPortVec.size();
  fInDataTotal.data.length(totalSize);

  memcpy(&(fInDataTotal.data[0]), &header[0], HEADER_BYTE_SIZE);
  auto index = HEADER_BYTE_SIZE;
  for (auto iPort = 0; iPort < nPorts; iPort++) {
    auto copySize = get_event_size(fInDataVec[iPort].data.length());
    memcpy(&(fInDataTotal.data[index]),
           &(fInDataVec[iPort].data[HEADER_BYTE_SIZE]), copySize);
    index += copySize;
  }
  memcpy(&(fInDataTotal.data[index]), &footer[0], FOOTER_BYTE_SIZE);

  return totalSize;
}

int Merger::write_OutPort1()
{
  ////////////////// send data from OutPort  //////////////////
  bool ret = m_OutPort1.write();

  //////////////////// check write status /////////////////////
  if (ret == false) {  // TIMEOUT or FATAL
    m_out1_status = check_outPort_status(m_OutPort1);
    if (m_out1_status == BUF_FATAL) {  // Fatal error
      fatal_error_report(OUTPORT_ERROR);
    }
    if (m_out1_status == BUF_TIMEOUT) {  // Timeout
      if (check_trans_lock()) {          // Check if stop command has come.
        set_trans_unlock();              // Transit to CONFIGURE state.
      }
      return -1;
    }
  } else {  // success
    ;
  }
  return 0;  // successfully done
}

int Merger::write_OutPort2()
{
  ////////////////// send data from OutPort  //////////////////
  bool ret = m_OutPort2.write();

  //////////////////// check write status /////////////////////
  if (ret == false) {  // TIMEOUT or FATAL
    m_out2_status = check_outPort_status(m_OutPort2);
    if (m_out2_status == BUF_FATAL) {  // Fatal error
      fatal_error_report(OUTPORT_ERROR);
    }
    if (m_out2_status == BUF_TIMEOUT) {  // Timeout
      if (check_trans_lock()) {          // Check if stop command has come.
        set_trans_unlock();              // Transit to CONFIGURE state.
      }
      return -1;
    }
  } else {  // success
    ;
  }
  return 0;  // successfully done
}

bool Merger::CheckInPorts()
{
  for (auto &&status : fInStatusVec) {
    if (status != BUF_SUCCESS) return false;
  }

  return true;
}

bool Merger::CheckOutPorts()
{
  if (m_out1_status != BUF_SUCCESS) return false;
  if (m_out2_status != BUF_SUCCESS) return false;

  return true;
}

int Merger::daq_run()
{
  if (m_debug) {
    std::cerr << "*** Merger::run" << std::endl;
  }

  unsigned int recvSize = 0;
  if (CheckOutPorts()) {
    recvSize = ReadInPorts();

    set_data_OutPort1(recvSize);
    set_data_OutPort2(recvSize);
  }

  if (CheckInPorts() && (m_out2_status != BUF_TIMEOUT)) {
    if (write_OutPort1() < 0) {  // TIMEOUT
      ;                          // do nothing
    } else {
      m_out1_status = BUF_SUCCESS;
    }
  }

  if (CheckInPorts() && (m_out1_status != BUF_TIMEOUT)) {
    if (write_OutPort2() < 0) {  // TIMEOUT
      ;                          // do nothing
    } else {
      m_out2_status = BUF_SUCCESS;
    }
  }

  if (m_debug) {
    std::cout << "Size: " << get_event_size(recvSize) << "\t"
              << "Sequence: " << get_sequence_num() << std::endl;
  }

  if (CheckInPorts() && CheckOutPorts()) {
    inc_sequence_num();  // increase sequence num.
    if (recvSize > 0)    // increase total data byte size
      inc_total_data_size(get_event_size(recvSize));
  }

  return 0;
}

extern "C" {
void MergerInit(RTC::Manager *manager)
{
  RTC::Properties profile(dispatcher_spec);
  manager->registerFactory(profile, RTC::Create<Merger>, RTC::Delete<Merger>);
}
};
