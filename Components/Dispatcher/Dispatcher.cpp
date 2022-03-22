// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#include "Dispatcher.h"

using DAQMW::FatalType::DATAPATH_DISCONNECTED;
using DAQMW::FatalType::INPORT_ERROR;
using DAQMW::FatalType::OUTPORT_ERROR;
using DAQMW::FatalType::USER_DEFINED_ERROR1;

// Module specification
// Change following items to suit your component's spec.
static const char* dispatcher_spec[] = {"implementation_id",
                                        "Dispatcher",
                                        "type_name",
                                        "Dispatcher",
                                        "description",
                                        "Dispatcher component",
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

Dispatcher::Dispatcher(RTC::Manager* manager)
    : DAQMW::DaqComponentBase(manager),
      m_InPort("dispatcher_in", m_in_data),
      m_OutPort1("dispatcher_out1", m_out1_data),
      m_OutPort2("dispatcher_out2", m_out2_data),
      m_in_status(BUF_SUCCESS),
      m_out1_status(BUF_SUCCESS),
      m_out2_status(BUF_SUCCESS),
      m_in_timeout_counter(0),
      m_out1_timeout_counter(0),
      m_out2_timeout_counter(0),
      m_inport_recv_data_size(0),
      m_debug(false) {
  // Registration: InPort/OutPort/Service

  // Set OutPort buffers
  registerInPort("dispatcher_in", m_InPort);
  registerOutPort("dispatcher_out1", m_OutPort1);
  registerOutPort("dispatcher_out2", m_OutPort2);

  init_command_port();
  init_state_table();
  set_comp_name("DISPATCHER");
}

Dispatcher::~Dispatcher() {}

RTC::ReturnCode_t Dispatcher::onInitialize() {
  if (m_debug) {
    std::cerr << "Dispatcher::onInitialize()" << std::endl;
  }

  return RTC::RTC_OK;
}

RTC::ReturnCode_t Dispatcher::onExecute(RTC::UniqueId ec_id) {
  daq_do();

  return RTC::RTC_OK;
}

int Dispatcher::daq_dummy() { return 0; }

int Dispatcher::daq_configure() {
  std::cerr << "*** Dispatcher::configure" << std::endl;

  ::NVList* paramList;
  paramList = m_daq_service0.getCompParams();
  parse_params(paramList);

  return 0;
}

int Dispatcher::parse_params(::NVList* list) {
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

int Dispatcher::daq_unconfigure() {
  std::cerr << "*** Dispatcher::unconfigure" << std::endl;

  return 0;
}

int Dispatcher::daq_start() {
  std::cerr << "*** Dispatcher::start" << std::endl;
  m_in_status = BUF_SUCCESS;
  m_out1_status = BUF_SUCCESS;
  m_out2_status = BUF_SUCCESS;

  return 0;
}

int Dispatcher::daq_stop() {
  std::cerr << "*** Dispatcher::stop" << std::endl;
  reset_InPort();
  return 0;
}

int Dispatcher::daq_pause() {
  std::cerr << "*** Dispatcher::pause" << std::endl;

  return 0;
}

int Dispatcher::daq_resume() {
  std::cerr << "*** Dispatcher::resume" << std::endl;

  return 0;
}

int Dispatcher::read_data_from_detectors() {
  int received_data_size = 0;
  /// write your logic here
  return received_data_size;
}

int Dispatcher::set_data_OutPort1(unsigned int data_byte_size) {
  /// set OutPort buffer length
  m_out1_data.data.length(data_byte_size);
  memcpy(&(m_out1_data.data[0]), &m_in_data.data[0], data_byte_size);
  return 0;
}

int Dispatcher::set_data_OutPort2(unsigned int data_byte_size) {
  /// set OutPort buffer length
  m_out2_data.data.length(data_byte_size);
  memcpy(&(m_out2_data.data[0]), &(m_in_data.data[0]), data_byte_size);

  return 0;
}

int Dispatcher::reset_InPort() {
  TimedOctetSeq dummy_data;

  while (m_InPort.isEmpty() == false) {
    m_InPort >> dummy_data;
  }
  // std::cerr << "*** Dispatcher::InPort flushed\n";
  return 0;
}

unsigned int Dispatcher::read_InPort() {
  /////////////// read data from InPort Buffer ///////////////
  unsigned int recv_byte_size = 0;
  bool ret = m_InPort.read();

  //////////////////// check read status /////////////////////
  if (ret == false) {  // false: TIMEOUT or FATAL
    m_in_status = check_inPort_status(m_InPort);
    if (m_in_status == BUF_TIMEOUT) {  // Buffer empty.
      m_in_timeout_counter++;
      if (check_trans_lock()) {  // Check if stop command has come.
        set_trans_unlock();      // Transit to CONFIGURE state.
      }
    } else if (m_in_status == BUF_FATAL) {  // Fatal error
      fatal_error_report(INPORT_ERROR);
    }
  } else {  // success
    m_in_timeout_counter = 0;
    recv_byte_size = m_in_data.data.length();
    m_in_status = BUF_SUCCESS;
  }
  if (m_debug) {
    std::cerr << "m_in_data.data.length():" << recv_byte_size << std::endl;
  }

  return recv_byte_size;
}

int Dispatcher::write_OutPort1() {
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
      m_out1_timeout_counter++;
      return -1;
    }
  } else {  // success
    m_out1_timeout_counter = 0;
  }
  return 0;  // successfully done
}

int Dispatcher::write_OutPort2() {
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
      m_out2_timeout_counter++;
      return -1;
    }
  } else {  // success
    m_out2_timeout_counter = 0;
  }
  return 0;  // successfully done
}

int Dispatcher::daq_run() {
  if (m_debug) {
    std::cerr << "*** Dispatcher::run" << std::endl;
  }

  if ((m_out1_status != BUF_TIMEOUT) && (m_out2_status != BUF_TIMEOUT)) {
    m_inport_recv_data_size = read_InPort();

    if (m_inport_recv_data_size == 0) {  // TIMEOUT
      return 0;
    } else {
      check_header_footer(m_in_data, m_inport_recv_data_size);
      set_data_OutPort1(m_inport_recv_data_size);
      set_data_OutPort2(m_inport_recv_data_size);
    }
  }

  if ((m_in_status != BUF_TIMEOUT) && (m_out2_status != BUF_TIMEOUT)) {
    if (write_OutPort1() < 0) {  // TIMEOUT
      ;                          // do nothing
    } else {
      m_out1_status = BUF_SUCCESS;
    }
  }

  if ((m_in_status != BUF_TIMEOUT) && (m_out1_status != BUF_TIMEOUT)) {
    if (write_OutPort2() < 0) {  // TIMEOUT
      ;                          // do nothing
    } else {
      m_out2_status = BUF_SUCCESS;
    }
  }

  if ((m_in_status != BUF_TIMEOUT) && (m_out1_status != BUF_TIMEOUT) &&
      (m_out2_status != BUF_TIMEOUT)) {
    inc_sequence_num();  // increase sequence num.
    unsigned int event_data_size = get_event_size(m_inport_recv_data_size);
    if (event_data_size > 0)
      inc_total_data_size(event_data_size);  // increase total data byte size
  }

  return 0;
}

extern "C" {
void DispatcherInit(RTC::Manager* manager) {
  RTC::Properties profile(dispatcher_spec);
  manager->registerFactory(profile, RTC::Create<Dispatcher>,
                           RTC::Delete<Dispatcher>);
}
};
