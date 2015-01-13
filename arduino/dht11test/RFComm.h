/// \class RFComm
/// \brief RF transmission for Arduino
///
/// How to use it:
///
/// ~~~{.cpp}
/// #include "RCSwitch.h"
///
/// RFComm rfSend(4 /* pin rf */, 13 /* pin led */);
///
/// void setup() {
///   RFComm::init();
///   rfSend.setupSend();
/// }
///
/// void loop() {
///   const unsigned int dataSize = 4;
///   uint8_t data[dataSize];
///   /* fill data[] */
///   rfSend.send(data, dataSize);
/// }
/// ~~~
///
/// Source of inspiration [is here](http://code.google.com/p/rc-switch/).
///
/// \author Dario Berzano

#ifndef RFComm_h
#define RFComm_h

#include "Arduino.h"

/// Protocol 1
#define RFCPROTO_1 0
/// Protocol 2
#define RFCPROTO_2 1
/// Protocol 3
#define RFCPROTO_3 2

/// Symbol for bit 0
#define RFCSYM_0    0
/// Symbol for bit 1
#define RFCSYM_1    1
/// Symbol for sync
#define RFCSYM_SYNC 2

/// A symbol: a sequence of variable length high and low signals.
///
/// Signal lengths are relative to a pulse length.
typedef struct {
  uint8_t hi;  ///< Relative duration of the high pulse
  uint8_t lo;  ///< Relative duration of the low pulse
} symbol_t;

/// A protocol definition
typedef struct {
  unsigned int pulseLength_us;  ///< Pulse length, in µs
  symbol_t symbols[3];          ///< Symbol shapes
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
