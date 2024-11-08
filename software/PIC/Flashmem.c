


#define MOSI PIN_A0
#define MISO PIN_A1
#define SCK  PIN_A3

#define CSS  PIN_A2
//#use spi(SPI1,MASTER,  BAUD=100000, BITS=8, STREAM=CFM, MODE=0)
#use spi(MASTER, CLK=SCK, DI=MISO, DO=MOSI, BAUD=400000, BITS=8, STREAM=CFM, MODE=0, force_SW)


void WRITE_ENABLE()
{
   Output_Low(CSS);
   spi_xfer(CFM,0x06);                // Send 0x06 for write enable (Write enable Command)
   Output_High(CSS);
   return;
}

void SECTOR_ERASE(unsigned int32 sector_address, char Sector_size, int16 timedelay =1000)
{
   unsigned int8 adsress[4];
   
   adsress[0]  = (unsigned int8)((sector_address>>24) & 0xFF);   // 0x _ _ 00 00 00
   adsress[1]  = (unsigned int8)((sector_address>>16) & 0xFF);   // 0x 00 _ _ 00 00
   adsress[2]  = (unsigned int8)((sector_address>>8) & 0xFF);    // 0x 00 00 _ _ 00
   adsress[3]  = (unsigned int8)((sector_address) & 0xFF);       // 0x 00 00 00 _ _
   
   WRITE_ENABLE();
   
   Output_Low(CSS);             //lower the CS PIN

   ///////////////////////////////////////////////////////////////////
   if( Sector_size == 4  ) spi_xfer(CFM,0x21);                    // 4KB Sector erase
   if( Sector_size == 32 ) spi_xfer(CFM,0x5C);                    // 32KB Sector erase
   if( Sector_size == 64 ) spi_xfer(CFM,0xDC);                    // 64KB Sector erase
   
   spi_xfer(CFM,adsress[0]);   
   spi_xfer(CFM,adsress[1]);    
   spi_xfer(CFM,adsress[2]);    
   spi_xfer(CFM,adsress[3]);
   //////////////////////////////////////////////////////////////////
 
   Output_High(CSS);;           //take CS PIN higher back
   delay_ms(timedelay);  
   return;
}

void BYTE_WRITE(unsigned int32 byte_address, int8 data)
{
   unsigned int8 adsress[4];
   
   //Byte extraction
   adsress[0]  = (unsigned int8)((byte_address>>24) & 0xFF);   // 0x _ _ 00 00 00
   adsress[1]  = (unsigned int8)((byte_address>>16) & 0xFF);   // 0x 00 _ _ 00 00
   adsress[2]  = (unsigned int8)((byte_address>>8) & 0xFF);    // 0x 00 00 _ _ 00
   adsress[3]  = (unsigned int8)((byte_address) & 0xFF);       // 0x 00 00 00 _ _
   
   WRITE_ENABLE();
   
   Output_Low(CSS);           //lower the CS PIN
 
   
   ////////////////////////////////////////////////////////////////
   spi_xfer(CFM,0x12); //Byte WRITE COMAND  (0x12)
   spi_xfer(CFM,adsress[0]);    
   spi_xfer(CFM,adsress[1]);    
   spi_xfer(CFM,adsress[2]);    
   spi_xfer(CFM,adsress[3]);
   
   spi_xfer(CFM,data); 
   ////////////////////////////////////////////////////////////////
   
   Output_High(CSS);         //take CS PIN higher back

   return;
}

unsigned int8 BYTE_READ(unsigned INT32 ADDRESS)
{

   unsigned int8 adsress[4];
   //Byte extraction
   adsress[0]  = (unsigned int8)((ADDRESS>>24) & 0xFF);   // 0x _ _ 00 00 00
   adsress[1]  = (unsigned int8)((ADDRESS>>16) & 0xFF);   // 0x 00 _ _ 00 00
   adsress[2]  = (unsigned int8)((ADDRESS>>8) & 0xFF);    // 0x 00 00 _ _ 00
   adsress[3]  = (unsigned int8)((ADDRESS) & 0xFF);       // 0x 00 00 00 _ _
   
   Output_Low(CSS);                //lower the CS PIN
 
    //////////////////////////////////////////////////////////////////
   int8 data;
   spi_xfer(CFM,0X13);  //READ DATA COMAND   (0x13)
   spi_xfer(CFM,adsress[0]);
   spi_xfer(CFM,adsress[1]);
   spi_xfer(CFM,adsress[2]);
   spi_xfer(CFM,adsress[3]);
 
   data = spi_xfer(CFM);
   //////////////////////////////////////////////////////////////////
 
   Output_High(CSS);               //take CS PIN higher back
   return data;
}
