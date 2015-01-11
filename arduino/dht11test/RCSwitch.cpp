/// \class RCSwitch

#include "RCSwitch.h"

#if not defined( RCSwitchDisableReceiving )
unsigned long RCSwitch::nReceivedValue = NULL;
unsigned int RCSwitch::nReceivedBitlength = 0;
unsigned int RCSwitch::nReceivedDelay = 0;
unsigned int RCSwitch::nReceivedProtocol = 0;
int RCSwitch::nReceiveTolerance = 60;
#endif
unsigned int RCSwitch::timings[RCSWITCH_MAX_CHANGES];

RCSwitch::RCSwitch() {
  this->nTransmitterPin = -1;
  this->setPulseLength(350);
  this->setRepeatTransmit(10);
  this->setProtocol(1);
  #if not defined( RCSwitchDisableReceiving )
  this->nReceiverInterrupt = -1;
  this->setReceiveTolerance(60);
  RCSwitch::nReceivedValue = NULL;
  #endif
}

/**
  * Sets the protocol to send.
  */
void RCSwitch::setProtocol(int nProtocol) {
  this->nProtocol = nProtocol;
  if (nProtocol == 1){
    this->setPulseLength(350);
  }
  else if (nProtocol == 2) {
    this->setPulseLength(650);
  }
  else if (nProtocol == 3) {
    this->setPulseLength(100);
  }
}

/// Sets the protocol to send with pulse length in microseconds.
///
/// \param nProtocol Protocol version (1, 2 or 3)
/// \param nPulseLength Pulse length in µs
void RCSwitch::setProtocol(int nProtocol, int nPulseLength) {
  this->nProtocol = nProtocol;
  this->setPulseLength(nPulseLength);
}


/// Sets pulse length in microseconds.
///
/// \param nPulseLength Pulse length in µs
void RCSwitch::setPulseLength(int nPulseLength) {
  this->nPulseLength = nPulseLength;
}

/// Sets Repeat Transmits.
///
/// \param nRepeatTransmit How many times a transmission is repeated
void RCSwitch::setRepeatTransmit(int nRepeatTransmit) {
  this->nRepeatTransmit = nRepeatTransmit;
}

/// Set Receiving Tolerance.
///
/// \param nPercent Receiving tolerance (%)
#if not defined( RCSwitchDisableReceiving )
void RCSwitch::setReceiveTolerance(int nPercent) {
  RCSwitch::nReceiveTolerance = nPercent;
}
#endif
  
/// Enable transmissions.
///
/// \param nTransmitterPin Arduino Pin to which the sender is connected to
void RCSwitch::enableTransmit(int nTransmitterPin) {
  this->nTransmitterPin = nTransmitterPin;
  pinMode(this->nTransmitterPin, OUTPUT);
}

/// Disable transmissions.
void RCSwitch::disableTransmit() {
  this->nTransmitterPin = -1;
}

/// Switch a remote switch on (Type D REV).
///
/// \param sGroup Code of the switch group (A, B, C, D)
/// \param nDevice Number of the switch itself (1..3)
void RCSwitch::switchOn(char sGroup, int nDevice) {
  this->sendTriState( this->getCodeWordD(sGroup, nDevice, true) );
}

/// Switch a remote switch off (Type D REV).
/// 
/// \param sGroup Code of the switch group (A, B, C, D)
/// \param nDevice Number of the switch itself (1..3)
void RCSwitch::switchOff(char sGroup, int nDevice) {
  this->sendTriState( this->getCodeWordD(sGroup, nDevice, false) );
}

/// Switch a remote switch on (Type C Intertechno).
/// 
/// \param sFamily Familycode (a..f)
/// \param nGroup Number of group (1..4)
/// \param nDevice Number of device (1..4)
void RCSwitch::switchOn(char sFamily, int nGroup, int nDevice) {
  this->sendTriState( this->getCodeWordC(sFamily, nGroup, nDevice, true) );
}

/// Switch a remote switch off (Type C Intertechno)
/// 
/// \param sFamily Familycode (a..f)
/// \param nGroup Number of group (1..4)
/// \param nDevice Number of device (1..4)
void RCSwitch::switchOff(char sFamily, int nGroup, int nDevice) {
  this->sendTriState( this->getCodeWordC(sFamily, nGroup, nDevice, false) );
}

/// Switch a remote switch on (Type B with two rotary/sliding switches)
/// 
/// \param nAddressCode Number of the switch group (1..4)
/// \param nChannelCode Number of the switch itself (1..4)
void RCSwitch::switchOn(int nAddressCode, int nChannelCode) {
  this->sendTriState( this->getCodeWordB(nAddressCode, nChannelCode, true) );
}

/// Switch a remote switch off (Type B with two rotary/sliding switches)
/// 
/// \param nAddressCode  Number of the switch group (1..4)
/// \param nChannelCode  Number of the switch itself (1..4)
void RCSwitch::switchOff(int nAddressCode, int nChannelCode) {
  this->sendTriState( this->getCodeWordB(nAddressCode, nChannelCode, false) );
}

/// Switch a remote switch on (Type A with 10 pole DIP switches)
/// 
/// \param sGroup Code of the switch group (refers to DIP switches 1..5 where "1" = on and "0" =
///               off, if all DIP switches are on it's "11111")
/// \param sDevice Code of the switch device (refers to DIP switches 6..10 (A..E) where "1" = on and
///                "0" = off, if all DIP switches are on it's "11111")
void RCSwitch::switchOn(char* sGroup, char* sDevice) {
    this->sendTriState( this->getCodeWordA(sGroup, sDevice, true) );
}

/// Switch a remote switch off (Type A with 10 pole DIP switches)
/// 
/// \param sGroup Code of the switch group (refers to DIP switches 1..5 where "1" = on and "0" =
///               off, if all DIP switches are on it's "11111")
/// \param sDevice Code of the switch device (refers to DIP switches 6..10 (A..E) where "1" = on and
///                "0" = off, if all DIP switches are on it's "11111")
void RCSwitch::switchOff(char* sGroup, char* sDevice) {
    this->sendTriState( this->getCodeWordA(sGroup, sDevice, false) );
}

/// Gets the Code Word to be sent for Type B.
///
/// A Code Word consists of 9 address bits, 3 data bits and one sync bit but in our case only the
/// first 8 address bits and the last 2 data bits were used.
///
/// A Code Bit can have 4 different states: "F" (floating), "0" (low), "1" (high), "S" (synchronous
/// bit).
///
/// 4 bits address (switch group) | 4 bits address (switch number) | 1 bit address (not used, so never mind) | 1 bit address (not used, so never mind) | 2 data bits (on/off) | 1 sync bit
/// ----------------------------- | ------------------------------ | --------------------------------------- | --------------------------------------- | -------------------- | ----------
/// 1=0FFF 2=F0FF 3=FF0F 4=FFF0   | 1=0FFF 2=F0FF 3=FF0F 4=FFF0    | F                                       | F                                       | on=FF off=F0         | S         
///
/// \param nAddressCode Number of the switch group (1..4)
/// \param nChannelCode Number of the switch itself (1..4)
/// \param bStatus Wether to switch on (true) or off (false)
///
/// \return A char[13], representing the Code Word to be sent
char* RCSwitch::getCodeWordB(int nAddressCode, int nChannelCode, boolean bStatus) {
   int nReturnPos = 0;
   static char sReturn[13];
   
   char* code[5] = { "FFFF", "0FFF", "F0FF", "FF0F", "FFF0" };
   if (nAddressCode < 1 || nAddressCode > 4 || nChannelCode < 1 || nChannelCode > 4) {
    return '\0';
   }
   for (int i = 0; i<4; i++) {
     sReturn[nReturnPos++] = code[nAddressCode][i];
   }

   for (int i = 0; i<4; i++) {
     sReturn[nReturnPos++] = code[nChannelCode][i];
   }
   
   sReturn[nReturnPos++] = 'F';
   sReturn[nReturnPos++] = 'F';
   sReturn[nReturnPos++] = 'F';
   
   if (bStatus) {
      sReturn[nReturnPos++] = 'F';
   } else {
      sReturn[nReturnPos++] = '0';
   }
   
   sReturn[nReturnPos] = '\0';
   
   return sReturn;
}

/// Gets the Code Word to be sent for Type A.
///
/// \return A char[13], representing the Code Word to be sent
char* RCSwitch::getCodeWordA(char* sGroup, char* sDevice, boolean bOn) {
    static char sDipSwitches[13];
    int i = 0;
    int j = 0;
    
    for (i=0; i < 5; i++) {
        if (sGroup[i] == '0') {
            sDipSwitches[j++] = 'F';
        } else {
            sDipSwitches[j++] = '0';
        }
    }

    for (i=0; i < 5; i++) {
        if (sDevice[i] == '0') {
            sDipSwitches[j++] = 'F';
        } else {
            sDipSwitches[j++] = '0';
        }
    }

    if ( bOn ) {
        sDipSwitches[j++] = '0';
        sDipSwitches[j++] = 'F';
    } else {
        sDipSwitches[j++] = 'F';
        sDipSwitches[j++] = '0';
    }

    sDipSwitches[j] = '\0';

    return sDipSwitches;
}

/// Gets the Code Word to be sent for Type C (Intertechno).
///
/// \return A char[13], representing the Code Word to be sent
char* RCSwitch::getCodeWordC(char sFamily, int nGroup, int nDevice, boolean bStatus) {
  static char sReturn[13];
  int nReturnPos = 0;
  
  if ( (byte)sFamily < 97 || (byte)sFamily > 112 || nGroup < 1 || nGroup > 4 || nDevice < 1 || nDevice > 4) {
    return '\0';
  }
  
  char* sDeviceGroupCode =  dec2binWzerofill(  (nDevice-1) + (nGroup-1)*4, 4  );
  char familycode[16][5] = { "0000", "F000", "0F00", "FF00", "00F0", "F0F0", "0FF0", "FFF0", "000F", "F00F", "0F0F", "FF0F", "00FF", "F0FF", "0FFF", "FFFF" };
  for (int i = 0; i<4; i++) {
    sReturn[nReturnPos++] = familycode[ (int)sFamily - 97 ][i];
  }
  for (int i = 0; i<4; i++) {
    sReturn[nReturnPos++] = (sDeviceGroupCode[3-i] == '1' ? 'F' : '0');
  }
  sReturn[nReturnPos++] = '0';
  sReturn[nReturnPos++] = 'F';
  sReturn[nReturnPos++] = 'F';
  if (bStatus) {
    sReturn[nReturnPos++] = 'F';
  } else {
    sReturn[nReturnPos++] = '0';
  }
  sReturn[nReturnPos] = '\0';
  return sReturn;
}

/// Decoding for the REV Switch Type.
/// 
/// A Code Word consists of 7 address bits and 5 command data bits.
///
/// A Code Bit can have 3 different states: "F" (floating), "0" (low), "1" (high).
/// 
/// | 4 bits address (switch group) | 3 bits address (device number) | 5 bits (command data) |
/// | ----------------------------- | ------------------------------ | --------------------- |
/// | A=1FFF B=F1FF C=FF1F D=FFF1   | 1=0FFF 2=F0FF 3=FF0F 4=FFF0    | on=00010 off=00001    |
/// 
/// Source: http://www.the-intruder.net/funksteckdosen-von-rev-uber-arduino-ansteuern/
/// 
/// \param sGroup Name of the switch group (A..D, resp. a..d) 
/// \param nDevice Number of the switch itself (1..3)
/// \param bStatus Wether to switch on (true) or off (false)
/// 
/// \return Returns a char[13], representing the Tristate to be sent
char* RCSwitch::getCodeWordD(char sGroup, int nDevice, boolean bStatus){
    static char sReturn[13];
    int nReturnPos = 0;

    // Building 4 bits address
    // (Potential problem if dec2binWcharfill not returning correct string)
    char *sGroupCode;
    switch(sGroup){
        case 'a':
        case 'A':
            sGroupCode = dec2binWcharfill(8, 4, 'F'); break;
        case 'b':
        case 'B':
            sGroupCode = dec2binWcharfill(4, 4, 'F'); break;
        case 'c':
        case 'C':
            sGroupCode = dec2binWcharfill(2, 4, 'F'); break;
        case 'd':
        case 'D':
            sGroupCode = dec2binWcharfill(1, 4, 'F'); break;
        default:
            return '\0';
    }
    
    for (int i = 0; i<4; i++)
    {
        sReturn[nReturnPos++] = sGroupCode[i];
    }


    // Building 3 bits address
    // (Potential problem if dec2binWcharfill not returning correct string)
    char *sDevice;
    switch(nDevice) {
        case 1:
            sDevice = dec2binWcharfill(4, 3, 'F'); break;
        case 2:
            sDevice = dec2binWcharfill(2, 3, 'F'); break;
        case 3:
            sDevice = dec2binWcharfill(1, 3, 'F'); break;
        default:
            return '\0';
    }

    for (int i = 0; i<3; i++)
        sReturn[nReturnPos++] = sDevice[i];

    // fill up rest with zeros
    for (int i = 0; i<5; i++)
        sReturn[nReturnPos++] = '0';

    // encode on or off
    if (bStatus)
        sReturn[10] = '1';
    else
        sReturn[11] = '1';

    // last position terminate string
    sReturn[12] = '\0';
    return sReturn;

}

/// Sends a tristate.
///
/// \see getCodeWordB()
///
/// \param sCodeWord A string beginning with 1, 0, F or S
void RCSwitch::sendTriState(char* sCodeWord) {
  for (int nRepeat=0; nRepeat<nRepeatTransmit; nRepeat++) {
    int i = 0;
    while (sCodeWord[i] != '\0') {
      switch(sCodeWord[i]) {
        case '0':
          this->sendT0();
        break;
        case 'F':
          this->sendTF();
        break;
        case '1':
          this->sendT1();
        break;
      }
      i++;
    }
    this->sendSync();    
  }
}

/// Send a max 32 bit value.
///
/// \param Code A value of max 32 bits
/// \param length Length in bits of the code to send
void RCSwitch::send(unsigned long Code, unsigned int length) {
  this->send( this->dec2binWzerofill(Code, length) );
}

/// Send a string.
///
/// \param sCodeWord Null-terminated string to send
void RCSwitch::send(char* sCodeWord) {
  for (int nRepeat=0; nRepeat<nRepeatTransmit; nRepeat++) {
    int i = 0;
    while (sCodeWord[i] != '\0') {
      switch(sCodeWord[i]) {
        case '0':
          this->send0();
        break;
        case '1':
          this->send1();
        break;
      }
      i++;
    }
    this->sendSync();
  }
}

/// Raw send a number of high pulses followed by a number of low pulses.
///
/// \param nHighPulses Number of high pulses
/// \param nLowPulses Number of low pulses
void RCSwitch::transmit(int nHighPulses, int nLowPulses) {
  #if not defined ( RCSwitchDisableReceiving )
  boolean disabled_Receive = false;
  int nReceiverInterrupt_backup = nReceiverInterrupt;
  #endif
  if (this->nTransmitterPin != -1) {
    #if not defined( RCSwitchDisableReceiving )
    if (this->nReceiverInterrupt != -1) {
        this->disableReceive();
        disabled_Receive = true;
    }
    #endif
    digitalWrite(this->nTransmitterPin, HIGH);
    delayMicroseconds( this->nPulseLength * nHighPulses);
    digitalWrite(this->nTransmitterPin, LOW);
    delayMicroseconds( this->nPulseLength * nLowPulses);
    
    #if not defined( RCSwitchDisableReceiving )
    if(disabled_Receive){
        this->enableReceive(nReceiverInterrupt_backup);
    }
    #endif
  }
}

/// Sends symbol for bit '0', according to the current protocol.
///
/// Symbol shapes:
///
/// ~~~
///  _       _    
/// | |___  | |__
///
/// ^^^^^^  ^^^^^
/// prot1   prot2
/// ~~~
void RCSwitch::send0() {
  if (this->nProtocol == 1) {
    this->transmit(1, 3);
  }
  else if (this->nProtocol == 2) {
    this->transmit(1, 2);
  }
  else if (this->nProtocol == 3) {
    this->transmit(4, 11);
  }
}

/// Sends symbol for bit '1', according to the current protocol.
///
/// Symbol shapes:
///
/// ~~~
///  ___     __    
/// |   |_  |  |_
///
/// ^^^^^^  ^^^^^
/// prot1   prot2
/// ~~~
void RCSwitch::send1() {
  if (this->nProtocol == 1) {
    this->transmit(3, 1);
  }
  else if (this->nProtocol == 2) {
    this->transmit(2, 1);
  }
  else if (this->nProtocol == 3) {
    this->transmit(9, 6);
  }
}

/// Sends tri-state symbol for '0'.
///
/// Symbol shape:
///
/// ~~~
///  _     _
/// | |___| |___
/// ~~~
void RCSwitch::sendT0() {
  this->transmit(1, 3);
  this->transmit(1, 3);
}

/// Sends tri-state symbol for '1'.
///
/// Symbol shape:
///
/// ~~~
///  ___   ___
/// |   |_|   |_
/// ~~~
void RCSwitch::sendT1() {
  this->transmit(3,1);
  this->transmit(3,1);
}

/// Sends tri-state symbol for 'F'.
///
/// Symbol shape:
///
/// ~~~
///  _     ___
/// | |___|   |_
/// ~~~
void RCSwitch::sendTF() {
  this->transmit(1, 3);
  this->transmit(3, 1);
}

/// Sends a sync bit.
///
/// Shape for Protocol 1:
/// ~~~
///  _
/// | |_______________________________
/// ~~~
///
/// Shape for Protocol 2:
/// ~~~
///  _
/// | |__________
/// ~~~
void RCSwitch::sendSync() {

    if (this->nProtocol == 1){
        this->transmit(1,31);
    }
    else if (this->nProtocol == 2) {
        this->transmit(1,10);
    }
    else if (this->nProtocol == 3) {
        this->transmit(1,71);
    }
}

#if not defined( RCSwitchDisableReceiving )

/// Enables receiving data.
///
/// \param interrupt Interrupt associated with reception
void RCSwitch::enableReceive(int interrupt) {
  this->nReceiverInterrupt = interrupt;
  this->enableReceive();
}

/// Enables receiving data.
void RCSwitch::enableReceive() {
  if (this->nReceiverInterrupt != -1) {
    RCSwitch::nReceivedValue = NULL;
    RCSwitch::nReceivedBitlength = NULL;
    attachInterrupt(this->nReceiverInterrupt, handleInterrupt, CHANGE);
  }
}

/// Disables receiving data.
void RCSwitch::disableReceive() {
  detachInterrupt(this->nReceiverInterrupt);
  this->nReceiverInterrupt = -1;
}

/// Is receiving data available?
///
/// \return true if it is avaliable, false otherwise
bool RCSwitch::available() {
  return RCSwitch::nReceivedValue != NULL;
}

/// Reset available state.
void RCSwitch::resetAvailable() {
  RCSwitch::nReceivedValue = NULL;
}

/// Gets received value.
///
/// \see getReceivedBitlength()
///
/// \return A 32 bit value
unsigned long RCSwitch::getReceivedValue() {
    return RCSwitch::nReceivedValue;
}

/// Gets number of bits received.
///
/// \see getReceivedValue()
///
/// \return Number of bits received.
unsigned int RCSwitch::getReceivedBitlength() {
  return RCSwitch::nReceivedBitlength;
}

/// Gets received delay.
unsigned int RCSwitch::getReceivedDelay() {
  return RCSwitch::nReceivedDelay;
}

/// Gets protocol used for reception.
unsigned int RCSwitch::getReceivedProtocol() {
  return RCSwitch::nReceivedProtocol;
}

/// Gets raw data received.
unsigned int* RCSwitch::getReceivedRawdata() {
  return RCSwitch::timings;
}

/// Receives data from Protocol 1.
bool RCSwitch::receiveProtocol1(unsigned int changeCount) {
    
  unsigned long code = 0;
  unsigned long delay = RCSwitch::timings[0] / 31;
  unsigned long delayTolerance = delay * RCSwitch::nReceiveTolerance * 0.01;    

  for (int i = 1; i<changeCount ; i=i+2) {
  
    if (RCSwitch::timings[i] > delay-delayTolerance && RCSwitch::timings[i] < delay+delayTolerance && RCSwitch::timings[i+1] > delay*3-delayTolerance && RCSwitch::timings[i+1] < delay*3+delayTolerance) {
      code = code << 1;
    }
    else if (RCSwitch::timings[i] > delay*3-delayTolerance && RCSwitch::timings[i] < delay*3+delayTolerance && RCSwitch::timings[i+1] > delay-delayTolerance && RCSwitch::timings[i+1] < delay+delayTolerance) {
      code+=1;
      code = code << 1;
    }
    else {
      // Failed
      i = changeCount;
      code = 0;
    }

  }
  code = code >> 1;
  if (changeCount > 6) {    // ignore < 4bit values as there are no devices sending 4bit values => noise
    RCSwitch::nReceivedValue = code;
    RCSwitch::nReceivedBitlength = changeCount / 2;
    RCSwitch::nReceivedDelay = delay;
    RCSwitch::nReceivedProtocol = 1;
  }

  if (code == 0) {
    return false;
  }
  else if (code != 0) {
    return true;
  }
}

/// Receives data from Protocol 2.
bool RCSwitch::receiveProtocol2(unsigned int changeCount) {
    
  unsigned long code = 0;
  unsigned long delay = RCSwitch::timings[0] / 10;
  unsigned long delayTolerance = delay * RCSwitch::nReceiveTolerance * 0.01;    

  for (int i = 1; i<changeCount ; i=i+2) {

    if (RCSwitch::timings[i] > delay-delayTolerance && RCSwitch::timings[i] < delay+delayTolerance && RCSwitch::timings[i+1] > delay*2-delayTolerance && RCSwitch::timings[i+1] < delay*2+delayTolerance) {
      code = code << 1;
    }
    else if (RCSwitch::timings[i] > delay*2-delayTolerance && RCSwitch::timings[i] < delay*2+delayTolerance && RCSwitch::timings[i+1] > delay-delayTolerance && RCSwitch::timings[i+1] < delay+delayTolerance) {
      code+=1;
      code = code << 1;
    }
    else {
      // Failed
      i = changeCount;
      code = 0;
    }

  }      
  code = code >> 1;
    if (changeCount > 6) {    // ignore < 4bit values as there are no devices sending 4bit values => noise
      RCSwitch::nReceivedValue = code;
      RCSwitch::nReceivedBitlength = changeCount / 2;
      RCSwitch::nReceivedDelay = delay;
      RCSwitch::nReceivedProtocol = 2;
    }

    if (code == 0) {
      return false;
    }
    else if (code != 0) {
      return true;
    }

}

/// Receives data from Protocol 3 (used by BL35P02).
bool RCSwitch::receiveProtocol3(unsigned int changeCount) {
    
  unsigned long code = 0;
  unsigned long delay = RCSwitch::timings[0] / PROTOCOL3_SYNC_FACTOR;
  unsigned long delayTolerance = delay * RCSwitch::nReceiveTolerance * 0.01;    

  for (int i = 1; i<changeCount ; i=i+2) {
    if  (RCSwitch::timings[i]   > delay*PROTOCOL3_0_HIGH_CYCLES - delayTolerance
      && RCSwitch::timings[i]   < delay*PROTOCOL3_0_HIGH_CYCLES + delayTolerance
      && RCSwitch::timings[i+1] > delay*PROTOCOL3_0_LOW_CYCLES  - delayTolerance
      && RCSwitch::timings[i+1] < delay*PROTOCOL3_0_LOW_CYCLES  + delayTolerance) {
      code = code << 1;
    }
    else if (RCSwitch::timings[i]     > delay*PROTOCOL3_1_HIGH_CYCLES - delayTolerance
            && RCSwitch::timings[i]   < delay*PROTOCOL3_1_HIGH_CYCLES + delayTolerance
            && RCSwitch::timings[i+1] > delay*PROTOCOL3_1_LOW_CYCLES  - delayTolerance
            && RCSwitch::timings[i+1] < delay*PROTOCOL3_1_LOW_CYCLES  + delayTolerance) {
      code+=1;
      code = code << 1;
    }
    else {
      // Failed
      i = changeCount;
      code = 0;
    }
  }
  code = code >> 1;
  if (changeCount > 6) {    // ignore < 4bit values as there are no devices sending 4bit values => noise
    RCSwitch::nReceivedValue = code;
    RCSwitch::nReceivedBitlength = changeCount / 2;
    RCSwitch::nReceivedDelay = delay;
    RCSwitch::nReceivedProtocol = 3;
  }

  if (code == 0) {
    return false;
  }
  else if (code != 0) {
    return true;
  }
}

/// Reception interrupt handler.
void RCSwitch::handleInterrupt() {

  static unsigned int duration;
  static unsigned int changeCount;
  static unsigned long lastTime;
  static unsigned int repeatCount;
  

  long time = micros();
  duration = time - lastTime;
 
  if (duration > 5000 && duration > RCSwitch::timings[0] - 200 && duration < RCSwitch::timings[0] + 200) {
    repeatCount++;
    changeCount--;
    if (repeatCount == 2) {
      if (receiveProtocol1(changeCount) == false){
        if (receiveProtocol2(changeCount) == false){
          if (receiveProtocol3(changeCount) == false){
            //failed
          }
        }
      }
      repeatCount = 0;
    }
    changeCount = 0;
  } else if (duration > 5000) {
    changeCount = 0;
  }
 
  if (changeCount >= RCSWITCH_MAX_CHANGES) {
    changeCount = 0;
    repeatCount = 0;
  }
  RCSwitch::timings[changeCount++] = duration;
  lastTime = time;  
}
#endif

/// Converts a decimal value to its binary representation.
///
/// \param Dec The number (max 32 bits)
/// \param bitLength Number of input bits
///
/// \return An array of chars
char* RCSwitch::dec2binWzerofill(unsigned long Dec, unsigned int bitLength) {
  return dec2binWcharfill(Dec, bitLength, '0');
}

/// Converts a decimal value to its binary representation.
///
/// \param Dec The number (max 32 bits)
/// \param bitLength Number of input bits
/// \param fill Char used to fill
///
/// \return An array of chars
char* RCSwitch::dec2binWcharfill(unsigned long Dec, unsigned int bitLength, char fill) {
  static char bin[64];
  unsigned int i=0;

  while (Dec > 0) {
    bin[32+i++] = ((Dec & 1) > 0) ? '1' : fill;
    Dec = Dec >> 1;
  }

  for (unsigned int j = 0; j<bitLength; j++) {
    if (j >= bitLength - i) {
      bin[j] = bin[ 31 + i - (j - (bitLength - i)) ];
    }
    else {
      bin[j] = fill;
    }
  }
  bin[bitLength] = '\0';
  
  return bin;
}
