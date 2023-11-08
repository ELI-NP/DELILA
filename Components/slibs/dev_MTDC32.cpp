#include "../include/VMEDevice.hpp"
#include "../include/VMEController.hpp"
#include "../include/TreeData.h"


#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iomanip>




//g++ -c -Wall -fPIC -I../include dev_MTDC32.cpp -ldl
//g++ -shared -o dev_MTDC32.so dev_MTDC32.o /home/gant/DELILA-main/Components/src/VMEController.o /home/gant/DELILA-main/Components/src/VMEDevice.o -ldl





class MTDC32 : public VMEDevice
{
    public:
    MTDC32(){};
    ~MTDC32(){};


    int modId;
    uint32_t addr_offset;


    
    int win_start;
    int b_width;
    int res_opt;
    int trig_source;
    int trig_opt;
    int irq_data_thr;
    int max_data_transf;
    int ttl_opt;//TTL is active for 1 and inactive for 0
    int mod_nr;
    int ext_ts;
    int irq_l;


    //stores the total number of events
    uint64_t evNumber;


    








    void read_cfg();
    std::unique_ptr<VMEController> mod_configure(std::unique_ptr<VMEController> my_contr, uint32_t addr_offset, int modId);
    std::unique_ptr<VMEController> mod_start(std::unique_ptr<VMEController> my_contr);
    std::unique_ptr<VMEController> mod_stop(std::unique_ptr<VMEController> my_contr);
    std::unique_ptr<VMEController> mod_run(std::unique_ptr<VMEController> my_contr, std::shared_ptr<std::vector<TreeData>> t_data, int *fNEvents);
    int mod_checkifreader();

};



void MTDC32::read_cfg()
{

    std::string disk_loc_begin = "/home/gant/DELILA-main/Components/slibs/conf_MTDC32_0x";

    std::stringstream disk_loc_mid;
    disk_loc_mid << std::hex << this->addr_offset;

    std::string disk_loc_end = ".json";

    std::string disk_loc = disk_loc_begin + disk_loc_mid.str() + disk_loc_end;




    std::ifstream conf_file(disk_loc.c_str());
    if(!conf_file.is_open()){
        std::cerr<<"Failed to open config file "<<disk_loc<<std::endl;
    }else{
        std::cout<<"File opened successfully "<<disk_loc<<std::endl;
    }



    nlohmann::json conf_data;
    bool has_exception = false;

    try
    {
        conf_data = nlohmann::json::parse(conf_file);
    }
    catch(nlohmann::json::parse_error &ex)
    {
        has_exception = true;
        std::cerr<<"Parse error at byte "<<ex.byte<<std::endl;
    }

 
    if(has_exception == false){
        
        this->mod_nr = stoi(conf_data["mod_nr"].get<std::string>());
        this->irq_l = stoi(conf_data["irq_l"].get<std::string>());
        this->irq_data_thr = stoi(conf_data["irq_data_thr"].get<std::string>());
        this->max_data_transf = stoi(conf_data["max_data_transf"].get<std::string>());
        this->ext_ts = stoi(conf_data["ext_ts"].get<std::string>());
        this->res_opt = stoi(conf_data["res_opt"].get<std::string>());
        this->win_start = stoi(conf_data["win_start"].get<std::string>());
        this->b_width = stoi(conf_data["b_width"].get<std::string>());
        this->trig_opt = stoi(conf_data["trig_opt"].get<std::string>());
        this->ttl_opt = stoi(conf_data["ttl_opt"].get<std::string>());
        this->trig_source = stoi(conf_data["trig_source"].get<std::string>(), 0, 2);











        std::cout<<"mod_nr="<<this->mod_nr<<" irql="<<this->irq_l<<" irqdt="<<this->irq_data_thr
        <<" mdt="<<this->max_data_transf<<" ets="<<this->ext_ts<<" ro="<<this->res_opt
        <<" ws="<<this->win_start<<" bw="<<this->b_width<<" to="<<this->trig_opt
        <<" ttlo="<<this->ttl_opt<<" ts="<<this->trig_source<<std::endl;



    }else{
        std::cerr<<"Coud not set parameters due to exception"<<std::endl;
    }
     


}


std::unique_ptr<VMEController> MTDC32::mod_configure(std::unique_ptr<VMEController> my_contr, uint32_t addr_offset, int modId)
{

    std::cout<<"\nConfigure for mod "<<modId<<" begin"<<std::endl;

    this->addr_offset = addr_offset;
    this->modId = modId;

    read_cfg();

    //set address modifier
    my_contr->utilsVMEsetAddrMod(0);
    
    //set data width
    my_contr->utilsVMEsetDataW(0);



    //stop data transfer
    my_contr->utilsVMEwrite(0x603A + addr_offset, 0);

    //init FIFO
    my_contr->utilsVMEwrite(0x603C + addr_offset, 1);

    //set multi event mode 3
    my_contr->utilsVMEwrite(0x6036 + addr_offset, 3);

    //clear BERR
    my_contr->utilsVMEwrite(0x6034 + addr_offset, 1);

    //set irq level
    my_contr->utilsVMEwrite(0x6010 + addr_offset, this->irq_l);

    //set irq vector
    my_contr->utilsVMEwrite(0x6012 + addr_offset, 0);

    //set trig ecl
    my_contr->utilsVMEwrite(0x6068 + addr_offset, 1);

    //set win start
    my_contr->utilsVMEwrite(0x6050 + addr_offset, this->win_start);

    //set bank width
    my_contr->utilsVMEwrite(0x6054 + addr_offset, this->b_width);

    //set resolution option
    my_contr->utilsVMEwrite(0x6042 + addr_offset, this->res_opt);


    //set trig source
    my_contr->utilsVMEwrite(0x6058 + addr_offset, this->trig_source);

    //set irq data transf
    my_contr->utilsVMEwrite(0x6018 + addr_offset, this->irq_data_thr);

    //set max data transf
    my_contr->utilsVMEwrite(0x601A + addr_offset, this->max_data_transf);


    //run if ttl
    if(this->ttl_opt){

        //TTL bank th
        my_contr->utilsVMEwrite(0x6078 + addr_offset, 255);

        //TTL pos sig edge
        my_contr->utilsVMEwrite(0x6060 + addr_offset, 3);

    }

    //set time stamp option
    //0 event counter; 1 time stamp; 3 extended time stamp
    my_contr->utilsVMEwrite(0x6038 + addr_offset, ext_ts);




    //set event number to 0
    this->evNumber = 0;



    printf("Configure for mod %d end\n\n", modId);
    fflush(stdout);




    return std::move(my_contr);



}




std::unique_ptr<VMEController> MTDC32::mod_start(std::unique_ptr<VMEController> my_contr)
{

    printf("Start for mod %d begin\n", this->modId);
    fflush(stdout);

    //set address modifier
    my_contr->utilsVMEsetAddrMod(0);
    //set data width
    my_contr->utilsVMEsetDataW(0);

    //start data transfer
    my_contr->utilsVMEwrite(0x603A + this->addr_offset, 1);


    //reset counter for ext ts
    if(ext_ts != 0){
        my_contr->utilsVMEwrite(this->addr_offset + 0x6090, 1);
    }

    //start test
    //(*ptr_write)(0x6070 + addr_offset, 0b111);



    printf("Start for mod %d end\n", this->modId);
    fflush(stdout);


    return std::move(my_contr);

}



std::unique_ptr<VMEController> MTDC32::mod_stop(std::unique_ptr<VMEController> my_contr)
{

    printf("Stop for mod %d begin\n", this->modId);
    fflush(stdout);



    //set address modifier
    my_contr->utilsVMEsetAddrMod(0);
    //set data width
    my_contr->utilsVMEsetDataW(0);

    //stop data transfer
    my_contr->utilsVMEwrite(0x603A + this->addr_offset, 0);



    //stop test
    //(*ptr_write)(0x6070 + addr_offset, 0);


    printf("Stop for mod %d end\n", this->modId);
    fflush(stdout);




    return std::move(my_contr);

}




std::unique_ptr<VMEController> MTDC32::mod_run(std::unique_ptr<VMEController> my_contr, std::shared_ptr<std::vector<TreeData>> t_data, int *fNEvents)
{

    //set nr of events to 0
    (*fNEvents) = 0;

    if(my_contr->utilsVMEirqCheck(this->irq_l) == 1){

        //allocate memory for data
        uint32_t *dataBuff = (uint32_t*)malloc(50000*sizeof(uint32_t));


        //amount of data transfered
        int buffSize = -1;

        //set address modifier for blt read
        my_contr->utilsVMEsetAddrMod(1);
        //set data width
        my_contr->utilsVMEsetDataW(1);


        my_contr->utilsVMEbltRead(this->addr_offset, 50000, dataBuff, &buffSize);

        std::cout<<"Buffsize is "<<std::dec<<buffSize<<std::endl;


        //set set data width and am for single write
        my_contr->utilsVMEsetAddrMod(0);
        //set data width
        my_contr->utilsVMEsetDataW(0);


        //reset BERR
        my_contr->utilsVMEwrite(addr_offset + 0x6034, 1);


        if(ext_ts == 0){

            for(int i = 0; i<buffSize; i++){


                //std::cerr<<"The data is: "<<std::bitset<32>(dataBuff[i])<<std::endl;


                if((dataBuff[i]>>30) == 1){

                    if((dataBuff[i]>>24) != 64){

                        printf("Event header invalid\n");
                            
                    }

                }else if((dataBuff[i]>>30) == 0){

                    if((dataBuff[i]>>22) == 16){


                        TreeData loc_data;

                        loc_data.Mod = this->mod_nr;
                        loc_data.Ch = ((dataBuff[i]>>16)&0b0000000000011111);
                        loc_data.TimeStamp = this->evNumber;
                        loc_data.FineTS = static_cast<double>(dataBuff[i] & 0x0000FFFF);
                        loc_data.ChargeLong = 0xFFFF;
                        loc_data.ChargeShort = 0xFFFF;
                        loc_data.RecordLength = 1;
                        loc_data.Extras = 0;//0xFFFF0032;



                        t_data->push_back(loc_data);


                        (*fNEvents)++;


                    }else if(dataBuff[i] != 0){

                        printf("Data event invalid\n");
                            

                    }


                }else if((dataBuff[i]>>30) == 3){

                    //std::cerr<<"Evnr "<<evNumber<<std::endl;
                    this->evNumber++;

                }



                    



                   
                
            }
        }else if(ext_ts == 3){


            //low bits of extended time stamp
            uint16_t ext_ts_lb = 0;

            //high bits of extended time stamp
            uint64_t ext_ts_hb = 0;

            //tells if time stamp is valid
            bool ts_valid = false;

            //nr of events in header
            int ev_in_h = 0;


            for(int i = 0; i<buffSize; i++){




                if((dataBuff[i]>>30) == 1){

                    ev_in_h = 0;

                    ts_valid = false;

                    ext_ts_lb = 0;

                    ext_ts_hb = 0;

                    if((dataBuff[i]>>24) != 64){
                        printf("Event header invalid\n");
                    }

                }else if((dataBuff[i]>>30) == 0){

                    if((dataBuff[i]>>22) == 16){


                        TreeData loc_data;

                        loc_data.Mod = mod_nr;
                        loc_data.Ch = ((dataBuff[i]>>16)&0b0000000000011111);


                        //nonsense value that gets changed
                        loc_data.TimeStamp = 0xFFFFFFFFFFFFFFFF;


                        loc_data.FineTS = static_cast<double>(dataBuff[i] & 0x0000FFFF);;
                        loc_data.ChargeLong = 0xFFFF;
                        loc_data.ChargeShort = 0xFFFF;
                        loc_data.RecordLength = 1;
                        loc_data.Extras = 0xFFFF0032;


                            


                        t_data->push_back(loc_data);

                        (*fNEvents)++;

                        ev_in_h++;

                    }else if((dataBuff[i]>>16) == 1152){

                        ext_ts_hb = (dataBuff[i] & 0x0000FFFF);
                        ts_valid = true;
                    

                    }else if(dataBuff[i] != 0){

                        printf("Data event invalid\n");
                           

                    }

                } else if ((dataBuff[i] >> 30) == 3) {

                  // first 2 bits are always 11 for EOE mark; we only use 30 for
                  // the time stamp
                  ext_ts_lb = (dataBuff[i] & 0x3FFFFFFF);

                  // checks if the 16 low bits of extended time stamp were
                  // present
                  if (ts_valid) {

                    for (int iter_eoe = 0; iter_eoe < ev_in_h; iter_eoe++) {

                      t_data->at((*fNEvents) - (ev_in_h - iter_eoe)).TimeStamp =
                          (ext_ts_hb << 30) + ext_ts_lb;
                    }

                  } else {

                    printf("\nTime stamp invalid!!!\n");
                  }
                }
            }
        }else if(ext_ts == 1){



            //time stamp bits
            uint64_t ts_b = 0;

            //tells if time stamp is valid
            bool ts_valid = false;

            //nr of events in header
            int ev_in_h = 0;


            for(int i = 0; i<buffSize; i++){




                if((dataBuff[i]>>30) == 1){

                    ev_in_h = 0;

                    ts_valid = false;

                    ts_b = 0;

                    if((dataBuff[i]>>24) != 64){
                        printf("Event header invalid\n");
                    }

                }else if((dataBuff[i]>>30) == 0){

                    if((dataBuff[i]>>22) == 16){


                        TreeData loc_data;

                        loc_data.Mod = mod_nr;
                        loc_data.Ch = ((dataBuff[i]>>16)&0b0000000000011111);


                        //nonsense value that gets changed
                        loc_data.TimeStamp = 0xFFFFFFFFFFFFFFFF;


                        loc_data.FineTS = static_cast<double>(dataBuff[i] & 0x0000FFFF);;
                        loc_data.ChargeLong = 0xFFFF;
                        loc_data.ChargeShort = 0xFFFF;
                        loc_data.RecordLength = 1;
                        loc_data.Extras = 0xFFFF0032;


                            


                        t_data->push_back(loc_data);

                        (*fNEvents)++;

                        ev_in_h++;

                    }else if(dataBuff[i] != 0){

                        printf("Data event invalid\n");
                           

                    }

                }else if((dataBuff[i]>>30) == 3){

                    //first 2 bits are always 11 for EOE mark; we only use 30 for the time stamp
                    ts_b = (dataBuff[i] & 0x3FFFFFFF);


                    for(int iter_eoe = 0; iter_eoe<ev_in_h; iter_eoe++){

                        t_data->at((*fNEvents) - (ev_in_h - iter_eoe)).TimeStamp = ts_b;

                    }

                        


                }


            }
        }




        free(dataBuff);



    }



    


    return std::move(my_contr);

}





































//checks if the device outputs data
//0 for no and 1 for yes
int MTDC32::mod_checkifreader()
{
    return 1;
}





extern "C"
{


    std::unique_ptr<VMEDevice> MakeDevObj()
    {
        return std::unique_ptr<VMEDevice>(new MTDC32());
    }



}