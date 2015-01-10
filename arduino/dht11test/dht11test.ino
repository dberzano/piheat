/// \file dht11test.ino
/// \brief DHT11 sample program for Arduino
///
/// Reads data from a DHT11 sensor and outputs it on the serial console. Outputs instant data every
/// second, plus a "moving average" of the last `MAVG_SIZE` values.
///
/// Digital pin for data can be configured with `PIN_DHT11`.
///
/// \author Dario Berzano


#include "dht11.h"

/// Digital PIN of the Arduino connected to the DHT11
#define PIN_DHT11 2

/// Size of the moving average array
#define MAVG_SIZE 10

dht11 DHT11(PIN_DHT11, MAVG_SIZE);
int count = 0;

/// First function called of the sketck.
void setup() {
  Serial.begin(115200);
  Serial.println("BOOTING DHT11 TEST PROGRAM");
}

/// Main loop.
void loop() {

  int ans = DHT11.read();

  switch (ans) {

    case DHT11_OK:

      Serial.print("OK TEMP ");
      Serial.print( DHT11.get_last_temperature() );
      Serial.print("C HUMI ");
      Serial.print( DHT11.get_last_humidity() );
      Serial.println("%");

      Serial.print("AVG TEMP ");
      Serial.print( DHT11.get_weighted_temperature(), 2 );
      Serial.print("C HUMI ");
      Serial.print( DHT11.get_weighted_humidity(), 2 );
      Serial.println("%");

    break;

    case DHT11_ERR_CKSUM:
      Serial.println("ERR CHECKSUM");
    break;

    case DHT11_ERR_TMOUT:
      Serial.println("ERR TIMEOUT");
    break;

  }

  delay(1000);
}
