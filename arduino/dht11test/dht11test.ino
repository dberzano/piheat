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
#include "RFComm.h"

/// Digital PIN of the Arduino connected to the DHT11
#define PIN_DHT11 2

/// Digital PIN of the Arduino connected to the RF transmitter
#define PIN_RF 4

/// Digital PIN of the Arduino connected to a LED synced with the RF transmitter
#define PIN_LED 13

/// Size of the moving average array
#define MAVG_SIZE 10

dht11 DHT11(PIN_DHT11, MAVG_SIZE);
int count = 0;

RFComm rfSend = RFComm(PIN_RF, PIN_LED);

/// First function called of the sketck.
void setup() {
  Serial.begin(115200);
  Serial.println("BOOTING DHT11 TEST PROGRAM");

  // Init static things
  RFComm::init();

  // Setup RF pin (and debug LED) for sending data
  rfSend.setupSend();
}

/// Main loop.
void loop() {

  int ans = DHT11.read();
  int avgt, avgtDec, avgh, avghDec;
  unsigned long rfulong;
  uint8_t *rfdata = (uint8_t *)&rfulong;

  switch (ans) {

    case DHT11_OK:

      Serial.print("OK TEMP ");
      Serial.print( DHT11.get_last_temperature() );
      Serial.print("C HUMI ");
      Serial.print( DHT11.get_last_humidity() );
      Serial.println("%");

      avgt = DHT11.get_weighted_temperature(&avgtDec);
      Serial.print("AVG TEMP ");
      Serial.print(avgt);
      Serial.print("C");
      Serial.print(avgtDec);

      avgh = DHT11.get_weighted_humidity(&avghDec);
      Serial.print(" HUMI ");
      Serial.print(avgh);
      Serial.print("%");
      Serial.println(avghDec);

      // Prepare output array (32 bits)
      rfdata[0] = 123;
      rfdata[1] = (uint8_t)avgt;
      rfdata[2] = (uint8_t)avgtDec;
      rfdata[3] = (uint8_t)avgh;

      // Print out values to send as a 32 bit integer (remember: Arduino is Little Endian)
      Serial.print("SENDING ");
      Serial.println(rfulong);

      // Carry out RF transmission (rfsize = number of bytes)
      rfSend.send(rfdata, sizeof(rfulong));

    break;

    case DHT11_ERR_CKSUM:
      Serial.println("ERR CHECKSUM");
    break;

    case DHT11_ERR_TMOUT:
      Serial.println("ERR TIMEOUT");
    break;

  }

  delay(5000);
}
