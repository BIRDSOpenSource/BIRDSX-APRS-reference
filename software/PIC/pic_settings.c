char MBOSS_TO_APRS_ARRAY[50];
char APRS_TO_MBOSS_ARRAY[100];

char TNC_TO_PIC_ARRAY[500];

unsigned int8  Mode_Flag = 0x0E ;
unsigned int32 SNF_LOCATION = 0x001A0000 ;
unsigned int32 DGP_LOCATION = 0x001B0000 ;


// **********************UART_1 Setting (TNC_PORT) ****************************
//
//
//*****************************************************************************
#pin_select U1TX=PIN_B2
#pin_select U1RX=PIN_C7
#use rs232(UART1,baud=19200, parity=N, bits=8, stream= TNC,errors)

unsigned int8 UART1_Buffer[210];
unsigned int8 UART1_Byte_Counter = 0;
unsigned int8 UART1_Overflow = 0;
unsigned int8 UART1_Read_Byte_counter = 0;
unsigned int8 UART1_Temp_byte = 0;

#INT_RDA
Void SERIAL_ISR1()    // MAIN PIC uart1 interupt loop
{
   if( UART1_Byte_Counter < 200 )
   {
      UART1_Buffer[UART1_Byte_Counter] = fgetc(TNC);// TNC PORT Data Array, Load to the UART1 Buffer Array
      UART1_Byte_Counter++;                         // finally UART1_Byte_Counter=199 value
   }
   
   else UART1_Overflow = fgetc(TNC);
}

unsigned int8 UART1_Available()
{
   return UART1_Byte_Counter ;  // UART1_Byte_Counter = 199
}

unsigned int8 UART1_Read()
{
   if (UART1_Byte_Counter>0)
   {    
      UART1_Temp_byte = UART1_Buffer[UART1_Read_Byte_counter];
      
      UART1_Byte_Counter--;
      UART1_Read_Byte_counter++;
      if(UART1_Byte_Counter == 0) UART1_Read_Byte_counter = 0;
      return UART1_Temp_byte; 
   }
   
   if (UART1_Byte_Counter == 0)
   { 
      UART1_Read_Byte_counter = 0;
      UART1_Temp_byte = 0x00;
      return UART1_Temp_byte; 
   }
 
}
void UART1_flush()
{
   while( UART1_Available() )  UART1_Read();
}

// **********************UART_2 Setting (Debug_Port) **************************
//
//
//*****************************************************************************


unsigned int8  UART2_Buffer[500];
unsigned int16 UART2_Byte_Counter = 0;
unsigned int8  UART2_Overflow = 0;
unsigned int8  UART2_Read_Byte_counter = 0;
unsigned int8  UART2_Temp_byte = 0;


#use rs232(baud=19200, xmit = PIN_B6, rcv = PIN_B7, parity=N,  bits=8, stream= debug, errors)

#INT_RDA2
Void SERIAL_ISR2()     // MAIN PIC uart2 interupt loop
{
   if( UART2_Byte_Counter < 500 )
   {
      UART2_Buffer[UART2_Byte_Counter] = fgetc(debug);
      UART2_Byte_Counter++;
   }
   
   else UART2_Overflow = fgetc(debug);
}

//****************** UART 2 Available******************
//*****************************************************

unsigned int16 UART2_Available()
{
   if (UART2_Byte_Counter > 0) 
   {
    return UART2_Byte_Counter ;
   }
  
   else return 0;
}

//****************** UART 2 Read **********************
//*****************************************************

unsigned int8 UART2_Read()
{
   if (UART2_Byte_Counter>0)
   {    
      UART2_Temp_byte = UART2_Buffer[UART2_Read_Byte_counter];
      
      UART2_Byte_Counter--;
      UART2_Read_Byte_counter++;
      if(UART2_Byte_Counter == 0) UART2_Read_Byte_counter = 0;
      return UART2_Temp_byte; 
   }
   
   if (UART2_Byte_Counter == 0)
   { 
      UART2_Read_Byte_counter = 0;
      UART2_Temp_byte = 0x00;
      return UART2_Temp_byte; 
   }
 
}



//*********************************************************************************
// Check MBOSS and debug ports command
// debug prot use for just check the command and MBOSS is the functional prot too.
//*********************************************************************************

char DEBUG_CMD[50];
void prnt();




void TNC_MANUAL_SETTINGS_COMMAND()
{
   if( UART2_Available() )
   {
      delay_ms(30);
      int _size = UART2_Available();
      for(int i = 0; i<51; i++)
      {
         DEBUG_CMD[i] = UART2_Read();
      }
      
      fprintf(debug, "Received CMD - >> ");

      for(int i=0; i<_size; i++)
      {
         fputc(DEBUG_CMD[i], debug);
      }
      fprintf(debug,"\n\r");
      
      if( DEBUG_CMD[0] == 0x69 && DEBUG_CMD[1] == 0x69 && DEBUG_CMD[2] == 0x69 ) // cheack the i i i inputs and jump to flag 0xDD place
      {
         UF = 0xDD ;
         fprintf(debug, "UF Flag = %X\n\r", UF);
      }   
      
      if( DEBUG_CMD[0] == 0x69 && DEBUG_CMD[1] == 0x69 && DEBUG_CMD[2] == 0x6A )// cheack the i i j inputs and jump to flag 0xCC place
      {
         UF = 0xCC ;
         fprintf(debug, "UF Flag = %X\n\r", UF);
      }    
      
      if( DEBUG_CMD[0] != 0x69 )
      {
         for(int i=0; i<_size; i++)
         {
            fputc(DEBUG_CMD[i], TNC);
         }
         delay_ms(30);
         fputc(0x0A, TNC);
         //delay_ms(1000);
      }
   }
}


// ********************check UART DATA Incomming from MBOSS *******************
//
//*****************************************************************************


void CHECK_UART_INCOMING_FROM_MBOSS(int Len = 9 )
{
   if( kbhit(MBOSS) )
   {
     int c = 0;
     MBOSS_TO_APRS_ARRAY[c] = fgetc(MBOSS); 
     c++;
      
      for(unsigned int32 i = 0; i<10000; i++)
      {
         if( kbhit(MBOSS) ) 
         {
           MBOSS_TO_APRS_ARRAY[c] = fgetc(MBOSS);
           c++;
           if( c >=Len) break ;
         }
      }
   }
}



void CLEAR_DATA_ARRAY(char array[],int size)
{
   for(int i=0; i<size; i++)
   {
      array[i]=0;
   }
}
