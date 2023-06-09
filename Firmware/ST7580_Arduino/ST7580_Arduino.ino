/**
ST7580.ino
Author: Jarvis Hill (hilljarvis@gmail.com)
Purpose: 
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
#define ACK 0x06
#define NAK 0x15

//MIB Objects
#define Modem_configuration     0x00
#define PHY_configuration       0x01
#define SS_key                  0x02
#define Last_data_indication    0x04
#define Last_TX_confirm         0x05
#define PHY_Data                0x06
#define DL_Data                 0x07
#define SS_Data                 0x08
#define Host_interface_timeout  0x09
#define Firmware_version        0x0A

//TODO: ADD MIB Object PAYLOAD bytes
#define Modem_configuration_PAYLOAD_SIZE      1
#define PHY_configuration_PAYLOAD_SIZE        14
#define SS_key_PAYLOAD_SIZE                   16
#define Last_data_indication_PAYLOAD_SIZE     4
#define Last_TX_confirm_PAYLOAD_SIZE          5
#define PHY_Data_PAYLOAD_SIZE                 10
#define DL_Data_PAYLOAD_SIZE                  8
#define SS_Data_PAYLOAD_SIZE                  10
#define Host_interface_timeout_PAYLOAD_SIZE   3
#define Firmware_version_PAYLOAD_SIZE         4
        
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

 //Local variables
 uint16_t i = 0;
 char  buf[10]= "";           //init and empty the buffer
 
 //Check modem status
 while(1){
    while (!(Serial.available() > 0));                    //Wait for USART data to be available 
    buf[i] = Serial.read();
    //delay(1);
    if (buf[i] == 0x09) break;
    i++; 
  }

  //TODO: Update this to return the status bytes because value won't be static 25-MAR-2023
  //Need to test the code below 02-APR-2023
  if (buf[1] == 0x09) return true;
  else return false; 
} 


//TODO: Pass in Request CMD, received data buffer and received data size in bytes
void Host_to_ST7580_MIB_ReadRequest(uint8_t request_obj, uint8_t tx_req_data_length, uint8_t rx_req_data_length){

  //Init local variables 
  uint16_t i = 0;
  uint16_t tx_checksum = 0x0000;
  uint16_t rx_checksum = 0x0000;
  //char  buf[10]= "";           //init and empty the buffer
  uint8_t  rx_buf[50]= {0};
  
  //Calc checksum
  tx_checksum = MIB_ReadRequest + request_obj + tx_req_data_length;
   
  //Ensure T_REQ is HIGH
  PORTB |= (1 << PORTB4);
  delay(100);
  
  //Drive T_REQ LOW to start status req from modem
  PORTB &= ~(1 << PORTB4);

  //TODO: determine if loop needs to break if status message not received 25-MAR-2023
  STATUS_FLAG = ST7580_query_status();
  //if(STATUS_FLAG == false) return;
  delay(10);

  //Send local from from host to modem within TSR (200ms) of modem status response or else modem will timeout and host local fram will not be interpreted by modem
  //Local frame format STX|Length|Command Code|DATA|Checksum
  Serial.write(STX);                            //Send STX (Start of text delimiter) byte
  delayMicroseconds(250);                       //Allow STX to be written out before driving T_REQ HIGH
  PORTB |= (1 << PORTB4);                       //Drive T_REQ HIGH after sending STX byte from MCU to modem
  delayMicroseconds(250);                       //Delay to allow T_REQ to go HIGH before next local frame byte is sent from MCU to modem
  Serial.write(tx_req_data_length);             //Send byte length of data field from host to modem
  Serial.write(MIB_ReadRequest);                //Send command code from MCU to modem
  Serial.write(request_obj);                    //Send MIB Object index from host to modem
  Serial.write(tx_checksum & 0xFF);             //Send Checksum (2 bytes long, LSByte first (e.x. checksum = 0x0017, send 0x17 first then 0x00)
  Serial.write((tx_checksum & 0xFF00)>>8);
  
  //Read and buffer modem response (including ACK byte)
  uint8_t num_read_bytes = 6 + rx_req_data_length;             //constant 6 comes from ACK + STX + CONFIRM + PAYLOAD DATA LENGTH + 2 CHECKSUM BYTES
  for (i = 0; i < num_read_bytes; i++){
    while (!(Serial.available() > 0));                      //Wait for rx byte
    rx_buf[i] = Serial.read();
  }

  //Calculate RX checksum excluding ACK and STX bytes
  uint8_t checksum_bytes = num_read_bytes - 4;                //constant 4 because we want to exclude ACK, STX and 2 CHECKSUM bytes from checksum calculation
  for (i = 0; i < checksum_bytes; i++){ 
    rx_checksum += rx_buf[i+2];                               //data for checksum calc starts are buffer index 2 because ACK and STX bytes are stored in 
                                                              //indexes 0 and 1
  }

  /*Respond with an ACK from host to modem if message 
  recevied checksum and calculate checksum is correct
  */
  delayMicroseconds(250);
  if ((rx_checksum & 0xFF) ==  rx_buf[(num_read_bytes - 2)]){    //only confirming LSB of received 2 byte checksum
    Serial.write(ACK);
  }
  else{
    Serial.write(NAK);
  }

  
  //memcpy(buf,temp,n);                                       //Copy received message into global message buffer
}



//TODO: Pass in Request CMD, received data buffer and received data size in bytes 
//AND or consilidate this into the ReadRequest function 02-APR-2023
void Host_to_ST7580_MIB_WriteRequest(uint8_t request_obj, uint8_t tx_req_data_length, uint8_t rx_req_data_length){

  //Init local variables 
  uint16_t i = 0;
  uint16_t tx_checksum = 0x0000;
  uint16_t rx_checksum = 0x0000;
  char  buf[10]= "";                //init and empty the buffer
  char  rx_buf[50]= "";
  uint8_t request_obj_data = 0x10;
  
  //Calc checksum
  //checksum = MIB_WriteRequest + request_obj + req_data_length;
  tx_checksum = MIB_WriteRequest + request_obj + tx_req_data_length + request_obj_data;
   
  //Ensure T_REQ is HIGH
  PORTB |= (1 << PORTB4);
  delay(100);
  
  //Drive T_REQ LOW to start status req from modem
  PORTB &= ~(1 << PORTB4);

  //TODO: determine if loop needs to break if status message not received 25-MAR-2023
  STATUS_FLAG = ST7580_query_status();
  //if(STATUS_FLAG == false) return;
  delay(10);

  //Send local from from host to modem within TSR (200ms) of modem status response or else modem will timeout and host local fram will not be interpreted by modem
  //Local frame format STX|Length|Command Code|DATA|Checksum
  Serial.write(STX);                            //Send STX (Start of text delimiter) byte
  delayMicroseconds(250);                       //Allow STX to be written out before driving T_REQ HIGH
  PORTB |= (1 << PORTB4);                       //Drive T_REQ HIGH after sending STX byte from MCU to modem
  delayMicroseconds(250);                       //Delay to allow T_REQ to go HIGH before next local frame byte is sent from MCU to modem
  Serial.write(tx_req_data_length);                //Send byte length of data field from host to modem
  Serial.write(MIB_WriteRequest);               //Send command code from MCU to modem
  Serial.write(request_obj);                    //Send MIB Object index from host to modem
  Serial.write(request_obj_data);                    //data to write to modem object 
  Serial.write(tx_checksum & 0xFF);                //Send Checksum (2 bytes long, LSByte first (e.x. checksum = 0x0017, send 0x17 first then 0x00)
  Serial.write((tx_checksum & 0xFF00)>>8);
  
  //Read and buffer modem response (including ACK byte)
  uint8_t num_read_bytes = 6 + rx_req_data_length;             //constant 6 comes from ACK + STX + CONFIRM + PAYLOAD DATA LENGTH + 2 CHECKSUM BYTES
  for (i = 0; i < num_read_bytes; i++){
    while (!(Serial.available() > 0));                      //Wait for rx byte
    rx_buf[i] = Serial.read();
  }

  //Calculate RX checksum excluding ACK and STX bytes
  uint8_t checksum_bytes = num_read_bytes - 4;              //constant 4 because we want to exclude ACK, STX and 2 CHECKSUM bytes from checksum calculation
  for (i = 0; i < checksum_bytes; i++){
    rx_checksum += rx_buf[i+2];                             //data for checksum calc starts are buffer index 2 because ACK and STX bytes are stored in 
                                                            //indexes 0 and 1
  }

  /*Respond with an ACK from host to modem if message 
  recevied checksum and calculate checksum is correct
  */
  delayMicroseconds(250);
  if ((rx_checksum & 0xFF) == rx_buf[num_read_bytes - 2]){   //only confirming LSB of received 2 byte checksum
    Serial.write(ACK);
  }
  else{
    Serial.write(NAK);
  }

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
 
  Host_to_ST7580_MIB_ReadRequest(Firmware_version, 1, Firmware_version_PAYLOAD_SIZE);                       //ACK does not happen when device firmware version is read but calc and received checksum match 02-APR-2023
  //Host_to_ST7580_MIB_WriteRequest(Modem_configuration, 2,1);                                              //Write modem configuration 
  //delay(100);

}
