// -*- C++ -*-
/*!
 * @file 
 * @brief
 * @date
 * @author
 *
 */

#ifndef VMECONDUCTOR_H
#define VMECONDUCTOR_H


#include <chrono>
#include <deque>
#include <memory>
#include <string>
#include <vector>
#include <dlfcn.h>

#include "DaqComponentBase.h"
#include "../include/TDataContainer.hpp"
#include "../include/TreeData.h"
#include "../include/VMEDevice.hpp"
#include "../include/VMEController.hpp"

using namespace RTC;

class VMEConductor
    : public DAQMW::DaqComponentBase
{
public:
    VMEConductor(RTC::Manager* manager);
    ~VMEConductor();

    // The initialize action (on CREATED->ALIVE transition)
    // former rtc_init_entry()
    virtual RTC::ReturnCode_t onInitialize();

    // The execution action that is invoked periodically
    // former rtc_active_do()
    virtual RTC::ReturnCode_t onExecute(RTC::UniqueId ec_id);

private:
    TimedOctetSeq          m_out_data;
    OutPort<TimedOctetSeq> m_OutPort;

private:
    int daq_dummy();
    int daq_configure();
    int daq_unconfigure();
    int daq_start();
    int daq_run();
    int daq_stop();
    int daq_pause();
    int daq_resume();

    int parse_params(::NVList* list);
    int read_data_from_detectors();
    int set_data();
    int write_OutPort();

    static const int SEND_BUFFER_SIZE = 4096;
    unsigned char m_data[SEND_BUFFER_SIZE];
    unsigned int m_recv_byte_size;

    BufferStatus m_out_status;
    bool m_debug;


    //created in configure, destoryed in unconfigure
    std::unique_ptr<VMEController> vmeCont;
    std::vector<std::unique_ptr<VMEDevice>> vmeDev;

    //number of modules
    int nrMods = 0;

    //used to get the handle from dlopen device
    void *modHandle;

    //saves the handle from dlopen controller
    void *contHandle;

    //stores the address of the controller from the xml file
    std::string contName;
 
    //stores the module names
    std::vector<std::string>modName_v;
    //stores the module addresses
    std::vector<int>modAddr_v;
    //stores the handles returned by dlopen
    std::vector<uint64_t*>handleSave_v;


    unsigned int fCounter = 0;
    TDataContainer fDataContainer;
    std::chrono::system_clock::time_point fStartTime;
    int fNEvents = 0;

    //global event number
    uint64_t evNumber = 0;








};


extern "C"
{
    void VMEConductorInit(RTC::Manager* manager);
};

#endif // VMECONDUCTOR_H
