#include <SFward_FM_Code_v3.h>
#include<String.h>
#INCLUDE <stdlibm.h>
#include<math.h> 
#include <SFward_AX25_v2.h>

#define commandSizeMB 7
#define delayTX 10  // Confirmed working good for sending data to MB. 5ms is not working!
//#define dataSizeAPRS 300 // = (x) + info size.... x=39 for the first 10 bytes, x= 40 till 99 bytes
#define APRS_packet_max_length 300

int32 rcved_data_adrs;
int32 MyAdress;

int32 i=0; //defined in SFward_AX25
unsigned int1 actionMB=0;
unsigned int1 actionAPRS=0;
//***********************************
int8 satellite_name[]="BIRDJP";
//***********************************
int8 callsign_size_SFward=0;
int8 paraguayCallsign_GS01[]="JG6YBW";
int8 paraguayGS01_ID=0x11;
int8 philippineCallsign_GS01[]="JG6YBA";
int8 philippineGS01_ID=0x22;
int8 japanCallsign_GS01[]="JG6YBC";
int8 japanGS01_ID=0x33;
int8 GS_ID_SFward=0x00;

int8 senderCallsign[8];
int8 info_size_SFward=0;
int8 Packet_no_APRS=0x99;
int8 info_APRS[67];
int8 info_end_SFward[]={0x88,0x88};

int8 byteNumAPRS=0;
unsigned int8 byteNumMB=0;
unsigned int8 rec_bytes_number_APRS=0; //CHANGE IT TO INT16 to receive bigger packets ( Might unleash issues!!! )
unsigned int8 receivedDataMB=0;
unsigned int8 dataAPRS[APRS_packet_max_length];
unsigned int8 commandLine[commandSizeMB];//={0,0,0,0,0,0,0,0};
unsigned int8 lastCommandLine[commandSizeMB];

int32 addressInTheCommand=0;

long packetLength=81;
//int32 packetLength = 81;


#define TLE_data_inFM 0x3000
#define TLE_size 35
int8 TLE_array[TLE_size]={1,2,3,4,5,6,7,1,2,3,4,5,6,7,1,2,3,4,5,6,7,1,2,3,4,5,6,7,1,2,3,4,5,6,7};
int8 TLE_array_header[]="TLE";

int8 packetsNoInTheCommand=0;
//unsigned int32 packetsNoInTheCommand=0;
int8 Command_00_ackMB=0x77;
int8 Command_DD_ackMB=0x33;
int8 need_restart_ackMB=0x44;
int1 isCommandData=0;
//int1 isSFwardData=0;
int1 isDuplicatedData=0;

#define satellite_info_pointer 0x00002000 // 00 00 20 00
#define SF_Data_start_Adrs 0x00011000 // 00 01 10 00
#define SF_Data_Addres_pointer 0x00010000 //  00 01 00 00

int32 SF_Data_Addres=SF_Data_start_Adrs;

unsigned int8 first_adsress[4]={0x00,0x01,0x10,0x00};
unsigned int8 after_last_adsress[4];

#define log_array_SFward_in_FM 0x6000 // 00 00 60 00
#define log_array_SFward_adrs_pointer 0x7000 // 00 00 70 00
int8 no_of_SFward_callsigns=0;
#define SFward_packet_array_columns 10
#define max_no_of_SFward_callsigns 100
int8 log_array_SFward[max_no_of_SFward_callsigns][SFward_packet_array_columns]={  {"\0BIRDS5\0\0"},{"\0JG6YBA\0\0\0"},{"\0JG6YBC\0\0\0"}  };

#define rcved_data_adrs_pointer 0x05FEF000 // 00 05 FE F0 00
#define rcved_data_in_FM 0x05FF0000 // 00 05 FF 00 00


#INT_RDA2 //from APRS
void rxAPRS()
{
dataAPRS[byteNumAPRS]=getc(serial2APRS);
//if (byteNumAPRS==dataSizeAPRS-1) {
if (dataAPRS[byteNumAPRS]==0x0A)
{
if(dataAPRS[byteNumAPRS-1]==0x0D)
{
actionAPRS=1; rec_bytes_number_APRS=byteNumAPRS;
}
}
byteNumAPRS = (byteNumAPRS+1);//% dataSizeAPRS;
}

#INT_RDA3 // from Mission Boss
void rxMB()
{
//RCSTA1bits.OERR = 0;
receivedDataMB=fgetc(serial2MB);
//putc(receivedDataMB,serial2MB);

if (receivedDataMB==0xE0) {byteNumMB=0;isCommandData=1; fprintf(debugPort,"\r\n Received an E0 \r\n");}
commandLine[byteNumMB]=receivedDataMB;

//if (receivedDataMB==0x0F) {byteNumMB=0;isCommandData=1; fprintf(debugPort,"\r\n Received an 0F \r\n");}
//commandLine[byteNumMB]=receivedDataMB;

fprintf(debugPort,"from MB we received %X, number of command bytes rcvd=%u \r\n",receivedDataMB,byteNumMB); //try to move it to the main function

if (byteNumMB==commandSizeMB-1) {
actionMB=1;
byteNumMB=0;
isCommandData=0;
fprintf(debugPort,"full command is received \r\n\r\n");
}

if (isCommandData==1) {byteNumMB = ((byteNumMB + 1) % commandSizeMB);}

}

void write_satellite_info()
{
   //delay_ms(10000);
   fprintf(debugPort,"Writing_satellite_info\r\n");
   SUBSECTOR_4KB_ERASE4(satellite_info_pointer);
   WRITE_DATA_N_BYTE4(satellite_info_pointer,sizeof(satellite_name),satellite_name);

}

void prepare_DummyData(int32 packetsNo)
{
//delay_ms(10000);
SUBSECTOR_4KB_ERASE4(0);
WRITE_DATA_N_DUMMY_BYTE4(0,packetsNo*81);
fprintf(debugPort,"\r\nFinished writing %lu packets of dummy Bytes in SF_FM\r\n\r\n",(packetsNo));
}

void show_FM_data(void)
{
fprintf(debugPort,"\r\nFM Data from addrss %X%X%X%X:\r\n",first_adsress[0],first_adsress[1],first_adsress[2],first_adsress[3]);
//fprintf(debugPort,"\r\nFM Data:\r\n");
SF_Data_Addres=make32(first_adsress[0],first_adsress[1],first_adsress[2],first_adsress[3]);

   for (i=SF_Data_start_Adrs;i<SF_Data_start_Adrs+162;++i)
   {
      if ((i-SF_Data_start_Adrs)%25==0) {fprintf(debugPort,"\r\n");}
      fprintf(debugPort,"%X ",READ_DATA_BYTE4(i));
      fputc(READ_DATA_BYTE4(i),serial2MB);
      delay_ms(delayTX);
   }
fprintf(debugPort,"\r\n");
}

// Ack to Mission Boss
void sendACKtoMB(int8 ackByte)
{
   fputc(ackByte,serial2MB);
   fprintf(debugPort," This ack is sent: %X \r\n",ackByte);
}


// E0 command from Mission Boss
void executeE0Command(int32 sendAddres,long packetsNoInTheCommand)
{
//unsigned int8 i;
//unsigned int32 i;
unsigned int8 data2MB;

fprintf(debugPort,"\r\n\r\n");
   for (int32 i=0;i<(packetsNoInTheCommand*packetLength);++i)
   {
   
      if (i%20==0) {fprintf(debugPort,"\r\n");}
      data2MB=READ_DATA_BYTE4(sendAddres+i);
      fputc(data2MB,serial2MB);
      //fputc(data2MB,debugPort);
      fprintf(debugPort,"%X ",data2MB);
      delay_ms(delayTX); 
   }
//fprintf(debugPort,"\r\nFinished sending %u packets, %u Bytes\r\n\r\n",packetsNoInTheCommand,(packetsNoInTheCommand*81));
//fprintf(debugPort,"\r\n %X\r\n", m);
fprintf(debugPort,"\r\nFinished sending %lu packets\r\n\r\n",packetsNoInTheCommand);

}

void show_SFward_info_array(void)
{
   int8 rawLoop=0;
   int8 columnLoop=0;

   
   fprintf(debugPort,"SFward packet array in RAM\r\n");
   fprintf(debugPort,"no_of_SFward_callsigns=%u\r\n",no_of_SFward_callsigns);
   for (rawLoop=0;rawLoop<no_of_SFward_callsigns;rawLoop++)
      {
         for (columnLoop=0;columnLoop<SFward_packet_array_columns;columnLoop++)
            {fprintf(debugPort,"%X ",log_array_SFward[rawLoop][columnLoop]);}
         fprintf(debugPort,"\r\n");
      }
      fprintf(debugPort,"\r\n");
   
//!   int16 no_of_SFward_callsigns_FM=make16(READ_DATA_BYTE4(pointer_no_of_SFward_callsigns),READ_DATA_BYTE4(pointer_no_of_SFward_callsigns+1));   
//!   int32 arrayElementAddrs=pointer_no_of_SFward_callsigns+2;

//!   fprintf(debugPort,"\r\nSFward packet array in Flash Mem.\r\n");
//!   fprintf(debugPort,"no_of_SFward_callsigns=%lu\r\n",no_of_SFward_callsigns_FM);
//!   for (rawLoop=0;rawLoop<no_of_SFward_callsigns_FM;rawLoop++)
//!      {
//!         for (columnLoop=0;columnLoop<SFward_packet_array_columns;columnLoop++)
//!            {fprintf(debugPort,"%X ",READ_DATA_BYTE4(arrayElementAddrs++));}
//!         fprintf(debugPort,"\r\n");
//!      }
}

void write_log_entery_FM(int8 raw_index)
{
   //int8 rawLoop=0;
   int8 columnLoop=0;
   int8 temp_one_raw_array[SFward_packet_array_columns];
   int32 arrayElementAddrs=make16(READ_DATA_BYTE4(log_array_SFward_adrs_pointer+2),READ_DATA_BYTE4(log_array_SFward_adrs_pointer+3));
   
   if (arrayElementAddrs >= (log_array_SFward_in_FM+0xFFD-SFward_packet_array_columns) )
   {
      SUBSECTOR_4KB_ERASE4(log_array_SFward_adrs_pointer);
      SUBSECTOR_4KB_ERASE4(log_array_SFward_in_FM);
      fprintf(debugPort,"Writing log_array_SFward_adrs_pointer to FM\r\n");
      arrayElementAddrs=log_array_SFward_in_FM;
      WRITE_DATA_BYTE4(log_array_SFward_adrs_pointer+2,arrayElementAddrs>>8);
      WRITE_DATA_BYTE4(log_array_SFward_adrs_pointer+3,arrayElementAddrs);
   }

    for (columnLoop=0;columnLoop<SFward_packet_array_columns;columnLoop++)
    {
         temp_one_raw_array[columnLoop]=log_array_SFward[raw_index][columnLoop];
    }
    arrayElementAddrs=WRITE_DATA_N_BYTE4(arrayElementAddrs,SFward_packet_array_columns,temp_one_raw_array);
    
    SUBSECTOR_4KB_ERASE4(log_array_SFward_adrs_pointer);
    WRITE_DATA_BYTE4(log_array_SFward_adrs_pointer+2,arrayElementAddrs>>8);
    WRITE_DATA_BYTE4(log_array_SFward_adrs_pointer+3,arrayElementAddrs);
}

void update_SFward_info_array_RAM (void)
{
      //fprintf(debugPort,"\r\nUpdating SFward packet array in the RAM\r\n");
      int8 rawLoop=0;
      int8 columnLoop=0;
      int1 new_entery=1;
      int8 raw_index=0;
      
      for (rawLoop=0;rawLoop<=no_of_SFward_callsigns;rawLoop++) //should be < not <= , right?        //to be removed to allow duplication 
      {
         if (log_array_SFward[rawLoop][9]==Packet_no_APRS){
            if (log_array_SFward[rawLoop][8]==info_size_SFward)
               if (log_array_SFward[rawLoop][6]==senderCallsign[5])
                  if (log_array_SFward[rawLoop][5]==senderCallsign[4])
                     if (log_array_SFward[rawLoop][4]==senderCallsign[3])
                        if (log_array_SFward[rawLoop][3]==senderCallsign[2])
                           if (log_array_SFward[rawLoop][2]==senderCallsign[1])
                              if (log_array_SFward[rawLoop][1]==senderCallsign[0])
                                 //{new_entery=0;}                                                 //change made here
                                 {new_entery=1;}
         }
      }

      if (new_entery==0)
      {
         
         fprintf(debugPort,"\r\nDuplicated,,, \r\n"); 
         fprintf(debugPort,"no_of_SFward_log_enteries: %u\r\n\r\n",no_of_SFward_callsigns);
         isDuplicatedData=1;
       }
      
      else //if (new_entery==1)
      {
         fprintf(debugPort,"\r\nAdding NEW entery in SFward packet array in the RAM\r\n");
         if (no_of_SFward_callsigns>=max_no_of_SFward_callsigns) {no_of_SFward_callsigns=0;}
         no_of_SFward_callsigns++;
         raw_index=no_of_SFward_callsigns-1;
         isDuplicatedData=0;
         
            log_array_SFward[raw_index][0]=no_of_SFward_callsigns;
            for (columnLoop=1;columnLoop<=6;columnLoop++) {log_array_SFward[raw_index][columnLoop]=senderCallsign[columnLoop-1];}
            log_array_SFward[raw_index][7]=0;
            log_array_SFward[raw_index][8]=info_size_SFward;
            log_array_SFward[raw_index][9]=Packet_no_APRS;
            
            write_log_entery_FM(raw_index);
            show_SFward_info_array();
      } //if new_entery end 
}

void prepare_SFwardData(int8 *SF_data)
{
 //fprintf(debugPort,"Sender Callsign: ");
// for (i=0;i<=5;++i) {putc(senderCallsing[i]=SF_data[i],debugPort);}
    i=0; 
   //fprintf(debugPort,"\r\nCALLSIGN\r\n");
   while (SF_data[i]!=0x3E) 
   {
      senderCallsign[i]=SF_data[i]; 
      
      //putc(senderCallsign[i],debugPort); 
      i++;
   } //0x3E='>'
   //fprintf(debugPort,"\r\n");
   callsign_size_SFward=i;
   //fprintf(debugPort,"Callsign size = %d\r\n",callsign_size_SFward);
   
  
 if ( memcmp(senderCallsign, paraguayCallsign_GS01, 6)==0 ) 
 {
 //fprintf(debugPort,"A packet is received from paraguay GS01 \r\n");
 GS_ID_SFward=paraguayGS01_ID;
 }
 else if ( memcmp(senderCallsign, philippineCallsign_GS01, 6)==0 )
 {
 //fprintf(debugPort,"A packet is received from philippine GS01 \r\n");
 GS_ID_SFward=philippineGS01_ID;
 }
  else if ( memcmp(senderCallsign, japanCallsign_GS01, 6)==0 )
 {
 //fprintf(debugPort,"A packet is received from Japan GS01 \r\n");
 GS_ID_SFward=japanGS01_ID;
 }
 else
 fprintf(debugPort,"A packet is received from new GS \r\n");
 
   int8 msgStart=0;
   int8 msgEnd=0;
   int8 counterFF=0;
   i=0;
   while (SF_data[i]!=0x7B) {msgEnd=i++;} //7B='{'
   //while (SF_data[i]!='a') {msgStart=i--; }                            //need to be changed   
   while (SF_data[i]!='G') {msgStart=i--; }                              //changed to this
//!   
//!   fprintf(debugPort,"msgStart=%X \r\n",SF_data[msgStart]);
//!   fprintf(debugPort,"msgEnd=%X \r\n",SF_data[msgEnd]);
//!   fprintf(debugPort,"value=%X \r\n",msgEnd-msgStart);
   fprintf(debugPort,"SF DATA: ");
   
   
   for (i=0;i<=(msgEnd-msgStart);i++)                                            //changed this to remove callsign from SFward data
   {
      //putc(info_APRS[i]=SF_data[msgStart+i],debugPort);
      putc(SF_data[msgStart+i],debugPort);
      //fprintf(debugPort, SF_data[msgStart+i]);
   }
   fprintf(debugPort,"\r\n");
   info_size_SFward=i++;
   //fprintf(debugPort,"SF data size = %d\r\n",info_size_SFward);
   
     
   //fprintf(debugPort,"first packet_no_APRS = %C\r\n",SF_data[msgEnd+2]); // +2 to go to the packet number first byte
   //fprintf(debugPort,"last  packet_no_APRS = %C\r\n",SF_data[rec_bytes_number_APRS-3]); //-3 to go to the packet number first byte
   
   Packet_no_APRS=0;
   int8 power=0;
   for (i=rec_bytes_number_APRS-3;i>=msgEnd+2;i--)
   Packet_no_APRS=Packet_no_APRS+(SF_data[i]-'0')*pow(10,power++);
   
   //fprintf(debugPort,"Packet_no = %d (%X hex)\r\n",Packet_no_APRS,Packet_no_APRS);
   
//!   
//!   
//! 
//! fprintf(debugPort,"SF Data: ");
//! i=0;
//! while (SF_data[32+i]!=0x7B)  {putc(info_APRS[i]=SF_data[32+i],debugPort); i++;}  //0x7B='{'
//! //34 is the first byte of the msg
//! fprintf(debugPort,"\r\n");
//!  
//! fprintf(debugPort,"SF data size = %d\r\n",info_size_SFward=i);
   
   update_SFward_info_array_RAM();

}

void sendAckVHF(void)
{
   
}

int1 is_sent_to_me (int8 *SF_data)
{
   i=0;
   while (i < rec_bytes_number_APRS)
   {
      if (SF_data[i]==0x3A && SF_data[i+1]==0x3A)
      {
         if (SF_data[i+2]=='B')
            if (SF_data[i+3]=='I')
               if (SF_data[i+4]=='R')
                  if (SF_data[i+5]=='D')
                     //if ((SF_data[i+6]=='J')||(SF_data[i+6]=='S'))
                     if ((SF_data[i+6]==READ_DATA_BYTE4(0x2004))||(SF_data[i+6]=='S'))
                        //if ((SF_data[i+7]=='P')||(SF_data[i+7]=='4'))
                        if ((SF_data[i+7]==READ_DATA_BYTE4(0x2005))||(SF_data[i+7]=='5'))
                        {
                           //fprintf(debugPort,"Received data is sent to THIS satellite \r\n");
                           return 1;
                        }
                        else return 0;
      }
      i++;
    }
    return 0;
}

int1 is_valid_SFward (int8 *SF_data)
{
   if (SF_data[32]=='S')
      if (SF_data[33]=='F')
      {
         if(SF_data[34]=='V') {sendAckVHF();}
         return 1;
      }
  else return 1; //should be return 0 
  return 1;
}

void write_SFwad_data_toFM(int8 *SF_data)
{
   int32 subsector4kbNo;
   
   first_adsress[0]=READ_DATA_BYTE4(SF_Data_Addres_pointer  );
   first_adsress[1]=READ_DATA_BYTE4(SF_Data_Addres_pointer+1);
   first_adsress[2]=READ_DATA_BYTE4(SF_Data_Addres_pointer+2);
   first_adsress[3]=READ_DATA_BYTE4(SF_Data_Addres_pointer+3);

   SF_Data_Addres=make32(first_adsress[0],first_adsress[1],first_adsress[2],first_adsress[3]);
   
   if ((SF_Data_Addres >= 0x05FEE000 )||(SF_Data_Addres <= SF_Data_start_Adrs-1 ))
   {
      SUBSECTOR_4KB_ERASE4(SF_Data_start_Adrs);
      SUBSECTOR_4KB_ERASE4(SF_Data_Addres_pointer); // we need to erase all sectors not the first one only, 
      SF_Data_Addres=SF_Data_start_Adrs;
      WRITE_DATA_BYTE4(SF_Data_Addres_pointer  ,SF_Data_Addres>>24);
      WRITE_DATA_BYTE4(SF_Data_Addres_pointer+1,SF_Data_Addres>>16);
      WRITE_DATA_BYTE4(SF_Data_Addres_pointer+2,SF_Data_Addres>>8);
      WRITE_DATA_BYTE4(SF_Data_Addres_pointer+3,SF_Data_Addres);
   }
   
   for (subsector4kbNo=adrs2subsector4kb(SF_Data_Addres)+1; subsector4kbNo<=adrs2subsector4kb(SF_Data_Addres+rec_bytes_number_APRS);subsector4kbNo++)
   {
      SUBSECTOR_4KB_ERASE4(subsector4kbNo*0x1000);
   }

   WRITE_DATA_BYTE4(SF_Data_Addres++,GS_ID_SFward);delay_ms(delayTX);
   WRITE_DATA_BYTE4(SF_Data_Addres++,GS_ID_SFward);delay_ms(delayTX);   //change this to use multible bytes write functoin
   if (1==0)//(GS_ID_SFward==0) 
   {
      WRITE_DATA_BYTE4(SF_Data_Addres++,callsign_size_SFward);delay_ms(delayTX);
      for (i=0;i>callsign_size_SFward;i++)
      {
         WRITE_DATA_BYTE4(SF_Data_Addres++,senderCallsign[i]);
         delay_ms(10);
      }
   }
   SF_Data_Addres=WRITE_DATA_N_BYTE4(SF_Data_Addres,callsign_size_SFward,senderCallsign); // Check that the callsign_size_SFward is the correct value
   
   WRITE_DATA_BYTE4(SF_Data_Addres++,0xFF);delay_ms(delayTX);
   WRITE_DATA_BYTE4(SF_Data_Addres++,info_size_SFward);delay_ms(delayTX);

   i=0;
   while (SF_data[32+i]!=0x7B)  //0x7B='{'  
   {
      //WRITE_DATA_BYTE4(SF_Data_Addres++,SF_data[32+i]);
      WRITE_DATA_BYTE4(SF_Data_Addres++,info_APRS[i]);

      //SF_Data_Addres++;
      delay_ms(10);
      i++;
   }
   WRITE_DATA_BYTE4(SF_Data_Addres++,info_end_SFward[0]);delay_ms(10);
   WRITE_DATA_BYTE4(SF_Data_Addres++,info_end_SFward[1]);delay_ms(10);
   fprintf(debugPort,"Done writing SF_Data from address %X%X%X%X \r\n ",first_adsress[0],first_adsress[1],first_adsress[2],first_adsress[3]);

   SUBSECTOR_4KB_ERASE4(SF_Data_Addres_pointer);
   after_last_adsress[0]  = (unsigned int8)((SF_Data_Addres>>24) & 0xFF);
   after_last_adsress[1]  = (unsigned int8)((SF_Data_Addres>>16) & 0xFF);
   after_last_adsress[2]  = (unsigned int8)((SF_Data_Addres>>8 ) & 0xFF);
   after_last_adsress[3]  = (unsigned int8)((SF_Data_Addres    ) & 0xFF); 
   //fprintf(debugPort,"Untill address %X%X%X%X -1 \r\n\r\n ",after_last_adsress[0],after_last_adsress[1],after_last_adsress[2],after_last_adsress[3]);
   fprintf(debugPort,"Untill address %lX \r\n\r\n ",SF_Data_Addres-1);


   WRITE_DATA_BYTE4(SF_Data_Addres_pointer  ,after_last_adsress[0]);
   WRITE_DATA_BYTE4(SF_Data_Addres_pointer+1,after_last_adsress[1]);
   WRITE_DATA_BYTE4(SF_Data_Addres_pointer+2,after_last_adsress[2]);
   WRITE_DATA_BYTE4(SF_Data_Addres_pointer+3,after_last_adsress[3]);
   
   fprintf(debugPort,"FINISHED WRITING\r\n ");

}

void save_rcved_data_in_FM(int8 *SF_data, int16 size)
{
   fprintf(debugPort,"\r\nSaving APRS data\r\n");
   //SUBSECTOR_4KB_ERASE4(rcved_data_adrs_pointer);
   //SUBSECTOR_4KB_ERASE4(rcved_data_in_FM);
   rcved_data_adrs=make32(READ_DATA_BYTE4(rcved_data_adrs_pointer),READ_DATA_BYTE4(rcved_data_adrs_pointer+1),READ_DATA_BYTE4(rcved_data_adrs_pointer+2),READ_DATA_BYTE4(rcved_data_adrs_pointer+3));
   int32 subsector4kbNo;
   
   if ((rcved_data_adrs >= 0x07FFF000 )||(rcved_data_adrs <= rcved_data_in_FM-1 ))
   {
       
       fprintf(debugport,"\r\nMemory is full, Erasing now & writing Afresh\r\n");
       
      SUBSECTOR_4KB_ERASE4(rcved_data_adrs_pointer);
      SUBSECTOR_4KB_ERASE4(rcved_data_in_FM); // we need to erase all sectors not the first one only, 
      rcved_data_adrs=rcved_data_in_FM;
      WRITE_DATA_BYTE4(rcved_data_adrs_pointer, rcved_data_adrs>>24);
      WRITE_DATA_BYTE4(rcved_data_adrs_pointer+1,rcved_data_adrs>>16);
      WRITE_DATA_BYTE4(rcved_data_adrs_pointer+2,rcved_data_adrs>>8);
      WRITE_DATA_BYTE4(rcved_data_adrs_pointer+3,rcved_data_adrs);
   }
   
   //fprintf(debugport,"data will be written from subsector4kb %lu to subsector4kb %lu\r\n",adrs2subsector4kb(rcved_data_adrs),adrs2subsector4kb(rcved_data_adrs+size));
   for (subsector4kbNo=adrs2subsector4kb(rcved_data_adrs)+1; subsector4kbNo<=adrs2subsector4kb(rcved_data_adrs+size);subsector4kbNo++)
   {
      SUBSECTOR_4KB_ERASE4(subsector4kbNo*0x1000);
   }
   
   fprintf(debugport,"\r\nAppending Memory\r\n");
   fprintf(debugPort, "%lX", rcved_data_adrs);
   fprintf(debugport,"\r\n");
   MyAdress = rcved_data_adrs;
   
   
   rcved_data_adrs=WRITE_DATA_N_BYTE4(rcved_data_adrs,size,SF_data);
   
   SUBSECTOR_4KB_ERASE4(rcved_data_adrs_pointer);
   WRITE_DATA_BYTE4(rcved_data_adrs_pointer,  rcved_data_adrs>>24);
   WRITE_DATA_BYTE4(rcved_data_adrs_pointer+1,rcved_data_adrs>>16);
   WRITE_DATA_BYTE4(rcved_data_adrs_pointer+2,rcved_data_adrs>>8);
   WRITE_DATA_BYTE4(rcved_data_adrs_pointer+3,rcved_data_adrs);
}

void reset_SFward_FM_data(void)
{
      SUBSECTOR_4KB_ERASE4(SF_Data_Addres_pointer);
      SUBSECTOR_4KB_ERASE4(SF_Data_start_Adrs);
      
      WRITE_DATA_BYTE4(SF_Data_Addres_pointer  ,SF_Data_start_Adrs>>24);
      WRITE_DATA_BYTE4(SF_Data_Addres_pointer+1,SF_Data_start_Adrs>>16);
      WRITE_DATA_BYTE4(SF_Data_Addres_pointer+2,SF_Data_start_Adrs>>8);
      WRITE_DATA_BYTE4(SF_Data_Addres_pointer+3,SF_Data_start_Adrs);
      fprintf(debugPort,"saving address and memory for SFward data are reset \r\n");
      show_FM_data();
}

void reset_all(void)
{
      fprintf(debugPort,"Major reset is \r\n");
      //die_erase_command();
      //erase_sectors(1535,2017);
      //erase_sectors(0,4);

      SUBSECTOR_4KB_ERASE4(0);
      prepare_DummyData(3);
      
      SUBSECTOR_4KB_ERASE4(satellite_info_pointer);
      write_satellite_info();

      SUBSECTOR_4KB_ERASE4(log_array_SFward_in_FM);
      SUBSECTOR_4KB_ERASE4(log_array_SFward_adrs_pointer);
      WRITE_DATA_BYTE4(log_array_SFward_adrs_pointer  ,log_array_SFward_in_FM>>24);
      WRITE_DATA_BYTE4(log_array_SFward_adrs_pointer+1,log_array_SFward_in_FM>>16);
      WRITE_DATA_BYTE4(log_array_SFward_adrs_pointer+2,log_array_SFward_in_FM>>8);
      WRITE_DATA_BYTE4(log_array_SFward_adrs_pointer+3,log_array_SFward_in_FM);

      SUBSECTOR_4KB_ERASE4(SF_Data_start_Adrs);
      SUBSECTOR_4KB_ERASE4(SF_Data_Addres_pointer);
      WRITE_DATA_BYTE4(SF_Data_Addres_pointer  ,SF_Data_start_Adrs>>24);
      WRITE_DATA_BYTE4(SF_Data_Addres_pointer+1,SF_Data_start_Adrs>>16);
      WRITE_DATA_BYTE4(SF_Data_Addres_pointer+2,SF_Data_start_Adrs>>8);
      WRITE_DATA_BYTE4(SF_Data_Addres_pointer+3,SF_Data_start_Adrs);
      
      SUBSECTOR_4KB_ERASE4(rcved_data_in_FM);
      SUBSECTOR_4KB_ERASE4(rcved_data_adrs_pointer);
      WRITE_DATA_BYTE4(rcved_data_adrs_pointer, rcved_data_in_FM>>24);
      WRITE_DATA_BYTE4(rcved_data_adrs_pointer+1,rcved_data_in_FM>>16);
      WRITE_DATA_BYTE4(rcved_data_adrs_pointer+2,rcved_data_in_FM>>8);
      WRITE_DATA_BYTE4(rcved_data_adrs_pointer+3,rcved_data_in_FM);
      
      fprintf(debugPort,"Major reset is finished \r\n");
}

void settings(void){

fprintf(debugPort,"\r\n\r\n");
fprintf(debugPort," Birds5 SFward Program Started v2\r\n");
output_high(PIN_B2); // enable APRS
fprintf(debugPort,"APRS_DP Enabled\r\n");
ENABLE_4BYTES_ADDRESS();
//MyAdress = rcved_data_adrs;
fprintf(debugPort,"Writing to SFwardFM will start from address %X%X%X%X\r\n",READ_DATA_BYTE4(SF_Data_Addres_pointer),READ_DATA_BYTE4(SF_Data_Addres_pointer+1),READ_DATA_BYTE4(SF_Data_Addres_pointer+2),READ_DATA_BYTE4(SF_Data_Addres_pointer+3));
fprintf(debugPort,"address pointer %X%X%X%X\r\n",READ_DATA_BYTE4(rcved_data_adrs_pointer),READ_DATA_BYTE4(rcved_data_adrs_pointer+1),READ_DATA_BYTE4(rcved_data_adrs_pointer+2),READ_DATA_BYTE4(rcved_data_adrs_pointer+3));


while(isBusy()) 
{
fprintf(debugPort,"Cannot access the Flash Memory, SR=%X\r\n",ReadSR());
delay_ms(1000); 
sendACKtoMB(need_restart_ackMB);
};

//reset_all();


if ((READ_DATA_BYTE4(satellite_info_pointer+6)!=0x00)||(READ_DATA_BYTE4(satellite_info_pointer)!='B') ){write_satellite_info();}

if (READ_DATA_BYTE4(SF_Data_Addres_pointer)==0xFF)
   {
      reset_SFward_FM_data();
   }

//fprintf(debugPort,"Writing to SFwardFM will start from address %X%X%X%X\r\n",READ_DATA_BYTE4(SF_Data_Addres_pointer),READ_DATA_BYTE4(SF_Data_Addres_pointer+1),READ_DATA_BYTE4(SF_Data_Addres_pointer+2),READ_DATA_BYTE4(SF_Data_Addres_pointer+3));

enable_interrupts(GLOBAL);
enable_interrupts(INT_RDA2);
enable_interrupts(INT_RDA3);

}

void main()

{
   settings();

   int8 SF_data[APRS_packet_max_length];
   int8 APRS_packet_no=0;
   int8 APRS_info[67]="HelloWorld my name is Edgar";
      
//int16 *SF_data;
//!int * iptr;
//!iptr=malloc(1);
//!int *ptr = (int8*) calloc(10 ,sizeof (int8));
//!if (ptr == NULL) 
//!{
//!  printf("Could not allocate memory\n");
//!} 
//!else
//!  printf("Memory allocated successfully.\n");


   while(TRUE)
   {
    //if (kbhit(serial2APRS))  //check if a character has been received
    //c = getc(serial2APRS);  


//******************************************************************************   
   //if (byteNumAPRS>=45){fprintf(debugPort,"DataSize= %u\r\n",sizeof(dataAPRS));}
   //if (kbhit(serial2APRS)==1) {}
   
   if (actionAPRS==1){
   //fprintf(debugPort,"\r\nThe number of received bytes =%u\r\n",(rec_bytes_number_APRS));
   fprintf(debugPort,"\r\n***********************************************\r\n");
   for (i=0;i<=rec_bytes_number_APRS;++i) {SF_data[i]=dataAPRS[i]; putc(SF_data[i],debugPort);}

  delay_ms(5);

   if (is_sent_to_me(SF_data)) //if the messege is sent to BIRDS4
      if (is_valid_SFward(SF_data))
         {
            prepare_SFwardData(SF_data);
            if (!isDuplicatedData) 
               write_SFwad_data_toFM(SF_data);
               //show_FM_data();
         }
      else fprintf(debugPort,"Non SFward msg was received, it was not saved to FM\r\n");
  
  if (!isDuplicatedData) 
  save_rcved_data_in_FM(SF_data,rec_bytes_number_APRS);
  //for (i=0;i<=rec_bytes_number_APRS;++i) {fprintf(debugPort,"%X ",SF_data[i]);}
   actionAPRS=0;
   byteNumAPRS=0;
   }
   
//******************************************************************************
   if (actionMB==1)
   {
      fprintf(debugPort," This command is received from MB ");
      fprintf(debugPort,"%X",commandLine[0]);
      fprintf(debugPort,"%X",commandLine[1]);
      fprintf(debugPort,"%X",commandLine[2]);
      fprintf(debugPort,"%X",commandLine[3]);
      fprintf(debugPort,"%X",commandLine[4]);
      fprintf(debugPort,"%X",commandLine[5]);
      fprintf(debugPort,"%X",commandLine[6]);
      fprintf(debugPort,"\r\n");
      
      if (commandLine[1]!=0xAA)
      {
         if ((commandLine[5]==0x00)||(commandLine[5]==0xA2))
         {
            memcpy(lastCommandLine,commandLine,sizeof(commandLine));
            addressInTheCommand=  make32(lastCommandLine[commandSizeMB-6],lastCommandLine[commandSizeMB-5],lastCommandLine[commandSizeMB-4],lastCommandLine[commandSizeMB-3]);
            packetsNoInTheCommand=lastCommandLine[commandSizeMB-1];
      
            fprintf(debugPort,"addressInTheCommand  = %lX \r\n",addressInTheCommand);
            fprintf(debugPort,"packetsNoInTheCommand= %lX \r\n",packetsNoInTheCommand);
      
            fprintf(debugPort," E0 command, send %u packets from %X%X%X%X address.\r\n",lastCommandLine[commandSizeMB-1],lastCommandLine[commandSizeMB-6],lastCommandLine[commandSizeMB-5],lastCommandLine[commandSizeMB-4],lastCommandLine[commandSizeMB-3]);

            delay_ms(700);
            //sendACKtoMB(Command_00_ackMB);
            executeE0Command(addressInTheCommand,packetsNoInTheCommand); //
            //executeE0Command(addressInTheCommand,1);
         }
         else if ((commandLine[5]==0xFF)||(commandLine[5]==0xEE))
         {
            reset_SFward_FM_data();  
            reset_all();
         }
         else if ( commandLine[5]==0xAA)
         {
            show_FM_data();
         }
         else if ((commandLine[5]==0xCC)||(commandLine[5]==0xA4))
         {
              fprintf(debugPort,"APRS_info_before the sending:\r\n");
              for (i=0;i<( sizeof(TLE_array_header)+sizeof(TLE_array) );i++)
              {
              APRS_info[i]=READ_DATA_BYTE4(  (int32)(TLE_data_inFM+i)  );
              fprintf(debugPort,"%X ",APRS_info[i]);
              }
              fprintf(debugPort,"\r\n");
              
              fprintf(debugPort,"EXPEREMENT ...\r\n");
              int8 APRS_info_str[120];
              int8 APRS_info_str_temp[3];
              
              sprintf(APRS_info_str,"%X",APRS_info[0]);
              for (i=1;i<39;i++)
              {
              sprintf(APRS_info_str_temp,"%X",APRS_info[i]);
              strcat (APRS_info_str,APRS_info_str_temp);
              fprintf(debugPort,"%s \r\n",APRS_info_str);
              }
              fprintf(debugPort,"%s \r\n",APRS_info_str);

              fprintf(debugPort,"SENDING ...\r\n");
              if (APRS_packet_no>=99) APRS_packet_no=0;
              send_APRS(++APRS_packet_no,APRS_info_str); delay_ms(1000);
              send_APRS(  APRS_packet_no,APRS_info_str); delay_ms(1000);
              send_APRS(  APRS_packet_no,APRS_info_str); 
              fprintf(debugPort,"DONE SENDING \r\n");
         }
         else if ( commandLine[5]==0xBB)
         {
            fprintf(debugPort,"SENDING ...\r\n");
            //sendingAX25();
            fprintf(debugPort,"DONE SENDING \r\n");
            
                          //sendingAX25();
              //send_APRS(data_to_send_APRS,sizeof(data_to_send_APRS));
              //downlink_serial();
              //send_acknowledgment_packet();

              RX_KISSframe_len=sizeof(RX_KISSframe);
              RX_packet_len = KISSframe_to_AX25packet(RX_KISSframe, RX_KISSframe_len, RX_packet);
              
              fprintf(debugPort,"Ax.25 frame as hex:\r\n ");
              for (i=0;i<RX_packet_len;i++) 
              {
                  putc(RX_packet[i],serial2APRS);
                  fprintf(debugPort,"%X ",RX_packet[i]);
              }
              putc('\r',serial2APRS);
              fprintf(debugPort,"\r\n ");
              
              fprintf(debugPort,"Ax.25 frame as Asci:\r\n ");
              for (i=0;i<RX_packet_len;i++) 
              {
                  fprintf(debugPort,"%C",RX_packet[i]);
              }
              fprintf(debugPort,"\r\n ");
              
              
              int numberofreceivedpackets = 0;
               if(RX_packet_len>16)
               {         
                  int8 parsing_result = parse_received_packet();
                     //send_acknowledgment_packet();                     
         //!            if( parsing_result == 0x01 )
         //!            {
         //!               //Send ACK packet if the received packet was a sensor data packet
         //!               if(RX_info_field[2]==0x00) 
         //!               {
         //!                  send_acknowledgment_packet();
         //!                  numberofreceivedpackets++;
         //!                  puts("");
         //!                  printf("Number of Received Packets: %i", numberofreceivedpackets);
         //!                  puts("");
         //!                  printf("AX.25 packet length (inc. CRC): %li", RX_packet_len+2);
         //!                  puts(""); puts(""); puts("");
         //!               }
         //!            }
          
               }    
         }
         else if((commandLine[5]==0xA9))
         {
          
         unsigned int8 bytee[4];
//!          bytee[0] = MyAdress &0x000000ff;
//!          bytee[1] = (MyAdress &0x0000ff00)>>8;
//!          bytee[2] = (MyAdress &0x00ff0000)>>16;
//!          bytee[3] = (MyAdress &0xff000000)>>24;
//!          
//!         
          
          
          bytee[0] = READ_DATA_BYTE4(rcved_data_adrs_pointer);
          bytee[1] = READ_DATA_BYTE4(rcved_data_adrs_pointer+1);
          bytee[2] = READ_DATA_BYTE4(rcved_data_adrs_pointer+2);
          bytee[3] = READ_DATA_BYTE4(rcved_data_adrs_pointer+3);
           
                
          delay_ms(10);
          //fputc(byte3, serial2MB);
          //delay_ms(10);
          //fputc(byte2, serial2MB);
          //delay_ms(10);
          //fputc(byte1, serial2MB);
          //delay_ms(10);
          //fputc(byte0, serial2MB);
          //delay_ms(10);
          //fputc(data2MB,serial2MB);
         
        //byte4 = datahex(MyAdress);
        //fputc(byte4, serial2MB);
        
        
        //unsigned int8 bytee[4];
   //Byte extraction
   //bytee[0]  = (unsigned int8)((MyAdress>>24) & 0xFF);                          // 0x _ _ 00 00 00
   //bytee[1]  = (unsigned int8)((MyAdress>>16) & 0xFF);                          // 0x 00 _ _ 00 00
   //bytee[2]  = (unsigned int8)((MyAdress>>8) & 0xFF);                           // 0x 00 00 _ _ 00
   //bytee[3]  = (unsigned int8)((MyAdress) & 0xFF);  
        fprintf(debugport,"\r\n");
        fprintf(debugport,"\r\nprinting the adress in byte form\r\n");
        delay_ms(1000);
        fputc(Command_00_ackMB,serial2MB);
        for(int x =0; x<4; x++)
        {
        fprintf(debugPort, "%X", bytee[x]);
        delay_ms(10);
       
        fputc(bytee[x],serial2MB);
        }
        
        for(int y =0; y<76; y++)
        {
        fprintf(debugPort, "%X", Command_00_ackMB);
        delay_ms(10);
       
        fputc(Command_00_ackMB,serial2MB);
        }
        
         
         }
         
         else if ((commandLine[5]==0xDD)||(commandLine[5]==0xA3))
         {
            int32 adrs_temp;
            
            disable_interrupts(INT_RDA3);
            sendACKtoMB(Command_DD_ackMB);
            fprintf(debugPort,"Received TLE data: ");
            for (i=0;i<TLE_size;i++)
            {
               TLE_array[i]=fgetc(serial2MB);               
               if (i%25==0) {fprintf(debugPort,"\r\n");}
               fprintf(debugPort,"%X ",TLE_array[i]);
            }
            SUBSECTOR_4KB_ERASE4(TLE_data_inFM);
            adrs_temp=WRITE_DATA_N_BYTE4(TLE_data_inFM, sizeof(TLE_array_header),TLE_array_header);
            WRITE_DATA_N_BYTE4(adrs_temp, sizeof(TLE_array),TLE_array);
            
            //fprintf(debugPort,"\r\nTLE_size:%u .. sizeof(TLE_array): %u .. They Should be same",TLE_size,sizeof(TLE_array));
            fprintf(debugPort,"\r\n Reception of TLE is finished\r\n");
            //delay_ms(100);
            enable_interrupts(INT_RDA3);
         }
      }
      
      actionMB=0;
   }
//******************************************************************************

   
   }
}


