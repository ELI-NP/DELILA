// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#include "VMEConductor.h"

// cat /tmp/daqmw/log.VMEConductorComp

using DAQMW::FatalType::DATAPATH_DISCONNECTED;
using DAQMW::FatalType::OUTPORT_ERROR;
using DAQMW::FatalType::USER_DEFINED_ERROR1;

// Module specification
// Change following items to suit your component's spec.
static const char *vmeconductor_spec[] = {"implementation_id",
                                          "VMEConductor",
                                          "type_name",
                                          "VMEConductor",
                                          "description",
                                          "VMEConductor component",
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

VMEConductor::VMEConductor(RTC::Manager *manager)
    : DAQMW::DaqComponentBase(manager),
      m_OutPort("vmeconductor_out", m_out_data), m_recv_byte_size(0),
      m_out_status(BUF_SUCCESS),

      m_debug(false),

      evNumber(0) {
  // Registration: InPort/OutPort/Service

  // Set OutPort buffers
  registerOutPort("vmeconductor_out", m_OutPort);

  init_command_port();
  init_state_table();
  set_comp_name("VMECONDUCTOR");
}

VMEConductor::~VMEConductor() {}

RTC::ReturnCode_t VMEConductor::onInitialize() {
  if (m_debug) {
    std::cerr << "VMEConductor::onInitialize()" << std::endl;
  }

  return RTC::RTC_OK;
}

RTC::ReturnCode_t VMEConductor::onExecute(RTC::UniqueId ec_id) {
  daq_do();

  return RTC::RTC_OK;
}

int VMEConductor::daq_dummy() { return 0; }

int VMEConductor::daq_configure() {
  std::cerr << "*** VMEConductor::configure" << std::endl;

  nrMods = 0;

  ::NVList *paramList;
  paramList = m_daq_service0.getCompParams();
  parse_params(paramList);

  // store dlerror() value
  char *error_s;

  contHandle = dlopen(contName.c_str(), RTLD_GLOBAL | RTLD_NOW);
  error_s = dlerror();
  if (error_s != NULL)
    std::cout << "Dlerror dlopen cont code: " << error_s << std::endl;

  auto contClassGen =
      reinterpret_cast<ControllerCreateFnc *>(dlsym(contHandle, "MakeContObj"));
  error_s = dlerror();
  if (error_s != NULL)
    std::cerr << "Dlerror dlsym cont code: " << error_s << std::endl;

  vmeCont = contClassGen();

  vmeCont->utilsVMEinit();

  for (int i = 0; i < nrMods; i += 2) {

    std::cout << "Configure conductor mod " << i / 2 << std::endl;

    // connect to the shared objects
    modHandle = dlopen(modName_v[i / 2].c_str(), RTLD_GLOBAL | RTLD_NOW);
    error_s = dlerror();
    if (error_s != NULL)
      std::cout << "Dlerror dlopen dev code: " << error_s << std::endl;

    // save the handles
    handleSave_v.push_back((uint64_t *)modHandle);

    auto devClassGen =
        reinterpret_cast<DeviceCreateFnc *>(dlsym(modHandle, "MakeDevObj"));
    error_s = dlerror();
    if (error_s != NULL)
      std::cout << "Dlerror dlsym dev code: " << error_s << std::endl;

    vmeDev.push_back(devClassGen());

    vmeCont = vmeDev[i / 2]->mod_configure(std::move(vmeCont), modAddr_v[i / 2],
                                           (i / 2));
  }

  return 0;
}

int VMEConductor::parse_params(::NVList *list) {
  std::cerr << "param list length:" << (*list).length() << std::endl;

  bool nameExists;
  bool addrExists;

  std::string currMod;

  int len = (*list).length();
  for (int i = 0; i < len; i += 2) {
    std::string sname = (std::string)(*list)[i].value;
    std::string svalue = (std::string)(*list)[i + 1].value;

    std::cerr << "sname: " << sname << "  ";
    std::cerr << "value: " << svalue << std::endl;

    // initially we set the values to false
    if (i % 4 == 0) {

      nameExists = false;
      addrExists = false;
    }

    // name and address must be "modNameX" and "modAddrX" where X is a number
    // from 0 to the number of modules here we set the value of X we check for
    currMod = std::to_string(i / 4);

    // check params for modules and their addresses
    if (sname == ("modName" + currMod)) {

      modName_v.push_back(svalue);

      // we found the name so we set nameExists to true
      nameExists = true;

    } else if (sname == ("modAddr" + currMod)) {

      modAddr_v.push_back(stoi(svalue, nullptr, 16));

      // we found the address so we set addrExists to true
      addrExists = true;

    } else if (sname == "controller") {

      contName = svalue;
    }

    // if both the name and address exist the module is valid
    // if not it is not counted
    if (i % 4 != 0) {

      if ((nameExists == false) && (addrExists == true)) {

        modAddr_v.pop_back();

      } else if ((nameExists == true) && (addrExists == false)) {

        modName_v.pop_back();

      } else if ((nameExists == true) && (addrExists == true)) {

        nrMods += 2; // DO NOT CHANGE!!!!!!

        // For some reason dlsym doesn't recognise the function symbols if
        // nrMods is an odd number. I have no idea why. It's working right now so
        // don't change it unless you have figured it out. If it stops working
        // then RIP.
      }
    }
  }

  std::cout << "Number of modules is " << nrMods / 2 << std::endl;

  return 0;
}

int VMEConductor::daq_unconfigure() {
  std::cerr << "*** VMEConductor::unconfigure" << std::endl;

  // store dlerror() value
  char *error_s;

  std::vector<std::unique_ptr<VMEDevice>>().swap(vmeDev);

  for (int i = 0; i < nrMods; i += 2) {
    modHandle = handleSave_v[i / 2];

    vmeDev.pop_back();

    dlclose(modHandle);
    error_s = dlerror();
    if (m_debug && (error_s != NULL))
      std::cout << "Dlerror dlclose config code: " << error_s << std::endl;
  }

  vmeCont->utilsVMEend();

  dlclose(contHandle);

  // reset number of modules
  nrMods = 0;

  return 0;
}

int VMEConductor::daq_start() {
  std::cerr << "*** VMEConductor::start" << std::endl;

  m_out_status = BUF_SUCCESS;

  fDataContainer = TDataContainer(200000000);

  fStartTime = std::chrono::system_clock::now();

  // use the start function
  for (int i = 0; i < nrMods; i += 2) {

    vmeCont = vmeDev[i / 2]->mod_start(std::move(vmeCont));
  }

  return 0;
}

int VMEConductor::daq_stop() {
  std::cerr << "*** VMEConductor::stop" << std::endl;

  auto stopTime = std::chrono::system_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                      stopTime - fStartTime)
                      .count();
  auto dataSize = get_total_byte_size();
  auto dataRate = 1000. * dataSize / duration;

  if (m_debug)
    std::cout << dataRate << " B/s" << std::endl;

  // call stop function

  for (int i = 0; i < nrMods; i += 2) {

    vmeCont = vmeDev[i / 2]->mod_stop(std::move(vmeCont));
  }

  return 0;
}

int VMEConductor::daq_pause() {
  std::cerr << "*** VMEConductor::pause" << std::endl;

  return 0;
}

int VMEConductor::daq_resume() {
  std::cerr << "*** VMEConductor::resume" << std::endl;

  return 0;
}

int VMEConductor::read_data_from_detectors() {
  int received_data_size = 0;
  /// write your logic here

  constexpr auto sizeMod = sizeof(TreeData::Mod);
  constexpr auto sizeCh = sizeof(TreeData::Ch);
  constexpr auto sizeTS = sizeof(TreeData::TimeStamp);
  constexpr auto sizeFineTS = sizeof(TreeData::FineTS);
  constexpr auto sizeLong = sizeof(TreeData::ChargeLong);
  constexpr auto sizeShort = sizeof(TreeData::ChargeShort);
  constexpr auto sizeRL = sizeof(TreeData::RecordLength);

  TreeData *data = new TreeData();

  // run for each module connected to the CAEN controller
  for (int iter = 0; iter < nrMods; iter += 2) {

    // std::shared_ptr<std::vector<TreeData>> t_data_v;
    std::shared_ptr<std::vector<TreeData>> t_data_v =
        std::make_shared<std::vector<TreeData>>();

    modHandle = handleSave_v[iter / 2];

    // check if there is any data to read
    // some modules, like V812 don't send data
    if (vmeDev[iter / 2]->mod_checkifreader() == 1) {

      // call the run function
      vmeCont =
          vmeDev[iter / 2]->mod_run(std::move(vmeCont), t_data_v, &fNEvents);

      for (auto i = 0; i < fNEvents; i++) {

        data->Mod = t_data_v->at(i).Mod;
        data->Ch = t_data_v->at(i).Ch;
        data->FineTS = t_data_v->at(i).FineTS;
        data->TimeStamp = t_data_v->at(i).TimeStamp;
        data->ChargeLong = t_data_v->at(i).ChargeLong;
        data->ChargeShort = t_data_v->at(i).ChargeShort;
        data->Extras = t_data_v->at(i).Extras;
        data->RecordLength = t_data_v->at(i).RecordLength;

        if (m_debug)
          std::cout << "la i " << i << " avem ts " << data->FineTS << " si "
                    << data->TimeStamp << std::endl;

        const auto oneHitSize =
            sizeMod + sizeCh + sizeTS + sizeFineTS + sizeLong + sizeShort +
            sizeRL + (sizeof(TreeData::Trace1[0]) * data->RecordLength);

        std::vector<char> hit;
        hit.resize(oneHitSize);
        auto index = 0;

        memcpy(&hit[index], &(data->Mod), sizeMod);
        index += sizeMod;
        received_data_size += sizeMod;

        memcpy(&hit[index], &(data->Ch), sizeCh);
        index += sizeCh;
        received_data_size += sizeCh;

        memcpy(&hit[index], &(data->TimeStamp), sizeTS);
        index += sizeTS;
        received_data_size += sizeTS;

        memcpy(&hit[index], &(data->FineTS), sizeFineTS);
        index += sizeFineTS;
        received_data_size += sizeFineTS;

        memcpy(&hit[index], &(data->ChargeLong), sizeLong);
        index += sizeLong;
        received_data_size += sizeLong;

        memcpy(&hit[index], &(data->ChargeShort), sizeShort);
        index += sizeShort;
        received_data_size += sizeShort;

        memcpy(&hit[index], &(data->RecordLength), sizeRL);
        index += sizeRL;
        received_data_size += sizeRL;

        if (data->RecordLength > 0) {
          const auto sizeTrace =
              sizeof(TreeData::Trace1[0]) * data->RecordLength;
          data->Trace1.resize(data->RecordLength);

          for (auto iSample = 0; iSample < data->RecordLength; iSample++) {

            data->Trace1[iSample] = 0;
          }
          memcpy(&hit[index], &(data->Trace1[0]), sizeTrace);
          index += sizeTrace;
          received_data_size += sizeTrace;
        }

        fDataContainer.AddData(hit);
      }
    }
  }

  if (m_debug) {
    std::cerr << received_data_size << std::endl;
  }

  return received_data_size;
}

int VMEConductor::set_data() {
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

int VMEConductor::write_OutPort() {
  ////////////////// send data from OutPort  //////////////////
  bool ret = m_OutPort.write();

  //////////////////// check write status /////////////////////
  if (ret == false) { // TIMEOUT or FATAL
    m_out_status = check_outPort_status(m_OutPort);
    if (m_out_status == BUF_FATAL) { // Fatal error
      fatal_error_report(OUTPORT_ERROR);
    }
    if (m_out_status == BUF_TIMEOUT) { // Timeout
      return -1;
    }
  } else {
    m_out_status = BUF_SUCCESS; // successfully done
  }

  return 0;
}

int VMEConductor::daq_run() {
  if (m_debug) {
    std::cerr << "*** VMEConductor::run" << std::endl;
  }

  if (check_trans_lock()) { // check if stop command has come
    set_trans_unlock();     // transit to CONFIGURED state
    return 0;
  }

  int sentDataSize = 0;

  if (m_out_status ==
      BUF_SUCCESS) { // previous OutPort.write() successfully done
    if (fDataContainer.GetSize() == 0) {
      fCounter = 0;
      read_data_from_detectors();
    }
    sentDataSize = set_data(); // set data to OutPort Buffer
  }

  if (m_debug) {
    std::cout << "Size: " << sentDataSize << "\t"
              << "Sequence: " << get_sequence_num() << std::endl;
  }

  if (write_OutPort() < 0) {
    if (m_debug) {
      std::cout << m_out_status << std::endl;
    }
    // } else if (sentDataSize > 0) {  // OutPort write successfully done
  } else {                             // OutPort write successfully done
    inc_sequence_num();                // increase sequence num.
    inc_total_data_size(sentDataSize); // increase total data byte size
  }

  return 0;
}

extern "C" {
void VMEConductorInit(RTC::Manager *manager) {
  RTC::Properties profile(vmeconductor_spec);
  manager->registerFactory(profile, RTC::Create<VMEConductor>,
                           RTC::Delete<VMEConductor>);
}
};
