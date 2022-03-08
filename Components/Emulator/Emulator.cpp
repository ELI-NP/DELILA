// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */
#include "Emulator.h"

#include <cfloat>
#include <climits>

#include "../include/TreeData.h"

using DAQMW::FatalType::DATAPATH_DISCONNECTED;
using DAQMW::FatalType::OUTPORT_ERROR;
using DAQMW::FatalType::USER_DEFINED_ERROR1;

// Module specification
// Change following items to suit your component's spec.
static const char *emulator_spec[] = {"implementation_id",
                                      "Emulator",
                                      "type_name",
                                      "Emulator",
                                      "description",
                                      "Emulator component",
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

Emulator::Emulator(RTC::Manager *manager)
    : DAQMW::DaqComponentBase(manager),
      m_OutPort("emulator_out", m_out_data),
      m_recv_byte_size(0),
      m_out_status(BUF_SUCCESS),

      m_debug(true)
{
  // Registration: InPort/OutPort/Service

  // Set OutPort buffers
  registerOutPort("emulator_out", m_OutPort);

  init_command_port();
  init_state_table();
  set_comp_name("EMULATOR");

  std::random_device seedGen;
  fRandom.seed(seedGen());

  fNEvents = 1000;
}

Emulator::~Emulator() {}

RTC::ReturnCode_t Emulator::onInitialize()
{
  if (m_debug) {
    std::cerr << "Emulator::onInitialize()" << std::endl;
  }

  return RTC::RTC_OK;
}

RTC::ReturnCode_t Emulator::onExecute(RTC::UniqueId ec_id)
{
  daq_do();

  return RTC::RTC_OK;
}

int Emulator::daq_dummy() { return 0; }

int Emulator::daq_configure()
{
  std::cerr << "*** Emulator::configure" << std::endl;

  ::NVList *paramList;
  paramList = m_daq_service0.getCompParams();
  parse_params(paramList);

  return 0;
}

int Emulator::parse_params(::NVList *list)
{
  std::cerr << "param list length:" << (*list).length() << std::endl;

  int len = (*list).length();
  for (int i = 0; i < len; i += 2) {
    std::string sname = (std::string)(*list)[i].value;
    std::string svalue = (std::string)(*list)[i + 1].value;

    std::cerr << "sname: " << sname << "  ";
    std::cerr << "value: " << svalue << std::endl;

    if (sname == "NEvents") {
      fNEvents = std::stoi(svalue);
    }
  }

  return 0;
}

int Emulator::daq_unconfigure()
{
  std::cerr << "*** Emulator::unconfigure" << std::endl;

  return 0;
}

int Emulator::daq_start()
{
  std::cerr << "*** Emulator::start" << std::endl;

  m_out_status = BUF_SUCCESS;

  fDataContainer = TDataContainer(200000000);

  fStartTime = std::chrono::system_clock::now();

  return 0;
}

int Emulator::daq_stop()
{
  std::cerr << "*** Emulator::stop" << std::endl;

  auto stopTime = std::chrono::system_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                      stopTime - fStartTime)
                      .count();
  auto dataSize = get_total_byte_size();
  auto dataRate = 1000. * dataSize / duration;

  std::cout << dataRate << " B/s" << std::endl;

  return 0;
}

int Emulator::daq_pause()
{
  std::cerr << "*** Emulator::pause" << std::endl;

  return 0;
}

int Emulator::daq_resume()
{
  std::cerr << "*** Emulator::resume" << std::endl;

  return 0;
}

int Emulator::read_data_from_detectors()
{
  if (m_debug) {
    std::cerr << "Generate data" << std::endl;
  }
  int received_data_size = 0;
  /// write your logic here

  constexpr auto sizeMod = sizeof(TreeData::Mod);
  constexpr auto sizeCh = sizeof(TreeData::Ch);
  constexpr auto sizeTS = sizeof(TreeData::TimeStamp);
  constexpr auto sizeFineTS = sizeof(TreeData::FineTS);
  constexpr auto sizeLong = sizeof(TreeData::ChargeLong);
  constexpr auto sizeShort = sizeof(TreeData::ChargeShort);
  constexpr auto sizeRL = sizeof(TreeData::RecordLength);

  TreeData data;

  std::uniform_int_distribution<> doOrNot(0, 9);
  std::uniform_int_distribution<> rand8(0, 7);
  std::uniform_int_distribution<> rand16(0, 15);
  std::uniform_int_distribution<> randInt(0, INT_MAX);
  std::uniform_real_distribution<> randDouble(0., DBL_MAX);
  std::normal_distribution<> randGaussian(1000.0, 100.0);

  if (doOrNot(fRandom) == 0) {
    for (auto i = 0; i < fNEvents; i++) {
      data.Mod = rand8(fRandom);
      data.Ch = rand16(fRandom);
      data.TimeStamp = randInt(fRandom);
      data.FineTS = randDouble(fRandom);
      data.ChargeLong = randGaussian(fRandom);
      data.ChargeShort = randGaussian(fRandom);
      data.RecordLength = 0;

      const auto oneHitSize = sizeMod + sizeCh + sizeTS + sizeFineTS +
                              sizeLong + sizeShort + sizeRL +
                              (sizeof(TreeData::Trace1[0]) * data.RecordLength);

      std::vector<char> hit;
      hit.resize(oneHitSize);
      auto index = 0;

      memcpy(&hit[index], &(data.Mod), sizeMod);
      index += sizeMod;
      received_data_size += sizeMod;

      memcpy(&hit[index], &(data.Ch), sizeCh);
      index += sizeCh;
      received_data_size += sizeCh;

      memcpy(&hit[index], &(data.TimeStamp), sizeTS);
      index += sizeTS;
      received_data_size += sizeTS;

      memcpy(&hit[index], &(data.FineTS), sizeFineTS);
      index += sizeFineTS;
      received_data_size += sizeFineTS;

      memcpy(&hit[index], &(data.ChargeLong), sizeLong);
      index += sizeLong;
      received_data_size += sizeLong;

      memcpy(&hit[index], &(data.ChargeShort), sizeShort);
      index += sizeShort;
      received_data_size += sizeShort;

      memcpy(&hit[index], &(data.RecordLength), sizeRL);
      index += sizeRL;
      received_data_size += sizeRL;

      if (data.RecordLength > 0) {
        const auto sizeTrace = sizeof(TreeData::Trace1[0]) * data.RecordLength;
        memcpy(&hit[index], &(data.Trace1[0]), sizeTrace);
        index += sizeTrace;
        received_data_size += sizeTrace;
      }

      fDataContainer.AddData(hit);
    }
  }

  if (m_debug) {
    std::cerr << received_data_size << std::endl;
  }

  return received_data_size;
}

int Emulator::set_data()
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

int Emulator::write_OutPort()
{
  if (m_debug) {
    std::cerr << "*** Emulator::write_Outport" << std::endl;
  }
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

int Emulator::daq_run()
{
  if (m_debug) {
    std::cerr << "*** Emulator::run" << std::endl;
  }

  if (check_trans_lock()) {  // check if stop command has come
    set_trans_unlock();      // transit to CONFIGURED state
    return 0;
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

  if (m_debug) {
    std::cout << "Size: " << sentDataSize << "\t"
              << "Sequence: " << get_sequence_num() << std::endl;
  }

  if (write_OutPort() < 0) {
    std::cout << m_out_status << std::endl;
    // } else if (sentDataSize > 0) {  // OutPort write successfully done
  } else {                              // OutPort write successfully done
    inc_sequence_num();                 // increase sequence num.
    inc_total_data_size(sentDataSize);  // increase total data byte size
  }

  return 0;
}

extern "C" {
void EmulatorInit(RTC::Manager *manager)
{
  RTC::Properties profile(emulator_spec);
  manager->registerFactory(profile, RTC::Create<Emulator>,
                           RTC::Delete<Emulator>);
}
};
