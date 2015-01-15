/// \class RFComm
/// \brief RF send/receive for Arduino and Raspberry Pi
///
/// How to use it for sending on Arduino:
///
/// ~~~{.cpp}
/// #include "RFComm.h"
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
/// How to use it for receiving on a Raspberry Pi (**note:** requires
/// [wiringPi](http://wiringpi.com/) to work, and
/// [see here](See https://projects.drogon.net/raspberry-pi/wiringpi/pins/) for
/// PIN assignment):
///
/// ~~~{.cpp}
/// #include "RFComm.h"
///
/// int main(int argc, char *argv[]) {
/// 
///   wiringPiSetup();
///   RFComm::init();
///   RFComm rfRecv(2 /* pin rf */);
///   rfRecv.setupRecv();
/// 
///   const uint8_t *buf;
///   size_t len;
/// 
///   while (true) {
///     if ( (len = rfRecv.recv(&buf, &proto)) ) {
///       printf("%u bytes:", len);
///       for (size_t i=0; i<len; i++) {
///         printf(" %3u", buf[i]);
///       }
///       puts("");
///     }
///   }
/// 
///   return 0;
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
#include <cstdint>
#include <cstring>
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

/// Maximum string length supported when receiving, in bytes
#define RFCMAXBYTES 8
/// Maximum number of high/low or low/high changes to capture. It must accommodate: 1 bit (long
/// sync) + 2 * data_bits (high+low per bit) + 1 bit (short terminator, will be discarded). For 32
/// bits, 66 changes is the minimum allowed; for 64 bits, 130
#define RFCMAXCHANGES ( (RFCMAXBYTES)*8*2+1+1 )
/// States above this length (in µs) are considered sync signals
#define RFCMAXSYNC_US 5000
/// Tolerance on the sync signal length measurement (in µs)
#define RFCSYNCTOL_US 200
/// Number of consecutive sync signals to receive before extracting data. Larger values lead to a
/// greater sync precision between devices, but require emitter to repeat sends more often
#define RFCNSYNC 2
/// Pulse length tolerance, in percentage
#define RFCPULSETOL_PCT 50

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
    void setupSend(unsigned int proto = RFCPROTO_1);
    void setupRecv();
    void send(uint8_t *buf, size_t len);
    size_t recv(const uint8_t **buf, const proto_t **proto = NULL);

    static void init();

  private:

    int mPinData;  ///< Pin attached to the RF transmitter
    int mPinLed;   ///< Pin used for the debug LED (-1 if not connected)

    unsigned int mRepeatSendTimes;  ///< How many times a single data is sent
    unsigned int mProto;  ///< Protocol version (`RFCPROTO_1`, etc.)

    static uint8_t sRecvData[RFCMAXBYTES];  ///< Buffer of bytes for received data
    static size_t sRecvDataLen;  ///< Length, in bytes, of received data
    static proto_t *sRecvProto;  ///< Protocol used to receive latest data
    static proto_t sSymToPulses[3];  ///< Bit to pulse conversion for the different protocols
    static unsigned int sTimings_us[RFCMAXCHANGES];  ///< Raw hi and lo pulse lenghts to decode

    inline static bool isInRange(unsigned long value, unsigned long center, unsigned long sigma);

    void sendSymbol(symbol_t &sym, unsigned int pulseLen_us);

    static void recvIntHandler();
    static bool decodeProto(unsigned int numChanges, proto_t *proto);
};

#endif
