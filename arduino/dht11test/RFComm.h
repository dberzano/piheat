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
///   const size_t dataSize = 4;
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

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#define RFCPRINT(x) Serial.print(x);
#define RFCPRINTLN(x) Serial.println(x);
#else
#include <iostream>
#include <wiringPi.h>
#include <cstddef>
#include <stdint.h>
#define RFCPRINT(x) std::cout << (x);
#define RFCPRINTLN(x) std::cout << (x) << std::endl;
#endif

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

/// Maximum number of high/low or low/high changes to capture
#define RFCMAXCHANGES 67
/// States above this length (in µs) are considered sync signals
#define RFCMAXSYNC_US 5000
/// Tolerance on the sync signal length measurement (in µs)
#define RFCSYNCTOL_US 200
/// Number of consecutive sync signals to receive before extracting data. Larger values lead to a
/// greater sync precision between devices, but require emitter to repeat sends more often
#define RFCNSYNC 3

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

    RFComm(int pinData, int pinLed = -1);
    void setupSend();
    void setupRecv();
    void send(uint8_t *buf, size_t len);
    void setProto(unsigned int proto);

    static void init();

  private:

    int mPinData;  ///< Pin attached to the RF transmitter
    int mPinLed;   ///< Pin used for the debug LED (-1 if not connected)

    unsigned int mRepeatSendTimes;
    unsigned int mProto;

    static proto_t sSymToPulses[3];
    static unsigned int sTimings_us[RFCMAXCHANGES];

    void sendSymbol(symbol_t &sym, unsigned int pulseLen_us);

    static void recvIntHandler();
};

#endif
