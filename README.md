# ST7580 Demo Boards
The purpose of this repo is to serve as a sort cliff notes for using the ST7580 powerline modem. It is meant to deconvolute the ST datasheets and provide a more clear and consise way to use the modem via the JARVIE Stratus or ST X-NUCLEO-PLM01A1 demo boards.
# Table of Contents
# JARViE_Stratus
Coming Soon! 
# X-NUCLEO-PLM01A1
The X-NUCLEO-PLM01A1 expansion board for STM32 Nucleo is based on the ST7580 FSK, PSK multi-mode power line networking system-on-chip. It provides an affordable and easy-to-use solution for the development of connectivity applications based on power line communication. It lets you easily evaluate the communication features of the ST7580 based on a DC two-wire link between two boards.
You can also perform evaluation on an AC power line by connecting the X-NUCLEO-PLM01A1 to an STEVAL-XPLM01CPL board providing effective AC coupling and isolation. The X-NUCLEO-PLM01A1 is interfaced with the STM32 controller via UART and GPIO pins and is compatible with the Arduino
UNO R3 (default configuration) and ST morpho (optional) connectors. 
<p align="center">
  <img width="560" height="400" src="https://github.com/DudeYarvie/JARViE_Stratus/blob/main/References/Pics/X_NUCLEO_PLM01A1.PNG" />
</p>

## Hardware Setup
Two or more power line communication (PLC) modems are rquired to create powerline network. A processor must be used to interface with the modem in order to send and receive messages transmitted over the powerline.  The modems serve as a D/A and A/D converter where the analog block interfaces with the powerline and the digital block interfaces with the processor. The setup below shows how to create a powerline network with two NUCLEO-PLM01A1 ST7580 demo boards stacked on top of ST NUCLEO microcontroller development boards. There are two power supplies, one 12VDC supply to power the hardware and another PLM power supply to create the powerline the ST7580 modems will communicate over.

<p align="center">
  <img width="860" height="700" src="https://github.com/DudeYarvie/JARViE_Stratus/blob/main/References/Pics/HW_Setup.PNG" />
</p>

One additional thing to note, in order for the PLM DC supply generating the powerline to stay voltage compliant (i.e. maintain its set voltage and not enter constant-current mode) it must source enough current to the X-NUCLEO-PLM01A1 PLM_CONNECTOR to overcome the input impedance of its analog front-end.  At DC, the input impedance of the PLM01A1 front-end looks like 150 Ohms. So, the DC supply needs to be able to source a minimum current of I = V / 150 Ohms, where V is the voltage set point of the supply.  Two PLM01A1 boards look like a load of 75 Ohms on the line (150 || 150), so the supply needs to be able to source a minimum current of I = V / 75 Ohms.  In general, the supply needs to source a minimum current of,

<p align="center">I = V / (150 / N), where V = supply set voltage, N = number of PLM01A1 boards </p>

### Jumpers 
<p align="center" >
  <img width="660" height="500" src="https://github.com/DudeYarvie/JARViE_Stratus/blob/main/References/Pics/Jumpers.png" />
</p>

##  Host-Modem Interface
All communications between the external host (MCU) and the modem are done over a 3-wire bus shared between the devices.  The bus consists of a half-duplexed UART and a discrete digital signal.  
* **TXD** carries data from the ST7580 to the host
* **RXD** carries data from the host to the ST7580
* **T_REQ** modem request input, used by the host to let the modem know the host wants to send it data 
<p align="center" >
  <img width="460" height="300" src="https://github.com/DudeYarvie/JARViE_Stratus/blob/main/References/Pics/Host_modem_interface.PNG" />
</p>

## Host-Modem Communication
The host can first check if the modem is free/ready to receive commands by driving the T_REQ signal LOW. The modem will respond with a two byte wide status message.  Byte0 will always be 0x3F (preamble) and byte1 will contain modem status information.

Byte Index|Bit Index|Description|Available Values 
|:-:|:-:|:--|:--|
|0|-|Status message first byte|3Fh|
|1|0|Configuration status|0: autoreconfiguration correctly occurred, 1: autoreconfiguration occurred with errors or at least the Modem Config, PHY Config or SS Key MIB objects hasn’t changed its default value after boot|
||1|Transmission status|0: the ST7580 is not transmitting a power line frame, 1: the ST7580 is transmitting a power line frame|
||2|Reception status|0: the ST7580 is not receiving a power line frame, 1: the ST7580 is receiving a power line frame|
||3-4|Active PLC layer|0: PHY layer, 1: DL layer, 2: SS layer, 3: ST7580 not configured|
||5|Overcurrent flag|0: no overcurrent event on last transmission, 1: last transmission generated at least one overcurrent event|
||6-7|Est. modem temp|0: T < 70 °C (typical), 1: 70 °C < T <100 °C (typical), 2: 100 °C < T < 125 °C (typical), 3: T > 125 °C (typical)|

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
|HI|Host interface, 3-wire bust shared between the host MCU and ST7580 modem|
|UW|Unique word, a predefined sequence used to mark the start of a physical frame. The physical layer also provides SNR estimation on the received unique word|

## Powerline Data Transmission
Messages can be transmitted on the powerline by wrapping them up in one of the following frame formats. The frame format the ST7580 device will adhere to for sending and receiving messages must be configured by the external host. 
* Physical (PHY) 
* DataLink (DL)
* Security Services (SS)

### DataLink
The embedded DL layer hosted in the protocol controller offers framing and error correction services. The external host is allowed to choose the CRC algorithm used (length, endianness, fields involved in calculation) through the modem configuration (0x00) MIB object. The length field is automatically handled by ST7580 and its value is by default equal to the length of payload and CRC fields.

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
STX|PAYLOAD SIZE|DL_DataIndication|PAYLOAD|16-bit CRC
|:-:|:-:|:-:|:-:|:-:|
02 | 05 | 52 | 18 19 0D 5C AB | 9C 01

***PAYLOAD:*** IndicationData + message

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
Hosted in the PHY processor, implements two different modulation schemes for communication through power line: a B-FSK modulation up to
9.6 kbps and a multi-mode PSK modulation with channel quality estimation, dual channel receiving mode and convolutional coding, delivering a throughput up to 28.8 kbps.

### Security Services
Provides authentication to the payload using cryptographic algorithms based on AES with 128-bit keys. Authentication is provided appending to user data an AES-CMAC digest. A dedicated key stored in the MIB object SS key 02h, is used for both transmitting and receiving frames. 
