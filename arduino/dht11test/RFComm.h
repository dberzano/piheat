/// \class RFComm
/// \brief RF transmission for Arduino
///
/// Source of inspiration [is here](http://code.google.com/p/rc-switch/).
///
/// \author Dario Berzano

#ifndef RFComm_h
#define RFComm_h

#include "Arduino.h"

#define RFCPROTO_1 0
#define RFCPROTO_2 1
#define RFCPROTO_3 2

#define RFCSYM_0    0
#define RFCSYM_1    1
#define RFCSYM_SYNC 2

typedef struct {
  uint8_t hi;
  uint8_t lo;
} symbol_t;

typedef struct {
  unsigned int pulseLength_us;
  symbol_t symbols[3];
} proto_t;

class RFComm {

  public:

    RFComm(int _dataPin, int _ledPin = -1);
    void setupSend();
    void send(uint8_t *buf, unsigned int len);
    void setProto(unsigned int _proto);

    static void init();

  private:

    int dataPin;  ///< Pin attached to the RF transmitter
    int ledPin;   ///< Pin used for the debug led (-1 if not connected)

    unsigned int nRepeatSend = 10;
    unsigned int proto;

    static proto_t symToPulses[3];

    void sendSymbol(symbol_t &sym, unsigned int pulseLen_us);
    
};

#endif
