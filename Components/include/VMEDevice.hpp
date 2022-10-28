#ifndef VMEDevice_hpp
#define VMEDevice_hpp 1

#include <vector>
#include <memory>
#include "../include/VMEController.hpp"
#include "../include/TreeData.h"

class VMEDevice
{
    public:
    VMEDevice();
    virtual ~VMEDevice();

    virtual void read_cfg() = 0;
    virtual std::unique_ptr<VMEController> mod_configure(std::unique_ptr<VMEController> my_contr, uint32_t addr_offset, int modId) = 0;
    virtual std::unique_ptr<VMEController> mod_start(std::unique_ptr<VMEController> my_contr) = 0;
    virtual std::unique_ptr<VMEController> mod_stop(std::unique_ptr<VMEController> my_contr) = 0;
    virtual std::unique_ptr<VMEController> mod_run(std::unique_ptr<VMEController> my_contr, std::shared_ptr<std::vector<TreeData>> t_data, int *fNEvents) = 0;
    virtual int mod_checkifreader() = 0;


};

typedef std::unique_ptr<VMEDevice> DeviceCreateFnc();









#endif