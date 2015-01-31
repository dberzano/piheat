/// \class RFComm

#include "RFComm.h"

/// Constructor.
///
/// \param pinData Pin corresponding to the RF transmitter
/// \param pinLed Pin connected to a LED for debug (-1 for nothing) 
RFComm::RFComm(int pinData, int pinLed) : 
  mPinData(pinData), mPinLed(pinLed), mRepeatSendTimes(10), mProto(rfProtoV1) {}

/// Sets up the instance for sending data (as opposed to receiving).
///
/// \param proto Protocol version
/// \param repeat How many times should the signal be sent per burst
void RFComm::setupSend(protoid_t proto, unsigned int repeat) {
  mProto = proto;
  mRepeatSendTimes = repeat;
  pinMode(mPinData, OUTPUT);
  if (mPinLed != -1) {
    pinMode(mPinLed, OUTPUT);
  }
}

/// Sets up the instance for receiving data (as opposed to sending).
///
/// \param callback Function to call when done receiving a meaningful string (can be NULL)
void RFComm::setupRecv( void (*callback)(size_t len, uint8_t *data, protoid_t protoid) ) {
  pinMode(mPinData, INPUT);
  if (mPinLed != -1) {
    pinMode(mPinLed, OUTPUT);
  }
  sRecvCallback = callback;  // can be null
  #ifdef ARDUINO
  attachInterrupt(mPinData, recvIntHandler, CHANGE);
  #else
  wiringPiISR(mPinData, INT_EDGE_BOTH, &recvIntHandler);
  #endif
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
          sendSymbol( sSymToPulses[mProto].symbols[rfSym1], sSymToPulses[mProto].pulseLength_us );
        }
        else {
          sendSymbol( sSymToPulses[mProto].symbols[rfSym0], sSymToPulses[mProto].pulseLength_us );
        }
        x = x << 1;

      }
    }
    sendSymbol( sSymToPulses[mProto].symbols[rfSymSync], sSymToPulses[mProto].pulseLength_us );
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

  sSymToPulses[rfProtoV1].pulseLength_us = 350;
  sSymToPulses[rfProtoV1].symbols[rfSym0]    = {  1,  3 };
  sSymToPulses[rfProtoV1].symbols[rfSym1]    = {  3,  1 };
  sSymToPulses[rfProtoV1].symbols[rfSymSync] = {  1, 31 };

  sSymToPulses[rfProtoV2].pulseLength_us = 650;
  sSymToPulses[rfProtoV2].symbols[rfSym0]    = {  1,  2 };
  sSymToPulses[rfProtoV2].symbols[rfSym1]    = {  2,  1 };
  sSymToPulses[rfProtoV2].symbols[rfSymSync] = {  1, 10 };

  sSymToPulses[rfProtoV3].pulseLength_us = 100;
  sSymToPulses[rfProtoV3].symbols[rfSym0]    = {  4, 11 };
  sSymToPulses[rfProtoV3].symbols[rfSym1]    = {  9,  6 };
  sSymToPulses[rfProtoV3].symbols[rfSymSync] = {  1, 71 };

}

/// Gets received data. Pointer to data destination must be provided. Subsequent calls will return
/// zero until fresh data is available.
///
/// \param buf Function will store there a pointer to data
/// \param proto A protocol struct is returned there
///
/// \return Number of available bytes
size_t RFComm::recv(const uint8_t **buf, protoid_t *protoid) {
  *buf = sRecvData;
  size_t bufLen = sRecvDataLen;
  sRecvDataLen = 0;  // reset read!
  if (protoid != NULL) {
    *protoid = sRecvProto;
  }
  return bufLen;
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
bool RFComm::decodeProto(unsigned int numChanges, protoid_t protoid) {

  proto_t *proto = &sSymToPulses[protoid];

  unsigned long refLen_us = sTimings_us[0] / proto->symbols[rfSymSync].lo;
  unsigned long refLenTol_us = refLen_us * RFCPULSETOL_PCT / 100;

  unsigned long sym1hi_us = refLen_us * proto->symbols[rfSym1].hi;
  unsigned long sym1lo_us = refLen_us * proto->symbols[rfSym1].lo;
  unsigned long sym0hi_us = refLen_us * proto->symbols[rfSym0].hi;
  unsigned long sym0lo_us = refLen_us * proto->symbols[rfSym0].lo;

  static uint8_t bufRecv[RFCMAXBYTES];
  size_t countBufRecv = 1;

  size_t idxRecv;

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
      if (idxRecv >= 2) {
        idxRecv -= 2;
      }
      else {
        idxRecv = 0;
      }
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
  sRecvProto = protoid;
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

        bool recvOk = true;

        if ( !decodeProto(countChanges, rfProtoV1) ) {
          if ( !decodeProto(countChanges, rfProtoV2) ) {
            if ( !decodeProto(countChanges, rfProtoV3) ) {
              // No suitable protocol found
              recvOk = false;
            }
          }
        }

        if (recvOk && sRecvCallback != NULL) {
          // Call the callback function, if any
          sRecvCallback(sRecvDataLen, sRecvData, sRecvProto);
        }

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
protoid_t RFComm::sRecvProto = rfProtoV1;
proto_t RFComm::sSymToPulses[3] = {};
unsigned int RFComm::sTimings_us[RFCMAXCHANGES];
void (*RFComm::sRecvCallback)(size_t len, uint8_t *data, protoid_t protoid) = NULL;
