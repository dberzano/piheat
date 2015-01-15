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

/// Sets up the instance for receiving data (as opposed to sending).
void RFComm::setupRecv() {
  pinMode(mPinData, INPUT);
  if (mPinLed != -1) {
    pinMode(mPinLed, OUTPUT);
  }

  #ifdef ARDUINO
  attachInterrupt(mPinData, recvIntHandler, CHANGE);
  #else
  wiringPiISR(mPinData, INT_EDGE_BOTH, &recvIntHandler);
  #endif
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

/// Sets some initial static variables. Pulse lengths are used when sending, but
/// they are automatically detected when receiving.
///
/// This function **must** be called before starting using this library.
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

/// Check if a value is within a symmetric range centered somewhere.
///
/// \param value Value to check
/// \param center Center value
/// \param sigma Maximum positive or negative distance from center tolerated
///
/// \return true if value is within range, false otherwise
bool RFComm::isInRange(unsigned long value, unsigned long center, unsigned long sigma) {
  return ( value > center-sigma && value < center+sigma );
}

/// Tries to decode a bit string using the given protocol.
///
/// \return true on success, false on failure
bool RFComm::decodeProto(unsigned int numChanges, proto_t *proto) {

  unsigned long refLen_us = sTimings_us[0] / proto->symbols[RFCSYM_SYNC].lo;
  unsigned long refLenTol_us = refLen_us * RFCPULSETOL_PCT / 100;

  unsigned long sym1hi_us = refLen_us * proto->symbols[RFCSYM_1].hi;
  unsigned long sym1lo_us = refLen_us * proto->symbols[RFCSYM_1].lo;
  unsigned long sym0hi_us = refLen_us * proto->symbols[RFCSYM_0].hi;
  unsigned long sym0lo_us = refLen_us * proto->symbols[RFCSYM_0].lo;

  static uint8_t bufRecv[RFCMAXBYTES];
  size_t countBufRecv = 1;

  int idxRecv;

  uint8_t *curByte = bufRecv;
  *curByte = 0;

  for (idxRecv=1; idxRecv<numChanges; idxRecv+=2) {

    if ( isInRange(sTimings_us[idxRecv], sym1hi_us, refLenTol_us) &&
         isInRange(sTimings_us[idxRecv+1], sym1lo_us, refLenTol_us) )
    {
      *curByte = (*curByte << 1) + 1;
    }
    else if ( isInRange(sTimings_us[idxRecv], sym0hi_us, refLenTol_us) &&
              isInRange(sTimings_us[idxRecv+1], sym0lo_us, refLenTol_us) )
    {
      *curByte = *curByte << 1;
    }
    else {
      // Cannot continue decoding: keep what we have obtained so far
      idxRecv -= 2;
      break;
    }

    if ((idxRecv+1) % 16 == 0) {
      curByte = &bufRecv[countBufRecv++];
      *curByte = 0;
    }

  }

  if (idxRecv < 15) {
    // Ignore strings < 1 byte (noise)
    sRecvDataLen = 0;
    return false;
  }

  // Adjust buffer count (value is in bytes)
  countBufRecv = (idxRecv+14)/16;

  // All OK: transfer data to the static buffers
  memcpy(sRecvData, bufRecv, countBufRecv);
  sRecvDataLen = countBufRecv;
  return true;
}

/// Interrupt handler. Remember: this is a static function!
void RFComm::recvIntHandler() { 

  static unsigned int duration_us;
  static unsigned int countChanges;
  static unsigned long lastTime_us;
  static unsigned int countSyncSignals;

  long time_us = micros();
  duration_us = time_us - lastTime_us;

  if (duration_us > RFCMAXSYNC_US) {

    if (duration_us > sTimings_us[0] - RFCSYNCTOL_US && 
        duration_us < sTimings_us[0] + RFCSYNCTOL_US) {

      // This signal is long and it is similar in length to the first signal: consider it a sync

      if (++countSyncSignals == RFCNSYNC) {
        countChanges--;  // skip the short "hi" before this long "lo"

        // decode data here //
        decodeProto(countChanges, &sSymToPulses[RFCPROTO_1]);

        countSyncSignals = 0;
      }

    }

    // Reset: start collecting data from scratch at next interrupt
    countChanges = 0;
  }
  else if (countChanges >= RFCMAXCHANGES) {
    // Too much data, maybe just garbage? Start over
    countChanges = 0;
    countSyncSignals = 0;
  }

  sTimings_us[countChanges++] = duration_us;
  lastTime_us = time_us;
}

uint8_t RFComm::sRecvData[RFCMAXBYTES] = {};
size_t RFComm::sRecvDataLen = 0;
proto_t RFComm::sSymToPulses[3] = {};
unsigned int RFComm::sTimings_us[RFCMAXCHANGES];
