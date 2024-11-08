#include <18F67J94.h>
//#device ICD=TRUE
//#use delay(crystal=20000000,restart_wdt)


#fuses PR, MS, STVREN, SOSC_DIG, NOPLL, NODEBUG, BROWNOUT_SW, NOWDT, NOPROTECT 
#fuses NOIOL1WAY, NOXINST, CLOCKOUT, WINDIS 

#device ADC=16
#use delay(crystal=8Mhz, clock=32Mhz)

#define SPIPORT SFward_FM
#define SPIPORT_2 SFward_FM
#define SPIPORT_3 SFward_FM

#define CS_PIN PIN_A2
#define CS_PIN_2 PIN_A2
#define CS_PIN_3 PIN_A2

#define READ_ID               0x9F
#define READ_STATUS_REG       0x05 
#define READ3_DATA_BYTES      0x03  //0x03 for 3byte 0x13 for 3byte
#define READ4_DATA_BYTES      0x13  //0x03 for 3byte 0x13 for 3byte
#define ENABLE_WRITE          0x06
#define DISABLE_WRITE         0x04
#define WRITE3_PAGE           0x02  //0x02 for 3byte 0x12 for 4byte
#define WRITE4_PAGE           0x12  //0x02 for 3byte 0x12 for 4byte
#define ERASE3_SECTOR         0xD8  //0xD8 for 3byte 0xDC for 4byte
#define ERASE4_SECTOR         0xDC  //0xD8 for 3byte 0xDC for 4byte
#define ENABLE_BYTE4          0xB7
#define ERASE3_4KB_SUBSECTOR  0x20
#define ERASE4_4KB_SUBSECTOR  0x21
#define DIE_ERASE             0xC4

#use spi (MASTER, CLK=PIN_A3, DI=PIN_A1, DO=PIN_A0, BAUD=1000000, MODE=0, BITS=8, STREAM=SFward_FM)
//#use spi (MASTER, CLK=PIN_A3, DI=PIN_A1, DO=PIN_A0, BAUD=1000000, MODE=0, BITS=8, STREAM=SFward_FM)
//#use spi (MASTER, CLK=PIN_A3, DI=PIN_A1, DO=PIN_A0, BAUD=1000000, MODE=0, BITS=8, STREAM=SFward_FM)

#define debugPort debug

#PIN_SELECT U2TX=PIN_C7
#PIN_SELECT U2RX=PIN_C6 
//#use rs232(UART2,baud=19200,parity=N,bits=8,stream=serial2APRS,SAMPLE_EARLY,ERRORS)
#use rs232(UART2,baud=19200,parity=N,bits=8,stream=serial2APRS,ERRORS,TIMEOUT=700)

#PIN_SELECT U3TX=PIN_E1 //connected with C0 in MB2
#PIN_SELECT U3RX=PIN_E0 //connected with C1 in MB2
#use rs232(UART3,baud=9600,parity=N,bits=8,stream=serial2MB,ERRORS,TIMEOUT=300) // removed ,RECEIVE_BUFFER=10

#use rs232(baud=19200,parity=N,xmit=pin_E6,bits=8,stream=debug,ERRORS)
//#use rs232(baud=19200,parity=N,xmit=pin_C7,rcv=pin_C6,bits=8,stream=serial2APRS)

int8 writing_delay=20;
int1 showDebug =0;

int32 adrs2subsector4kb(int32 adrs)
{
   int32 subsector4kbNo=adrs/0x1000;
   return subsector4kbNo;
}

int1 isBusy(void)
{
   int8 SR;
   output_low(CS_PIN_3);
   spi_xfer(SPIPORT,READ_STATUS_REG); 
   SR=spi_xfer(SPIPORT,READ_STATUS_REG);
   output_high(CS_PIN_3);
   //fprintf(debugPort,"SR=%X\r\n",SR);
   return bit_test(SR, 0); //return the busy flag
}



int8 ReadSR(){
 //delay_ms(2);
 int8 SR=0xFF;
 output_low(CS_PIN_2);
 delay_ms(10);
 spi_xfer(SPIPORT,READ_STATUS_REG); 
 SR=spi_xfer(SPIPORT,READ_STATUS_REG); 
 delay_ms(10);
 output_high(CS_PIN_2);
 return SR;
}

void ENABLE_4BYTES_ADDRESS()
{
 while(isBusy());
 output_low(CS_PIN_3);
 spi_xfer(SPIPORT_3,ENABLE_BYTE4);               
 output_high(CS_PIN_3);
 while(isBusy());
 return;
}

void WRITE_ENABLE3(){
 while(isBusy());
 output_low(CS_PIN_2);
 spi_xfer(SPIPORT_2,ENABLE_WRITE);                //Send 0x06
 output_high(CS_PIN_2);  
 while(isBusy());
 return;
 
}
void WRITE_ENABLE4(){
 while(isBusy());
 output_low(CS_PIN_3);
 spi_xfer(SPIPORT_3,ENABLE_WRITE);                //Send 0x06
 output_high(CS_PIN_3);  
 while(isBusy());
 return;
}
void WRITE_DISABLE4(){
 while(isBusy());
 output_low(CS_PIN_3);
 spi_xfer(SPIPORT_3,DISABLE_WRITE);                //Send 0x06
 output_high(CS_PIN_3);  
 while(isBusy());
 return;
}

void sector_erase3(unsigned int32 sector_address)
{
   fprintf(debugPort,"Erasing Sector64kb adrs %lX\r\n",sector_address);
   unsigned int8 adsress[4];
   
   adsress[0]  = (unsigned int8)((sector_address>>24) & 0xFF);                   // 0x _ _ 00 00 00
   adsress[1]  = (unsigned int8)((sector_address>>16) & 0xFF);                   // 0x 00 _ _ 00 00
   adsress[2]  = (unsigned int8)((sector_address>>8) & 0xFF);                    // 0x 00 00 _ _ 00
   adsress[3]  = (unsigned int8)((sector_address) & 0xFF);                       // 0x 00 00 00 _ _
   
   
   WRITE_ENABLE3();
   output_low(CS_PIN_2);                                                         //lower the CS PIN
   delay_us(2);
   
   ///////////////////////////////////////////////////////////////////
   spi_xfer(SPIPORT_2,ERASE3_SECTOR);                                             //SECTOR ERASE COMAND   (0xDC)
   
   //spi_xfer(SPIPORT_2,adsress[0]);   
   spi_xfer(SPIPORT_2,adsress[1]);    
   spi_xfer(SPIPORT_2,adsress[2]);    
   spi_xfer(SPIPORT_2,adsress[3]);
   //////////////////////////////////////////////////////////////////
   
   delay_us(2);
   output_high(CS_PIN_2);                                                        //take CS PIN higher back
   while(isBusy());
   return;
}
void sector_erase4(unsigned int32 sector_address)
{
   fprintf(debugPort,"Erasing Sector64kb adrs %lX\r\n",sector_address);
   unsigned int8 adsress[4];
   
   adsress[0]  = (unsigned int8)((sector_address>>24) & 0xFF);   // 0x _ _ 00 00 00
   adsress[1]  = (unsigned int8)((sector_address>>16) & 0xFF);   // 0x 00 _ _ 00 00
   adsress[2]  = (unsigned int8)((sector_address>>8) & 0xFF);    // 0x 00 00 _ _ 00
   adsress[3]  = (unsigned int8)((sector_address) & 0xFF);       // 0x 00 00 00 _ _
   
   
   WRITE_ENABLE4();
   output_low(CS_PIN_3);             //lower the CS PIN
   delay_us(2);
   
   ///////////////////////////////////////////////////////////////////
   spi_xfer(SPIPORT_3,ERASE4_SECTOR); //SECTOR ERASE COMAND   (0xDC)
   
   spi_xfer(SPIPORT_3,adsress[0]);   
   spi_xfer(SPIPORT_3,adsress[1]);    
   spi_xfer(SPIPORT_3,adsress[2]);    
   spi_xfer(SPIPORT_3,adsress[3]);
   //////////////////////////////////////////////////////////////////
   
   delay_us(2);
   output_high(CS_PIN_3);           //take CS PIN higher back
   //delay_ms(1000);  
   while(isBusy());
   return;
}


void SUBSECTOR_4KB_ERASE3(unsigned int32 sector_address)                      //Funcion que borra un sector de 4KB de la COM Flash
{
   fprintf(debugPort,"Erasing subsector4kb adrs %lX\r\n",sector_address);
   unsigned int8 adsress[4];
   
   adsress[0]  = (unsigned int8)((sector_address>>24) & 0xFF);                   // 0x _ _ 00 00 00
   adsress[1]  = (unsigned int8)((sector_address>>16) & 0xFF);                   // 0x 00 _ _ 00 00
   adsress[2]  = (unsigned int8)((sector_address>>8) & 0xFF);                    // 0x 00 00 _ _ 00
   adsress[3]  = (unsigned int8)((sector_address) & 0xFF);                       // 0x 00 00 00 _ _
   
   
   WRITE_ENABLE3();                                                           //Funcion que habilita escritura en COM Flash
   output_low(CS_PIN_2);                                                         //lower the CS PIN
   delay_us(2);
   
   ///////////////////////////////////////////////////////////////////
   spi_xfer(SPIPORT_2,ERASE3_4KB_SUBSECTOR); //SECTOR ERASE COMAND   (0xDC)
   
   //spi_xfer(SPIPORT_2,adsress[0]);   
   spi_xfer(SPIPORT_2,adsress[1]);    
   spi_xfer(SPIPORT_2,adsress[2]);    
   spi_xfer(SPIPORT_2,adsress[3]);
   //////////////////////////////////////////////////////////////////
   
   delay_us(2);
   output_high(CS_PIN_2);                                                        //take CS PIN higher back
   while(isBusy()); 
   
   return;
}
void SUBSECTOR_4KB_ERASE4(unsigned int32 sector_address)                      //Funcion que borra un sector de 4KB de la Mission Flash
{
   fprintf(debugPort,"Erasing subsector4kb adrs %lX\r\n",sector_address);
   unsigned int8 adsress[4];
   
   adsress[0]  = (unsigned int8)((sector_address>>24) & 0xFF);                   // 0x _ _ 00 00 00
   adsress[1]  = (unsigned int8)((sector_address>>16) & 0xFF);                   // 0x 00 _ _ 00 00
   adsress[2]  = (unsigned int8)((sector_address>>8) & 0xFF);                    // 0x 00 00 _ _ 00
   adsress[3]  = (unsigned int8)((sector_address) & 0xFF);                       // 0x 00 00 00 _ _
   
   
   WRITE_ENABLE4();                                                           //Funcion que habilita escritura en Mission Flash
   output_low(CS_PIN_3);                                                         //lower the CS PIN
   delay_us(2);
   
   ///////////////////////////////////////////////////////////////////
   spi_xfer(SPIPORT_3,ERASE4_4KB_SUBSECTOR);                                      //SECTOR ERASE COMAND   (0xDC)
   
   spi_xfer(SPIPORT_3,adsress[0]);   
   spi_xfer(SPIPORT_3,adsress[1]);    
   spi_xfer(SPIPORT_3,adsress[2]);    
   spi_xfer(SPIPORT_3,adsress[3]);
   //////////////////////////////////////////////////////////////////
   
   delay_us(2);
   output_high(CS_PIN_3);                                                        //take CS PIN higher back
   while(isBusy()); 
   
   return;
}

void erase_sectors(int16 startingSector, int16 endingSector)
{
   int32 sector; 
   for (sector=startingSector;sector<=endingSector;sector++)
   {
      sector_erase4((int32) sector*16*0x1000);
      fprintf(debugPort,"Sector %lu is erased\r\n", sector);
   }
}
void die_erase_command(void)
{
   //fprintf(debugPort,"die_erase_command started\r\n");
   unsigned int8 adsress[4]={0,0,0,0};
   WRITE_ENABLE4();
   output_low(CS_PIN_3);             //lower the CS PIN
   delay_us(2);
   
   ///////////////////////////////////////////////////////////////////
   spi_xfer(SPIPORT_3,DIE_ERASE); 
   
   spi_xfer(SPIPORT_3,adsress[0]);   
   spi_xfer(SPIPORT_3,adsress[1]);    
   spi_xfer(SPIPORT_3,adsress[2]);    
   spi_xfer(SPIPORT_3,adsress[3]);
   //////////////////////////////////////////////////////////////////
    
   delay_us(2);
   //delay_ms(50000);
   output_high(CS_PIN_3);           //take CS PIN higher back
   
   while(isBusy());
   
   //fprintf(debugPort,"die_erase_command finished\r\n");
   return;
}


void WRITE_DATA_BYTE3(unsigned int32 page_address, int8 data)                 //Funcion que escribe un Byte en la COM Flash
{
   unsigned int8 adsress[4];
   
   //Byte extraction
   adsress[0]  = (unsigned int8)((page_address>>24) & 0xFF);                     // 0x _ _ 00 00 00
   adsress[1]  = (unsigned int8)((page_address>>16) & 0xFF);                     // 0x 00 _ _ 00 00
   adsress[2]  = (unsigned int8)((page_address>>8) & 0xFF);                      // 0x 00 00 _ _ 00
   adsress[3]  = (unsigned int8)((page_address) & 0xFF);                         // 0x 00 00 00 _ _
   
   WRITE_ENABLE3();
   output_low(CS_PIN_2);                                                         //lower the CS PIN
   spi_xfer(SPIPORT_2,WRITE3_PAGE);                                               //PAGE WRITE COMAND  (0x12)
  
   //spi_xfer(SPIPORT_2,adsress[0]);    
   spi_xfer(SPIPORT_2,adsress[1]);    
   spi_xfer(SPIPORT_2,adsress[2]);    
   spi_xfer(SPIPORT_2,adsress[3]);
   
   spi_xfer(SPIPORT_2,data); 
   ////////////////////////////////////////////////////////////////
   
   output_high(CS_PIN_2);                                                        //take CS PIN higher back
   while(isBusy());
   return;
}
void WRITE_DATA_BYTE4(unsigned int32 page_address, int8 data)                 //Funcion que escribe un Byte en la Mission Flash
{
   while(isBusy());
   unsigned int8 adsress[4];
   
   //Byte extraction
   adsress[0]  = (unsigned int8)((page_address>>24) & 0xFF);                     // 0x _ _ 00 00 00
   adsress[1]  = (unsigned int8)((page_address>>16) & 0xFF);                     // 0x 00 _ _ 00 00
   adsress[2]  = (unsigned int8)((page_address>>8) & 0xFF);                      // 0x 00 00 _ _ 00
   adsress[3]  = (unsigned int8)((page_address) & 0xFF);                         // 0x 00 00 00 _ _
   
   WRITE_ENABLE4();
   output_low(CS_PIN_3);                                                         //lower the CS PIN
   //delay_us(2);
   
   spi_xfer(SPIPORT_3,WRITE4_PAGE);                                               //PAGE WRITE COMAND  (0x12)
  
   spi_xfer(SPIPORT_3,adsress[0]);    
   spi_xfer(SPIPORT_3,adsress[1]);    
   spi_xfer(SPIPORT_3,adsress[2]);    
   spi_xfer(SPIPORT_3,adsress[3]);
   
   spi_xfer(SPIPORT_3,data); 
   ////////////////////////////////////////////////////////////////
   
   output_high(CS_PIN_3);                                                        //take CS PIN higher back
   //delay_ms(writing_delay);  
   WRITE_DISABLE4();
   while(isBusy());
   return;
}


int8 READ_DATA_BYTE3(unsigned INT32 ADDRESS)
{

 unsigned int8 adsress[4];
   //Byte extraction
   adsress[0]  = (unsigned int8)((ADDRESS>>24) & 0xFF);                          // 0x _ _ 00 00 00
   adsress[1]  = (unsigned int8)((ADDRESS>>16) & 0xFF);                          // 0x 00 _ _ 00 00
   adsress[2]  = (unsigned int8)((ADDRESS>>8) & 0xFF);                           // 0x 00 00 _ _ 00
   adsress[3]  = (unsigned int8)((ADDRESS) & 0xFF);                              // 0x 00 00 00 _ _
   
 output_low(CS_PIN_2);                                                           //lower the CS PIN
 
 //////////////////////////////////////////////////////////////////
 int8 data;
 spi_xfer(SPIPORT_2,READ3_DATA_BYTES);                                            //READ DATA COMAND   (0x13)
 
 //spi_xfer(SPIPORT_2,adsress[0]);
 spi_xfer(SPIPORT_2,adsress[1]);
 spi_xfer(SPIPORT_2,adsress[2]);
 spi_xfer(SPIPORT_2,adsress[3]);
 data = spi_xfer(SPIPORT_2);
 //////////////////////////////////////////////////////////////////
 
 output_high(CS_PIN_2);//take CS PIN higher back
 while(isBusy());
 return data;
 
}
int8 READ_DATA_BYTE4(unsigned INT32 ADDRESS)
{

 unsigned int8 adsress[4];
   //Byte extraction
   adsress[0]  = (unsigned int8)((ADDRESS>>24) & 0xFF);                          // 0x _ _ 00 00 00
   adsress[1]  = (unsigned int8)((ADDRESS>>16) & 0xFF);                          // 0x 00 _ _ 00 00
   adsress[2]  = (unsigned int8)((ADDRESS>>8) & 0xFF);                           // 0x 00 00 _ _ 00
   adsress[3]  = (unsigned int8)((ADDRESS) & 0xFF);                              // 0x 00 00 00 _ _
   
 output_low(CS_PIN_3);                                                           //lower the CS PIN
 
 //////////////////////////////////////////////////////////////////
 int8 data;
 spi_xfer(SPIPORT_3,READ4_DATA_BYTES);                                            //READ DATA COMAND   (0x13)
 
 spi_xfer(SPIPORT_3,adsress[0]);
 spi_xfer(SPIPORT_3,adsress[1]);
 spi_xfer(SPIPORT_3,adsress[2]);
 spi_xfer(SPIPORT_3,adsress[3]);
 data = spi_xfer(SPIPORT_3);
 //////////////////////////////////////////////////////////////////
 
 output_high(CS_PIN_3);                                                          //take CS PIN higher back
 while(isBusy());
 return data;
 
}



INT32 WRITE_DATA_N_BYTE4(INT32 writing_address,int32 size, int8 *data) //read and send the specified data
{
   fprintf(debugPort,"Writing_address=%lX should end in %lX \r\n",writing_address,writing_address+size-1);
   int32 byteNo=0;
   int32 pageNo=0;
   int32 pageNoCounter=0;
   int8 counter;
   int16 rest = 256-(writing_address % 256);
   //fprintf(debugPort,"rest= %lu\r\n",rest);

   int8 addsress_array[4];
   
   addsress_array[0]  = (int8)((writing_address>>24) & 0xFF);      
   addsress_array[1]  = (int8)((writing_address>>16) & 0xFF);      
   addsress_array[2]  = (int8)((writing_address>>8) & 0xFF);       
   addsress_array[3]  = (int8)((writing_address) & 0xFF);          
   
   if ((writing_address % 256)!=0)
   {
   WRITE_ENABLE4();
   output_low(CS_PIN_3);                                                         //lower the CS PIN
   spi_xfer(SPIPORT_3,WRITE4_PAGE);                                               //PAGE WRITE COMAND  (0x12)
  
   spi_xfer(SPIPORT_3,addsress_array[0]);    
   spi_xfer(SPIPORT_3,addsress_array[1]);    
   spi_xfer(SPIPORT_3,addsress_array[2]);    
   spi_xfer(SPIPORT_3,addsress_array[3]);   
   while ((byteNo<rest)&&(byteNo<size)) 
   {
      spi_xfer(SPIPORT_3,data[byteNo]); 
      if (byteNo%16==0) {fprintf(debugPort,"\r\n");}
      fprintf(debugPort,"%X ",data[byteNo]);
      byteNo++;
   }
   fprintf(debugPort,"\r\n");
   //fprintf(debugPort,"Wrote the rest which is =%lu\r\n",rest);
   output_high(CS_PIN_3);                                                        //take CS PIN higher back
   delay_ms(writing_delay);  
   WRITE_DISABLE4();

   //fprintf(debugPort,"Writing address was: %lX \r\n",(writing_address));
   //fprintf(debugPort,"data size was: %ld \r\n",(size));
   writing_address=writing_address+byteNo;
   //fprintf(debugPort,"writing_address= %lX\r\n",writing_address);
   //size=size-byteNo;
   //fprintf(debugPort,"Writing address became: %lX \r\n\r\n",(writing_address));
   //fprintf(debugPort,"data size became: %ld \r\n",(size));
   }   
   
   
   //size=size-byteNo;
   pageNo=(int8)(size/256);
   //fprintf(debugPort,"number of pages: %u\r\n",(int8)pageNo);
   
   pageNoCounter=0;
   while (pageNoCounter<=pageNo) //it is <= to run it one more time
   {
      //fprintf(debugPort,"big loop...pageNoCounter=%lu \r\n ",pageNoCounter);
      addsress_array[0]  = (int8)((writing_address>>24) & 0xFF);
      addsress_array[1]  = (int8)((writing_address>>16) & 0xFF);      
      addsress_array[2]  = (int8)((writing_address>>8) & 0xFF);       
      addsress_array[3]  = (int8)((writing_address) & 0xFF);    
      
      WRITE_ENABLE4();
      output_low(CS_PIN_3);                                                         //lower the CS PIN
      spi_xfer(SPIPORT_3,WRITE4_PAGE);                                               //PAGE WRITE COMAND  (0x12)
  
      spi_xfer(SPIPORT_3,addsress_array[0]);    
      spi_xfer(SPIPORT_3,addsress_array[1]);    
      spi_xfer(SPIPORT_3,addsress_array[2]);    
      spi_xfer(SPIPORT_3,addsress_array[3]);  
      
      //byteNo=0;
      counter=0; // the number of bytes written in pageNoCounter loop
      while ((counter<256)&&(byteNo<size)) 
      {
         //fprintf(debugPort,"small loop...\r\n ");
         spi_xfer(SPIPORT_3,data[byteNo]);
         if (counter%16==0) {fprintf(debugPort,"\r\n");}
         fprintf(debugPort,"%X ",data[byteNo]);
         byteNo++;
         counter++;
      }
   fprintf(debugPort,"\r\n");
   //writing_address=writing_address+256;
   writing_address=writing_address+counter;
   //fprintf(debugPort,"counter= %lX\r\n",counter);
   //fprintf(debugPort,"Next writing_address=%lX \r\n",writing_address);
   pageNoCounter++;
   output_high(CS_PIN_3);                                                        //take CS PIN higher back
   delay_ms(writing_delay);  
   WRITE_DISABLE4();
   }
   return writing_address;
   while(isBusy());
}



INT32 WRITE_DATA_N_DUMMY_BYTE4(INT32 writing_address,int32 size) //read and send the specified data
{
   int32 byteNo=0;
   int32 pageNo=0;
   int32 pageNoCounter=0;
   int8 counter;
   int16 rest = 256-(writing_address % 256);

   int8 addsress_array[4];
   
   addsress_array[0]  = (int8)((writing_address>>24) & 0xFF);      
   addsress_array[1]  = (int8)((writing_address>>16) & 0xFF);      
   addsress_array[2]  = (int8)((writing_address>>8) & 0xFF);       
   addsress_array[3]  = (int8)((writing_address) & 0xFF);          
   
   if(showDebug) fprintf(debugPort,"Writing...\r\n ");
if ((writing_address % 256)!=0)
{
   WRITE_ENABLE4();
   output_low(CS_PIN_3);                                                         //lower the CS PIN
   spi_xfer(SPIPORT_3,WRITE4_PAGE);                                               //PAGE WRITE COMAND  (0x12)
  
   spi_xfer(SPIPORT_3,addsress_array[0]);    
   spi_xfer(SPIPORT_3,addsress_array[1]);    
   spi_xfer(SPIPORT_3,addsress_array[2]);    
   spi_xfer(SPIPORT_3,addsress_array[3]);   
   while ((byteNo<rest)&&(pageNoCounter*256+byteNo<size))
   {
      spi_xfer(SPIPORT_3,byteNo); 
      if(showDebug)
      {
         if (byteNo%16==0) {fprintf(debugPort,"\r\n");}
         fprintf(debugPort,"%X ",byteNo);
      }
      byteNo++;
   }
   output_high(CS_PIN_3);                                                        //take CS PIN higher back
   delay_ms(writing_delay);  
   WRITE_DISABLE4();

   //fprintf(debugPort,"Writing address was: %lX \r\n",(writing_address));
   //fprintf(debugPort,"data size was: %ld \r\n",(size));
   writing_address=writing_address+rest;
   //size=size-byteNo;
   //fprintf(debugPort,"Writing address became: %lX \r\n\r\n",(writing_address));
   //fprintf(debugPort,"data size became: %ld \r\n",(size));
}   
   
   pageNo=(int8)(size/256);
   //fprintf(debugPort,"number of pages: %u\r\n",(int8)pageNo);
   
   pageNoCounter=0;
   while (pageNoCounter<=pageNo)
   {
      //fprintf(debugPort,"big loop...\r\n ");
      addsress_array[0]  = (int8)((writing_address>>24) & 0xFF);
      addsress_array[1]  = (int8)((writing_address>>16) & 0xFF);      
      addsress_array[2]  = (int8)((writing_address>>8) & 0xFF);       
      addsress_array[3]  = (int8)((writing_address) & 0xFF);    
      
      WRITE_ENABLE4();
      output_low(CS_PIN_3);                                                         //lower the CS PIN
      spi_xfer(SPIPORT_3,WRITE4_PAGE);                                               //PAGE WRITE COMAND  (0x12)
  
      spi_xfer(SPIPORT_3,addsress_array[0]);    
      spi_xfer(SPIPORT_3,addsress_array[1]);    
      spi_xfer(SPIPORT_3,addsress_array[2]);    
      spi_xfer(SPIPORT_3,addsress_array[3]);  
      
      //byteNo=0;
      counter=0;
      while ((counter<256)&&(pageNoCounter*256+byteNo<size))
      {
         //fprintf(debugPort,"small loop...\r\n ");
         spi_xfer(SPIPORT_3,pageNoCounter*256+byteNo);
         if(showDebug)
         {
            if (byteNo%16==0) {fprintf(debugPort,"\r\n");}
            fprintf(debugPort,"%X ",pageNoCounter*256+byteNo);
         }
         byteNo++;
         counter++;
      }
   //writing_address=writing_address+256;
   writing_address=writing_address+counter;
   pageNoCounter++;
   output_high(CS_PIN_3);                                                        //take CS PIN higher back
   delay_ms(writing_delay);  
   WRITE_DISABLE4();
   }
   return writing_address;
}


INT32 WRITE_DATA_N_DUMMY_BYTE4_show(INT32 writing_address,int32 size) 
{
   fprintf(debugPort,"Writing_address=%lX should end in %lX \r\n",writing_address,writing_address+size-1);
   int32 byteNo=0;
   int32 pageNo=0;
   int32 pageNoCounter=0;
   int16 counter;
   int16 rest = 256-(writing_address % 256);
   fprintf(debugPort,"rest= %lu\r\n",rest);

   int8 addsress_array[4];
   
   addsress_array[0]  = (int8)((writing_address>>24) & 0xFF);      
   addsress_array[1]  = (int8)((writing_address>>16) & 0xFF);      
   addsress_array[2]  = (int8)((writing_address>>8) & 0xFF);       
   addsress_array[3]  = (int8)((writing_address) & 0xFF);          
   
   if ((writing_address % 256)!=0)
   {
      while ((byteNo<rest)&&(pageNoCounter*256+byteNo<size)) // do we need (pageNoCounter*256+byteNo<size) ?????????
      {
         fprintf(debugPort,"\r\n byteNo=%X ",byteNo);
         byteNo++;
      }
      fprintf(debugPort,"\r\n");
      fprintf(debugPort,"Wrote the rest which is =%lu\r\n",rest);

   
      fprintf(debugPort,"Writing address was: %lX \r\n",(writing_address));
      //fprintf(debugPort,"data size was: %ld \r\n",(size));
      writing_address=writing_address+byteNo;
      fprintf(debugPort,"writing_address became= %lX\r\n",writing_address);
      //size=size-byteNo;
      //fprintf(debugPort,"Writing address became: %lX \r\n\r\n",(writing_address));
      //fprintf(debugPort,"data size became: %ld \r\n",(size));
   }   
   
   
   //size=size-byteNo;
   pageNo=(int8)(size/256);
   fprintf(debugPort,"number of pages: %u\r\n",(int8)pageNo);
   
   pageNoCounter=0;
   fprintf(debugPort,"pageNoCounter= %lX ",pageNoCounter);
   while (pageNoCounter<=pageNo) //it is <= to run it one more time
   {
      fprintf(debugPort,"big loop...pageNoCounter=%lu \r\n ",pageNoCounter);
      addsress_array[0]  = (int8)((writing_address>>24) & 0xFF);
      addsress_array[1]  = (int8)((writing_address>>16) & 0xFF);      
      addsress_array[2]  = (int8)((writing_address>>8) & 0xFF);       
      addsress_array[3]  = (int8)((writing_address) & 0xFF);    
      
      //byteNo=0;
      counter=0; // the number of bytes written in pageNoCounter loop
      while ((counter<256)&&(byteNo<size)) // do we need pageNoCounter*256????
      {
         fprintf(debugPort,"\r\npageNoCounter=%lX pageNoCounter*256+byteNo=%lX byteNo=%lX counter=%lX",pageNoCounter,pageNoCounter*256+byteNo,byteNo,counter);
         //fprintf(debugPort,"\r\n                   byteNo=%lX counter=%lX",byteNo,counter);
         byteNo++;
         counter++;
      }
   fprintf(debugPort,"\r\n");
   //writing_address=writing_address+256;
      fprintf(debugPort,"Writing address was: %lX \r\n",(writing_address));
   writing_address=writing_address+counter;
      fprintf(debugPort,"writing_address became= %lX\r\n",writing_address);
   //fprintf(debugPort,"counter= %lX\r\n",counter);
   //fprintf(debugPort,"Next writing_address=%lX \r\n",writing_address);
   pageNoCounter++;

   }
   return writing_address;

}
