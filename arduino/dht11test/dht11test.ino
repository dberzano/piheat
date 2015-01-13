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
  unsigned long rfbuf = 0;
  uint8_t *rfdata = (uint8_t *)&rfbuf;

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

      // Send 24 bits. Array is actually an unsigned long (32 bits). Since Arduino is Little-Endian,
      // this is the byte order:
      // rfdata[4] rfdata[2] rfdata[1] rfdata[0]
      rfdata[0] = 123;
      rfdata[1] = (uint8_t)DHT11.get_weighted_temperature();
      rfdata[2] = (uint8_t)DHT11.get_weighted_humidity();

      // Print out the values to send (for debug)
      for (int i=0; i<3; i++) {
        Serial.print(i);
        Serial.print(":");
        Serial.print(rfdata[i]);
        Serial.print(" ");
      }
      Serial.println(rfbuf);

      // Carry out RF transmission (3 = number of bytes)
      rfSend.send( rfdata, 3 );

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
