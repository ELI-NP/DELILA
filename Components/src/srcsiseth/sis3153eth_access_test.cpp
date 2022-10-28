//============================================================================
// Name        : sis3153eth_access_test.cpp
// Author      : th
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================
#include "../../include/includesiseth/project_system_define.h"		//define LINUX or WINDOWS
#include "../../include/includesiseth/project_interface_define.h"   //define Interface (sis1100/sis310x, sis3150usb or Ethnernet UDP)


#include <iostream>
using namespace std;

//#include <iostream>
//#include <string.h>
#include <stdio.h>
#include <stdlib.h>




#include "sis3153usb.h"
#include "vme_interface_class.h"



#ifdef ETHERNET_VME_INTERFACE
	#include "sis3153ETH_vme_class.h"
	sis3153eth *gl_vme_crate;
	char  gl_sis3153_ip_addr_string[32] = "212.60.16.200";
	//	char  gl_sis3153_ip_addr_string[32] = "192.168.1.11";

	#ifdef LINUX
		#include <sys/types.h>
		#include <sys/socket.h>
	#endif

	#ifdef WINDOWS
	#include <winsock2.h>
	#pragma comment(lib, "ws2_32.lib")
	//#pragma comment(lib, "wsock32.lib")
	typedef int socklen_t;

/*	long WinsockStartup()
	{
	  long rc;

	  WORD wVersionRequested;
	  WSADATA wsaData;
	  wVersionRequested = MAKEWORD(2, 1);

	  rc = WSAStartup( wVersionRequested, &wsaData );
	  return rc;
	}
	*/
	#endif
#endif





unsigned int gl_dma_buffer[0x10000] ;

int main(int argc, char *argv[])
{

	unsigned int addr;
	unsigned int data ;
	unsigned int i ;
	unsigned int request_nof_words ;
	unsigned int got_nof_words ;
	unsigned int written_nof_words ;

	unsigned char uchar_data  ;
	unsigned short ushort_data  ;
	cout << "sis3153eth_access_test" << endl; // prints sis3316_access_test_sis3153eth

	   //char char_command[256];
	char  ip_addr_string[32] ;
	unsigned int vme_base_address ;
	char ch_string[64] ;
	int int_ch ;
	int return_code ;

#ifdef ETHERNET_UDP_INTERFACE

	char  pc_ip_addr_string[32] ;

	//strcpy(sis3316_ip_addr_string, gl_sis3316_ip_addr_string) ; // SIS3316 IP address
	//strcpy(sis3316_ip_addr_string,"212.60.16.200") ; // SIS3316 IP address
	strcpy(sis3316_ip_addr_string,"192.168.1.100") ; // SIS3316 IP address
#endif


	// default
	vme_base_address = 0x30000000 ;
	strcpy(ip_addr_string,"192.168.1.100") ; // SIS3153 IP address

	   if (argc > 1) {
	#ifdef raus
		   /* Save command line into string "command" */
		   memset(char_command,0,sizeof(char_command));
		   for (i=1;i<argc;i++) {
				strcat(char_command,argv[i]);
				strcat(char_command," ");
			}
			printf("gl_command %s    \n", char_command);
	#endif


			while ((int_ch = getopt(argc, argv, "?hI:")) != -1)
				switch (int_ch) {
				  case 'I':
						sscanf(optarg,"%s", ch_string) ;
						printf("-I %s    \n", ch_string );
						strcpy(ip_addr_string,ch_string) ;
						break;
				  case 'X':
					sscanf(optarg,"%X", &vme_base_address) ;
					break;
				  case '?':
				  case 'h':
				  default:
						printf("   \n");
					printf("Usage: %s  [-?h] [-I ip]  ", argv[0]);
					printf("   \n");
					printf("   \n");
				    printf("   -I string     SIS3153 IP Address       	Default = %s\n", ip_addr_string);
					printf("   \n");
					printf("   -h            Print this message\n");
					printf("   \n");
					exit(1);
				  }

		 } // if (argc > 1)
		printf("\n");




#ifdef ETHERNET_VME_INTERFACE
	sis3153eth *vme_crate;
	sis3153eth(&vme_crate, ip_addr_string);
#endif


	char char_messages[128] ;
	unsigned int nof_found_devices ;

	// open Vme Interface device
	return_code = vme_crate->vmeopen ();  // open Vme interface
	vme_crate->get_vmeopen_messages (char_messages, &nof_found_devices);  // open Vme interface
    printf("get_vmeopen_messages = %s , nof_found_devices %d \n",char_messages, nof_found_devices);


	return_code = vme_crate->udp_sis3153_register_read (SIS3153USB_CONTROL_STATUS, &data); //
	printf("udp_sis3153_register_read: addr = 0x%08X    data = 0x%08X    return_code = 0x%08X \n", SIS3153USB_CONTROL_STATUS, data,return_code);
	return_code = vme_crate->udp_sis3153_register_read (SIS3153USB_MODID_VERSION, &data); //
	printf("udp_sis3153_register_read: addr = 0x%08X    data = 0x%08X    return_code = 0x%08X \n", SIS3153USB_MODID_VERSION, data,return_code);
	return_code = vme_crate->udp_sis3153_register_read (SIS3153USB_SERIAL_NUMBER_REG, &data); //
	printf("udp_sis3153_register_read: addr = 0x%08X    data = 0x%08X    return_code = 0x%08X \n", SIS3153USB_SERIAL_NUMBER_REG, data,return_code);
	return_code = vme_crate->udp_sis3153_register_read (SIS3153USB_LEMO_IO_CTRL_REG, &data); //
	printf("udp_sis3153_register_read: addr = 0x%08X    data = 0x%08X    return_code = 0x%08X \n", SIS3153USB_LEMO_IO_CTRL_REG, data,return_code);

	printf("\n");

	addr = SIS3153USB_CONTROL_STATUS;
	data = 0x1;
	return_code = vme_crate->udp_sis3153_register_write (addr, data); //
	printf("udp_sis3153_register_write: addr = 0x%08X    data = 0x%08X    return_code = 0x%08X \n", SIS3153USB_CONTROL_STATUS, data,return_code);

	usleep(500000);

	addr = SIS3153USB_CONTROL_STATUS;
	data = 0x10000;
	return_code = vme_crate->udp_sis3153_register_write (addr, data); //
	printf("udp_sis3153_register_write: addr = 0x%08X    data = 0x%08X    return_code = 0x%08X \n", SIS3153USB_CONTROL_STATUS, data,return_code);

	usleep(500000);

	do {
	#define INTERNAL_REGISTER
	#ifdef INTERNAL_REGISTER
		return_code = vme_crate->udp_sis3153_register_read (SIS3153USB_CONTROL_STATUS, &data); //
		printf("udp_sis3153_register_read: \taddr = 0x%08X    \tdata = 0x%08X    \treturn_code = 0x%08X \n", SIS3153USB_CONTROL_STATUS, data,return_code);
		return_code = vme_crate->udp_sis3153_register_read (SIS3153USB_MODID_VERSION, &data); //
		printf("udp_sis3153_register_read: \taddr = 0x%08X    \tdata = 0x%08X    \treturn_code = 0x%08X \n", SIS3153USB_MODID_VERSION, data,return_code);
		return_code = vme_crate->udp_sis3153_register_read (SIS3153USB_SERIAL_NUMBER_REG, &data); //
		printf("udp_sis3153_register_read: \taddr = 0x%08X    \tdata = 0x%08X    \treturn_code = 0x%08X \n", SIS3153USB_SERIAL_NUMBER_REG, data,return_code);
		return_code = vme_crate->udp_sis3153_register_read (SIS3153USB_LEMO_IO_CTRL_REG, &data); //
		printf("udp_sis3153_register_read: \taddr = 0x%08X    \tdata = 0x%08X    \treturn_code = 0x%08X \n", SIS3153USB_LEMO_IO_CTRL_REG, data,return_code);

		printf("\n");

		addr = SIS3153USB_CONTROL_STATUS;
		data = 0x1;
		return_code = vme_crate->udp_sis3153_register_write (addr, data); //
		printf("udp_sis3153_register_write: \taddr = 0x%08X    \tdata = 0x%08X    \treturn_code = 0x%08X \n", SIS3153USB_CONTROL_STATUS, data,return_code);

		usleep(500000);

		addr = SIS3153USB_CONTROL_STATUS;
		data = 0x10000;
		return_code = vme_crate->udp_sis3153_register_write (addr, data); //
		printf("udp_sis3153_register_write: \taddr = 0x%08X    \tdata = 0x%08X    \treturn_code = 0x%08X \n", SIS3153USB_CONTROL_STATUS, data,return_code);

	#endif


	#define VME_D8_READ_TEST
	#ifdef VME_D8_READ_TEST

		printf("\n");
		addr = 0 ;
		data = 0x12345678 ;
		return_code = vme_crate->vme_A32D32_write (addr, data); //
		printf("vme_A32D32_write: \t\taddr = 0x%08X    \tdata = 0x%08X    \treturn_code = 0x%08X \n", addr, data,return_code);

		addr = 0x0 ;
		return_code = vme_crate->vme_A32D8_read (addr, &uchar_data); //
		printf("vme_A32D32_read:  \t\taddr = 0x%08X    \tuchar_data = 0x%02X    \treturn_code = 0x%08X \n", addr, uchar_data,return_code);

		addr = 0x1 ;
		return_code = vme_crate->vme_A32D8_read (addr, &uchar_data); //
		printf("vme_A32D32_read:  \t\taddr = 0x%08X    \tuchar_data = 0x%02X    \treturn_code = 0x%08X \n", addr, uchar_data,return_code);

		addr = 0x2 ;
		return_code = vme_crate->vme_A32D8_read (addr, &uchar_data); //
		printf("vme_A32D32_read:  \t\taddr = 0x%08X    \tuchar_data = 0x%02X    \treturn_code = 0x%08X \n", addr, uchar_data,return_code);

		addr = 0x3 ;
		return_code = vme_crate->vme_A32D8_read (addr, &uchar_data); //
		printf("vme_A32D32_read:  \t\taddr = 0x%08X    \tuchar_data = 0x%02X    \treturn_code = 0x%08X \n", addr, uchar_data,return_code);

	#endif


	#define VME_D16_READ_TEST
	#ifdef VME_D16_READ_TEST

		printf("\n");
		addr = 0 ;
		data = 0x12345678 ;
		return_code = vme_crate->vme_A32D32_write (addr, data); //
		printf("vme_A32D32_write: \t\taddr = 0x%08X    \tdata = 0x%08X    \treturn_code = 0x%08X \n", addr, data,return_code);

		addr = 0x0 ;
		return_code = vme_crate->vme_A32D16_read (addr, &ushort_data); //
		printf("vme_A32D16_read:  \t\taddr = 0x%08X    \tushort_data = 0x%04X \treturn_code = 0x%08X \n", addr, ushort_data,return_code);

		addr = 0x2 ;
		return_code = vme_crate->vme_A32D16_read (addr, &ushort_data); //
		printf("vme_A32D16_read:  \t\taddr = 0x%08X    \tushort_data = 0x%04X \treturn_code = 0x%08X \n", addr, ushort_data,return_code);
	#endif

	#define VME_D8_WRITE_TEST
	#ifdef VME_D8_WRITE_TEST

		printf("\n");

		addr = 0x0 ;
		uchar_data = 0x12 ;
		return_code = vme_crate->vme_A32D8_write (addr, uchar_data); //
		printf("vme_A32D8_write:  \t\taddr = 0x%08X    \tuchar_data = 0x%04X    \treturn_code = 0x%08X \n", addr, uchar_data,return_code);

		addr = 0x1 ;
		uchar_data = 0x34 ;
		return_code = vme_crate->vme_A32D8_write (addr, uchar_data); //
		printf("vme_A32D8_write:  \t\taddr = 0x%08X    \tuchar_data = 0x%04X    \treturn_code = 0x%08X \n", addr, uchar_data,return_code);

		addr = 0x2 ;
		uchar_data = 0x56 ;
		return_code = vme_crate->vme_A32D8_write (addr, uchar_data); //
		printf("vme_A32D8_write:  \t\taddr = 0x%08X    \tuchar_data = 0x%04X    \treturn_code = 0x%08X \n", addr, uchar_data,return_code);

		addr = 0x3 ;
		uchar_data = 0x78 ;
		return_code = vme_crate->vme_A32D8_write (addr, uchar_data); //
		printf("vme_A32D8_write:  \t\taddr = 0x%08X    \tuchar_data = 0x%04X    \treturn_code = 0x%08X \n", addr, uchar_data,return_code);

		addr = 0 ;
		return_code = vme_crate->vme_A32D32_read (addr, &data); //
		printf("vme_A32D32_read:  \t\taddr = 0x%08X    \tdata = 0x%08X    \treturn_code = 0x%08X \n", addr, data,return_code);

	#endif

	#define VME_D16_WRITE_TEST
	#ifdef VME_D16_WRITE_TEST

		printf("\n");

		addr = 0x0 ;
		ushort_data = 0x1122 ;
		return_code = vme_crate->vme_A32D16_write (addr, ushort_data); //
		printf("vme_A32D16_write: \t\taddr = 0x%08X    \tushort_data = 0x%04X \treturn_code = 0x%08X \n", addr, ushort_data,return_code);

		addr = 0x2 ;
		ushort_data = 0x3344 ;
		return_code = vme_crate->vme_A32D16_write (addr, ushort_data); //
		printf("vme_A32D16_write: \t\taddr = 0x%08X    \tushort_data = 0x%04X \treturn_code = 0x%08X \n", addr, ushort_data,return_code);

		addr = 0 ;
		return_code = vme_crate->vme_A32D32_read (addr, &data); //
		printf("vme_A32D32_read:  \t\taddr = 0x%08X    \tdata = 0x%08X    \treturn_code = 0x%08X \n", addr, data,return_code);

	#endif


	#define SIS_MODULE_READ
	#ifdef SIS_MODULE_READ
		printf("\n");
		addr = 0x30000004 ;
		return_code = vme_crate->vme_A32D32_read (addr, &data); //
		printf("vme_A32D32_read (SIS3316):  \taddr = 0x%08X    \tdata = 0x%08X    \treturn_code = 0x%08X \n", addr, data,return_code);
	#endif


	#define USER_MODULE_TEST
	#ifdef USER_MODULE_TEST
		printf("\n");
		addr = 0xEE1100FC ; // Caen
		return_code = vme_crate->vme_A32D16_read (addr, &ushort_data); //
		printf("vme_A32D16_read (Caen): \taddr = 0x%08X    \tushort_data = 0x%04X  \treturn_code = 0x%08X \n", addr, ushort_data,return_code);

		addr = 0x405c ; // ISEG upper 16 bit of VendorID
		return_code = vme_crate->vme_A16D16_read (addr, &ushort_data); //
		printf("vme_A16D16_read (ISEG): \taddr = 0x%08X    \tushort_data = 0x%04X  \treturn_code = 0x%08X \n", addr, ushort_data,return_code);

		addr = 0x405e ; // ISEG lower 16 bit of VendorID
		return_code = vme_crate->vme_A16D16_read (addr, &ushort_data); //
		printf("vme_A16D16_read (ISEG): \taddr = 0x%08X    \tushort_data = 0x%04X  \treturn_code = 0x%08X \n", addr, ushort_data,return_code);

	#endif



	#define VME_BLT32_READ_TEST
	#ifdef VME_BLT32_READ_TEST

		printf("\n");
		request_nof_words = 0x400 ;
		addr = 0 ;
		data = 0 ;
		for (i=0;i<request_nof_words;i++) {
			return_code = vme_crate->vme_A32D32_write (addr, data); //
			//printf("vme_A32D32_write: \t\taddr = 0x%08X    \tdata = 0x%08X    \treturn_code = 0x%08X \n", addr, data,return_code);
			addr = addr + 4 ;
			data = data + 1 ;
		}

		for (i=0;i<request_nof_words;i++) {
			gl_dma_buffer[i] = 0 ;
		}
		addr = 0x0 ;
		return_code = vme_crate->vme_A32BLT32_read (addr, gl_dma_buffer, request_nof_words, &got_nof_words); //
		printf("vme_A32BLT32_read:  \t\taddr = 0x%08X    \tgot_nof_words = 0x%08X \treturn_code = 0x%08X \n", addr, got_nof_words,return_code);


	#endif

	#define VME_BLT32_WRITE_TEST
	#ifdef VME_BLT32_WRITE_TEST

		printf("\n");
		request_nof_words = 0x10000 ;
		addr = 0 ;
		for (i=0;i<request_nof_words;i++) {
			gl_dma_buffer[i] = i ;
		}
		return_code = vme_crate->vme_A32BLT32_write (addr, gl_dma_buffer, request_nof_words, &written_nof_words); //
		printf("vme_A32BLT32_write:  \t\taddr = 0x%08X    \tgot_nof_words = 0x%08X \treturn_code = 0x%08X \n", addr, written_nof_words,return_code);


		for (i=0;i<request_nof_words;i++) {
			gl_dma_buffer[i] = 0 ;
		}
		addr = 0x0 ;
		return_code = vme_crate->vme_A32BLT32_read (addr, gl_dma_buffer, request_nof_words, &got_nof_words); //
		printf("vme_A32BLT32_read:  \t\taddr = 0x%08X    \tgot_nof_words = 0x%08X \treturn_code = 0x%08X \n", addr, got_nof_words,return_code);

	// check
		for (i=0;i<request_nof_words;i++) {
			if (gl_dma_buffer[i] != i) {
				printf("Error: check vme_A32BLT32_write - vme_A32BLT32_read\n");
				printf("i = 0x%08X    \twritten = 0x%08X \tread = 0x%08X \n", i, i,gl_dma_buffer[i]);
				usleep(500000);
			}
		}



	#endif



		printf("\n");
		printf("\n");
		printf("\n");
		usleep(500000);
	} while(1);



	return 0;
}

