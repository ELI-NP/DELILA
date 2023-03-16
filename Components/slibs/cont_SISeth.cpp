#include "../include/VMEController.hpp"

#include<iostream>
#include<chrono>
#include<bitset>
#include<fstream>

#include "../include/includesiseth/project_system_define.h"
#include "../include/includesiseth/project_interface_define.h"
#include "../include/includesiseth/sis3153usb.h"
#include "../include/includesiseth/vme_interface_class.h"
#include "../include/includesiseth/sis3153ETH_vme_class.h"


//g++ -c -Wall -fPIC -I../include -I../include/includesiseth /home/gant/DELILA-main/Components/include/includesiseth/sis3153ETH_vme_class.cpp  cont_SISeth.cpp -ldl 
//g++ -shared -o cont_SISeth.so cont_SISeth.o /home/gant/DELILA-main/Components/src/VMEController.o sis3153ETH_vme_class.o -ldl




class SISeth : public VMEController
{
    public:
    SISeth(){};
    ~SISeth(){};


    sis3153eth *sis_crate;
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

void SISeth::utilsVMEinit(void)
{

    char ip_addr_str[32];

    std::ifstream file_data;
    file_data.open("/home/gant/DELILA-main/Components/slibs/SISip.conf");

    if(!file_data){

        std::cerr<<"Ip file could not be opened"<<std::endl;

    }

    file_data >>ip_addr_str;

    std::cout<<"Ip is "<<ip_addr_str<<std::endl;

    //strcpy(ip_addr_str, "192.168.0.198");
    sis3153eth(&this->sis_crate, ip_addr_str);

    int err;

    err = this->sis_crate->get_UdpSocketStatus();
    if(err != 0){
        std::cout<<"Error udp socket status code: "<<err<<std::endl;
    }



    std::cout<<"SIS init end"<<std::endl;
    
}

void SISeth::utilsVMEend(void)
{

    this->sis_crate->vmeclose();
    delete(this->sis_crate);

}

void SISeth::utilsVMEread(uint32_t busAddr, uint16_t *dataRead)
{

    int err;
    if((this->addr_mod_opt == 0) && (this->data_w_opt == 0)){
        
        err = this->sis_crate->vme_A32D16_read(busAddr, dataRead);
        if(err != 0){
            std::cout<<"Error read code: "<<err<<" hex "<<std::hex<<err<<std::endl;
        }

    }


}

void SISeth::utilsVMEwrite(uint32_t busAddr, int data)
{

    uint16_t data_loc = static_cast<uint32_t>(data);
    int err;
    if((this->addr_mod_opt == 0) && (this->data_w_opt == 0)){
        
        err = this->sis_crate->vme_A32D16_write(busAddr, data_loc);
        if(err != 0){
            std::cout<<"Error write code: "<<err<<" hex "<<std::hex<<err<<std::endl;
        }

    }
    
}

void SISeth::utilsVMEbltRead(uint32_t busAddr, int sizeBytes, uint32_t *dataBuff, int *dataTransf)
{

    uint32_t gotWords;
    int err = 0;



    if((this->addr_mod_opt == 1) && (this->data_w_opt == 1)){
        
        err = this->sis_crate->vme_A32BLT32FIFO_read(busAddr, dataBuff, sizeBytes, &gotWords);
        if(err != 0){
            std::cout<<"Error blt read code: "<<err<<std::endl;
        }

    }

    


    (*dataTransf) = static_cast<int>(gotWords);




}

int SISeth::utilsVMEirqCheck(uint32_t lineNr)
{

    int err = 0;
    uint32_t data_reg = 0;
    err = this->sis_crate->udp_sis3153_register_read(0x12, &data_reg);
    if(err != 0){
        std::cout<<"Error read reg irq code "<<err<<std::endl;
    }
    
    
    if(((data_reg>>lineNr) & 1) == 1){
        std::cout<<"irq status: "<<std::bitset<32>(data_reg)<<std::endl;
        return 1;
    }

/*     if(data_reg != 0){
        std::cout<<"irq status: "<<std::bitset<32>(data_reg)<<std::endl;
        return 1;
    } */

    return 0;

}



void SISeth::utilsVMEsetAddrMod(int newaddrmod)
{

    this->addr_mod_opt = newaddrmod;

}

void SISeth::utilsVMEsetDataW(int newdataw)
{

    this->data_w_opt = newdataw;

}

















extern "C"{
    std::unique_ptr<VMEController> MakeContObj()
    {
        return std::unique_ptr<VMEController>(new SISeth());
    }
}