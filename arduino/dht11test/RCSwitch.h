/// \class RCSwitch
/// \brief Arduino libary for remote control outlet switches
///
/// Original code [from here](http://code.google.com/p/rc-switch/).
///
/// \copyright Copyright (c) 2011 Suat Ozgur.  All right reserved.
///
/// \author Suat Özgür (main author)
/// \author Andre Koehler / info(at)tomate-online(dot)de
/// \author Gordeev Andrey Vladimirovich / gordeev(at)openpyro(dot)com
/// \author Skineffect / http://forum.ardumote.com/viewtopic.php?f=2&t=46
/// \author Dominik Fischer / dom_fischer(at)web(dot)de
/// \author Andreas Steinel / A.(lastname)(at)gmail(dot)com
/// \author Frank Oltmanns / (first name).(last name)(at)gmail(dot)com

#ifndef RCSwitch_h
#define RCSwitch_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#elif defined(ENERGIA) // LaunchPad, FraunchPad and StellarPad specific
#include "Energia.h"	
#else
#include "WProgram.h"
#endif

// At least for the ATTiny X4/X5, receiving has to be disabled due to missing libm depencies
// (udivmodhi4)
#if defined( __AVR_ATtinyX5__ ) or defined ( __AVR_ATtinyX4__ )
#define RCSwitchDisableReceiving
#endif

/// Number of maximum High/Low changes per packet: we can handle up to (unsigned long) =>
/// 32 bit * 2 H/L changes per bit + 2 for sync
#define RCSWITCH_MAX_CHANGES 67

#define PROTOCOL3_SYNC_FACTOR   71
#define PROTOCOL3_0_HIGH_CYCLES  4
#define PROTOCOL3_0_LOW_CYCLES  11
#define PROTOCOL3_1_HIGH_CYCLES  9
#define PROTOCOL3_1_LOW_CYCLES   6

class RCSwitch {

  public:

    RCSwitch(int _nLedDebugPin = -1);
    
    void switchOn(int nGroupNumber, int nSwitchNumber);
    void switchOff(int nGroupNumber, int nSwitchNumber);
    void switchOn(char* sGroup, int nSwitchNumber);
    void switchOff(char* sGroup, int nSwitchNumber);
    void switchOn(char sFamily, int nGroup, int nDevice);
    void switchOff(char sFamily, int nGroup, int nDevice);
    void switchOn(char* sGroup, char* sDevice);
    void switchOff(char* sGroup, char* sDevice);
    void switchOn(char sGroup, int nDevice);
    void switchOff(char sGroup, int nDevice);

    void sendTriState(char* Code);
    void send(unsigned long Code, unsigned int length);
    void send(char* Code);
    
    #if not defined( RCSwitchDisableReceiving )
    void enableReceive(int interrupt);
    void enableReceive();
    void disableReceive();
    bool available();
    void resetAvailable();
	
    unsigned long getReceivedValue();
    unsigned int getReceivedBitlength();
    unsigned int getReceivedDelay();
    unsigned int getReceivedProtocol();
    unsigned int* getReceivedRawdata();
    #endif
  
    void enableTransmit(int nTransmitterPin);
    void disableTransmit();
    void setPulseLength(int nPulseLength);
    void setRepeatTransmit(int nRepeatTransmit);

    #if not defined( RCSwitchDisableReceiving )
    void setReceiveTolerance(int nPercent);
    #endif

    void setProtocol(int nProtocol);
    void setProtocol(int nProtocol, int nPulseLength);
  
  private:

    char* getCodeWordB(int nGroupNumber, int nSwitchNumber, boolean bStatus);
    char* getCodeWordA(char* sGroup, int nSwitchNumber, boolean bStatus);
    char* getCodeWordA(char* sGroup, char* sDevice, boolean bStatus);
    char* getCodeWordC(char sFamily, int nGroup, int nDevice, boolean bStatus);
    char* getCodeWordD(char group, int nDevice, boolean bStatus);
    void sendT0();
    void sendT1();
    void sendTF();
    void send0();
    void send1();
    void sendSync();
    void transmit(int nHighPulses, int nLowPulses);

    static char* dec2binWzerofill(unsigned long dec, unsigned int length);
    static char* dec2binWcharfill(unsigned long dec, unsigned int length, char fill);
    
    #if not defined( RCSwitchDisableReceiving )
    static void handleInterrupt();
    static bool receiveProtocol1(unsigned int changeCount);
    static bool receiveProtocol2(unsigned int changeCount);
    static bool receiveProtocol3(unsigned int changeCount);
    int nReceiverInterrupt;
    #endif

    int nTransmitterPin;
    int nLedDebugPin;
    int nPulseLength;
    int nRepeatTransmit;
    char nProtocol;

    #if not defined( RCSwitchDisableReceiving )
    static int nReceiveTolerance;
    static unsigned long nReceivedValue;
    static unsigned int nReceivedBitlength;
    static unsigned int nReceivedDelay;
    static unsigned int nReceivedProtocol;
    #endif

    /// Contains sync timing, followed by a number of bits
    static unsigned int timings[RCSWITCH_MAX_CHANGES];
    
};

#endif
