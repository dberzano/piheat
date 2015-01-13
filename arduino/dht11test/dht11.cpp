/// \class dht11

#include "dht11.h"

/// Constructor.
///
/// \param pin Digital PIN assigned to this DHT11
/// \param histSz Number of history values to keep in memory
dht11::dht11(int pin, size_t histSz) :
  mPin(pin), mHistSz(histSz), mHistIdx(0), mHistNelm(0),
  mTemperature(DHT11_INVALID), mHumidity(DHT11_INVALID) {
  mHistTemp = new int[mHistSz];
  mHistHumi = new int[mHistSz];
}

/// Destructor.
dht11::~dht11() {
  delete[] mHistTemp;
  delete[] mHistHumi;
}

/// Calculates the arithmetic average of the input array.
///
/// \return Integer part of average: use `avgDec` for getting decimals
///
/// \param vals An array of int
/// \param n Number of elements of the array to consider (must be > 0, unchecked)
/// \param avgDec Pointer where to write the decimal part of the average
int dht11::avg(int *vals, size_t n, int *avgDec) {
  int buf = 0.;
  int avgInt;
  for (size_t i=0; i<n; i++) {
    buf += vals[i];
  }
  buf *= 100;
  buf = buf/n;  // avg*100
  avgInt = buf/100;
  *avgDec = buf - (100*avgInt);
  return avgInt;
}

/// Returns the averaged values of temperature from history.
///
/// \return An int representing the temperature in °C, or `DHT11_INVALID` if history is empty.
///
/// \param avgDec Pointer where to write the decimal part of the average
int dht11::get_weighted_temperature(int *avgDec) {
  if (mHistNelm == 0) {
    return DHT11_INVALID;
  }
  return avg(mHistTemp, mHistNelm, avgDec);
}

/// Returns the averaged values of humidity from history.
///
/// \return An int representing the humidity in %, or `DHT11_INVALID` if history is empty.
///
/// \param avgDec Pointer where to write the decimal part of the average
int dht11::get_weighted_humidity(int *avgDec) {
  if (mHistNelm == 0) {
    return DHT11_INVALID;
  }
  return avg(mHistHumi, mHistNelm, avgDec);
}

/// Returns the last temperature value read.
///
/// \return An int representing the temperature in °C, or `DHT11_INVALID` if no value was ever read.
int dht11::get_last_temperature() {
  return mTemperature;
}

/// Returns the last humidity value read.
///
/// \return An int representing the humidity in %, or `DHT11_INVALID` if no value was ever read.
int dht11::get_last_humidity() {
  return mHumidity;
}

/// Reads data from the DHT11.
///
/// On a successful read, values are stored in public members `temperature` and `humidity`. Plus,
/// they are inserted in an array for computing the moving average for a weighted output.
///
/// On an unsuccessful read, `temperature`, `humidity` and the historic values are left untouched,
/// *i.e.* the values obtained from the last successful read are stored.
///
/// \return `DHT11_OK` when successful, one of the `DHT11_ERR_*` values on error.
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
  pinMode(mPin, OUTPUT);
  digitalWrite(mPin, LOW);
  delay(18);
  digitalWrite(mPin, HIGH);
  delayMicroseconds(40);
  pinMode(mPin, INPUT);

  // Acknowledge or timeout?
  unsigned int loopCnt = 10000;
  while (digitalRead(mPin) == LOW) {
    if (loopCnt-- == 0) {
      return DHT11_ERR_TMOUT;
    }
  }

  loopCnt = 10000;
  while (digitalRead(mPin) == HIGH) {
    if (loopCnt-- == 0) {
      return DHT11_ERR_TMOUT;
    }
  }

  // Read output: 40 bits (5 bytes) or timeout
  for (int i=0; i<40; i++) {

    loopCnt = 10000;
    while (digitalRead(mPin) == LOW) {
      if (loopCnt-- == 0) {
        return DHT11_ERR_TMOUT;
      }
    }

    unsigned long t = micros();

    loopCnt = 10000;
    while (digitalRead(mPin) == HIGH) {
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

  uint8_t chksum = bits[0] + bits[2];  

  if (bits[4] != chksum) {
    // Checksums mismatch
    return DHT11_ERR_CKSUM;
  }

  // Everything OK: write output to the variables: bits[1] and bits[3] are always zero, therefore
  // they are omitted in formulas
  mHumidity = bits[0];
  mTemperature = bits[2];

  // Fill circular buffer for mobile average
  mHistTemp[mHistIdx] = mTemperature;
  mHistHumi[mHistIdx] = mHumidity;

  if (++mHistIdx == mHistSz) {
    mHistIdx = 0;
  }

  if (mHistNelm < mHistSz) {
    mHistNelm++;
  }

  return DHT11_OK;
}
