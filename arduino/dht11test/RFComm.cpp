/// \class RFComm

#include "RFComm.h"


/// Constructor.
///
/// \param pinData Pin corresponding to the RF transmitter
/// \param pinLed Pin connected to a LED for debug (-1 for nothing) 
RFComm::RFComm(int pinData, int pinLed) : 
  mPinData(pinData), mPinLed(pinLed), mProto(RFCPROTO_1), mRepeatSendTimes(10) {}

/// Sets up the instance for sending data (as opposed to receiving).
void RFComm::setupSend() {
  pinMode(mPinData, OUTPUT);
  if (mPinLed != -1) {
    pinMode(mPinLed, OUTPUT);
  }
}


/// Sets protocol.
///
/// \param proto Protocol, may be RFCPROTO_1, RFCPROTO_2 or RFCPROTO_3
void RFComm::setProto(unsigned int proto) {
  mProto = proto;
}


/// Sends data.
///
/// \param buf Buffer of bytes
/// \param len Length of buffer
void RFComm::send(uint8_t *buf, size_t len) {

  uint8_t x;
  const uint8_t mask = (1 << 7);

  for (unsigned int r=0; r<mRepeatSendTimes; r++) {

    for (size_t i=0; i<len; i++) {

      x = buf[i];
      for (uint8_t j=0; j<8; j++) {

        if ( x & mask ) {
          sendSymbol( sSymToPulses[mProto].symbols[RFCSYM_1], sSymToPulses[mProto].pulseLength_us );
        }
        else {
          sendSymbol( sSymToPulses[mProto].symbols[RFCSYM_0], sSymToPulses[mProto].pulseLength_us );
        }
        x = x << 1;

      }
    }
    sendSymbol( sSymToPulses[mProto].symbols[RFCSYM_SYNC], sSymToPulses[mProto].pulseLength_us );
  }

}


/// Sends a raw hi+lo ramp.
///
/// Shape:
///
/// ~~~
///  ------
/// |      |
///         ------
///  ^^^^^^ ^^^^^^
///    hi     lo
/// ~~~
///
/// \param sym Symbol to send (i.e., 0, 1, sync...)
/// \param pulseLen_us Pulse length, in us
void RFComm::sendSymbol(symbol_t &sym, unsigned int pulseLen_us) {

  digitalWrite(mPinData, HIGH);
  if (mPinLed != -1) {
    digitalWrite(mPinLed, HIGH);
  }
  delayMicroseconds( pulseLen_us * sym.hi );

  digitalWrite(mPinData, LOW);
  if (mPinLed != -1) {
    digitalWrite(mPinLed, LOW);
  }
  delayMicroseconds( pulseLen_us * sym.lo );

}


/// Sets some initial static variables.
///
/// Must be called when including this library.
void RFComm::init() {

  sSymToPulses[RFCPROTO_1].pulseLength_us = 350;
  sSymToPulses[RFCPROTO_1].symbols[RFCSYM_0]    = {  1,  3 };
  sSymToPulses[RFCPROTO_1].symbols[RFCSYM_1]    = {  3,  1 };
  sSymToPulses[RFCPROTO_1].symbols[RFCSYM_SYNC] = {  1, 31 };

  sSymToPulses[RFCPROTO_2].pulseLength_us = 650;
  sSymToPulses[RFCPROTO_2].symbols[RFCSYM_0]    = {  1,  2 };
  sSymToPulses[RFCPROTO_2].symbols[RFCSYM_1]    = {  2,  1 };
  sSymToPulses[RFCPROTO_2].symbols[RFCSYM_SYNC] = {  1, 10 };

  sSymToPulses[RFCPROTO_3].pulseLength_us = 100;
  sSymToPulses[RFCPROTO_3].symbols[RFCSYM_0]    = {  4, 11 };
  sSymToPulses[RFCPROTO_3].symbols[RFCSYM_1]    = {  9,  6 };
  sSymToPulses[RFCPROTO_3].symbols[RFCSYM_SYNC] = {  1, 71 };

}


proto_t RFComm::sSymToPulses[3] = {};
