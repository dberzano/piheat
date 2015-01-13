/// \class RFComm

#include "RFComm.h"
#include "Arduino.h"


/// Constructor.
///
/// \param _dataPin Pin corresponding to the RF transmitter
/// \param _ledPin Pin connected to a LED for debug (-1 for nothing) 
RFComm::RFComm(int _dataPin, int _ledPin) : 
  dataPin(_dataPin), ledPin(_ledPin), proto(RFCPROTO_1) {}

/// Sets up the instance for sending data (as opposed to receiving).
void RFComm::setupSend() {
  pinMode(dataPin, OUTPUT);
  if (ledPin != -1) {
    pinMode(ledPin, OUTPUT);
  }
}


/// Sets protocol.
///
/// \param proto Protocol, may be RFCPROTO_1, RFCPROTO_2 or RFCPROTO_3
void RFComm::setProto(unsigned int _proto) {
  proto = _proto;
}


/// Sends data.
///
/// \param buf Buffer of bytes
/// \param len Length of buffer
void RFComm::send(uint8_t *buf, unsigned int len) {

  uint8_t x;
  const uint8_t mask = (1 << 7);

  for (unsigned int r=0; r<nRepeatSend; r++) {

    for (unsigned int i=0; i<len; i++) {

      x = buf[i];
      for (unsigned int j=0; j<8; j++) {

        if ( x & mask ) {
          sendSymbol( symToPulses[proto].symbols[RFCSYM_1], symToPulses[proto].pulseLength_us );
        }
        else {
          sendSymbol( symToPulses[proto].symbols[RFCSYM_0], symToPulses[proto].pulseLength_us );
        }
        x = x << 1;

      }
    }
    sendSymbol( symToPulses[proto].symbols[RFCSYM_SYNC], symToPulses[proto].pulseLength_us );
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

  uint8_t i;

  digitalWrite(dataPin, HIGH);
  if (ledPin != -1) {
    digitalWrite(ledPin, HIGH);
  }
  delayMicroseconds( pulseLen_us * sym.hi );

  digitalWrite(dataPin, LOW);
  if (ledPin != -1) {
    digitalWrite(ledPin, LOW);
  }
  delayMicroseconds( pulseLen_us * sym.lo );

}


/// Sets some initial static variables.
///
/// Must be called when including this library.
void RFComm::init() {

  symToPulses[RFCPROTO_1].pulseLength_us = 350;
  symToPulses[RFCPROTO_1].symbols[RFCSYM_0]    = {  1,  3 };
  symToPulses[RFCPROTO_1].symbols[RFCSYM_1]    = {  3,  1 };
  symToPulses[RFCPROTO_1].symbols[RFCSYM_SYNC] = {  1, 31 };

  symToPulses[RFCPROTO_2].pulseLength_us = 650;
  symToPulses[RFCPROTO_2].symbols[RFCSYM_0]    = {  1,  2 };
  symToPulses[RFCPROTO_2].symbols[RFCSYM_1]    = {  2,  1 };
  symToPulses[RFCPROTO_2].symbols[RFCSYM_SYNC] = {  1, 10 };

  symToPulses[RFCPROTO_3].pulseLength_us = 100;
  symToPulses[RFCPROTO_3].symbols[RFCSYM_0]    = {  4, 11 };
  symToPulses[RFCPROTO_3].symbols[RFCSYM_1]    = {  9,  6 };
  symToPulses[RFCPROTO_3].symbols[RFCSYM_SYNC] = {  1, 71 };

}


proto_t RFComm::symToPulses[3] = {};
