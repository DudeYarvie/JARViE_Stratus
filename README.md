# JARViE_Stratus
ST7580 Powerline Modem 

![NUCLEO_PLM01A1](https://github.com/DudeYarvie/JARViE_Stratus/blob/main/References/Pics/X_NUCLEO_PLM01A1.PNG)

# Table of Contents
<details><summary>click to expand or hide</summary>
 
# Abbreviations
|Abbreviation| Description|
|:-:|:--|
|MIB|Management information base|
|PGA|Programmable gain amplifier|
|ZC|Zero-crossing|
|PHY|Physical layer|
|DL| Data link layer|
|CRC|Cyclic redundancy check|
|AES|Advanced encryption standard|
|UART|Universal asynchronous receiver transmitter|
|Tic|Inter character timeout|
|Tack|Acknowledge timeout|
|Tsr|Service request timeout|
|SS|Security services|
|BIO|Basic input output|
|HI|Host interface|
|UW|Unique word, a predefined sequence used to mark the start of a physical frame. The physical layer also provides SNR estimation on the received unique word|

# Hardware Setup
![Hardware_Setup](https://github.com/DudeYarvie/JARViE_Stratus/blob/main/References/Pics/HW_Setup.PNG)
## Jumpers 
## Quick Setup 

## Data Transmission
Messages can be transmitted on the powerline by wrapping them up in one of the following frame formats. The frame format the ST7580 device will adhere to for sending and receiving messages must be configured by the external host. 
* Physical (PHY) 
* DataLink
* Security Servics

### DataLink
The external host is allowed to choose the CRC algorithm used (length, endianness, fields involved in calculation) through the modem configuration (0x00) MIB object. The length field is automatically handled by ST7580 and its value is by default equal to the length of payload and CRC fields.

#### DL_DataRequest (50h)
Send this command to the ST7580 modem from a host MCU to broadcast a message on the actual powerline.

**Frame structure:**
Using default modem configurations (e.g. not changing the Modem Configuration register (00h)), data is sent big-endian, one byte at a time. By default, CRC is 16 bits wide but can be configured as 8, 16 or 32 bits wide. **Must send CRC least significant byte first** (ex. CRC  = 0xAABB, must send BBh then AAh to the modem).
STX|PAYLOAD SIZE|DL_DataRequest|PAYLOAD|16-bit CRC
|:-:|:-:|:-:|:-:|:-:|

**Example:** 
The host MCU wants to send a one byte message on the powerline.  If the modem receives the request successfully, it will respond to the MCU with a DL_DataConfirm message then transmit the equivalent analog modulated message out through it's TX_OUT pin.
STX|PAYLOAD SIZE|DL_DataRequest|PAYLOAD|16-bit CRC
|:-:|:-:|:-:|:-:|:-:|
02 | 02 | 50 | 04 AB | 01 01  

***PAYLOAD:*** DataRequestConfigurations + message

***DataRequestConfigurations (Byte 0):*** 0x04
|Bit Index|Value|Description|
|:-:|:-:|:--|
|0 | 0 | Custom / MIB frequency = 0 -> TX frequency is the high or low frequency as in PHY_Config MIB object (01h)|
| 1 | 0 | Frequency overwrite = 0 -> TX frequency is configured in PHY_Config MIB object (01h)|
| 2 | 1 | Frequency set = 1 -> TX frequency is the HighFrequency in PHY_Config MIB object (01h)|
| 3 | 0 | Gain selector = 0 -> TX gain set as in PHY Config MIB object (01h), -16dB +/- 0.35dB default|
| 4-6| 0 | Frame modulation = 0 -> B-PSK|
| 7 | 0 | Zero crossing synchronization = 0 -> Transmission frame starts on any instant|

***message:*** 0xAB

***16-bit CRC =*** PAYLOAD SIZE + DL_DataRequest + PAYLOAD = 0x0101 **(send LSByte first)**

### DL_DataConfirm (51h)
If a host MCU successfully sends a DL_DataRequest to the modem, the modem will respond with a DL_DataConfirm message.

**Frame structure:**
STX|PAYLOAD SIZE|DL_DataConfirm|PAYLOAD|16-bit CRC
|:-:|:-:|:-:|:-:|:-:|

**Example:** 
STX|PAYLOAD SIZE|DL_DataConfirm|PAYLOAD|16-bit CRC
|:-:|:-:|:-:|:-:|:-:|
02 | 05 | 51 | 54 54 00 F2 9E | 8E 02  

***PAYLOAD:*** 54 54 00 F2 9E

***(Byte 0):*** 0x54 (0b01010100)
|Bit Index|Value|Description|
|:-:|:-:|:--|
|0-1 | 0b00 | Max temp (bit 0-1) = 0b00 = T < 70 deg (typical)|
| 2-6 | 0 | Max gain (bit 2-6) = 0b10101 = default gain value of -16dB +/- 0.35dB,  A {dB} = (TX_GAIN - 31) + TX_GAINTOL, where TX_GAINTOL = 0.35dB|
| 7| - | unused|

***(Byte 1):*** 0x54 (0b01010100)
|Bit Index|Value|Description|
|:-:|:-:|:--|
|0-1 | 0b00 | Min temp (bit 0-1) = 0b00 = T < 70 deg (typical)|
| 2-6 | 0 | Min gain (bit 2-6) = 0b10101 = default gain value of -16dB +/- 0.35dB,  A {dB} = (TX_GAIN - 31) + TX_GAINTOL, where TX_GAINTOL = 0.35dB| 
| 7| - | unused|

***(Byte 2):*** 0x00 (0b00000000)
|Bit Index|Value|Description|
|:-:|:-:|:--|
|0-6 | 0b0000000 | Overcurrent events num. (bit 0-6) = 0b000000 = 0 OVC events|
| 7 | 0 | Overcurrent notification (bit 7) = 0 = max. output current value not reached|

***(Byte 3 & 4):*** 0xF29E (0b1111001010011110)

Delay between the last transmitted UW last bit and the mains zerocrossing (signed value), expressed in 13 µs step. Not sure what this value means at the moment since Zero crossing synchronization was set to 0 in the DL_DataRequest.

***16-bit CRC =*** = PAYLOAD SIZE + DL_ DataConfirm + PAYLOAD = 0x028E **(sent LSByte first)**
 
### DL_DataIndication (52h)
If a ST7580 modem receives a message over the powerline (e.g. on its RX_IN pin). It will send a DL_DataIndication to its host MCU. 

**Frame structure:**
STX|PAYLOAD SIZE|DL_DataIndication|PAYLOAD|16-bit CRC
|:-:|:-:|:-:|:-:|:-:|

**Example:** 
STX|PAYLOAD SIZE|DL_DataConfirm|PAYLOAD|16-bit CRC
|:-:|:-:|:-:|:-:|:-:|
02 | 05 | 52 | 18 19 0D 5C AB | 9C 01

***PAYLOAD:*** IndicaionData + message

***IndicationData:*** 18 19 0D 5C

***(Byte 0):*** 0x18 (0b00011000)
|Bit Index|Value|Description|
|:-:|:-:|:--|
|0-2 | 0b000 | Frame Modulation = 0b000 = B-PSK|
| 3 | 1 | RX Channel = high channel|
|4-7 | 0b0001 | PGA value = 0b0001 = -12dB, RXIN max range 8Vpp, see ST7580 datasheet PGA table|

***(Byte 1):*** 0x19 (0b00011001)

SNR = 25 dB estimated over the UW reception (signed value, valid for PSK received frames only, equal to 255 – no meaning – for FSK received frames)

***(Byte 2):*** 0x0D (0b00001101)

ZC delay = 13 * 13 µs = 169 µs. Delay between the received UW last bit and the mains zerocrossing (signed value), expressed in 13 µs step. This might not be correct sincea a ZC delay was not set in the DL_DataRequest and it's set to 0 by defualt in the modem config register (00h).

***PAYLOAD:*** AB

***16-bit CRC =*** = PAYLOAD SIZE + DL_ DataIndication + PAYLOAD = 0x019C  **(sent LSByte first)**

### Physical 
#### Frame Structure
### Security Services
#### Frame Structure

### DATA_IN to Tx Carrier Output Delay
