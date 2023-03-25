/**
ST7580.ino
Author: Jarvis Hill (hilljarvis@gmail.com)
Purpose: Example showing how to send a command to another JARViE PLM device (Demo Board, Shield, Shahara, etc.) and wait for a response
Date:16-MAR-2023
References: 
            -ST7580 datasheet
            -ST7580 UM9XXX User Manual
Releases: 
            a.0 - 1st release ever
**/


/*INCLUDES*/


/*GLOBALS*/
#define STX 0x02                        //Local frame header/delimiter  


//MIB Objects
#define Modem_configuration     0x00
#define PHY_configuration       0x01
#define SS_key                  0x02
#define Last_data_indication    0x04
#define Last TX confirm         0x05
#define PHY_Data                0x06
#define DL_Data                 0x07
#define SS_Data                 0x08
#define Host_interface_timeout  0x09
#define MIB_Object_FW           0x0A
        
//Request Commmand Codes
#define BIO_ResetRequest        0x3C
#define MIB_WriteRequest        0x08
#define MIB_ReadRequest         0x0C
#define MIB_EraseRequest        0x10
#define PingRequest             0x2C  
#define PHY_DataRequest         0x24
#define DL_DataRequest          0x50
#define SS_DataRequest          0x54

//Confirm Command Codes
#define BIO_ResetConfirm        0x3D
#define MIB_WriteConfirm        0x09
#define MIB_ReadConfirm         0x0D  
#define MIB_EraseConfirm        0x11
#define PingConfirm             0x2D
#define PHY_DataConfirm         0x25
#define DL_DataConfirm          0x51
#define SS_DataConfirm          0x55

//Error Command Codes 
#define BIO_ResetError          0x3F
#define MIB_WriteError          0x0B
#define MIB_ReadError           0x0F
#define MIB_EraseError          0x13
#define PHY_DataError           0x27
#define DL_DataError            0x53
#define SS_DataError            0x57
#define CMD_SyntaxError         0x36

//Indication Command Codes
#define BIO_ResetIndication     0x3E
#define PHY_DataIndication      0x26
#define DL_DataIndication       0x52
#define DL_SnifferIndication    0x5A
#define SS_DataIndication       0x56
#define SS_SnifferIndication    0x5E

//FLAGS
bool STATUS_FLAG = false; 


/*PROTOTYPES*/
void init_ST7580_io(){

  //Hold device in reset (active LOW)
  DDRD |= (1 << DDD7);
  PORTD &= ~(1 << PORTD7);
  
  //Tri-state PORTC5 and PORTC4 so ST7580 can drive RX and TX LEDs.  IC appears to init LEDs ON upon power-on or reset.
  DDRC  &=  ~(1 << DDC5)|~(1 << DDC4);
  PORTC &=  ~(1 << PORTC5)|~(1 << PORTC4);

  //Init UART control line T_REQ HIGH
  DDRB  |=  (1 << DDB4);
  PORTB |= (1 << PORTB4);

  //Init Arduino LED for debugging 
  DDRB  |=  (1 << DDB5);
  PORTB &=  ~(1 << PORTB5);

  delay(100);                         //Delay

}

void init_ST7580_uart(){
  //set uart baud to 57600
  Serial.begin(57600);
}

boolean  ST7580_query_status(){
 
 uint16_t i = 0;
 char  buf[10]= "";           //init and empty the buffer
 
 //Check modem status 
 for(i = 0; i <= 1; i++){
    while (!(Serial.available() > 0));                    //Wait for USART data to be available 
    buf[i] = Serial.read();
 }
 //return true;
 if (buf[1] == 0x09){ 
  return true;
 }
 else{
  return true;
 }

} 


//TODO: Pass in Request CMD, received data buffer and received data size in bytes
void Host_to_ST7580_MIB_ReadRequest(uint8_t request_obj, uint8_t req_data_length){

  //Init local variables 
  uint16_t i = 0;
  uint16_t checksum = 0x0000;
  char  buf[10]= "";           //init and empty the buffer
  
  //Calc checksum
  checksum = MIB_ReadRequest + request_obj + req_data_length;
   
  //Ensure T_REQ is HIGH
  PORTB |= (1 << PORTB4);
  delay(100);
  
  //Drive T_REQ LOW to start status req from modem
  PORTB &= ~(1 << PORTB4);

 //Check modem status
  while(1){
    while (!(Serial.available() > 0));                    //Wait for USART data to be available 
    buf[i] = Serial.read();
    //delay(1);
    if (buf[i] == 0x09) break;
    i++; 
  }
  delay(10);

  //Send local from from host to modem within TSR (200ms) of modem status response or else modem will timeout and host local fram will not be interpreted by modem
  //Local frame format STX|Length|Command Code|DATA|Checksum
  Serial.write(STX);                            //Send STX (Start of text delimiter) byte
  delayMicroseconds(250);                       //Allow STX to be written out before driving T_REQ HIGH
  PORTB |= (1 << PORTB4);                       //Drive T_REQ HIGH after sending STX byte from MCU to modem
  delayMicroseconds(250);                       //Delay to allow T_REQ to go HIGH before next local frame byte is sent from MCU to modem
  Serial.write(req_data_length);                //Send byte length of data field from host to modem
  Serial.write(MIB_ReadRequest);                //Send command code from MCU to modem
  Serial.write(request_obj);                    //Send MIB Object index from host to modem
  Serial.write(checksum & 0xFF);                //Send Checksum (2 bytes long, LSByte first (e.x. checksum = 0x0017, send 0x17 first then 0x00)
  Serial.write((checksum & 0xFF00)>>8);
  while (!(Serial.available() > 0));            //Wait for modem ACK
  
  //memcpy(buf,temp,n);                         //Copy received message into global message buffer
}

//Receive data from modem
/*
If the length and the checksum of the local frame are both correct, the external host
acknowledges with an ACK character. In other cases, it answers with a NAK character.
If the MCU sends a NAK to the ST7580, the ST7580 device repeats the frame only once after a delay corresponding to TACK,
changing the STX value to 03h
*/
void ST7580_RX_data(){
  //
}

void setup() {
  
  init_ST7580_io();
  delay(100);
  
  init_ST7580_uart();
  delay(100);

  //Enable modem (bring out of reset)
  PORTD |= (1 << PORTD7);
  delay(10);

  //ST7580_query_status();

  /*The ST7580 modem is always the communication master. In case of no local transfer, the
  ST7580 can initiate a local communication without taking into account the external host
  status. On the other hand, when the external host wants to send a local frame, it must first
  send a request through the T_REQ (transmitting request) input port. Then the ST7580
  answers with a status message allowing or not the reception of a frame (or any other
  command).*/
  //Host_to_ST7580_local_frame();

  //Receive data from modem
  //ST7580_RX_data();


}

void loop() {
 
  Host_to_ST7580_MIB_ReadRequest(Host_interface_timeout, 1);                        //Check device firmware version 

}