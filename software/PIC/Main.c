//***************************************************************************************
/*
                   

*/
//****************************************************************************************

#include <18F67J94.h>
#fuses NOWDT
#use delay(internal = 16MHz)
#use rs232(baud=9600, parity=N, xmit=PIN_E1,rcv=pin_E0,bits=8, stream= mboss,force_sw)
char UF = 0xCC;
unsigned int32 NEXT_B_ADD_DP = 0x00020000;
unsigned int32 NEXT_B_ADD_SF = 0x00030000;

#include <pic_settings.c>
#include <Flashmem.c>

char uartflag = 0;
#include <APRS Settings.c>
#include <Mode_Selection.c>



//********************************* Basic Setting ********************************
//
//********************************************************************************
char C_6;

void Setting()
{
   C_6 = input(PIN_C6);
   
   enable_interrupts(int_rda);
   enable_interrupts(int_rda2);
   enable_interrupts(GLOBAL);
   //delay_ms(3000);
   
   //TRYING_TO_PUT_DIGIPETER_TO_MONITOR_MODE(2);
   //boot_Kiss_Mode(1000);
   // TNC_Settings(2000)
   
   if( BYTE_READ(0x000AFFFF) != 0xFF ) SECTOR_ERASE(SNF_LOCATION,64);
   if( BYTE_READ(0x000BFFFF) != 0xFF ) SECTOR_ERASE(DGP_LOCATION,64);
}



void main()
{
   //delay_ms(2000);
   Setting();
   fprintf(debug, "PIC Booting \n\r"); 
   
   while(true)
   {  
      CHECK_UART_INCOMING_FROM_MBOSS();          // Function wrote in PIC Seeting.c 
      TNC_MANUAL_SETTINGS_COMMAND();             // Function wrote in PIC Seeting.c
      
      if( UF == 0xCC ) CHECK_KISS_FRAME_DATA_AND_SAVE(50000);
      
      if( UF == 0xDD )
      {
         while( UART1_Available() )
         {
            fputc(UART1_Read(), debug );
         }
      }
      
      if( (MBOSS_TO_APRS_ARRAY[0]==0xE0) && (MBOSS_TO_APRS_ARRAY[8]==0xED) ) // Frame Identification 
      {  
         //___________________________________________________________
         fprintf(debug, "CMD from Mission BOSS --> "); 
         for(int i = 0; i<9; i++)
         {
            fprintf(debug,"%X ", MBOSS_TO_APRS_ARRAY[i]);
         }
         fprintf(debug, "\n\r" );
         //___________________________________________________________
           
         Digipeter_ON()             ;                     // 0x
         Digipeter_OFF()            ;                     // 0x
         STORE_AND_FORWARD_ON()     ;                     // 0x
         DATA_TRANSFER_IN_DP_MODE() ;                     // 0x
         DATA_TRANSFER_IN_SNF_MODE();                     // 0x
         DATA_SECTORS_ERASE()       ;                     // 0x  //
         Beacon_ON()                ;                     // 0x
         Beacon_OFF()               ;                     // 0x//
         
         CLEAR_DATA_ARRAY(MBOSS_TO_APRS_ARRAY, 50); 
      }      
   }
}
