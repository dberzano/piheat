/// \class dht11

#include "dht11.h"

dht11::dht11(int _pin) : pin(_pin) {}

int dht11::read() {

  // Receive buffer
  uint8_t bits[5];
  uint8_t cnt = 7;
  uint8_t idx = 0;

  // Zero buffer
  for (int i=0; i<5; i++) {
    bits[i] = 0;
  }

  // Request sample
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  delay(18);
  digitalWrite(pin, HIGH);
  delayMicroseconds(40);
  pinMode(pin, INPUT);

  // Acknowledge or timeout?
  unsigned int loopCnt = 10000;
  while (digitalRead(pin) == LOW) {
    if (loopCnt-- == 0) {
      return DHT11_ERR_TMOUT;
    }
  }

  loopCnt = 10000;
  while (digitalRead(pin) == HIGH) {
    if (loopCnt-- == 0) {
      return DHT11_ERR_TMOUT;
    }
  }

  // Read output: 40 bits (5 bytes) or timeout
  for (int i=0; i<40; i++) {

    loopCnt = 10000;
    while (digitalRead(pin) == LOW) {
      if (loopCnt-- == 0) {
        return DHT11_ERR_TMOUT;
      }
    }

    unsigned long t = micros();

    loopCnt = 10000;
    while (digitalRead(pin) == HIGH) {
      if (loopCnt-- == 0) {
        return DHT11_ERR_TMOUT;
      }
    }

    if ((micros() - t) > 40) {
      bits[idx] |= (1 << cnt);
    }

    if (cnt == 0) {
      // Next byte?
      cnt = 7;  // Restart at MSB
      idx++;    // Next byte!
    }
    else {
      cnt--;
    }

  }

  // Write output to the variables: bits[1] and bits[3] are always zero, therefore they are omitted
  // in formulas
  humidity = bits[0]; 
  temperature = bits[2]; 

  uint8_t chksum = bits[0] + bits[2];  

  if (bits[4] != chksum) {
    // Checksums mismatch
    return DHT11_ERR_CKSUM;
  }

  return DHT11_OK;
}
