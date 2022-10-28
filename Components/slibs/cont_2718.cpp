#include "../include/VMEController.hpp"

#include<iostream>
#include "CAENComm.h"



//g++ -c -Wall -fPIC -lCAENComm -I../include cont_2718.cpp -ldl
//g++ -shared -o cont_2718.so cont_2718.o /home/gant/DELILA-main/Components/src/VMEController.o -ldl -lCAENComm





class VX2718 : public VMEController
{
    public:
    VX2718(){};
    ~VX2718(){};


    short bdLink = 0;
    short bdNum = 1;
    int bdHandlComm;
    int buffSize;
    int byteCount;
    int addr_mod_opt;
    int data_w_opt;




    void utilsVMEinit(void);
    void utilsVMEend(void);

    void utilsVMEread(uint32_t busAddr, uint16_t *dataRead);
    void utilsVMEwrite(uint32_t busAddr, int data);

    void utilsVMEbltRead(uint32_t busAddr, int sizeBytes, uint32_t *dataBuff, int *dataTransf);

    int utilsVMEirqCheck(uint32_t lineNr);

    void utilsVMEsetAddrMod(int newaddrmod);
    void utilsVMEsetDataW(int newdataw);


};


//ititialises the CAEN VX2178 module, returns a handle
void VX2718::utilsVMEinit(void)
{
 
    CAENComm_ErrorCode err;

    err = CAENComm_OpenDevice(CAENComm_OpticalLink, 1, 0, 0, &this->bdHandlComm);
    if(err !=0)
        std::cout<<"eroare open cod "<<err<<std::endl;
 
}

//ends the connection to the module
void VX2718::utilsVMEend(void)
{

    CAENComm_ErrorCode err;

    err = CAENComm_CloseDevice(this->bdHandlComm);
    if(err !=0)
        std::cout<<"eroare open cod "<<err<<std::endl;
}

//performs a read operation at busAddr
void VX2718::utilsVMEread(uint32_t busAddr, uint16_t *dataRead)
{

    CAENComm_ErrorCode err;

    if((this->addr_mod_opt == 0) && (this->data_w_opt == 0)){

        err = CAENComm_Read16(this->bdHandlComm, busAddr, dataRead);
        if(err !=0)
            std::cout<<"eroare write cod "<<err<<std::endl;

    }


}


//performs a write operation at busAddr
void VX2718::utilsVMEwrite(uint32_t busAddr, int data)
{
 
    uint16_t dataComm = data;
    CAENComm_ErrorCode err;

    if((this->addr_mod_opt == 0) && (this->data_w_opt == 0)){
        err = CAENComm_Write16(this->bdHandlComm, busAddr, dataComm);
        if(err !=0)
            std::cout<<"Error write code: "<<err<<" at addr: "<<std::hex<<busAddr<<std::endl;
    }

     

}

//performs a blt read operation at busAddr
//sizeBytes instructs how many bytes to read
//data is stored at dataBuff
//*dataTransf stores how many 32 bit words were transfered
void VX2718::utilsVMEbltRead(uint32_t busAddr, int sizeBytes, uint32_t *dataBuff, int *dataTransf)
{
      
    CAENComm_ErrorCode err;

    if((this->addr_mod_opt == 1) && (this->data_w_opt == 1)){
        err =  CAENComm_BLTRead(this->bdHandlComm, busAddr, dataBuff, sizeBytes, dataTransf);
        if(err !=0)
            std::cout<<"eroare blt read cod "<<err<<std::endl;
    }


}

void VX2718::utilsVMEsetAddrMod(int newaddrmod)
{

    this->addr_mod_opt = newaddrmod;

}

void VX2718::utilsVMEsetDataW(int newdataw)
{

    this->data_w_opt = newdataw;

}


int VX2718::utilsVMEirqCheck(uint32_t lineNr)
{


    CAENComm_ErrorCode err;

    err = CAENComm_IRQEnable(this->bdHandlComm);
    if(err !=0)
        std::cout<<"eroare irq enable cod "<<err<<std::endl;
 

    err = CAENComm_IRQWait(this->bdHandlComm, 100);
    if(err !=0)
        std::cout<<"eroare irq wait cod "<<err<<std::endl;

    return 1;

/*     uint8_t irq_mask;

    err = CAENComm_VMEIRQCheck(this->bdHandlComm, &irq_mask);
    if(err !=0)
        std::cout<<"eroare irq check cod "<<err<<std::endl;
    
    std::cout<<"irq status: "<<std::bitset<8>(irq_mask)<<" with irq line needed "<<lineNr<<std::endl;

    if((irq_mask & (1<<lineNr)) != 0){
        return 1;
    }



    return 0; */

}




extern "C"
{

    std::unique_ptr<VMEController> MakeContObj()
    {
        return std::unique_ptr<VMEController>(new VX2718());
    }
    
}
