// -*- C++ -*-
/*!
 * @file 
 * @brief
 * @date
 * @author
 *
 */

#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "DaqComponentBase.h"

using namespace RTC;

class Dispatcher
    : public DAQMW::DaqComponentBase
{
public:
    Dispatcher(RTC::Manager* manager);
    ~Dispatcher();

    // The initialize action (on CREATED->ALIVE transition)
    // former rtc_init_entry()
    virtual RTC::ReturnCode_t onInitialize();

    // The execution action that is invoked periodically
    // former rtc_active_do()
    virtual RTC::ReturnCode_t onExecute(RTC::UniqueId ec_id);

private:
    TimedOctetSeq          m_in_data;
    InPort<TimedOctetSeq> m_InPort;

    TimedOctetSeq          m_out1_data;
    OutPort<TimedOctetSeq> m_OutPort1;

    TimedOctetSeq          m_out2_data;
    OutPort<TimedOctetSeq> m_OutPort2;

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
    int set_data_OutPort1(unsigned int data_byte_size);
    int set_data_OutPort2(unsigned int data_byte_size);
    int reset_InPort();
    unsigned int read_InPort();
    int write_OutPort1();
    int write_OutPort2();

    static const int SEND_BUFFER_SIZE = 4096;
    unsigned char m_data[SEND_BUFFER_SIZE];

    BufferStatus m_in_status;
    BufferStatus m_out1_status;
    BufferStatus m_out2_status;

    unsigned int m_in_timeout_counter;  //timeout counter for InPort reading
    unsigned int m_out1_timeout_counter;//timeout counter for OutPort1 writing
    unsigned int m_out2_timeout_counter;//timeout counter for OutPort2 writing

    unsigned int m_inport_recv_data_size;
    bool m_debug;
};


extern "C"
{
    void DispatcherInit(RTC::Manager* manager);
};

#endif // DISPATCHER_H
