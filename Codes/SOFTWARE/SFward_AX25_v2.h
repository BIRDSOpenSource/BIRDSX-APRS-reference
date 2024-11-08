#include <stdint.h>
#include <ctype.h> 
#include <stdio.h>
 
 
//Start of ACS
#define LED_INDICATOR_PIN PIN_G0       //Pin used as indicator for testing
//#define TXONGOING_PIN PIN_B3           //Pin used to indicate that TNC module is still transmitting audio to BIM1H
//#define APSF_FM4_CS PIN_A3             //Chip Select 1, 0 means that S&F flash memory is selected, 1 means deselected

#define SOFTUART_RXPIN PIN_C7          //PIC16F1788 pin #18
#define SOFTUART_TXPIN PIN_C6          //PIC16F1788 pin #17

//#define APSF_FLAG PIN_A5   //pin used by APSF MCU to inform COM96 MCU that it has saved an uplink command received from BIRDS GS

#define UART_BAUDRATE 19.2      //in kbps
#define UART_BITPERIOD 52        //in us
#define UART_HALFBITPERIOD 26    //in us
#define UART_STOPBITPERIOD 52    //in us

#define INFO_FIELD_LEN  155       //number of bytes of AX.25 Information field
//#define UART_RXDATA_MAXLEN (2+16+INFO_FIELD_LEN+1+20)    //number of bytes of AX.25 frame including 2-byte KISS start, 6 bytes dest CS, 1 byte dest SSID, 6 bytes source CS, 1 byte source SSID, 1 byte control field, 1 byte PID, info field, 1-byte KISS end, 20-byte margin
//#define UART_TXDATA_MAXLEN (2+16+INFO_FIELD_LEN+1+20)    //number of bytes of AX.25 frame including 2-byte KISS start, 6 bytes dest CS, 1 byte dest SSID, 6 bytes source CS, 1 byte source SSID, 1 byte control field, 1 byte PID, info field, 1-byte KISS end, 20-byte margin   
#define UART_RXDATA_MAXLEN 512
#define UART_TXDATA_MAXLEN 128


#define TX_DEST_CALLSIGN   ((unsigned char*)"S&FGST")    //destination callsign for downlink
#define TX_SOURCE_CALLSIGN ((unsigned char*)"JG6YKM")    //source callsign for downlink
#define RX_DEST_CALLSIGN   ((unsigned char*)"JG6YKM")    //destination callsign for uplink
#define RX_SOURCE_CALLSIGN ((unsigned char*)"S&FGST")    //source callsign for uplink
#define RECEIVEPACKET_END  ';'  //End of transmission block

#define TNC_PROCDELAY   100   //in ms

#define FEND   (unsigned char)0xC0  //192
#define FESC   (unsigned char)0xDB  //219
#define TFEND  (unsigned char)0xDC  //220
#define TFESC  (unsigned char)0xDD  //221

//function prototypes
unsigned int16 KISSframe_to_AX25packet(unsigned char* KISSframe, unsigned int16 KISSframe_len, unsigned char* AX25packet);
unsigned int16 AX25packet_to_KISSframe(unsigned char* AX25packet, unsigned int16 AX25packet_len, unsigned char* KISSframe);
int8 parse_received_packet();
void send_acknowledgment_packet();
void send_acknowledgment_packet_forcommanduplink();
void send_TLE_packet();
void save_TLE();
void save_received_frame();


void softwareUART_init();
unsigned int16 receive_UARTdata(unsigned char* received_data);
unsigned int16 transmit_UARTdata(unsigned char* transmit_data, unsigned int16 transmit_data_len);
unsigned char softwareUART_receive_byte();



//Global variables
unsigned int8 TX_KISSframe_len = 0;    //number of bytes of received KISS frame
unsigned char TX_KISSframe[UART_TXDATA_MAXLEN] = {};
unsigned int8 TX_packet_len = 0;      //number of bytes of downlink command data to be transmitted, including dest/source CSs and SSIDs, control, PID, info fields, 2-byte FCS and RECEIVEPACKET_END, but excludiing end/start flags and bit stuffs
unsigned char TX_packet[UART_TXDATA_MAXLEN] = {};
//unsigned char TX_callsign_dest[6];
unsigned char TX_SSID_dest = 0x30;
//unsigned char TX_callsign_source[6];                              
unsigned char TX_SSID_source = 0x30;           
unsigned char TX_control = 0x3E;                                
unsigned char TX_PID = 0xF0; 


int16 RX_KISSframe_len = 0;    //number of bytes of received KISS frame
//unsigned int8 RX_KISSframe[UART_RXDATA_MAXLEN] = {};
//RX_KISSframe={0xC0, 0x00, 0x82, 0xA0 , 0x96 , 0x60 , 0x60 , 0x66 , 0x60 , 0x94 , 0x8E , 0x6C , 0xB2 , 0x84 , 0xAE , 0xEE , 0x82 , 0xA0 , 0xA4 , 0xA6 , 0x82 , 0xA8 , 0x61 , 0x03 , 0xF0 , 0x3A , 0x42 , 0x49 , 0x52 , 0x44 , 0x53 , 0x34 , 0x20 , 0x20 , 0x20 , 0x3A , 0x61 , 0x61 , 0x61 , 0x61 , 0x61 , 0x7B , 0x32 , 0x37 , 0x0D , 0xC0};

unsigned int8 RX_KISSframe[] ={0xC0, 0x00, 0x4A, 0x47 , 0x36 , 0x59 , 0x4B , 0x4D , 0x30 , 0x53 , 0x26 , 0x46 , 0x47 , 0x53 , 0x54 , 0x31 , 0x3E , 0xF0 , 0x53 , 0x46 , 0x00 , 0x00 , 0x51 , 0x31 , 0x38 , 0x30 , 0x37 , 0x30 , 0x31 , 0x33 , 0x31 , 0x35 , 0x30 , 0x35 , 0x31 , 0x38 , 0x55 , 0x37 , 0x31 , 0x30 , 0x31 , 0x33 , 0x31 , 0x35 , 0x32 , 0x30 , 0x32 , 0x32 , 0x35 , 0x53 , 0xC0};


unsigned int16 RX_packet_len = 0;    //number of bytes of uplink command data received including dest/source CSs and SSIDs, control, PID, info fields and 2-byte FCS, but excluding RECEIVEPACKET_END, end/start flags and bit stuffs
unsigned char RX_packet[UART_RXDATA_MAXLEN] = {};

unsigned char RX_callsign_dest[7] = {'\0','\0','\0','\0','\0','\0','\0'};
unsigned char RX_SSID_dest = 0x30;
unsigned char RX_callsign_source[7] = {'\0','\0','\0','\0','\0','\0','\0'};                                 
unsigned char RX_SSID_source = 0x30;           
unsigned char RX_control = 0x3E;                                
unsigned char RX_PID = 0xF0; 
unsigned char RX_info_field[256];
//unsigned char RX_FCS[2];
                
unsigned char info_header[2] = {'S', 'F'};                      
unsigned char info_footer[2] = {'S', 'F'};                        
                      
 
//Convert only one KISS frame to an AX.25 packet

unsigned int16 KISSframe_to_AX25packet(unsigned char* KISSframe, unsigned int16 KISSframe_len, unsigned char* AX25packet)
{
   unsigned int16 RX_AX25packet_len = 0;   //number of recently decoded bytes and serves as index to where the next decoded byte should be placed
   
   //Check if valid KISS frame
   if(!(RX_KISSframe[0]==FEND && RX_KISSframe[RX_KISSframe_len-1]==FEND)) return RX_AX25packet_len;
   
   //Else, assume a valid KISS frame, begin decoding at the third byte and stop before the end of frame
   for(unsigned int16 k=2; k < (RX_KISSframe_len-1); k++)
   {
      //Corresponding to received AX.25 data FEND
      if(RX_KISSframe[k]==TFESC && RX_KISSframe[k-1]==FESC) RX_packet[RX_AX25packet_len++]=FESC; 
      
      //Corresponding to received AX.25 data FESC
      else if(RX_KISSframe[k]==TFESC && RX_KISSframe[k-1]==FESC) RX_packet[RX_AX25packet_len++]=FESC;
      
      //Special ESC character was encountered in the last iteration but present byte is invalid
      else if(RX_KISSframe[k-1]==FESC && !(RX_KISSframe[k]==TFEND || RX_KISSframe[k]==TFESC))
      {
         RX_AX25packet_len = 0;
         break;
      }

      //Special ESC character encountered so examine the next byte
      else if (RX_KISSframe[k]==FESC) continue;    //Escape mode
      
      //No special character, just copy the received data as it is
      else RX_packet[RX_AX25packet_len++]=RX_KISSframe[k];
   
   }
   return RX_AX25packet_len;
}


      
int8 parse_received_packet()
{

   unsigned int16 index = 0;
  
   for(index=0; index<6; index++)  RX_callsign_dest[index] = RX_packet[index]; 
   RX_SSID_dest = RX_packet[6];
   //fprintf(debugPort,"RX_callsign_dest:%s\r\n",RX_callsign_dest);
   
   for(index=7; index<13; index++)  RX_callsign_source[index-7] = RX_packet[index]; 
   RX_SSID_source = RX_packet[13];
   //fprintf(debugPort,"RX_callsign_source:%s\r\n",RX_callsign_source);
   
   RX_control = RX_packet[14];
   RX_PID = RX_packet[15];
   
   for(index=16; index<RX_packet_len; index++)  RX_info_field[index-16] = RX_packet[index]; 
   //fprintf(debugPort,"RX_info_field:%s\r\n",RX_info_field);
   
   
   //print parsed data
   //puts("");
   fprintf(debugPort,"RX_callsign_dest: %s \r\n", RX_callsign_dest);
   //puts("");
   
   fprintf(debugPort,"RX_SSID_dest: %c", RX_SSID_dest);
   //puts("");

   fprintf(debugPort,"RX_callsign_source: %s \r\n", RX_callsign_source);
   //puts("");
   
   fprintf(debugPort,"RX_SSID_source: %c \r\n", RX_SSID_source);
   //puts("");

   fprintf(debugPort,"RX_control: %x \r\n", RX_control);
   //puts("");
   
   fprintf(debugPort,"RX_PID: %x \r\n", RX_PID);
   //puts("");
   
   //transmit_UARTdata((unsigned char*)"RX_info_field: ",15);
   //transmit_UARTdata(RX_info_field,RX_packet_len-16);
   //puts("");
   
   fprintf(debugPort,"RX_packet_len: %li \r\n", RX_packet_len);
   //puts("");

   fprintf(debugPort,"RX_info_field length: %li \r\n", RX_packet_len-16);
   //puts("");  
   
  
   if( strncmp(RX_callsign_dest,RX_DEST_CALLSIGN,6)==0 && 
       RX_info_field[0] == info_header[0] && 
       RX_info_field[1] == info_header[1] &&
       RX_info_field[RX_packet_len-16-2] == info_footer[0] &&
       RX_info_field[RX_packet_len-16-1] == info_footer[1]
     )
   {
      fprintf(debugPort,"Valid S&F packet! \r\n");
      return 1;    //Valid S&F packet
   }
   
   
   else if(  RX_packet_len == 30 && /*Excluding FCS*/
             strncmp(RX_callsign_dest,RX_DEST_CALLSIGN,6)==0 &&
             RX_info_field[0] == 0xAA && 
             RX_info_field[13] == 0xEE
          )
   {
      fprintf(debugPort,"Command uplink packet from BIRDS GS! \r\n");
      return 2;    //Valid S&F packet
   }   



   else
   {  
      fprintf(debugPort,"Invalid S&F packet! \r\n");
      return 0;   //Invalid S&F packet
   }
   
   
}

/*
int8 sendingAX25(void)
{
//   int8 SendData[] = {0x86,   0xA2,  0x40,  0x40,  0x40,  0x40,  0x60,  0xAE,  0x64,  0x8C,  0xA6, 0x40,   0x40,   0x68, 0xA4, 0x8A,  0x98,  0x82,   0xB2,  0x40,   0x61,  0x03, 0xF0,   0x54,   0x65,  0x73, 0x74};
   int8 SendData[] = {0x43,   0x51,  0x20,  0x20,  0x20,  0x20,  0x60,  0xAE,  0x64,  0x8C,  0xA6, 0x40,   0x40,   0x68, 0xA4, 0x8A,  0x98,  0x82,   0xB2,  0x40,   0x61,  0x03, 0xF0,   0x54,   0x65,  0x73, 0x74};

   
   
   #define AX25_MAX_length 100
   
   int8 AX25_packet[AX25_MAX_length];
   int8 AX25_info[]="HelloWorld";
   int8 callsign_dest[]="JG6YBW";
   int8 callsign_source[]="BIRDS4";
   int8 SSID_dest=0x30;
   int8 SSID_source=0x30;
   int8 AX25_control= 0x3E;                                
   int8 AX25_PID= 0xF0;
//!   int8 SSID_dest=0x61;
//!   int8 SSID_source=0x60;
//!   int8 AX25_control= 0x03;                                
//!   int8 AX25_PID= 0xF0;
   int8 AX25_length=sizeof(AX25_info)+16;
   int8 counter;
   int16 index = 0;
   int8 tx_delay=5;
   counter=0;
   while (counter<6) 
   {AX25_packet[index++]=callsign_source[counter++];}
   AX25_packet[index++]=SSID_source;
   
   counter=0;
   while (counter<6) 
   {AX25_packet[index++]=callsign_dest[counter++];}
   AX25_packet[index++]=SSID_dest;
   
   
   AX25_packet[index++]=AX25_control;
   AX25_packet[index++]=AX25_PID;
   
   counter=0;
   while (counter<sizeof(AX25_info)) 
   {AX25_packet[index++]=AX25_info[counter++];}
   
   AX25_length=index;
   
   
   //for(index=0; index<6; index++)  AX25_packet[index]=callsign_source[index]; 
   //AX25_packet[6]=SSID_source;
   
   
   
   //for(index=7; index<13; index++)  AX25_packet[index]=callsign_dest[index-7]; 
   //AX25_packet[13]=SSID_dest;
   //fprintf(debugPort,"RX_callsign_source:%s\r\n",RX_callsign_source);

   //AX25_packet[14]=AX25_control;
   //AX25_packet[15]=AX25_PID;

   //for(index=16; index<AX25_length-1; index++)  AX25_packet[index]=AX25_info[index-16]; 
   


//!
//!   for(index=0; index<sizeof(SendData); index++)  
//!   {
//!      fprintf(debugPort,"%X ",SendData[index]);
//!      putc(SendData[index],serial2APRS);
//!      delay_ms(tx_delay);
//!   }
//!   putc('\r',serial2APRS);
//!   fprintf(debugPort,"\r\n");
//!   
//!delay_ms(2000);
   
   for(index=0; index<AX25_length; index++)  
   {
      fprintf(debugPort,"%X ",AX25_packet[index]);
      putc(AX25_packet[index],serial2APRS);
      delay_ms(tx_delay);
   }
   putc('\r',serial2APRS);
   fprintf(debugPort,"\r\n");
}

*/



int8 send_APRS(int8 APRS_packet_no, int8 *APRS_info)
{
   int8 APRS_packet_info[4];
   //int8 APRS_info[]="HelloWorld";
   int8 packet_length;
   //int8 dest_callsign[]="DV1PUI   ";
   int8 dest_callsign[]="BIRDGS   ";
   int16 index = 0;
   int8 tx_delay=5;
   int8 APRS_packet_no_length;
   
   if (APRS_packet_no<10) APRS_packet_no_length=1;
   else APRS_packet_no_length=2;
   
   fprintf(debugPort,"APRS_Packet:\r\n");
   
      fprintf(debugPort,"%c",':');
      putc(':',serial2APRS);
      delay_ms(tx_delay);
   
   for(index=0; index<9; index++)
   {
      fprintf(debugPort,"%c",dest_callsign[index]);
      putc(dest_callsign[index],serial2APRS);
      delay_ms(tx_delay);
   }
      
      fprintf(debugPort,"%c",':');
      putc(':',serial2APRS);
      delay_ms(tx_delay);
      
      
   for(index=0; index<(78); index++)
   {
   fprintf(debugPort,"%c",APRS_info[index]);
   putc(APRS_info[index],serial2APRS);
   delay_ms(tx_delay);
   }
   

   sprintf(APRS_packet_info,"%c%u",'{',APRS_packet_no);
   packet_length=strlen(APRS_packet_info);
   //packet_length=sizeof(APRS_packet_info);
   //fprintf(debugPort,"packet_length=%u\r\n",packet_length);
   //fprintf(debugPort,"%s\r\n",APRS_packet_info);
   

   for(index=0; index<packet_length; index++)
   {
      fprintf(debugPort,"%c",APRS_packet_info[index]);
      putc(APRS_packet_info[index],serial2APRS);
      delay_ms(tx_delay);
   }
   putc('\r',serial2APRS);
   fprintf(debugPort,"\r\n");
   
   //delay_ms(200);
}



   
