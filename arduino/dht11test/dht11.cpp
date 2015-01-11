/// \class dht11

#include "dht11.h"

/// Constructor.
///
/// \param _pin Digital PIN assigned to this DHT11
/// \param _hist_sz Number of history values to keep in memory
dht11::dht11(int _pin, unsigned int _hist_sz) :
  pin(_pin), hist_sz(_hist_sz), hist_idx(0), hist_nelm(0),
  temperature(DHT11_INVALID), humidity(DHT11_INVALID) {
  temp_hist = new float[hist_sz];
  humi_hist = new float[hist_sz];
}

/// Destructor.
dht11::~dht11() {
  delete[] temp_hist;
  delete[] humi_hist;
}

/// Calculates the moving average.
///
/// \return The moving average
///
/// \param vals An array of float
/// \param n Number of elements of the array to consider (must be > 0, unchecked)
float dht11::mavg(float *vals, unsigned int n) {
  float sum = 0.;
  for (unsigned int i=0; i<n; i++) {
    sum += vals[i];
  }
  return sum/n;
}

/// Returns the averaged values of temperature from history.
///
/// \return A float representing the temperature in °C, or `DHT11_INVALID` if history is empty.
float dht11::get_weighted_temperature() {
  if (hist_nelm == 0) {
    return DHT11_INVALID;
  }
  return mavg(temp_hist, hist_nelm);
}

/// Returns the averaged values of humidity from history.
///
/// \return A float representing the humidity in %, or `DHT11_INVALID` if history is empty.
float dht11::get_weighted_humidity() {
  if (hist_nelm == 0) {
    return DHT11_INVALID;
  }
  return mavg(humi_hist, hist_nelm);
}

/// Returns the last temperature value read.
///
/// \return An int representing the temperature in °C, or `DHT11_INVALID` if no value was ever read.
int dht11::get_last_temperature() {
  return temperature;
}

/// Returns the last humidity value read.
///
/// \return An int representing the humidity in %, or `DHT11_INVALID` if no value was ever read.
int dht11::get_last_humidity() {
  return humidity;
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

  uint8_t chksum = bits[0] + bits[2];  

  if (bits[4] != chksum) {
    // Checksums mismatch
    return DHT11_ERR_CKSUM;
  }

  // Everything OK: write output to the variables: bits[1] and bits[3] are always zero, therefore
  // they are omitted in formulas
  humidity = bits[0];
  temperature = bits[2];

  // Fill circular buffer for mobile average
  temp_hist[hist_idx] = (float)temperature;
  humi_hist[hist_idx] = (float)humidity;

  if (++hist_idx == hist_sz) {
    hist_idx = 0;
  }

  if (hist_nelm < hist_sz) {
    hist_nelm++;
  }

  return DHT11_OK;
}
