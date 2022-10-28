#ifndef VMEController_hpp
#define VMEController_hpp 1


#include<memory>


class VMEController
{
    public:
    VMEController();
    virtual ~VMEController();



    virtual void utilsVMEinit(void) = 0;
    virtual void utilsVMEend(void) = 0;

    virtual void utilsVMEread(uint32_t busAddr, uint16_t *dataRead) = 0;
    virtual void utilsVMEwrite(uint32_t busAddr, int data) = 0;

    virtual void utilsVMEbltRead(uint32_t busAddr, int sizeBytes, uint32_t *dataBuff, int *dataTransf) = 0;

    virtual int utilsVMEirqCheck(uint32_t lineNr) = 0;

    virtual void utilsVMEsetAddrMod(int newaddrmod) = 0;
    virtual void utilsVMEsetDataW(int newdataw) = 0;

};

typedef std::unique_ptr<VMEController> ControllerCreateFnc();











#endif