

void prnt()
{
   while(UART1_available())
   {
      fputc(UART1_READ(), debug);
   }
}

//*************************** TNC Booting time Settings ***********************
//
//*****************************************************************************

void boot_Kiss_Mode(int16 t1)
{
   UART1_flush();
   fprintf(debug, "kiss command to TNC\n\r");
   fprintf(TNC, "kiss\n");
   delay_ms(t1);
   prnt();
}

//******************************** Trying to go to Monitor Mode ***************
//
//*****************************************************************************

void TRYING_TO_PUT_DIGIPETER_TO_MONITOR_MODE(int noftimes)
{
   char flagtnc = 0;
   for( int i = 0; i<noftimes; i++)
   {
      fprintf(debug, "TRY = %X\n\r", i+1);
      UART1_flush();
      fprintf(TNC, "monitor\n");
      delay_ms(1000);
      
      for(int j = 0; j<100 ;j++)
      {
         TNC_TO_PIC_ARRAY[j] = UART1_Read();
      }
      
      
      for(int m = 0; m<30; m++)
      {
         if( TNC_TO_PIC_ARRAY[m] == 0x55)
         {
            if( TNC_TO_PIC_ARRAY[m+29] == 0x65)
            {
               flagtnc = 0x69;
               fprintf(debug, "TNC is now in Monitor mode\n\r");
               break;
            }
         }
      }
      
      if( flagtnc == 0x69 ) break;
      delay_ms(2000);
   }
}

//******************Check KISS Frame Data and Save it In Flash*****************
//                        Digipeater Mode and S&F Mode
//*****************************************************************************
unsigned int32 MLC = 0;
unsigned int16 Sec_count = 0;
unsigned int16 header1 = 0;
unsigned int16 footer1 = 0;
unsigned int16 header2 = 0;
unsigned int16 footer2 = 0;

//unsigned int32 C_ADD;
unsigned int32 LBL_D = 0; 
unsigned int32 LBL_S = 0; 

void CHECK_KISS_FRAME_DATA_AND_SAVE(unsigned int16 num)
{
   MLC++;
   
   if(MLC >=num)
   {
      Sec_count++;
      fprintf(debug,"%Ld\n\r",Sec_count);
      MLC = 0;
      if(Sec_count>=200) Sec_count = 0;
      
      for(int16 i = 0; i<490; i++)
      {
         TNC_TO_PIC_ARRAY[i] = UART1_READ();
      }
      
      for(int i = 0; i<5; i++)
      {
         if(TNC_TO_PIC_ARRAY[i] == 0xC0)
         {
            header1 = i;
            
            for(int j = header1+1; j<120; j++)       // 
            {
               if(TNC_TO_PIC_ARRAY[j] == 0xC0)
               {
                  footer1 = j   ;
                  header2 = j+1 ;
                  break;
               }
            }
            
            for(int j = header2+1; j<250; j++)
            {
               if(TNC_TO_PIC_ARRAY[j] == 0xC0)
               {
                  footer2 = j   ;
                  break;
               }
            }
            
            if( TNC_TO_PIC_ARRAY[header1] == 0xC0 && TNC_TO_PIC_ARRAY[footer1] == 0xC0 )
            {
               if( Mode_Flag == 0x0D ) // Data Save IN DP Mode
               { 
                  unsigned int32 Location = make32( BYTE_READ(NEXT_B_ADD_DP+1), BYTE_READ(NEXT_B_ADD_DP+2), BYTE_READ(NEXT_B_ADD_DP+3), BYTE_READ(NEXT_B_ADD_DP+4) );
                  fprintf(Debug, "Location val = %LX\n\r ",Location );
                  if( location == 0xFFFFFFFF ) LBL_D = 0 ;
                  else LBL_D = location;
                  fprintf(Debug, "This packet start byte location = %Lu\n\r ",LBL_D );
                  
                  for( unsigned int32 i = 0; i<=footer1; i++ )
                  {
                     BYTE_WRITE( (DGP_LOCATION + i + LBL_D) , TNC_TO_PIC_ARRAY[i] );
                  }
                  
                  fprintf(debug,"Received packet Digipeter mode --> ");
                  {
                     for( unsigned int32 i = 0; i<=footer1; i++ )
                     {
                        fprintf(Debug, "%X ", BYTE_READ( (DGP_LOCATION + i + LBL_D)) );
                     }
                  }
                  fprintf(debug,"\n\r");
                  
                  LBL_D =  LBL_D+ footer1 + 1; // next place
                  
                  SECTOR_ERASE(NEXT_B_ADD_DP, 4);
                  BYTE_WRITE( NEXT_B_ADD_DP+1 , (char)( (LBL_D & 0xFF000000)>>24 ) );
                  BYTE_WRITE( NEXT_B_ADD_DP+2 , (char)( (LBL_D & 0x00FF0000)>>16 ) );
                  BYTE_WRITE( NEXT_B_ADD_DP+3 , (char)( (LBL_D & 0x0000FF00)>>8  ) );
                  BYTE_WRITE( NEXT_B_ADD_DP+4 , (char)( (LBL_D & 0x000000FF)     ) );
               }  
                      
               
               if( Mode_Flag == 0x1E )  // Data Save In S&F Mode
               { 
                  unsigned int32 Location = make32( BYTE_READ(NEXT_B_ADD_SF+1), BYTE_READ(NEXT_B_ADD_SF+2), BYTE_READ(NEXT_B_ADD_SF+3), BYTE_READ(NEXT_B_ADD_SF+4) );
                  fprintf(Debug, "Location val = %LX\n\r ",Location );
                  if( location == 0xFFFFFFFF ) LBL_S = 0 ;
                  else LBL_S = location;
                  fprintf(Debug, "This packet start byte location = %Lu\n\r ",LBL_S );
                  
                  for( unsigned int32 i = 0; i<=footer1; i++ )
                  {
                     BYTE_WRITE( (SNF_LOCATION + i + LBL_S) , TNC_TO_PIC_ARRAY[i] );
                  }
                  
                  fprintf(debug,"Received packet S&F mode --> ");
                  {
                     for( unsigned int32 i = 0; i<=footer1; i++ )
                     {
                        fprintf(Debug, "%X ", BYTE_READ( (SNF_LOCATION + i + LBL_S)) );
                     }
                  }
                  fprintf(debug,"\n\r");
                  
                  LBL_D =  LBL_S+ footer1 + 1; // next place
                  
                  SECTOR_ERASE(NEXT_B_ADD_SF, 4);
                  BYTE_WRITE( NEXT_B_ADD_SF+1 , (char)( (LBL_D & 0xFF000000)>>24 ) );
                  BYTE_WRITE( NEXT_B_ADD_SF+2 , (char)( (LBL_D & 0x00FF0000)>>16 ) );
                  BYTE_WRITE( NEXT_B_ADD_SF+3 , (char)( (LBL_D & 0x0000FF00)>>8  ) );
                  BYTE_WRITE( NEXT_B_ADD_SF+4, (char)( (LBL_D & 0x000000FF)     ) );
               }  
               
            }
            
            if( TNC_TO_PIC_ARRAY[header1] == 0xC0 && TNC_TO_PIC_ARRAY[footer1] == 0xC0 )
            {
               fprintf(debug,"Deigipeted packet --> ");
               for(int16 i = header2; i<=footer2; i++)
               {
                  fprintf(Debug,"%X ", TNC_TO_PIC_ARRAY[i]);
               }
               fprintf(debug,"\n\r");
            }
            break;
         } 
      }
      

   }
}
   









   
//!   fprintf(debug, "config\n\r");
//!   fprintf(TNC, "config\n");
//!   delay_ms(2000);
//!   prnt();
//!   
//!   fprintf(debug, "print\n\r");
//!   fprintf(TNC, "print\n");
//!   
  
  
 



//*************************************Digi ON*********************************
//
//*****************************************************************************

void Digipeter_ON(int16 t=1000)
{
   if (MBOSS_TO_APRS_ARRAY[2]== 0x00)
   {
      // Ack to mboss
      CLEAR_DATA_ARRAY(APRS_TO_MBOSS_ARRAY,10);
      APRS_TO_MBOSS_ARRAY[0] = 0x00;
      APRS_TO_MBOSS_ARRAY[1] = 0x00;
      APRS_TO_MBOSS_ARRAY[2] = 0x00;
      APRS_TO_MBOSS_ARRAY[3] = 0x00;
      APRS_TO_MBOSS_ARRAY[8] = 0x00;
      for(int i = 0; i<9; i++) fputc(APRS_TO_MBOSS_ARRAY[i], mboss);
      
      fprintf(debug, "Digipeater_ON command\n\r ");
      
      Delay_ms(4000);
      
      Mode_Flag = 0x00;   
      
      CLEAR_DATA_ARRAY(APRS_TO_MBOSS_ARRAY,10);
      APRS_TO_MBOSS_ARRAY[0] = 0x00;
      APRS_TO_MBOSS_ARRAY[1] = 0x00;
      APRS_TO_MBOSS_ARRAY[2] = 0x00;
      APRS_TO_MBOSS_ARRAY[3] = 0x00;
      APRS_TO_MBOSS_ARRAY[8] = 0x00;
      for(int i = 0; i<9; i++) fputc(APRS_TO_MBOSS_ARRAY[i], mboss);
      
       fprintf(debug, "Digipeater_ON command_ Exicuted\n\r ");
   }

}

//**********************************Digi OFF***********************************
//
//*****************************************************************************

void Digipeter_OFF(int16 t=1000)
{
   if( MBOSS_TO_APRS_ARRAY[2]== 0x00)
   {
      // Ack to mboss
      CLEAR_DATA_ARRAY(APRS_TO_MBOSS_ARRAY,10);
      APRS_TO_MBOSS_ARRAY[0] = 0x00;
      APRS_TO_MBOSS_ARRAY[1] = 0x00;
      APRS_TO_MBOSS_ARRAY[2] = 0x00;
      APRS_TO_MBOSS_ARRAY[3] = 0x00;
      APRS_TO_MBOSS_ARRAY[8] = 0x00;
      for(int i = 0; i<9; i++) fputc(APRS_TO_MBOSS_ARRAY[i], mboss);
      
      
      Delay_ms(4000);
      Mode_Flag = 0x00;
      
      
      CLEAR_DATA_ARRAY(APRS_TO_MBOSS_ARRAY,10);
      APRS_TO_MBOSS_ARRAY[0] = 0x00;
      APRS_TO_MBOSS_ARRAY[1] = 0x00;
      APRS_TO_MBOSS_ARRAY[2] = 0x00;
      APRS_TO_MBOSS_ARRAY[3] = 0x00;
      APRS_TO_MBOSS_ARRAY[8] = 0x00;
      for(int i = 0; i<9; i++) fputc(APRS_TO_MBOSS_ARRAY[i], mboss);
      
      fprintf(debug, "Digipeater_OFF command_Exicuted\n\r ");
   }
}


//************************************ S&F ON *********************************
//
//*****************************************************************************

void STORE_AND_FORWARD_ON(int16 t =2000)
{
   if( MBOSS_TO_APRS_ARRAY[2]== 0x00)
   {
      // Ack to mboss
      CLEAR_DATA_ARRAY(APRS_TO_MBOSS_ARRAY,10);
      APRS_TO_MBOSS_ARRAY[0] = 0x00;
      APRS_TO_MBOSS_ARRAY[1] = 0x00;
      APRS_TO_MBOSS_ARRAY[2] = 0x00;
      APRS_TO_MBOSS_ARRAY[3] = 0x00;
      APRS_TO_MBOSS_ARRAY[8] = 0x00;
      for(int i = 0; i<9; i++) fputc(APRS_TO_MBOSS_ARRAY[i], mboss);
      
      
      
      fprintf(debug, "Store and forward on command\n\r ");
      
      Delay_ms(4000);
      
      Mode_Flag = 0x00;
      
      CLEAR_DATA_ARRAY(APRS_TO_MBOSS_ARRAY,10);
      APRS_TO_MBOSS_ARRAY[0] = 0x00;
      APRS_TO_MBOSS_ARRAY[1] = 0x00;
      APRS_TO_MBOSS_ARRAY[2] = 0x00;
      APRS_TO_MBOSS_ARRAY[3] = 0x00;
      APRS_TO_MBOSS_ARRAY[8] = 0x00;
      for(int i = 0; i<9; i++) fputc(APRS_TO_MBOSS_ARRAY[i], mboss);
      
      fprintf(debug, "Store and forward on command_Exicuted\n\r ");
   }
}


//************************ DATA Transfer In SNF MODE ***************************
//
//*****************************************************************************


Void DATA_TRANSFER_IN_SNF_MODE()
{
   if( MBOSS_TO_APRS_ARRAY[1] == 0x11)
   {
      fprintf(debug, "DATA_TRANSFER_IN_SNF_MODE\n\r ");
      
      unsigned int32 Nofpackets = (unsigned int32)MBOSS_TO_APRS_ARRAY[2];
      
      unsigned int32 ADD = make32(0,0,MBOSS_TO_APRS_ARRAY[5],MBOSS_TO_APRS_ARRAY[6]);
      
      for(unsigned int32 i = 0; i< Nofpackets; i++)
      {  
         Delay_ms(50);
         for(unsigned int32 j = 0; j<100; j++)
         {
            APRS_TO_MBOSS_ARRAY[j] = BYTE_READ(SNF_LOCATION + ADD + j + 100*i);
            
            fputc(APRS_TO_MBOSS_ARRAY[j], mboss);
            fprintf(debug,"%X",APRS_TO_MBOSS_ARRAY[j]);
            delay_ms(1);
         }  
         fprintf(debug, "Packet number = %Lu\n\r ", i);
      }
   }
}

//************************ DATA Transfer In DP MODE ***************************
//
//*****************************************************************************


Void DATA_TRANSFER_IN_DP_MODE()
{
   if( MBOSS_TO_APRS_ARRAY[1] == 0x01)
   {
      fprintf(debug, "DATA_TRANSFER_IN_DP_MODE\n\r ");
      
      char Nofpackets = MBOSS_TO_APRS_ARRAY[2];
      
      unsigned int32 ADD = make32(0,0,MBOSS_TO_APRS_ARRAY[5],MBOSS_TO_APRS_ARRAY[6]);
      
      for(unsigned int32 i = 0; i< Nofpackets; i++)
      {  
         Delay_ms(50);
         for(unsigned int32 j = 0; j<100; j++)
         {
            APRS_TO_MBOSS_ARRAY[j] = BYTE_READ(DGP_LOCATION + ADD + j + 100*i);
            
            fputc(APRS_TO_MBOSS_ARRAY[j], mboss);
            delay_ms(1);
         }          
      }
   }
}


//***************************DATA Transfer IN S&F MODE*************************
//
//*****************************************************************************


//!Void DATA_TRANSFER_IN_SNF_MODE()
//!{
//!   if( MBOSS_TO_APRS_ARRAY[1]== 0x11)
//!   {
//!      fprintf(debug, "DATA_TRANSFER_IN_SNF_MODE\n\r ");
//!      
//!      unsigned int Nofpackets = make16(MBOSS_TO_APRS_ARRAY[6],MBOSS_TO_APRS_ARRAY[7]);
//!      
//!      for(unsigned int32 i = 0; i< Nofpackets; i++)
//!      {
//!         APRS_TO_MBOSS_ARRAY[0] = 0xE0;
//!         APRS_TO_MBOSS_ARRAY[1] = 0x01;
//!         APRS_TO_MBOSS_ARRAY[2] = (unsigned int8)(i+1) ;
//!         
//!         for(unsigned int32 j = 0; j<81; j++)
//!         {
//!            APRS_TO_MBOSS_ARRAY[3+j] = BYTE_READ(SNF_LOCATION + j + 81*i);
//!         }
//!         APRS_TO_MBOSS_ARRAY[84] = 0xED ;
//!         
//!         for(int i=0; i<=84; i++) fputc(APRS_TO_MBOSS_ARRAY[i], mboss);
//!         
//!         Delay_ms(250);
//!      }
//!   }
//!}




//****************************Data Sector Erase********************************
//
//*****************************************************************************

Void DATA_SECTORS_ERASE()
{
   if( MBOSS_TO_APRS_ARRAY[2]== 0x69)
   {
  //     Ack to mboss
      CLEAR_DATA_ARRAY(APRS_TO_MBOSS_ARRAY,10);
      APRS_TO_MBOSS_ARRAY[0] = 0xE0;
      APRS_TO_MBOSS_ARRAY[1] = 0x69;
      APRS_TO_MBOSS_ARRAY[2] = 0xA0;
      APRS_TO_MBOSS_ARRAY[8] = 0xED;
      for(int i = 0; i<9; i++) fputc(APRS_TO_MBOSS_ARRAY[i], mboss);
      
 //     fprintf(debug, "DATA_SECTORS_ERASE\n\r ");
      
      SECTOR_ERASE(SNF_LOCATION,64, 1500);
      SECTOR_ERASE(DGP_LOCATION,64, 1500);
      SECTOR_ERASE(NEXT_B_ADD_DP,64, 1500);
      SECTOR_ERASE(NEXT_B_ADD_SF,64, 1500);
      fprintf(debug, "Sectors erased\n\r ");
      
      
      CLEAR_DATA_ARRAY(APRS_TO_MBOSS_ARRAY,10);
      APRS_TO_MBOSS_ARRAY[0] = 0xE0;
      APRS_TO_MBOSS_ARRAY[1] = 0x69;
      APRS_TO_MBOSS_ARRAY[2] = 0xA1;
      APRS_TO_MBOSS_ARRAY[8] = 0xED;
      for(int i = 0; i<9; i++) fputc(APRS_TO_MBOSS_ARRAY[i], mboss);
      
       fprintf(debug, "Data_Secters_Erase_ Exicuted\n\r ");
   }
}
//*****************************************************************************
// *************************** Manual Setting *********************************
//*****************************************************************************


void Manual_Settings()
{
   fprintf(debug, "command -->> ");
   for(int i = 0; i<UART2_Byte_Counter; i++)
   {
      fputc(UART2_Buffer[i], debug);
   }
   
   
      fputc(0x0A, debug); // 0x0A Represent Enter in ACII or  \n
      fputc(0x0D, debug); // 0x0D Represent Carriage Return in Ascii or  \r
      
      
   for(int i = 0; i<UART2_Byte_Counter-1; i++)
   {
      fputc(UART2_Buffer[i], TNC);
      UART2_Buffer[i] = 0;
   }
   fputc(0x0A, TNC);
   
   UART2_Byte_Counter = 0;
   uartflag = 0 ;
}

