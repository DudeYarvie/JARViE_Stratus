# JARViE_Stratus
ST7580 Powerline Modem 

# Table of Contents
<details><summary>click to expand or hide</summary>

# HW Setup
## Jumpers 
## Quick Setup 

## Data Transmission
Messages can be transmitted on the powerline by wrapping them up into three different frame formats the modem can understand.  The frame format the ST7580 device will adhere to for sending and receiving messages must be configured by the external host. 
### Data Link
The external host is allowed to choose the CRC algorithm used (length, endianness, fields involved in calculation) through the modem configuration (0x00) MIB object. The length field is automatically handled by ST7580 and its value is by default equal to the length of payload and CRC fields.
#### Frame Structure
![datalink_frame_structure](https://github.com/DudeYarvie/JARViE_Stratus/blob/main/References/Pics/DataLink_Frame_Structure.PNG)
Provide high-level details on the structure.  Add the image from the data sheet. Add an example. 
#### DL_Indication
### Physical 
#### Frame Structure
### Security Services
#### Frame Structure

### DATA_IN to Tx Carrier Output Delay
