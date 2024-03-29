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
static const char *dispatcher_spec[] = {"implementation_id",
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

Dispatcher::Dispatcher(RTC::Manager *manager)
    : DAQMW::DaqComponentBase(manager),
      m_InPort1("dispatcher_in1", m_in1_data),
      m_InPort2("dispatcher_in2", m_in2_data),
      m_InPort3("dispatcher_in3", m_in3_data),
      m_InPort4("dispatcher_in4", m_in4_data),
      m_InPort5("dispatcher_in5", m_in5_data),
      m_InPort6("dispatcher_in6", m_in6_data),
      m_InPort7("dispatcher_in7", m_in7_data),
      m_OutPort1("dispatcher_out1", m_out1_data),
      m_OutPort2("dispatcher_out2", m_out2_data),
      m_in_status(BUF_SUCCESS),
      m_in1_status(BUF_SUCCESS),
      m_in2_status(BUF_SUCCESS),
      m_in3_status(BUF_SUCCESS),
      m_in4_status(BUF_SUCCESS),
      m_in5_status(BUF_SUCCESS),
      m_out1_status(BUF_SUCCESS),
      m_out2_status(BUF_SUCCESS),
      m_in1_timeout_counter(0),
      m_in2_timeout_counter(0),
      m_in3_timeout_counter(0),
      m_in4_timeout_counter(0),
      m_in5_timeout_counter(0),
      m_out1_timeout_counter(0),
      m_out2_timeout_counter(0),
      m_inport_recv_data_size(0),
      m_debug(true)
{
  // Registration: InPort/OutPort/Service

  // Set OutPort buffers
  registerInPort("dispatcher_in1", m_InPort1);
  registerInPort("dispatcher_in2", m_InPort2);
  registerInPort("dispatcher_in3", m_InPort3);
  registerInPort("dispatcher_in4", m_InPort4);
  registerInPort("dispatcher_in5", m_InPort5);
  registerInPort("dispatcher_in6", m_InPort6);
  registerInPort("dispatcher_in7", m_InPort7);
  registerOutPort("dispatcher_out1", m_OutPort1);
  registerOutPort("dispatcher_out2", m_OutPort2);

  init_command_port();
  init_state_table();
  set_comp_name("DISPATCHER");
}

Dispatcher::~Dispatcher() {}

RTC::ReturnCode_t Dispatcher::onInitialize()
{
  if (m_debug) {
    std::cerr << "Dispatcher::onInitialize()" << std::endl;
  }

  return RTC::RTC_OK;
}

RTC::ReturnCode_t Dispatcher::onExecute(RTC::UniqueId ec_id)
{
  daq_do();

  return RTC::RTC_OK;
}

int Dispatcher::daq_dummy() { return 0; }

int Dispatcher::daq_configure()
{
  std::cerr << "*** Dispatcher::configure" << std::endl;

  ::NVList *paramList;
  paramList = m_daq_service0.getCompParams();
  parse_params(paramList);

  return 0;
}

int Dispatcher::parse_params(::NVList *list)
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

int Dispatcher::daq_unconfigure()
{
  std::cerr << "*** Dispatcher::unconfigure" << std::endl;

  return 0;
}

int Dispatcher::daq_start()
{
  std::cerr << "*** Dispatcher::start" << std::endl;
  m_in_status = BUF_SUCCESS;
  m_in1_status = BUF_SUCCESS;
  m_in2_status = BUF_SUCCESS;
  m_in3_status = BUF_SUCCESS;
  m_in4_status = BUF_SUCCESS;
  m_in5_status = BUF_SUCCESS;
  m_in6_status = BUF_SUCCESS;
  m_in7_status = BUF_SUCCESS;
  m_out1_status = BUF_SUCCESS;
  m_out2_status = BUF_SUCCESS;

  return 0;
}

int Dispatcher::daq_stop()
{
  std::cerr << "*** Dispatcher::stop" << std::endl;
  reset_InPort();
  return 0;
}

int Dispatcher::daq_pause()
{
  std::cerr << "*** Dispatcher::pause" << std::endl;

  return 0;
}

int Dispatcher::daq_resume()
{
  std::cerr << "*** Dispatcher::resume" << std::endl;

  return 0;
}

int Dispatcher::read_data_from_detectors()
{
  int received_data_size = 0;
  /// write your logic here
  return received_data_size;
}

int Dispatcher::set_data_OutPort1(unsigned int data_byte_size)
{
  ///set OutPort buffer length
  m_out1_data.data.length(data_byte_size);
  memcpy(&(m_out1_data.data[0]), &m_in_data.data[0], data_byte_size);
  return 0;
}

int Dispatcher::set_data_OutPort2(unsigned int data_byte_size)
{
  ///set OutPort buffer length
  m_out2_data.data.length(data_byte_size);
  memcpy(&(m_out2_data.data[0]), &(m_in_data.data[0]), data_byte_size);

  return 0;
}

int Dispatcher::reset_InPort()
{
  TimedOctetSeq dummy_data;

  while (m_InPort1.isEmpty() == false) {
    m_InPort1 >> dummy_data;
  }
  while (m_InPort2.isEmpty() == false) {
    m_InPort2 >> dummy_data;
  }
  while (m_InPort3.isEmpty() == false) {
    m_InPort3 >> dummy_data;
  }
  while (m_InPort4.isEmpty() == false) {
    m_InPort4 >> dummy_data;
  }
  while (m_InPort5.isEmpty() == false) {
    m_InPort5 >> dummy_data;
  }
  while (m_InPort6.isEmpty() == false) {
    m_InPort6 >> dummy_data;
  }
  while (m_InPort7.isEmpty() == false) {
    m_InPort7 >> dummy_data;
  }
  //std::cerr << "*** Dispatcher::InPort flushed\n";
  return 0;
}

unsigned int Dispatcher::read_InPort()
{
  /////////////// read data from InPort Buffer ///////////////
  unsigned int recv_byte_size = 0;
  unsigned int dataSize1 = 0;
  unsigned int dataSize2 = 0;
  unsigned int dataSize3 = 0;
  unsigned int dataSize4 = 0;
  unsigned int dataSize5 = 0;
  unsigned int dataSize6 = 0;
  unsigned int dataSize7 = 0;
  const auto headerFooterSize = HEADER_BYTE_SIZE + FOOTER_BYTE_SIZE;

  // For port 1
  bool ret = m_InPort1.read();
  //////////////////// check read status /////////////////////
  if (ret == false) {  // false: TIMEOUT or FATAL
    m_in1_status = check_inPort_status(m_InPort1);
    if (m_in1_status == BUF_TIMEOUT) {  // Buffer empty.
      m_in1_timeout_counter++;
      if (check_trans_lock()) {  // Check if stop command has come.
        set_trans_unlock();      // Transit to CONFIGURE state.
      }
    } else if (m_in1_status == BUF_FATAL) {  // Fatal error
      fatal_error_report(INPORT_ERROR);
    }
  } else {  // success
    m_in1_timeout_counter = 0;
    dataSize1 = m_in1_data.data.length() - headerFooterSize;
    m_in1_status = BUF_SUCCESS;
  }
  if (m_debug) {
    std::cerr << "m_in1_data.data.length():" << dataSize1 << std::endl;
  }

  // For port 2
  ret = m_InPort2.read();
  //////////////////// check read status /////////////////////
  if (ret == false) {  // false: TIMEOUT or FATAL
    m_in2_status = check_inPort_status(m_InPort2);
    if (m_in2_status == BUF_TIMEOUT) {  // Buffer empty.
      m_in2_timeout_counter++;
      if (check_trans_lock()) {  // Check if stop command has come.
        set_trans_unlock();      // Transit to CONFIGURE state.
      }
    } else if (m_in2_status == BUF_FATAL) {  // Fatal error
      fatal_error_report(INPORT_ERROR);
    }
  } else {  // success
    m_in2_timeout_counter = 0;
    dataSize2 = m_in2_data.data.length() - headerFooterSize;
    m_in2_status = BUF_SUCCESS;
  }
  if (m_debug) {
    std::cerr << "m_in2_data.data.length():" << dataSize2 << std::endl;
  }

  // For port 3
  ret = m_InPort3.read();
  //////////////////// check read status /////////////////////
  if (ret == false) {  // false: TIMEOUT or FATAL
    m_in3_status = check_inPort_status(m_InPort3);
    if (m_in3_status == BUF_TIMEOUT) {  // Buffer empty.
      m_in3_timeout_counter++;
      if (check_trans_lock()) {  // Check if stop command has come.
        set_trans_unlock();      // Transit to CONFIGURE state.
      }
    } else if (m_in3_status == BUF_FATAL) {  // Fatal error
      fatal_error_report(INPORT_ERROR);
    }
  } else {  // success
    m_in3_timeout_counter = 0;
    dataSize3 = m_in3_data.data.length() - headerFooterSize;
    m_in3_status = BUF_SUCCESS;
  }
  if (m_debug) {
    std::cerr << "m_in3_data.data.length():" << dataSize3 << std::endl;
  }

  // For port 4
  ret = m_InPort4.read();
  //////////////////// check read status /////////////////////
  if (ret == false) {  // false: TIMEOUT or FATAL
    m_in4_status = check_inPort_status(m_InPort4);
    if (m_in4_status == BUF_TIMEOUT) {  // Buffer empty.
      m_in4_timeout_counter++;
      if (check_trans_lock()) {  // Check if stop command has come.
        set_trans_unlock();      // Transit to CONFIGURE state.
      }
    } else if (m_in4_status == BUF_FATAL) {  // Fatal error
      fatal_error_report(INPORT_ERROR);
    }
  } else {  // success
    m_in4_timeout_counter = 0;
    dataSize4 = m_in4_data.data.length() - headerFooterSize;
    m_in4_status = BUF_SUCCESS;
  }
  if (m_debug) {
    std::cerr << "m_in4_data.data.length():" << dataSize4 << std::endl;
  }

  // For port 5
  ret = m_InPort5.read();
  //////////////////// check read status /////////////////////
  if (ret == false) {  // false: TIMEOUT or FATAL
    m_in5_status = check_inPort_status(m_InPort5);
    if (m_in5_status == BUF_TIMEOUT) {  // Buffer empty.
      m_in5_timeout_counter++;
      if (check_trans_lock()) {  // Check if stop command has come.
        set_trans_unlock();      // Transit to CONFIGURE state.
      }
    } else if (m_in5_status == BUF_FATAL) {  // Fatal error
      fatal_error_report(INPORT_ERROR);
    }
  } else {  // success
    m_in5_timeout_counter = 0;
    dataSize5 = m_in5_data.data.length() - headerFooterSize;
    m_in5_status = BUF_SUCCESS;
  }
  if (m_debug) {
    std::cerr << "m_in5_data.data.length():" << dataSize5 << std::endl;
  }

  // For port 6
  ret = m_InPort6.read();
  //////////////////// check read status /////////////////////
  if (ret == false) {  // false: TIMEOUT or FATAL
    m_in6_status = check_inPort_status(m_InPort6);
    if (m_in6_status == BUF_TIMEOUT) {  // Buffer empty.
      m_in6_timeout_counter++;
      if (check_trans_lock()) {  // Check if stop command has come.
        set_trans_unlock();      // Transit to CONFIGURE state.
      }
    } else if (m_in6_status == BUF_FATAL) {  // Fatal error
      fatal_error_report(INPORT_ERROR);
    }
  } else {  // success
    m_in6_timeout_counter = 0;
    dataSize6 = m_in6_data.data.length() - headerFooterSize;
    m_in6_status = BUF_SUCCESS;
  }
  if (m_debug) {
    std::cerr << "m_in6_data.data.length():" << dataSize6 << std::endl;
  }

  // For port 7
  ret = m_InPort7.read();
  //////////////////// check read status /////////////////////
  if (ret == false) {  // false: TIMEOUT or FATAL
    m_in7_status = check_inPort_status(m_InPort7);
    if (m_in7_status == BUF_TIMEOUT) {  // Buffer empty.
      m_in7_timeout_counter++;
      if (check_trans_lock()) {  // Check if stop command has come.
        set_trans_unlock();      // Transit to CONFIGURE state.
      }
    } else if (m_in7_status == BUF_FATAL) {  // Fatal error
      fatal_error_report(INPORT_ERROR);
    }
  } else {  // success
    m_in7_timeout_counter = 0;
    dataSize7 = m_in7_data.data.length() - headerFooterSize;
    m_in7_status = BUF_SUCCESS;
  }
  if (m_debug) {
    std::cerr << "m_in7_data.data.length():" << dataSize7 << std::endl;
  }

  // For all ports
  auto totalDataSize = dataSize1 + dataSize2 + dataSize3 + dataSize4 +
                       dataSize5 + dataSize6 + dataSize7;

  if (m_debug) {
    std::cerr << "total size: " << totalDataSize << std::endl;
  }

  if (totalDataSize > 0) {
    // merge data
    unsigned char header[8];
    unsigned char footer[8];

    set_header(&header[0], totalDataSize);
    set_footer(&footer[0]);

    recv_byte_size = totalDataSize + headerFooterSize;
    m_in_data.data.length(recv_byte_size);
    memcpy(&(m_in_data.data[0]), &header[0], HEADER_BYTE_SIZE);
    auto dataIndex = HEADER_BYTE_SIZE;
    if (dataSize1 > 0) {
      memcpy(&(m_in_data.data[dataIndex]), &m_in1_data.data[HEADER_BYTE_SIZE],
             dataSize1);
      dataIndex += dataSize1;
    }
    if (dataSize2 > 0) {
      memcpy(&(m_in_data.data[dataIndex]), &m_in2_data.data[HEADER_BYTE_SIZE],
             dataSize2);
      dataIndex += dataSize2;
    }
    if (dataSize3 > 0) {
      memcpy(&(m_in_data.data[dataIndex]), &m_in3_data.data[HEADER_BYTE_SIZE],
             dataSize3);
      dataIndex += dataSize3;
    }
    if (dataSize4 > 0) {
      memcpy(&(m_in_data.data[dataIndex]), &m_in4_data.data[HEADER_BYTE_SIZE],
             dataSize4);
      dataIndex += dataSize4;
    }
    if (dataSize5 > 0) {
      memcpy(&(m_in_data.data[dataIndex]), &m_in5_data.data[HEADER_BYTE_SIZE],
             dataSize5);
      dataIndex += dataSize5;
    }
    if (dataSize6 > 0) {
      memcpy(&(m_in_data.data[dataIndex]), &m_in6_data.data[HEADER_BYTE_SIZE],
             dataSize6);
      dataIndex += dataSize6;
    }
    if (dataSize7 > 0) {
      memcpy(&(m_in_data.data[dataIndex]), &m_in7_data.data[HEADER_BYTE_SIZE],
             dataSize7);
      dataIndex += dataSize7;
    }
    memcpy(&(m_in_data.data[dataIndex]), &footer[0], FOOTER_BYTE_SIZE);

    if ((m_in1_status == BUF_SUCCESS) || (m_in2_status == BUF_SUCCESS) ||
        (m_in3_status == BUF_SUCCESS) || (m_in4_status == BUF_SUCCESS) ||
        (m_in5_status == BUF_SUCCESS) || (m_in6_status == BUF_SUCCESS) ||
        (m_in7_status == BUF_SUCCESS)) {
      m_in_status = BUF_SUCCESS;
    } else {
      m_in_status = BUF_TIMEOUT;
    }
  }

  return recv_byte_size;
}

int Dispatcher::write_OutPort1()
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
      m_out1_timeout_counter++;
      return -1;
    }
  } else {  // success
    m_out1_timeout_counter = 0;
  }
  return 0;  // successfully done
}

int Dispatcher::write_OutPort2()
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
      m_out2_timeout_counter++;
      return -1;
    }
  } else {  // success
    m_out2_timeout_counter = 0;
  }
  return 0;  // successfully done
}

int Dispatcher::daq_run()
{
  if (m_debug) {
    std::cerr << "*** Dispatcher::run" << std::endl;
  }

  if ((m_out1_status != BUF_TIMEOUT) && (m_out2_status != BUF_TIMEOUT)) {
    m_inport_recv_data_size = read_InPort();
    //std::cout << m_inport_recv_data_size << std::endl;

    if (m_inport_recv_data_size == 0) {  // TIMEOUT
      return 0;
    } else {
      //check_header_footer(m_in_data, m_inport_recv_data_size);
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
    inc_total_data_size(event_data_size);  // increase total data byte size
  }

  return 0;
}

extern "C" {
void DispatcherInit(RTC::Manager *manager)
{
  RTC::Properties profile(dispatcher_spec);
  manager->registerFactory(profile, RTC::Create<Dispatcher>,
                           RTC::Delete<Dispatcher>);
}
};
