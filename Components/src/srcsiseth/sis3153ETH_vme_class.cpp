/***************************************************************************/
/*                                                                         */
/*  Filename: sis3153ETH_vme_class.cpp                                     */
/*                                                                         */
/*  Funktion:                                                              */
/*                                                                         */
/*  Autor:                CT/TH                                            */
/*  date:                 26.06.2014                                       */
/*  last modification:    19.01.2016                                       */
/*                                                                         */
/* ----------------------------------------------------------------------- */
/*   class_lib_version: 1.0                                                */
/*                                                                         */
/* ----------------------------------------------------------------------- */
/*                                                                         */
/*  SIS  Struck Innovative Systeme GmbH                                    */
/*                                                                         */
/*  Harksheider Str. 102A                                                  */
/*  22399 Hamburg                                                          */
/*                                                                         */
/*  Tel. +49 (0)40 60 87 305 0                                             */
/*  Fax  +49 (0)40 60 87 305 20                                            */
/*                                                                         */
/*  http://www.struck.de                                                   */
/*                                                                         */
/*  ? 2016                                                                 */
/*                                                                         */
/***************************************************************************/

#include "../../include/includesiseth/project_system_define.h"		//define LINUX or WINDOWS
#include "../../include/includesiseth/project_interface_define.h"   //define Interface (sis1100/sis310x, sis3150usb or Ethnernet UDP)

#ifdef ETHERNET_VME_INTERFACE

#include "../../include/includesiseth/sis3153ETH_vme_class.h"

using namespace std;

sis3153eth::sis3153eth (void)
{
 	int status, i;
	strcpy (this->char_messages ,  "no valid UDP socket");

	this->udp_socket_status = -1 ;
	this->udp_port = 0xE000 ; // default

	this->myPC_sock_addr.sin_family = AF_INET;
	this->myPC_sock_addr.sin_port = htons(udp_port);
	this->myPC_sock_addr.sin_addr.s_addr = 0x0 ; //ADDR_ANY;
	memset(&(myPC_sock_addr.sin_zero),0,8);

	this->sis3153_sock_addr_in.sin_family = AF_INET;
	this->sis3153_sock_addr_in.sin_port = htons(udp_port);
	this->sis3153_sock_addr_in.sin_addr.s_addr = 0x0 ; //ADDR_ANY;
	memset(&(sis3153_sock_addr_in.sin_zero),0,8);
	// Create udp_socket
	this->udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (this->udp_socket == -1) {
		this->udp_socket_status = -1 ;
	}
	else {
		this->udp_socket_status = 0 ;
	}
 
	// Set  Receive Timeout
	this->recv_timeout_sec  = 0 ;
	this->recv_timeout_usec = 50000 ; // default 50ms

	status = this->set_UdpSocketOptionTimeout( ) ;

	this->jumbo_frame_enable        = 0 ;
	this->max_nofPacketsPerRequest  = 1;
	this->max_nof_read_lwords	= UDP_NORMAL_READ_PACKET_32bitSIZE;
	this->max_nof_write_lwords	= UDP_MAX_NOF_WRITE_32bitWords;
	
	this->packet_identifier   = 0 ;

	// Info counter
	this->info_udp_receive_timeout_counter                  = 0 ;
	this->info_wrong_cmd_ack_counter                        = 0 ;
	this->info_wrong_received_nof_bytes_counter             = 0 ;
	this->info_wrong_received_packet_id_counter             = 0 ;

	this->info_clear_UdpReceiveBuffer_counter               = 0 ;
	this->info_read_dma_packet_reorder_counter              = 0 ;

	this->udp_single_read_receive_ack_retry_counter         = 0 ;
	this->udp_single_read_req_retry_counter                 = 0 ;

	this->udp_single_write_receive_ack_retry_counter        = 0 ;
	this->udp_single_write_req_retry_counter                = 0 ;


	this->udp_dma_read_receive_ack_retry_counter            = 0 ;
	this->udp_dma_read_req_retry_counter                    = 0 ;

	this->udp_dma_write_receive_ack_retry_counter           = 0 ;
	this->udp_dma_write_req_retry_counter                   = 0 ;

}

/*************************************************************************************/

sis3153eth::sis3153eth (sis3153eth **eth_interface, char *device_ip)
{
	unsigned int i_socket ;
	char pc_ip_addr_string[32] ;
#ifdef WINDOWS
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD(2, 1);
	WSAStartup( wVersionRequested, &wsaData );
#endif
	for(i_socket=0; i_socket < MAX_SOCKETS; i_socket++){
		eth_interface[i_socket] = new sis3153eth;
		eth_interface[i_socket]->set_UdpSocketOptionBufSize(0x2000000);	// 335544432
		//eth_interface[i_socket]->set_UdpSocketBindToDevice("eth0");

		strcpy(pc_ip_addr_string,"") ; // empty if default Lan interface (Window: use IP address to bind in case of 2. 3. 4. .. LAN Interface)
		eth_interface[i_socket]->set_UdpSocketBindMyOwnPort(pc_ip_addr_string);
		eth_interface[i_socket]->set_UdpSocketSIS3153_IpAddress(device_ip);
	}

}

/*************************************************************************************/

int sis3153eth::vmeopen ( void )
{
	int status;
	UINT data;
	UINT found=0;

	strcpy (this->char_messages ,  "sis3153eth UDP port is open");

	return 0;
}

/*************************************************************************************/

int sis3153eth::vmeclose( void ){
#ifdef WINDOWS	
	int rc;
	if(!closesocket(this->udp_socket)){
		rc=0;
	}else{
		rc=1;
	}
#endif
	return 0;
}

/*************************************************************************************/

int sis3153eth::get_vmeopen_messages( CHAR* messages, UINT* nof_found_devices ){

	strcpy (messages,  this->char_messages);
	*nof_found_devices = 1 ;

	return 0;
}
	int udp_socket;
	unsigned int udp_port ;
	struct sockaddr_in sis3153_sock_addr_in   ;
	struct sockaddr_in myPC_sock_addr   ;


int sis3153eth::get_UdpSocketStatus(void ){
	return this->udp_socket_status;
}

int sis3153eth::get_UdpSocketPort(void ){
	return this->udp_port;
}


/*************************************************************************************/

int sis3153eth::set_UdpSocketOptionTimeout(void){
	int return_code ;

#ifdef LINUX
	struct timeval struct_time;
	struct_time.tv_sec  = this->recv_timeout_sec;
	struct_time.tv_usec = this->recv_timeout_usec; //  
	return_code = (setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&struct_time,sizeof(struct timeval)));
#endif

#ifdef WINDOWS
	int msec;
	msec = (1000 * this->recv_timeout_sec) + ((this->recv_timeout_usec+999) / 1000) ;
	return_code = (setsockopt(this->udp_socket, SOL_SOCKET, SO_RCVTIMEO,(char*)&msec,sizeof(msec)));
#endif
	return return_code ;
}


/*************************************************************************************/

	 
int sis3153eth::set_UdpSocketOptionBufSize( int sockbufsize ){
	int return_code ;
	return_code =  (setsockopt(this->udp_socket, SOL_SOCKET,SO_RCVBUF, (char *) &sockbufsize, (int)sizeof(sockbufsize)));
	return return_code ;
}


/*************************************************************************************/

int sis3153eth::set_UdpSocketBindToDevice( char* eth_device){
	int return_code=0;
#ifdef LINUX
#ifndef MAC_OSX
	return_code = setsockopt(this->udp_socket, SOL_SOCKET, SO_BINDTODEVICE, eth_device, sizeof(eth_device)) ;
#endif
#endif
	return return_code;
}

/*************************************************************************************/

int sis3153eth::set_UdpSocketBindMyOwnPort( char* pc_ip_addr_string){
	int return_code;

	this->myPC_sock_addr.sin_family = AF_INET;
	this->myPC_sock_addr.sin_port = htons(this->udp_port);
	this->myPC_sock_addr.sin_addr.s_addr = 0x0 ; //ADDR_ANY;
	memset(&(myPC_sock_addr.sin_zero),0,8);

	if(strlen(pc_ip_addr_string) != 0) {
		this->myPC_sock_addr.sin_addr.s_addr = inet_addr(pc_ip_addr_string);
	}

	do {
		return_code = bind(this->udp_socket,  (struct sockaddr *)&this->myPC_sock_addr, sizeof(this->myPC_sock_addr));
		if (return_code != 0) {
			this->udp_port++;
			this->myPC_sock_addr.sin_port = htons(this->udp_port);
		}
	} while ((return_code == -1) && (this->udp_port < 0xF000)) ;
	return return_code;
}

/*************************************************************************************/

int sis3153eth::set_UdpSocketSIS3153_IpAddress( char* sis3153_ip_addr_string){
	int return_code=0;
	char temp_ip_addr_string[32] ;
	struct hostent *hp;     /* host information */

	this->sis3153_sock_addr_in.sin_family = AF_INET;
	this->sis3153_sock_addr_in.sin_port = htons(this->udp_port);
	this->sis3153_sock_addr_in.sin_addr.s_addr = 0x0 ; //ADDR_ANY;
	memset(&(sis3153_sock_addr_in.sin_zero),0,8);

	strcpy(temp_ip_addr_string,"255.255.255.255") ; // 

	if(strlen(sis3153_ip_addr_string) != 0) {

/* sis3153_ip_addr_string beginn with "SIS3153" or "sis3153" and contains 12 characters: eg. "SIS3153-0123" [0123: Device-SN 123] */
		if(     (   (strncmp(sis3153_ip_addr_string, DHCP_DEVICE_NAME_LARGE_CASE, strlen(DHCP_DEVICE_NAME_LARGE_CASE)) == 0) 
			     || (strncmp(sis3153_ip_addr_string, DHCP_DEVICE_NAME_LOWER_CASE, strlen(DHCP_DEVICE_NAME_LOWER_CASE)) == 0))  
			 && (strlen(sis3153_ip_addr_string) == 12) ){

			hp = gethostbyname(sis3153_ip_addr_string);
			if(hp){
				memcpy((void *)&this->sis3153_sock_addr_in.sin_addr, hp->h_addr_list[0], hp->h_length);
			}
			else{
				this->sis3153_sock_addr_in.sin_addr.s_addr = inet_addr(temp_ip_addr_string); // invalid IP string will set the IP to 255.255.255.255
				return_code=-2;
			}
		}
		else{
			this->sis3153_sock_addr_in.sin_addr.s_addr = inet_addr(sis3153_ip_addr_string); // IP string will set the IP to sis3153_ip_addr_string
		}
	}
	else { 
		this->sis3153_sock_addr_in.sin_addr.s_addr = inet_addr(temp_ip_addr_string); // invalid IP string will set the IP to 255.255.255.255
		return_code = -1;		
	}

	if(this->sis3153_sock_addr_in.sin_addr.s_addr == 0xffffffff ) { // broadcast address
		return_code = -1 ;
	}
	if((this->sis3153_sock_addr_in.sin_addr.s_addr & 0xff) == 0x00 ) { // IP addresses 0.0.0.0 to 0.255.255.255 not allowed as Destination !  
		this->sis3153_sock_addr_in.sin_addr.s_addr = inet_addr(temp_ip_addr_string); // invalid IP string will set the IP to 255.255.255.255
		return_code = -3;	
	}

	return return_code;
}

/*************************************************************************************/

unsigned int sis3153eth::get_UdpSocketNofReadMaxLWordsPerRequest(void){

	return this->max_nof_read_lwords;
}

/*************************************************************************************/

unsigned int sis3153eth::get_UdpSocketNofWriteMaxLWordsPerRequest(void){

	return this->max_nof_write_lwords;
}

/*************************************************************************************/
 
unsigned int sis3153eth::get_class_lib_version(void){
	unsigned int version ;
	version = ((SIS3153ETH_VERSION_MAJOR & 0xff) << 8) + (SIS3153ETH_VERSION_MINOR & 0xff) ;
	return version;
}

/*************************************************************************************/


int sis3153eth::set_UdpSocketReceiveNofPackagesPerRequest(unsigned int nofPacketsPerRequest){
unsigned int nof_packets ;
unsigned int data ;
 
	this->udp_sis3153_register_read(0x4, &data);
	data |= 0x4; // 4 us gap
	this->udp_sis3153_register_write(0x4, data);

	nof_packets = nofPacketsPerRequest ;
	if(nofPacketsPerRequest == 0){
		nof_packets = 1 ;
	}
	if(nofPacketsPerRequest > UDP_MAX_PACKETS_PER_REQUEST){
		nof_packets = UDP_MAX_PACKETS_PER_REQUEST ;
	}
	
	if (this->jumbo_frame_enable == 0) {
		this->max_nof_read_lwords = nof_packets * UDP_NORMAL_READ_PACKET_32bitSIZE;
	}
	else {
		this->max_nof_read_lwords = nof_packets * UDP_JUMBO_READ_PACKET_32bitSIZE;
	}
	this->max_nofPacketsPerRequest = nof_packets ;
	return 0;
}


/*************************************************************************************/

int sis3153eth::get_UdpSocketJumboFrameStatus(void){
	UINT data;
	this->udp_sis3153_register_read(0x4, &data);
	return (data >> 4) & 0x1;
}

/*************************************************************************************/

int sis3153eth::set_UdpSocketEnableJumboFrame(void){
	UINT data;
	this->udp_sis3153_register_read(0x4, &data);
	data |= 0x10;
	this->udp_sis3153_register_write(0x4, data);
	this->jumbo_frame_enable   = 1 ;
	this->max_nof_read_lwords  = this->max_nofPacketsPerRequest * UDP_JUMBO_READ_PACKET_32bitSIZE;
	return 0;
}

/*************************************************************************************/

int sis3153eth::set_UdpSocketDisableJumboFrame(void){
	unsigned int data;
	this->udp_sis3153_register_read(0x4, &data);
	data &= ~0x10;
	this->udp_sis3153_register_write(0x4, data);
	this->jumbo_frame_enable   = 0 ;
	this->max_nof_read_lwords  = this->max_nofPacketsPerRequest * UDP_NORMAL_READ_PACKET_32bitSIZE;
	return 0;
}


/*************************************************************************************/

int sis3153eth::get_UdpSocketGapValue(void){
	UINT data;
	this->udp_sis3153_register_read(0x4, &data);
	return data & 0xF;
}

/*************************************************************************************/

int sis3153eth::set_UdpSocketGapValue(UINT gapValue){
	UINT data;
	this->udp_sis3153_register_read(0x4, &data);
	data = (data & 0xFFFFFFF0) | (gapValue & 0xF);
	this->udp_sis3153_register_write(0x4, data);
	return 0;
}



/*************************************************************************************/


/*************************************************************************************/
/*************************************************************************************/
int sis3153eth::clear_UdpReceiveBuffer(void){
	int return_code ;
	int i ;

#ifdef LINUX
	struct timeval struct_time;
	struct_time.tv_sec  = 0;
	struct_time.tv_usec = 10; //  very short
	return_code = (setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&struct_time,sizeof(struct timeval)));
	//printf("struct_time.tv_sec = %d   struct_time.tv_usec = %d\n",struct_time.tv_sec, struct_time.tv_usec);
#endif

#ifdef WINDOWS
	int msec;
	msec = 1 ;
	return_code = (setsockopt(this->udp_socket, SOL_SOCKET, SO_RCVTIMEO,(char*)&msec,sizeof(msec)));
	//printf("return_code = %d    msec = %d\n",return_code, msec);
#endif


#ifdef LINUX
    socklen_t addr_len;
#endif
#ifdef WIN
	int addr_len;
 #endif
   addr_len = sizeof(struct sockaddr);

   i = 0 ;	
   do {
		return_code = recvfrom(udp_socket, udp_recv_data, 9000, 0,  (struct sockaddr *)&this->sis3153_sock_addr_in, &addr_len );
		i++;
   } while ((return_code != -1) && (i<100))  ;

	if(return_code == -1) {
		return_code = 0;
	}
	else {
		return_code = i;
	}

	this->set_UdpSocketOptionTimeout(); // restore timeouts
	this->info_clear_UdpReceiveBuffer_counter++;
	return return_code ;
}

/*************************************************************************************/


int sis3153eth::udp_retransmit_cmd( int* receive_bytes, char* data_byte_ptr){
	int return_code;

#ifdef LINUX
    socklen_t addr_len;
#endif
#ifdef WIN
	int addr_len;
 #endif
    addr_len = sizeof(struct sockaddr);
	// write Cmd
    this->udp_send_data[0] = 0xEE ; // transmit
    return_code = sendto(this->udp_socket, udp_send_data, 1, 0, (struct sockaddr *)&this->sis3153_sock_addr_in, sizeof(struct sockaddr));
	*receive_bytes = recvfrom(this->udp_socket, this->udp_recv_data, 9000, 0,   (struct sockaddr *)&this->sis3153_sock_addr_in, &addr_len);
	if (*receive_bytes == -1) { // Timeout
		return -1 ;
	}
	memcpy( data_byte_ptr, &udp_recv_data[0], *receive_bytes) ;
	return 0;
}


/*************************************************************************************/


int sis3153eth::udp_reset_cmd( void){
	int return_code;
     // write Cmd
    this->udp_send_data[0] = 0xFF ; // reset
    return_code = sendto(this->udp_socket, udp_send_data, 1, 0, (struct sockaddr *)&this->sis3153_sock_addr_in, sizeof(struct sockaddr));

	return return_code;
}

/**************************************************************************************/





/**************************************************************************************/
void sis3153eth::build_header(unsigned char *buf, unsigned char cmd, unsigned char id, unsigned short len) {

	/* sanity check */
	if (buf != NULL) {
		*buf++ = cmd;
#ifdef VME_FPGA_VERSION_IS_0008_OR_HIGHER
		*buf++ = id;
#endif
		*buf++ =  (len - 1) & 0xFF;
		*buf++ = ((len - 1) >> 8) & 0xFF;
	}
}
/**************************************************************************************/
void sis3153eth::reorderLutInit(reorder_lut_t *lut, size_t len, size_t packetLen) {

	/* sanity check */
	if (lut != NULL) {
		// clear
		for (int i = 0; i < 32; i++) {
			lut[i].offs = NULL;
			lut[i].outstanding = false;
		}
		// fill offsets
		size_t offs = 0;
		for (int i = 0; i < 32 && offs < len; i++) {
			lut[i].offs = offs;
			lut[i].outstanding = true;
			offs += packetLen;
		}
	}
}
/**************************************************************************************/
unsigned char sis3153eth::reorderGetIdx(reorder_lut_t *lut, unsigned char currNum) {

	if (lut != NULL) {
		// find first occurence of lower 4 bits of currNum in index
		unsigned char idx = 0;
		bool found = false;
		for (int i = 0; i < 32 && !found; i++) {
			if ((i & 0xF) == currNum && lut[i].outstanding) {
				found = true;
				lut[i].outstanding = false;
				idx = i;
			}
		}
		if (found) {
			return idx;
		}
	}
	return 0;
}
/**************************************************************************************/





/**************************************************************************************/
/* udp_single_read                                                                    */
/* possible ReturnCodes:                                                              */     
/*       0     : OK                                                                   */
/*       0x111 : PROTOCOL_ERROR_CODE_TIMEOUT                                          */
/*       0x121 : wrong Packet Length after N Retransmit and M Request commands        */
/*       0x122 : wrong received Packet ID after N Retransmit and M Request commands   */ 
/*       0x211 : PROTOCOL_VME_CODE_BUS_ERROR                                          */
/**************************************************************************************/


int sis3153eth::udp_single_read ( unsigned int nof_read_words, UINT* addr_ptr, UINT* data_ptr)
{
    int i;
	int return_code;
#ifdef LINUX
	socklen_t addr_len;
#endif
#ifdef WIN
	int addr_len;
#endif
    unsigned int nof_words;
    unsigned int reg_addr;
	unsigned int expected_nof_words; 
	unsigned short status_reg;

	addr_len = sizeof(struct sockaddr);
	if (nof_read_words == 0) {
		return 0;
	}
	if (nof_read_words > 64) {
		nof_read_words = 64 ;
	}

#ifdef not_used
	/*** Handel Ax/D64 cycle***/
	if (this->vmeHead.Size == 3) {
		if(nof_read_words < 2)	return 3;
		nof_read_words -= nof_read_words%2;
		if(nof_read_words == 0)	return 3;
	}
#endif
	nof_words = nof_read_words + 2;										// Plus two VME Header 32bit-words

	/* request retry loop */
	int request_retry_counter = 0;
	do {

		// send Read Req Cmd
		this->udp_send_data[0] = 0x20 ; // send sis3153 Register Read Req Cmd
		this->udp_send_data[1] = (unsigned char)  (this->packet_identifier) ;        //  
		this->udp_send_data[2] = (unsigned char)  ((nof_words-1) & 0xff);           //  lower length
		this->udp_send_data[3] = (unsigned char) (((nof_words-1) >>  8) & 0xff);    //  upper length 

		this->vme_head_add(nof_read_words);

		for (i=0;i<nof_read_words;i++) {
			reg_addr = addr_ptr[i] ;
			this->udp_send_data[(4*i)+12] = (unsigned char)  (reg_addr & 0xff) ;        // address(7 dwonto 0)
			this->udp_send_data[(4*i)+13] = (unsigned char) ((reg_addr >>  8) & 0xff) ; // address(15 dwonto 8)
			this->udp_send_data[(4*i)+14] = (unsigned char) ((reg_addr >> 16) & 0xff) ; // address(23 dwonto 16)
			this->udp_send_data[(4*i)+15] = (unsigned char) ((reg_addr >> 24) & 0xff) ; // address(31 dwonto 24)
		}
		return_code = sendto(this->udp_socket, udp_send_data, 12 + (4*nof_read_words), 0,(struct sockaddr *)&this->sis3153_sock_addr_in, sizeof(struct sockaddr));

		int retransmit_retry_counter;
		retransmit_retry_counter = 0 ;
		do {
			// read Ackn.
			return_code = recvfrom(udp_socket, udp_recv_data, 9000, 0,(struct sockaddr *)&this->sis3153_sock_addr_in, &addr_len );
			if (return_code == -1) { // timeout
				if (retransmit_retry_counter < UDP_RETRANSMIT_RETRY)  { //  
					this->udp_send_data[0] = 0xEE ; // retransmit command, sis3316 will send last packet again
					sendto(this->udp_socket, udp_send_data, 1, 0, (struct sockaddr *)&this->sis3153_sock_addr_in, sizeof(struct sockaddr));
					this->udp_single_read_receive_ack_retry_counter++ ;
				}
				retransmit_retry_counter++ ;
			}
		} while ((return_code == -1) && (retransmit_retry_counter <= UDP_RETRANSMIT_RETRY)) ; // retry up to N times
#ifdef DEBUG_PRINTS
			printf("udp_single_read:  \tudp_recv_data[0] = 0x%02X    \treturn_code = 0x%08X \n", (unsigned char)udp_recv_data[0], return_code);
			printf("udp_single_read:  \t0x%02X  0x%02X \n", (unsigned char)udp_recv_data[1], (unsigned char)udp_recv_data[2]);
			printf("udp_single_read:  \t0x%02X  0x%02X 0x%02X 0x%02X \n", (unsigned char)udp_recv_data[6], (unsigned char)udp_recv_data[5], (unsigned char)udp_recv_data[4], (unsigned char)udp_recv_data[3]);
#endif

		if(return_code == -1) { // timeout
			this->info_udp_receive_timeout_counter++;
			return_code = PROTOCOL_ERROR_CODE_TIMEOUT ;
			}
		else {
			if (udp_recv_data[1] != this->packet_identifier) {
				return_code = PROTOCOL_ERROR_CODE_WRONG_PACKET_IDENTIFIER;
				request_retry_counter++;
				this->udp_single_read_req_retry_counter++;
			}
			else {
				if((udp_recv_data[0]&0x0F) == 0x02){	// vme: zero packet 
					return_code = PROTOCOL_VME_CODE_BUS_ERROR ;
				}
				else{ // 0x4 valid data
					if(return_code == (int)(3 + (4*nof_read_words))) {
						memcpy((unsigned char*)data_ptr, &udp_recv_data[3], nof_read_words*4) ;
						return_code = 0;
					}
					else { 
						return_code = PROTOCOL_ERROR_CODE_WRONG_NOF_RECEVEID_BYTES;
					}	
				}
			}	
		}

	} while ( (return_code == PROTOCOL_ERROR_CODE_WRONG_PACKET_IDENTIFIER)  && (request_retry_counter < UDP_REQUEST_RETRY));

	this->packet_identifier++;
	if (return_code != 0) {
		if(return_code == PROTOCOL_ERROR_CODE_WRONG_PACKET_IDENTIFIER)  {this->info_wrong_received_packet_id_counter++;}
		if(return_code == PROTOCOL_ERROR_CODE_WRONG_NOF_RECEVEID_BYTES) {this->info_wrong_received_nof_bytes_counter++;}
	}
	return return_code;

}

/**************************************************************************************/

/**************************************************************************************/
/* udp_single_write                                                                   */
/* possible ReturnCodes:                                                              */     
/*       0     : OK                                                                   */
/*       0x111 : PROTOCOL_ERROR_CODE_TIMEOUT                                          */
/*       0x122 : wrong received Packet ID after N Retransmit and M Request commands   */ 
/*       0x211 : PROTOCOL_VME_CODE_BUS_ERROR                                          */
/*       0x2xx : PROTOCOL_VME_CODES                                                   */
/**************************************************************************************/

int sis3153eth::udp_single_write ( unsigned int nof_write_words, UINT* addr_ptr, UINT* data_ptr)
{
    int i,j;
	int return_code;
#ifdef LINUX
	socklen_t addr_len;
#endif
#ifdef WIN
	int addr_len;
#endif
    unsigned int nof_words, temp;
    unsigned int reg_addr, reg_data;
    unsigned short nof_written_data;
	UINT local_work[2];

    addr_len = sizeof(struct sockaddr);
	if (nof_write_words == 0) {
		return 0;
	}

	if (nof_write_words > 64) {
		nof_write_words = 64 ;
	}

#ifdef not_used // !
	/*** Handel Ax/D64 cycle***/
	if (this->vmeHead.Size == 3) {
		if(nof_write_words < 2)		return 3;
		nof_write_words -= nof_write_words%2;
		if(nof_write_words == 0)	return 3;
	}
#endif

	switch(this->vmeHead.Size){
		case 0:			//  8Bit
			*data_ptr = ((*data_ptr)&0XFF)<< 24 | ((*data_ptr)&0XFF)<< 16 | ((*data_ptr)&0XFF) << 8 | ((*data_ptr)&0XFF);
			break;
		case 1:			// 16Bit
			*data_ptr = ((*data_ptr)&0XFFFF) << 16 | ((*data_ptr)&0XFFFF);
			break;
		case 2:			// 32Bit
		break;

	default:
		break;
	}

	nof_words = nof_write_words + 3;	// Addr + N * Data + 2 * VME Header
	 

	/* request retry loop */
	int request_retry_counter = 0;
	do {

		this->udp_send_data[0] = 0x20 ; // send udp_single_write Req Cmd
		this->udp_send_data[1] = (unsigned char)  (this->packet_identifier) ;			// packet_identifier
		this->udp_send_data[2] = (unsigned char)  ((nof_words-1) & 0xff);				//  lower length
		this->udp_send_data[3] = (unsigned char) (((nof_words-1) >>  8) & 0xff);		//  upper length 

		this->vme_head_add(nof_write_words);													// build vme head

		for (i=0;i<nof_write_words;i++) {
			reg_addr = addr_ptr[i] ;
			this->udp_send_data[(8*i)+12] = (unsigned char)  (reg_addr & 0xff) ;        // address(7 dwonto 0)
			this->udp_send_data[(8*i)+13] = (unsigned char) ((reg_addr >>  8) & 0xff) ; // address(15 dwonto 8)
			this->udp_send_data[(8*i)+14] = (unsigned char) ((reg_addr >> 16) & 0xff) ; // address(23 dwonto 16)
			this->udp_send_data[(8*i)+15] = (unsigned char) ((reg_addr >> 24) & 0xff) ; // address(31 dwonto 24)
			reg_data = data_ptr[i] ;
			this->udp_send_data[(8*i)+16]  = (unsigned char)  (reg_data & 0xff) ;        // reg_data(7 dwonto 0)
			this->udp_send_data[(8*i)+17]  = (unsigned char) ((reg_data >>  8) & 0xff) ; // reg_data(15 dwonto 8)
			this->udp_send_data[(8*i)+18]  = (unsigned char) ((reg_data >> 16) & 0xff) ; // reg_data(23 dwonto 16)
			this->udp_send_data[(8*i)+19]  = (unsigned char) ((reg_data >> 24) & 0xff) ; // reg_data(31 dwonto 24)
		}
		return_code = sendto(this->udp_socket, udp_send_data, 12 + (8*nof_write_words), 0, (struct sockaddr *)&this->sis3153_sock_addr_in, sizeof(struct sockaddr));

		int retransmit_retry_counter;
		retransmit_retry_counter = 0 ;
		do {
			// read Ackn.
			return_code = recvfrom(udp_socket, udp_recv_data, 9000, 0,  (struct sockaddr *)&this->sis3153_sock_addr_in, &addr_len );
			if (return_code == -1) { // timeout
				if (retransmit_retry_counter < UDP_RETRANSMIT_RETRY)  { //  
					this->udp_send_data[0] = 0xEE ; // retransmit command, sis3316 will send last packet again
					sendto(this->udp_socket, udp_send_data, 1, 0, (struct sockaddr *)&this->sis3153_sock_addr_in, sizeof(struct sockaddr));
					this->udp_single_write_receive_ack_retry_counter++ ;
				}
				retransmit_retry_counter++ ;
			}
		} while ((return_code == -1) && (retransmit_retry_counter <= UDP_RETRANSMIT_RETRY)) ; // retry up to N times
#ifdef DEBUG_PRINTS
			printf("udp_single_write:  \tudp_recv_data[0] = 0x%02X    \treturn_code = 0x%08X \n", (unsigned char)udp_recv_data[0], return_code);
			printf("udp_single_write:  \t0x%02X  0x%02X \n", (unsigned char)udp_recv_data[1], (unsigned char)udp_recv_data[2]);
			printf("udp_single_write:  \t0x%02X  0x%02X 0x%02X 0x%02X \n", (unsigned char)udp_recv_data[6], (unsigned char)udp_recv_data[5], (unsigned char)udp_recv_data[4], (unsigned char)udp_recv_data[3]);
#endif

		if(return_code == -1) { // timeout
			this->info_udp_receive_timeout_counter++;
			return_code = PROTOCOL_ERROR_CODE_TIMEOUT ;
			}
		else {
			if (udp_recv_data[1] != this->packet_identifier) {
				return_code = PROTOCOL_ERROR_CODE_WRONG_PACKET_IDENTIFIER;
				request_retry_counter++;
				this->udp_single_write_req_retry_counter++;
			}
			else {
				if((udp_recv_data[0]&0x0F) != 0x02){	// // vme: no zero packet 
					if(  udp_recv_data[6] == 0x02)  {	 
						// vme bus error
						return_code = 0x200 + (unsigned int) udp_recv_data[5];
					}
					else{
						return_code = PROTOCOL_VME_CODE_BUS_ERROR; //  
					}
				}
				else{ // Zero packets
					return_code = 0;
				}
			}	
		}

		//if(return_code != 7 ) {
			// packet not complete
			//return_code = PROTOCOL_ERROR_CODE_WRONG_NOF_RECEVEID_BYTES;
		//}


	} while ( (return_code == PROTOCOL_ERROR_CODE_WRONG_PACKET_IDENTIFIER)  && (request_retry_counter < UDP_REQUEST_RETRY));
		
	this->packet_identifier++;

	if (return_code != 0) {
		if(return_code == PROTOCOL_ERROR_CODE_WRONG_PACKET_IDENTIFIER)  {this->info_wrong_received_packet_id_counter++;}
		if(return_code == PROTOCOL_ERROR_CODE_WRONG_NOF_RECEVEID_BYTES) {this->info_wrong_received_nof_bytes_counter++;}
	}

	return return_code;

}

/*****************************************************************************************************************************/
/*****************************************************************************************************************************/
/**************************************************************************************/

/*******************************************************************************************/
/* udp_DMA_read, udp_sub_DMA_read                                                          */ 
/* possible ReturnCodes:                                                                   */  
/*       0     : OK                                                                        */
/*       0x111 : PROTOCOL_ERROR_CODE_TIMEOUT                                               */
/*       0x120 : wrong received Packet CMD after N Retransmit                              */
/*       0x122 : wrong received Packet ID after N Retransmit                               */
/*       0x124 : PROTOCOL_ERROR_CODE_HEADER_STATUS                                         */
/*                                                                                         */
/*       0x211 : PROTOCOL_VME_CODE_BUS_ERROR                                               */
/*                                                                                         */
/*       0x311 : PROTOCOL_DISORDER_AND_VME_CODE_BUS_ERROR                                  */
/*******************************************************************************************/

int sis3153eth::udp_sub_DMA_read ( unsigned int nof_read_words, UINT  addr, UINT* data_ptr, UINT* nof_got_words)
{
    int i;
	int return_code;
#ifdef LINUX
	socklen_t addr_len;
#endif
#ifdef WIN
	int addr_len;
#endif
    unsigned int nof_words;
  	int send_length;
 
	unsigned int udp_data_copy_to_buffer_index ;
	unsigned int nof_got_data_bytes_per_packet ;
	int rest_length_byte ;
//	unsigned int packet_cmd;
//	unsigned int vme_status;
	unsigned int packet_legh = 3;
	unsigned char* uchar_ptr;
	int receive_bytes;

	unsigned int  expected_nof_packets;
	unsigned int  max_expected_packet_byte_size;
	int retransmit_retry_counter;
	unsigned int uint_last_packet_no;
	unsigned int last_packet_byte_size;

	unsigned char uchar_packet_status;
	unsigned char uchar_packet_number;
	unsigned char uchar_soft_packet_number;
	unsigned char uchar_vme_status;

	reorder_lut_t reorder_lut[32];

	//unsigned int *data_ptr;
    addr_len = sizeof(struct sockaddr);

	*nof_got_words = 0 ;
	nof_words = nof_read_words ;
	if (nof_read_words == 0) {
		return 0 ;
		//nof_words = 1 ;
	}
	if (nof_read_words > this->max_nof_read_lwords) {
		nof_words = this->max_nof_read_lwords ;
	}
	
	

	/* request retry loop */

	// send Read Req Cmd
	this->udp_send_data[0] = 0x30 ; // send sis3153 Fifo Read Req Cmd
	this->udp_send_data[1] = (unsigned char)  (this->packet_identifier) ;        // packet_identifier
	this->udp_send_data[2] = (unsigned char)  ((packet_legh-1) & 0xff);           //  lower length
	this->udp_send_data[3] = (unsigned char) (((packet_legh-1) >>  8) & 0xff);    //  upper length 

	this->vme_head_add(nof_words);												// VME Header [4 - 12]

	send_length = 12;
	this->udp_send_data[send_length++] = (unsigned char)  (addr & 0xff) ;        // address(7 dwonto 0)
	this->udp_send_data[send_length++] = (unsigned char) ((addr >>  8) & 0xff) ; // address(15 dwonto 8)
	this->udp_send_data[send_length++] = (unsigned char) ((addr >> 16) & 0xff) ; // address(23 dwonto 16)
	this->udp_send_data[send_length++] = (unsigned char) ((addr >> 24) & 0xff) ; // address(31 dwonto 24)

	return_code = sendto(this->udp_socket, udp_send_data, send_length, 0, (struct sockaddr *)&this->sis3153_sock_addr_in, sizeof(struct sockaddr));
	//if (return_code != send_length) {
	// sendto error !
	//}

	uchar_ptr = (unsigned char* ) data_ptr ;
	rest_length_byte = 4 * nof_words ;
	udp_data_copy_to_buffer_index = 0 ;
	uchar_soft_packet_number = 0;

	//expected_nof_packets is used to force a retry cycle in case of timeout if expected_nof_packets == 1
	if (this->jumbo_frame_enable == 0) {
		expected_nof_packets = (nof_words + (UDP_NORMAL_READ_PACKET_32bitSIZE - 1)) / UDP_NORMAL_READ_PACKET_32bitSIZE ;
		max_expected_packet_byte_size = 4 * UDP_NORMAL_READ_PACKET_32bitSIZE ;
	}
	else {
		expected_nof_packets = (nof_words + (UDP_JUMBO_READ_PACKET_32bitSIZE - 1)) / UDP_JUMBO_READ_PACKET_32bitSIZE ;
		max_expected_packet_byte_size = 4 * UDP_JUMBO_READ_PACKET_32bitSIZE ;
	}
	uint_last_packet_no =  (unsigned char)  (expected_nof_packets - 1),
	last_packet_byte_size = (4 *nof_words) - (uint_last_packet_no * max_expected_packet_byte_size) ;

		// fill reorder lut
	reorderLutInit(reorder_lut, rest_length_byte, jumbo_frame_enable ? 4*UDP_JUMBO_READ_PACKET_32bitSIZE : 4*UDP_NORMAL_READ_PACKET_32bitSIZE);

	//int retransmit_retry_counter;
	retransmit_retry_counter = 0 ;
	do {
		unsigned char uchar_packet_disorder_flag  ;
		uchar_packet_disorder_flag = 0 ;

		do {
			// read Ackn.
			return_code = recvfrom(this->udp_socket, this->udp_recv_data, 9000, 0,   (struct sockaddr *)&this->sis3153_sock_addr_in, &addr_len);
			if (return_code == -1)   { // timeout
				if (expected_nof_packets == 1) { // in this case try a retry !
					if (retransmit_retry_counter < UDP_RETRANSMIT_RETRY)  { //  
						this->udp_send_data[0] = 0xEE ; // retransmit command, sis3316 send last packet again
						sendto(this->udp_socket, udp_send_data, 1, 0, (struct sockaddr *)&this->sis3153_sock_addr_in, sizeof(struct sockaddr));
						this->udp_dma_read_receive_ack_retry_counter++ ;
					}
					retransmit_retry_counter++ ;
				}
			}
		} while ((return_code == -1) && (retransmit_retry_counter <= UDP_RETRANSMIT_RETRY) && (expected_nof_packets == 1) ) ; // retry up to 3 times
#ifdef DEBUG_PRINTS
			printf("udp_sub_DMA_read:  \tudp_recv_data[0] = 0x%02X    \treturn_code = 0x%08X \n", (unsigned char)udp_recv_data[0], return_code);
			printf("udp_sub_DMA_read:  \t0x%02X  0x%02X \n", (unsigned char)udp_recv_data[1], (unsigned char)udp_recv_data[2]);
			printf("udp_sub_DMA_read:  \t0x%02X  0x%02X 0x%02X 0x%02X \n", (unsigned char)udp_recv_data[6], (unsigned char)udp_recv_data[5], (unsigned char)udp_recv_data[4], (unsigned char)udp_recv_data[3]);
#endif

		receive_bytes = return_code ;
		if(return_code == -1) { // timeout
			this->packet_identifier++;
			return -1 ;
		}

		// check packet_identifier
		if(udp_recv_data[1] != this->packet_identifier) {
			this->packet_identifier++;
			return PROTOCOL_ERROR_CODE_WRONG_PACKET_IDENTIFIER ;
		}

		// check Packet cmd
		if((udp_recv_data[0] & 0xf0) != 0x30) {
			this->packet_identifier++;
			return PROTOCOL_ERROR_CODE_WRONG_ACK ;
		}
		
		uchar_packet_status =  (udp_recv_data[2] & 0x70) >> 4 ;       // 
		if (uchar_packet_status != 0x0) {
			this->packet_identifier++;
			return PROTOCOL_ERROR_CODE_HEADER_STATUS ;
		}



		uchar_packet_number = (unsigned char) udp_recv_data[2] & 0xf ;
		// check Packet number
		if (uchar_packet_number != uchar_soft_packet_number) {
			uchar_packet_disorder_flag = 1 ;
			this->info_read_dma_packet_reorder_counter++; // only info --> will be reordered
			//printf("Info udp_sub_DMA_read: wrong packet order\n");
			// Observation: Windows in combination with a fire-wall (symantec) changes the order of the incoming packets (runs on multiple cores !) 
		}
		// prepare next soft_packet_number
		uchar_soft_packet_number = (uchar_soft_packet_number + 1) & 0xf;
		// adjust destination pointer based on packet number
		udp_data_copy_to_buffer_index = reorderGetIdx(reorder_lut, uchar_packet_number); // reorder packets preparation

		uchar_vme_status    =  udp_recv_data[0]  & 0x06  ;
		if (uchar_vme_status == 0x2) { // get no data : VME BusError or Arbitration Error
			this->packet_identifier++;
			if(uchar_packet_disorder_flag == 0) { // indicates right order of incoming packets !
				return PROTOCOL_VME_CODE_BUS_ERROR ;
			}
			else {
				return PROTOCOL_DISORDER_AND_VME_CODE_BUS_ERROR ; // --> VME BusError and packet disorder -> not to handle , yet
			}
		} // get no data : VME BusError or Arbitration Error

		// else get valid data (uchar_vme_status == 0x4)
		nof_got_data_bytes_per_packet = receive_bytes-3  ;
		return_code = 0 ;

		if (uchar_vme_status == 0x0) { // get packet 0 to N-1
			memcpy(uchar_ptr + reorder_lut[udp_data_copy_to_buffer_index].offs, &udp_recv_data[3], nof_got_data_bytes_per_packet) ;// reorder packets
			rest_length_byte = rest_length_byte - nof_got_data_bytes_per_packet ;
			*nof_got_words = *nof_got_words + (nof_got_data_bytes_per_packet/4) ;
		}
		else {

			if(uchar_packet_disorder_flag == 0) { // indicates right order of incoming packets !
				if((nof_got_data_bytes_per_packet == rest_length_byte) || (nof_got_data_bytes_per_packet == max_expected_packet_byte_size)) { // valid packet size
					memcpy(uchar_ptr + reorder_lut[udp_data_copy_to_buffer_index].offs, &udp_recv_data[3], nof_got_data_bytes_per_packet) ;// reorder packets
					rest_length_byte = rest_length_byte - nof_got_data_bytes_per_packet ;
					*nof_got_words = *nof_got_words + (nof_got_data_bytes_per_packet/4) ;
				}
				else { // wrong nof got bytes --> VME BusError
					memcpy(uchar_ptr + reorder_lut[udp_data_copy_to_buffer_index].offs, &udp_recv_data[3], nof_got_data_bytes_per_packet) ;// reorder packets
					rest_length_byte = rest_length_byte - nof_got_data_bytes_per_packet ;
					*nof_got_words = *nof_got_words + (nof_got_data_bytes_per_packet/4) ;
					this->packet_identifier++;
					return PROTOCOL_VME_CODE_BUS_ERROR ;
				}
			}
			else { // wrong order of incoming packets !
				if(nof_got_data_bytes_per_packet == max_expected_packet_byte_size) { // max packet size --> OK
					memcpy(uchar_ptr + reorder_lut[udp_data_copy_to_buffer_index].offs, &udp_recv_data[3], nof_got_data_bytes_per_packet) ;// reorder packets
					rest_length_byte = rest_length_byte - nof_got_data_bytes_per_packet ;
					*nof_got_words = *nof_got_words + (nof_got_data_bytes_per_packet/4) ;
				}
				else { // not max packet size
					if ( (nof_got_data_bytes_per_packet == last_packet_byte_size) && ( uchar_packet_number == (unsigned char)uint_last_packet_no) ) { // last packet with last packet size
						memcpy(uchar_ptr + reorder_lut[udp_data_copy_to_buffer_index].offs, &udp_recv_data[3], nof_got_data_bytes_per_packet) ;// reorder packets
						rest_length_byte = rest_length_byte - nof_got_data_bytes_per_packet ;
						*nof_got_words = *nof_got_words + (nof_got_data_bytes_per_packet/4) ;
					}
					else { // --> VME BusError and packet disorder -> not to handle , yet
						this->packet_identifier++;
						return PROTOCOL_DISORDER_AND_VME_CODE_BUS_ERROR ; // --> VME BusError and packet disorder -> not to handle , yet
					}
				}
			}
		}

	} while ((rest_length_byte > 0) && (return_code == 0)) ;
	//} while ((rest_length_byte > 0)) ;

	this->packet_identifier++;
    return 0;


}



int sis3153eth::udp_DMA_read ( unsigned int nof_read_words, UINT addr, UINT* data_ptr, UINT* got_nof_words )
{
    int error;
	int return_code;
	UINT new_addr;
    unsigned int rest_length_words;
    unsigned int req_nof_words;
    unsigned int data_buffer_index;
    unsigned int sub_got_nof_words;

	*got_nof_words = 0x0 ;
	if (nof_read_words == 0) {
		return 0 ;
	}

#ifdef not_used
	/*** Handel Ax/D64 cycle***/
	if (this->vmeHead.Size == 3) {
		if(nof_read_words < 2)		return 3;
		nof_read_words -= nof_read_words%2;
		if (nof_read_words == 0)	return 3;
	}
#endif
	error = 0 ;
	rest_length_words = nof_read_words;
	data_buffer_index = 0 ;
	new_addr = addr;

	do {
		if (rest_length_words >= this->max_nof_read_lwords) {
			req_nof_words = this->max_nof_read_lwords ;
		}
		else {
			req_nof_words = rest_length_words ;
		}
		//printf("* New_Addr: 0x%08X\t req_nof_words: 0x%08X\n", new_addr,req_nof_words);

		int request_retry = 0;
		do {
			return_code = this->udp_sub_DMA_read ( req_nof_words, new_addr, &data_ptr[data_buffer_index], &sub_got_nof_words) ;
			if(return_code == PROTOCOL_ERROR_CODE_WRONG_PACKET_IDENTIFIER) {
				this->clear_UdpReceiveBuffer();
			}
			request_retry++;
		} while((return_code == PROTOCOL_ERROR_CODE_WRONG_PACKET_IDENTIFIER) && (request_retry < UDP_REQUEST_RETRY+1));

		if ((return_code == 0) || (return_code == PROTOCOL_VME_CODE_BUS_ERROR)){ //  
			data_buffer_index = data_buffer_index + sub_got_nof_words ;
			rest_length_words = rest_length_words - sub_got_nof_words ; 
			if(this->vmeHead.F == 0){
				if(this->vmeHead.Space == 4){ // VME
					new_addr = new_addr + (sub_got_nof_words*4);
				}
				else {
					new_addr = new_addr + (sub_got_nof_words);
				}
			}
		}

	} while ((return_code == 0) && (rest_length_words>0)) ;
 

	if(return_code == -1) {
		return_code = PROTOCOL_ERROR_CODE_TIMEOUT ;
	}
	if(return_code == PROTOCOL_ERROR_CODE_WRONG_ACK) {
		this->clear_UdpReceiveBuffer();
	}
	if(return_code == PROTOCOL_ERROR_CODE_WRONG_NOF_RECEVEID_BYTES) {
		this->clear_UdpReceiveBuffer();
	}
	if(return_code == PROTOCOL_DISORDER_AND_VME_CODE_BUS_ERROR) {
		this->clear_UdpReceiveBuffer();
	}

	*got_nof_words = data_buffer_index ;
    return return_code;

}

/********************************************************************************************************************************/






int sis3153eth::udp_sub_DMA_write ( unsigned int nof_write_words, UINT  addr, UINT* data_ptr,  UINT* nof_written_words)
{
    int i, status;
	int return_code;
#ifdef LINUX
	socklen_t addr_len;
#endif
#ifdef WIN
	int addr_len;
#endif
    unsigned int nof_words;
    unsigned int send_data;
  	int send_length;
 
	unsigned int udp_data_copy_to_buffer_index ;
	unsigned int nof_read_data_bytes ;
	int rest_length_byte ;
	unsigned int soft_packet_number;
	unsigned int packet_number;
	unsigned int packet_status;
	unsigned int packet_cmd;
	unsigned int vme_status;
	int receive_bytes;
	int retransmit_retry_counter;
	unsigned char* uchar_ptr;
	unsigned char uchar_packet_status;
	unsigned char uchar_vme_status;

	//unsigned int *data_ptr;
    addr_len = sizeof(struct sockaddr);

	*nof_written_words = 0x0 ;
	nof_words = nof_write_words ;
	if (nof_write_words == 0) {
		return 0 ;
		//nof_words = 1 ;
	}
	if (nof_write_words > this->max_nof_write_lwords) {
		nof_words = this->max_nof_write_lwords ;
	}


	// send Read Req Cmd
	this->udp_send_data[0] = 0x30 ; // send udp_DMA_write Req Cmd

	this->udp_send_data[1] = (unsigned char)  (this->packet_identifier) ;        // address(7 dwonto 0)
	this->udp_send_data[2] = (unsigned char)  ((nof_words+2) & 0xff);           //  lower length
    this->udp_send_data[3] = (unsigned char) (((nof_words+2) >>  8) & 0xff);    //  upper length

	this->vme_head_add(nof_words);

	this->udp_send_data[12] = (unsigned char)  (addr & 0xff) ;        // address(7 dwonto 0)
	this->udp_send_data[13] = (unsigned char) ((addr >>  8) & 0xff) ; // address(15 dwonto 8)
	this->udp_send_data[14] = (unsigned char) ((addr >> 16) & 0xff) ; // address(23 dwonto 16)
	this->udp_send_data[15] = (unsigned char) ((addr >> 24) & 0xff) ; // address(31 dwonto 24)


	send_length = 16 + (4*nof_words);
	for (i=0;i<nof_words;i++) {
		send_data = data_ptr[i] ;
		this->udp_send_data[16+(4*i)] = (unsigned char)  (send_data & 0xff) ;        // send_data(7 dwonto 0)
		this->udp_send_data[17+(4*i)] = (unsigned char) ((send_data >>  8) & 0xff) ; // send_data(15 dwonto 8)
		this->udp_send_data[18+(4*i)] = (unsigned char) ((send_data >> 16) & 0xff) ; // send_data(23 dwonto 16)
		this->udp_send_data[19+(4*i)] = (unsigned char) ((send_data >> 24) & 0xff) ; // send_data(31 dwonto 24)
	}

    return_code = sendto(this->udp_socket, udp_send_data, send_length, 0, (struct sockaddr *)&this->sis3153_sock_addr_in, sizeof(struct sockaddr));
	//if (return_code != send_length) {
	//	printf("sendto: return_code = 0x%08x \n",return_code);
	//}

	retransmit_retry_counter = 0 ;
	do {
		// read Ackn.
		return_code = recvfrom(udp_socket, udp_recv_data, 9000, 0,  (struct sockaddr *)&this->sis3153_sock_addr_in, &addr_len );
		if (return_code == -1)   { // timeout
			if (retransmit_retry_counter < UDP_RETRANSMIT_RETRY)  { //  
				this->udp_send_data[0] = 0xEE ; // retransmit command, sis3316 send last packet again
				sendto(this->udp_socket, udp_send_data, 1, 0, (struct sockaddr *)&this->sis3153_sock_addr_in, sizeof(struct sockaddr));
				this->udp_dma_write_receive_ack_retry_counter++ ;
			}
			retransmit_retry_counter++ ;
		}
	} while ((return_code == -1) && (retransmit_retry_counter <= UDP_RETRANSMIT_RETRY) ) ; // retry up to N times
 
#ifdef DEBUG_PRINTS
		printf("udp_sub_DMA_write:  \tudp_recv_data[0] = 0x%02X    \treturn_code = 0x%08X \n", (unsigned char)udp_recv_data[0], return_code);
		printf("udp_sub_DMA_write:  \t0x%02X  0x%02X \n", (unsigned char)udp_recv_data[1], (unsigned char)udp_recv_data[2]);
		printf("udp_sub_DMA_write:  \t0x%02X  0x%02X 0x%02X 0x%02X \n", (unsigned char)udp_recv_data[6], (unsigned char)udp_recv_data[5], (unsigned char)udp_recv_data[4], (unsigned char)udp_recv_data[3]);
#endif

	receive_bytes = return_code ;
	if(return_code == -1) { // timeout
		this->packet_identifier++;
		return -1 ;
	}

	// check packet_identifier
	if(udp_recv_data[1] != this->packet_identifier) {
		this->packet_identifier++;
		return PROTOCOL_ERROR_CODE_WRONG_PACKET_IDENTIFIER ;
	}

	// check Packet cmd
	if((udp_recv_data[0] & 0xf0) != 0x30) {
		this->packet_identifier++;
		return PROTOCOL_ERROR_CODE_WRONG_ACK ;
	}
		
	uchar_packet_status =  (udp_recv_data[2] & 0x70) >> 4 ;       // 
	if (uchar_packet_status != 0x0) {
		this->packet_identifier++;
		return PROTOCOL_ERROR_CODE_HEADER_STATUS ;
	}

	if(receive_bytes != 7 ) {
		//packet not complete
		this->packet_identifier++;
		return PROTOCOL_ERROR_CODE_WRONG_NOF_RECEVEID_BYTES ;
	}


	uchar_vme_status    =  udp_recv_data[0]  & 0x06  ;
	if (uchar_vme_status == 0x2) { // OK // Zero packets
		this->packet_identifier++;
		*nof_written_words = nof_write_words ;
		return 0 ;
	}

	//else { // valid data (VME Error code and nof_written_words
	if(  udp_recv_data[6] == 0x02)  {	 
			// vme bus error
		return_code = 0x200 + (unsigned int) udp_recv_data[5];
		*nof_written_words = ( (((unsigned int) udp_recv_data[4]) << 8) + ((unsigned int) udp_recv_data[3]) ) / 4 ;
	}
	else{
		return_code = PROTOCOL_VME_CODE_BUS_ERROR; //  
		*nof_written_words = 0 ;
	}
	this->packet_identifier++;
    return return_code; 
}
/*******************************************************************************************/
/* udp_sub_DMA_write, udp_DMA_write                                                        */ 
/* possible ReturnCodes:                                                                   */  
/*       0     : OK                                                                        */
/*       -1    : PROTOCOL_ERROR_CODE_TIMEOUT                                               */
/*       0x111 : PROTOCOL_ERROR_CODE_TIMEOUT                                               */
/*       0x120 : wrong received Packet CMD after N Retransmit                              */
/*       0x121 : PROTOCOL_ERROR_CODE_WRONG_NOF_RECEVEID_BYTES                              */
/*       0x122 : wrong received Packet ID after N Retransmit                               */
/*       0x124 : PROTOCOL_ERROR_CODE_HEADER_STATUS                                         */
/*                                                                                         */
/*       0x211 : PROTOCOL_VME_CODE_BUS_ERROR                                               */
/*                                                                                         */
/*******************************************************************************************/


/**************************************************************************************/

int sis3153eth::udp_DMA_write ( unsigned int nof_write_words, UINT addr, UINT* data_ptr, UINT* written_nof_words )
{
    int error;
	int return_code;
    unsigned int rest_length_words;
    unsigned int req_nof_words;
    unsigned int data_buffer_index;
    unsigned int new_addr;
    unsigned int sub_written_nof_words;

	*written_nof_words = 0x0 ;
	if (nof_write_words == 0) {
		return 0 ;
	}

#ifdef not_used
	/*** Handel Ax/D64 cycle***/
	if (this->vmeHead.Size == 3) {
		if(nof_write_words < 2)		return -3;
		nof_write_words -= nof_write_words%2;
		if(nof_write_words == 0)	return -3;
	}
#endif
	error = 0 ;
	rest_length_words = nof_write_words ;
	data_buffer_index = 0 ;

	//printf("receive_bytes  %d   Ack = %2x  Status = %2x  data = %2x \n",receive_bytes, (unsigned char) udp_recv_data[0], (unsigned char) udp_recv_data[1], (unsigned char) udp_recv_data[2]);
	new_addr = addr;

	do {
		if (rest_length_words >= this->max_nof_write_lwords) {
			req_nof_words = this->max_nof_write_lwords ;
		}
		else {
			req_nof_words = rest_length_words ;
		}


		int request_retry = 0;
		do {
			return_code = this->udp_sub_DMA_write ( req_nof_words, new_addr, &data_ptr[data_buffer_index], &sub_written_nof_words  ) ;
			if(return_code == PROTOCOL_ERROR_CODE_WRONG_PACKET_IDENTIFIER) {
				this->clear_UdpReceiveBuffer();
			}
			request_retry++;
			this->udp_dma_write_req_retry_counter++;
		} while((return_code == PROTOCOL_ERROR_CODE_WRONG_PACKET_IDENTIFIER) && (request_retry < UDP_REQUEST_RETRY+1));


		if ((return_code == 0) || (return_code == PROTOCOL_VME_CODE_BUS_ERROR)){ //  
			data_buffer_index = data_buffer_index + sub_written_nof_words ;
			rest_length_words = rest_length_words - sub_written_nof_words ; 
			if(this->vmeHead.F == 0){
				if(this->vmeHead.Space == 4){ // VME
					new_addr = new_addr + (sub_written_nof_words*4);
				}
				else {
					new_addr = new_addr + (sub_written_nof_words);
				}
			}
		}
	} while ((return_code == 0) && (rest_length_words>0)) ;

	if(return_code == -1) {
		return_code = PROTOCOL_ERROR_CODE_TIMEOUT ;
	}
	if(return_code == PROTOCOL_ERROR_CODE_WRONG_ACK) {
		this->clear_UdpReceiveBuffer();
	}
	if(return_code == PROTOCOL_ERROR_CODE_WRONG_NOF_RECEVEID_BYTES) {
		this->clear_UdpReceiveBuffer();
	}

	*written_nof_words = data_buffer_index ;
    return return_code;

}

/**************************************************************************************/

int  sis3153eth::vme_head_add(unsigned int nof_words){
	
	int i,j;
	UINT local_work[2];

	if(this->vmeHead.Space == 4) { // VME
		if((this->vmeHead.Size == 0) && this->vmeHead.W){
			nof_words <<= 1;
		}
		else{
			if(this->vmeHead.Size == 3){
				nof_words <<= 2;
			}
			else{
				nof_words <<= this->vmeHead.Size;
			}
		}
	}

	if(this->vmeHead.Space == 1) { // register
		if(this->vmeHead.W){
	//		nof_words <<= 1;
		}
	}

	local_work[0] = (0xAAAA << 16) | (this->vmeHead.Space << 12) | (this->vmeHead.W << 11) | (this->vmeHead.F << 10) | (this->vmeHead.Size << 8) | ((nof_words>>16)&0xFF);
	local_work[1] = (this->vmeHead.Mode << 16) | (nof_words&0xFFFF); 

	j=4;
	for (i=0;i<2;i++) {
		this->udp_send_data[j++] = (unsigned char)  (local_work[i] & 0xff) ;        // address(7 dwonto 0)
		this->udp_send_data[j++] = (unsigned char) ((local_work[i] >>  8) & 0xff) ; // address(15 dwonto 8)
		this->udp_send_data[j++] = (unsigned char) ((local_work[i] >> 16) & 0xff) ; // address(23 dwonto 16)
		this->udp_send_data[j++] = (unsigned char) ((local_work[i] >> 24) & 0xff) ; // address(31 dwonto 24)
	}

	return 0;
}



/*************************************************************************************************************************************************/
/*************************************************************************************************************************************************/
/***                                                                                                                                           ***/
/***     "emulate" VME access routines                                                                                                         ***/
/***                                                                                                                                           ***/
/*************************************************************************************************************************************************/
/*************************************************************************************************************************************************/

int sis3153eth::udp_global_read(UINT addr, UINT* data, UINT request_nof_words, UINT space, UINT dma_fg, UINT fifo_fg, UINT size, UINT mode, UINT* got_nof_words)
{
	int return_code;
	unsigned int udp_address ;

	udp_address = addr;

	this->vmeHead.Space	= space;
	this->vmeHead.W		= 0;
	this->vmeHead.F		= fifo_fg;
	this->vmeHead.Size	= size;
	this->vmeHead.Mode	= mode;


	if(dma_fg == 1){
		return_code = this->udp_DMA_read(request_nof_words, udp_address, data, got_nof_words) ;
	}
	else{
		return_code = this->udp_single_read(request_nof_words, &udp_address, data)  ;
	}

	return return_code;
}

/**************************************************************************************/

int sis3153eth::udp_sis3153_register_read (UINT addr, UINT* data)
{
	int return_code;
	unsigned int udp_address ;

	udp_address = addr;

	this->vmeHead.Space	= 1;
	this->vmeHead.W		= 0;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 2;
	this->vmeHead.Mode	= 0x0000;

	return_code = this->udp_single_read(1, &udp_address, data)  ;
	
	return return_code;
}

/**************************************************************************************/

int sis3153eth::udp_sis3153_register_dma_read (UINT addr, UINT* data, UINT request_nof_bytes, UINT* got_nof_words )
{
	int return_code;
	unsigned int udp_address ;

	udp_address = addr;

	this->vmeHead.Space	= 1;
	this->vmeHead.W		= 0;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 2;
	this->vmeHead.Mode	= 0x0000;

	return_code = this->udp_DMA_read(request_nof_bytes, udp_address, data, got_nof_words) ;
	
	return return_code;
}

/**************************************************************************************/
 
 
/**************************************************************************************/
 
int sis3153eth::vme_IACK_D8_read (UINT vme_irq_level, UCHAR* data)  // new 19.01.2016
{
	int return_code;
	unsigned int udp_address ;
	UINT data_32 ;

	udp_address = ((vme_irq_level << 1)+1) ;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 0;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 0;
	this->vmeHead.Mode	= 0x4000 ; /*AM register*/

	return_code = this->udp_single_read(1, &udp_address, &data_32)  ;
	//printf("vme_IACK_D8_read: addr = 0x%08X    data = 0x%08X    return_code = 0x%08X \n", udp_address, data_32 ,return_code);
	*data = (UCHAR) (data_32) & 0xff ;
	
	return return_code;
}
/**************************************************************************************/
 
int sis3153eth::vme_A16D8_read (UINT addr, UCHAR* data)  // new 04.09.2015
{
	int return_code;
	unsigned int udp_address ;
	UINT data_32 ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 0;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 0;
	this->vmeHead.Mode	= 0x0029;

	return_code = this->udp_single_read(1, &udp_address, &data_32)  ;
	//printf("vme_A32D8_read: addr = 0x%08X    data = 0x%08X    return_code = 0x%08X \n", addr, data_32 ,return_code);
	if((addr & 0x1) == 0x0){
		*data = (UCHAR) (data_32 >> 8) & 0xff ;
	}
	else {
		*data = (UCHAR) (data_32) & 0xff ;
	}
	return return_code;
}
/**************************************************************************************/

int sis3153eth::vme_A16D16_read (UINT addr, USHORT* data)   // new 04.09.2015
{
	int return_code;
	unsigned int udp_address ;
	UINT data_32 ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 0;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 1;
	this->vmeHead.Mode	= 0x0029;

	return_code = this->udp_single_read(1, &udp_address, &data_32)  ;
	//printf("vme_A32D16_read: addr = 0x%08X    data = 0x%08X    return_code = 0x%08X \n", addr, data_32 ,return_code);
	*data = (USHORT) (data_32) & 0xffff ;

	return return_code;
}


/**************************************************************************************/
 
int sis3153eth::vme_A24D8_read (UINT addr, UCHAR* data)  // new 04.09.2015
{
	int return_code;
	unsigned int udp_address ;
	UINT data_32 ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 0;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 0;
	this->vmeHead.Mode	= 0x0039;

	return_code = this->udp_single_read(1, &udp_address, &data_32)  ;
	//printf("vme_A32D8_read: addr = 0x%08X    data = 0x%08X    return_code = 0x%08X \n", addr, data_32 ,return_code);
	if((addr & 0x1) == 0x0){
		*data = (UCHAR) (data_32 >> 8) & 0xff ;
	}
	else {
		*data = (UCHAR) (data_32) & 0xff ;
	}
	return return_code;
}


/**************************************************************************************/

int sis3153eth::vme_A24D16_read (UINT addr, USHORT* data)   // new 04.09.2015
{
	int return_code;
	unsigned int udp_address ;
	UINT data_32 ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 0;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 1;
	this->vmeHead.Mode	= 0x0039;

	return_code = this->udp_single_read(1, &udp_address, &data_32)  ;
	//printf("vme_A32D16_read: addr = 0x%08X    data = 0x%08X    return_code = 0x%08X \n", addr, data_32 ,return_code);
	*data = (USHORT) (data_32) & 0xffff ;

	return return_code;
}


int sis3153eth::vme_A24D32_read (UINT addr, UINT* data)
{
	int return_code;
	unsigned int udp_address ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 0;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 2;
	this->vmeHead.Mode	= 0x0039;

	return_code = this->udp_single_read(1, &udp_address, data)  ;

	return return_code;
}


/**************************************************************************************/



//int sis3153eth::vme_A32D8_read (UINT addr, UINT* data)
int sis3153eth::vme_A32D8_read (UINT addr, UCHAR* data)  // modified 04.09.2015
{
	int return_code;
	unsigned int udp_address ;
	UINT data_32 ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 0;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 0;
	this->vmeHead.Mode	= 0x0009;

	return_code = this->udp_single_read(1, &udp_address, &data_32)  ;
	//printf("vme_A32D8_read: addr = 0x%08X    data = 0x%08X    return_code = 0x%08X \n", addr, data_32 ,return_code);
	if((addr & 0x1) == 0x0){
		*data = (UCHAR) (data_32 >> 8) & 0xff ;
	}
	else {
		*data = (UCHAR) (data_32) & 0xff ;
	}
	return return_code;
}

/**************************************************************************************/

//int sis3153eth::vme_A32D16_read (UINT addr, UINT* data)
int sis3153eth::vme_A32D16_read (UINT addr, USHORT* data)   // modified 04.09.2015
{
	int return_code;
	unsigned int udp_address ;
	UINT data_32 ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 0;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 1;
	this->vmeHead.Mode	= 0x0009;

	return_code = this->udp_single_read(1, &udp_address, &data_32)  ;
	//printf("vme_A32D16_read: addr = 0x%08X    data = 0x%08X    return_code = 0x%08X \n", addr, data_32 ,return_code);
	*data = (USHORT) (data_32) & 0xffff ;

	return return_code;
}

/**************************************************************************************/

int sis3153eth::vme_A32D32_read (UINT addr, UINT* data)
{
	int return_code;
	unsigned int udp_address ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 0;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 2;
	this->vmeHead.Mode	= 0x0009;

	return_code = this->udp_single_read(1, &udp_address, data)  ;

	return return_code;
}

/**************************************************************************************/

int sis3153eth::vme_A32DMA_D32_read (UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words )
{

	int return_code;
	unsigned int udp_address ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 0;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 2;
	this->vmeHead.Mode	= 0x0009;

	return_code = this->udp_DMA_read(request_nof_words, udp_address, data, got_nof_words)  ;

	return return_code; // 
}

/**************************************************************************************/

int sis3153eth::vme_A32BLT32_read (UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words )
{
	int return_code;
	unsigned int udp_address ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 0;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 2;
	this->vmeHead.Mode	= 0x000B;

	return_code = this->udp_DMA_read(request_nof_words, udp_address, data, got_nof_words)  ;

	return return_code; // 
}


/**************************************************************************************/

int sis3153eth::vme_A32MBLT64_read (UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words )
{
	int return_code;
	unsigned int udp_address ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 0;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 3;
	this->vmeHead.Mode	= 0x0008;

	return_code = this->udp_DMA_read(request_nof_words, udp_address, data, got_nof_words)  ;

	return return_code; // 
}

/**************************************************************************************/

int sis3153eth::vme_A32_2EVME_read (UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words )
{
	int return_code;
	unsigned int udp_address ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 0;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 3;
	this->vmeHead.Mode	= 0x0020;

	return_code = this->udp_DMA_read(request_nof_words, udp_address, data, got_nof_words)  ;

	return return_code; // 
}

/**************************************************************************************/

int sis3153eth::vme_A32_2ESST160_read (UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words )
{
	int return_code;
	unsigned int udp_address ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 0;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 3;
	this->vmeHead.Mode	= 0x0060;

	return_code = this->udp_DMA_read(request_nof_words, udp_address, data, got_nof_words)  ;

	return return_code; // 
}

/**************************************************************************************/

int sis3153eth::vme_A32_2ESST267_read (UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words )
{
	int return_code;
	unsigned int udp_address ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 0;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 3;
	this->vmeHead.Mode	= 0x0160;

	return_code = this->udp_DMA_read(request_nof_words, udp_address, data, got_nof_words)  ;

	return return_code; // 
}

/**************************************************************************************/

int sis3153eth::vme_A32_2ESST320_read (UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words )
{
	int return_code;
	unsigned int udp_address ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 0;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 3;
	this->vmeHead.Mode	= 0x0260;

	return_code = this->udp_DMA_read(request_nof_words, udp_address, data, got_nof_words)  ;

	return return_code; // 
}

/**************************************************************************************/
/**************************************************************************************/

int sis3153eth::vme_A32DMA_D32FIFO_read (UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words )
{
	int return_code;
	unsigned int udp_address ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 0;
	this->vmeHead.F		= 1;
	this->vmeHead.Size	= 2;
	this->vmeHead.Mode	= 0x0009;

	return_code = this->udp_DMA_read(request_nof_words, udp_address, data, got_nof_words)  ;
	
	return return_code; // 
}

/**************************************************************************************/

int sis3153eth::vme_A32BLT32FIFO_read (UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words )
{
	int return_code;
	unsigned int udp_address ;
	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 0;
	this->vmeHead.F		= 1;
	this->vmeHead.Size	= 2;
	this->vmeHead.Mode	= 0x000B;

	return_code = this->udp_DMA_read(request_nof_words, udp_address, data, got_nof_words)  ;

	return return_code; // 
}

/**************************************************************************************/

int sis3153eth::vme_A32MBLT64FIFO_read (UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words )
{
	int return_code;
	unsigned int udp_address ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 0;
	this->vmeHead.F		= 1;
	this->vmeHead.Size	= 3;
	this->vmeHead.Mode	= 0x0008;

	return_code = this->udp_DMA_read(request_nof_words, udp_address, data, got_nof_words)  ;

	return return_code; // 
}

/**************************************************************************************/

int sis3153eth::vme_A32_2EVMEFIFO_read (UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words )
{
	int return_code;
	unsigned int udp_address ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 0;
	this->vmeHead.F		= 1;
	this->vmeHead.Size	= 3;
	this->vmeHead.Mode	= 0x0020;

	return_code = this->udp_DMA_read(request_nof_words, udp_address, data, got_nof_words)  ;

	return return_code; // 
}

/**************************************************************************************/

int sis3153eth::vme_A32_2ESST160FIFO_read (UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words )
{
	int return_code;
	unsigned int udp_address ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 0;
	this->vmeHead.F		= 1;
	this->vmeHead.Size	= 3;
	this->vmeHead.Mode	= 0x0060;

	return_code = this->udp_DMA_read(request_nof_words, udp_address, data, got_nof_words)  ;

	return return_code; // 
}

/**************************************************************************************/

int sis3153eth::vme_A32_2ESST267FIFO_read (UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words )
{
	int return_code;
	unsigned int udp_address ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 0;
	this->vmeHead.F		= 1;
	this->vmeHead.Size	= 3;
	this->vmeHead.Mode	= 0x0160;

	return_code = this->udp_DMA_read(request_nof_words, udp_address, data, got_nof_words)  ;

	return return_code; // 
}

/**************************************************************************************/

int sis3153eth::vme_A32_2ESST320FIFO_read (UINT addr, UINT* data, UINT request_nof_words, UINT* got_nof_words )
{
	int return_code;
	unsigned int udp_address ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 0;
	this->vmeHead.F		= 1;
	this->vmeHead.Size	= 3;
	this->vmeHead.Mode	= 0x0260;

	return_code = this->udp_DMA_read(request_nof_words, udp_address, data, got_nof_words)  ;

	return return_code; //
}


/**************************************************************************************/
/**************************************************************************************/

int sis3153eth::udp_global_write(UINT addr, UINT* data, UINT request_nof_words, UINT space, UINT dma_fg, UINT fifo_fg, UINT size, UINT mode, UINT* written_nof_words)
{
	int return_code;
	unsigned int udp_address ;

	udp_address = addr;

	this->vmeHead.Space	= space;
	this->vmeHead.W		= 1;
	this->vmeHead.F		= fifo_fg;
	this->vmeHead.Size	= size;
	this->vmeHead.Mode	= mode;


	if(dma_fg == 1){
		return_code = this->udp_DMA_write(request_nof_words, udp_address, data, written_nof_words) ;
	}
	else{
		return_code = this->udp_single_write(request_nof_words, &udp_address, data)  ;
	}

	return return_code;
}

/**************************************************************************************/

int sis3153eth::udp_sis3153_register_write (UINT addr, UINT data)
{
	int return_code;

	this->vmeHead.Space	= 1;
	this->vmeHead.W		= 1;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 2;
	this->vmeHead.Mode	= 0x0000;

	return_code = this->udp_single_write(1, &addr, &data)  ;
	
	return return_code;
}

/**************************************************************************************/

int sis3153eth::udp_sis3153_register_dma_write (UINT addr, UINT* data, UINT request_nof_words, UINT* written_nof_words)
{
	int return_code;

	this->vmeHead.Space	= 1;
	this->vmeHead.W		= 1;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 2;
	this->vmeHead.Mode	= 0x0000;

	return_code = this->udp_DMA_write(request_nof_words, addr, data, written_nof_words)  ;
	
	return return_code;
}




/**************************************************************************************/

int sis3153eth::vme_A16D8_write (UINT addr, UCHAR data)  // new 04.09.2015
{
	int return_code;
	unsigned int udp_address ;
	UINT data_32 = (UINT)(data & 0xff);
	data_32 = data_32 + (data_32 << 8) + (data_32 << 16) + (data_32 << 24);

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 1;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 0;
	this->vmeHead.Mode	= 0x0029;

	return_code = this->udp_single_write(1, &udp_address, &data_32);
	
	return return_code;
}


int sis3153eth::vme_A16D16_write (UINT addr, USHORT data)  // new 04.09.2015
{
	int return_code;
	unsigned int udp_address ;
	UINT data_32 = (UINT)(data & 0xffff);
	data_32 = data_32 + (data_32 << 16);

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 1;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 1;
	this->vmeHead.Mode	= 0x0029;

	return_code = this->udp_single_write(1, &udp_address, &data_32);
	
	return return_code;
}


/**************************************************************************************/

int sis3153eth::vme_A24D8_write (UINT addr, UCHAR data)  // new 04.09.2015
{
	int return_code;
	unsigned int udp_address ;
	UINT data_32 = (UINT)(data & 0xff);
	data_32 = data_32 + (data_32 << 8) + (data_32 << 16) + (data_32 << 24);

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 1;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 0;
	this->vmeHead.Mode	= 0x0039;

	return_code = this->udp_single_write(1, &udp_address, &data_32);
	
	return return_code;
}


int sis3153eth::vme_A24D16_write (UINT addr, USHORT data)  // new 04.09.2015
{
	int return_code;
	unsigned int udp_address ;
	UINT data_32 = (UINT)(data & 0xffff);
	data_32 = data_32 + (data_32 << 16);

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 1;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 1;
	this->vmeHead.Mode	= 0x0039;

	return_code = this->udp_single_write(1, &udp_address, &data_32);
	
	return return_code;
}

/**************************************************************************************/

int sis3153eth::vme_A24D32_write (UINT addr, UINT data)
{
	int return_code;
	unsigned int udp_address ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 1;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 2;
	this->vmeHead.Mode	= 0x0039;

	return_code = this->udp_single_write(1, &udp_address, &data);
	
	return return_code;
}




/**************************************************************************************/

//int sis3153eth::vme_A32D8_write (UINT addr, UINT data)
int sis3153eth::vme_A32D8_write (UINT addr, UCHAR data)  // modified 04.09.2015
{
	int return_code;
	unsigned int udp_address ;
	UINT data_32 = (UINT)(data & 0xff);
	data_32 = data_32 + (data_32 << 8) + (data_32 << 16) + (data_32 << 24);

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 1;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 0;
	this->vmeHead.Mode	= 0x0009;

	return_code = this->udp_single_write(1, &udp_address, &data_32);
	
	return return_code;
}

/**************************************************************************************/

//int sis3153eth::vme_A32D16_write (UINT addr, UINT data)
int sis3153eth::vme_A32D16_write (UINT addr, USHORT data)  // modified 04.09.2015
{
	int return_code;
	unsigned int udp_address ;
	UINT data_32 = (UINT)(data & 0xffff);
	data_32 = data_32 + (data_32 << 16);

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 1;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 1;
	this->vmeHead.Mode	= 0x0009;

	return_code = this->udp_single_write(1, &udp_address, &data_32);
	
	return return_code;
}

/**************************************************************************************/

int sis3153eth::vme_A32D32_write (UINT addr, UINT data)
{
	int return_code;
	unsigned int udp_address ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 1;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 2;
	this->vmeHead.Mode	= 0x0009;

	return_code = this->udp_single_write(1, &udp_address, &data);
	
	return return_code;
}


/**************************************************************************************/

int sis3153eth::vme_A32DMA_D32_write (UINT addr, UINT* data, UINT request_nof_words, UINT* written_nof_words )
{
	int return_code;
	unsigned int udp_address ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 1;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 2;
	this->vmeHead.Mode	= 0x0009;

	return_code = this->udp_DMA_write(request_nof_words, udp_address, data, written_nof_words)  ;

	return return_code; // 
}
/**************************************************************************************/

int sis3153eth::vme_A32BLT32_write (UINT addr, UINT* data, UINT request_nof_words, UINT* written_nof_words )
{
	int return_code;
	unsigned int udp_address ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 1;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 2;
	this->vmeHead.Mode	= 0x000B;

	return_code = this->udp_DMA_write(request_nof_words, udp_address, data, written_nof_words)  ;

	return return_code; // 
}
/**************************************************************************************/

int sis3153eth::vme_A32MBLT64_write (UINT addr, UINT* data, UINT request_nof_words, UINT* written_nof_words )
{
	int return_code;
	unsigned int udp_address ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 1;
	this->vmeHead.F		= 0;
	this->vmeHead.Size	= 3;
	this->vmeHead.Mode	= 0x0008;

	return_code = this->udp_DMA_write(request_nof_words, udp_address, data, written_nof_words)  ;

	return return_code; // 
}

/**************************************************************************************/

int sis3153eth::vme_A32DMA_D32FIFO_write (UINT addr, UINT* data, UINT request_nof_words, UINT* written_nof_words )
{
	int return_code;
	unsigned int udp_address ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 1;
	this->vmeHead.F		= 1;
	this->vmeHead.Size	= 2;
	this->vmeHead.Mode	= 0x0009;

	return_code = this->udp_DMA_write(request_nof_words, udp_address, data, written_nof_words)  ;

	return return_code; // 
}

/**************************************************************************************/

int sis3153eth::vme_A32BLT32FIFO_write (UINT addr, UINT* data, UINT request_nof_words, UINT* written_nof_words )
{
	int return_code;
	unsigned int udp_address ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 1;
	this->vmeHead.F		= 1;
	this->vmeHead.Size	= 2;
	this->vmeHead.Mode	= 0x0009;

	return_code = this->udp_DMA_write(request_nof_words, udp_address, data, written_nof_words)  ;

	return return_code; // 
}

/**************************************************************************************/

int sis3153eth::vme_A32MBLT64FIFO_write (UINT addr, UINT* data, UINT request_nof_words, UINT* written_nof_words )
{
	int return_code;
	unsigned int udp_address ;

	udp_address = addr;

	this->vmeHead.Space	= 4;
	this->vmeHead.W		= 1;
	this->vmeHead.F		= 1;
	this->vmeHead.Size	= 3;
	this->vmeHead.Mode	= 0x0008;

	return_code = this->udp_DMA_write(request_nof_words, udp_address, data, written_nof_words)  ;

	return return_code; // 
}


/**************************************************************************************/

int sis3153eth::vme_IRQ_Status_read( UINT* data ) 
{
	return -1; // not implemented
}


#endif
