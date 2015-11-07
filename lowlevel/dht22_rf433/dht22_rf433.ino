/// \file dht22_rf433.ino
/// \brief Sample Arduino Sketch for the dht22 and RFComm classes
///
/// Reads data from a DHT22 sensor and outputs it on the serial console. Outputs instant data every
/// second, plus a "moving average" of the last `MAVG_SIZE` values.
///
/// Digital pin for data can be configured with `PIN_DHT22`.
///
/// \author Dario Berzano


#include "dht22.h"
#include "RFComm.h"

/// Digital PIN of the Arduino connected to the DHT22
#define PIN_DHT22 3

/// Digital PIN of the Arduino connected to the RF transmitter
#define PIN_RF 4

/// Digital PIN of the Arduino connected to a LED synced with the RF transmitter
#define PIN_LED 13

/// Size of the moving average array
#define MAVG_SIZE 10

/// Unique identifier of this RF transmitter (8 bit max)
#define RF_UUID 34

dht22 DHT22(PIN_DHT22, MAVG_SIZE);
int count = 0;

RFComm rfSend = RFComm(PIN_RF, PIN_LED);

/// First function called of the sketch.
void setup() {
  Serial.begin(115200);
  Serial.println("BOOTING DHT22 TEST PROGRAM");

  // Init static things
  RFComm::init();

  // Setup instance for sending, and using protocol
  rfSend.setupSend(rfProtoV1, 20);
}

/// Main loop.
void loop() {

  int ans = DHT22.read();
  int avgt, avgtDec, avgh, avghDec;
  uint8_t rfdata[8];

  switch (ans) {

    case DHT22_OK:

      Serial.print("OK TEMP ");
      Serial.print( DHT22.get_last_temperature() );
      Serial.print("C HUMI ");
      Serial.print( DHT22.get_last_humidity() );
      Serial.println("%");

      avgt = DHT22.get_weighted_temperature(&avgtDec);
      Serial.print("AVG TEMP ");
      Serial.print(avgt);
      Serial.print("C");
      Serial.print(avgtDec);

      avgh = DHT22.get_weighted_humidity(&avghDec);
      Serial.print(" HUMI ");
      Serial.print(avgh);
      Serial.print("%");
      Serial.println(avghDec);

      //// Prepare output array (32 bits)
      //rfdata[0] = RF_UUID;  // uuid
      //rfdata[1] = (uint8_t)avgt;
      //rfdata[2] = (uint8_t)avgtDec;
      //rfdata[3] = (uint8_t)avgh;
      //rfdata[4] = (uint8_t)avghDec;
      //rfdata[5] = (uint8_t)( avgt + avgh );  // checksum
      //rfdata[6] = (uint8_t)( avgtDec + avghDec );  // checksum
      //rfdata[7] = 0;  // null

      //// Print out values to send (note: Arduino uses Little Endianness)
      //Serial.print("SENDING");
      //for (int i=0; i<8; i++) {
      //  Serial.print(" ");
      //  Serial.print(rfdata[i]);
      //}
      //Serial.println("");

      //// Carry out RF transmission (rfsize = number of bytes)
      //rfSend.send(rfdata, 8);

    break;

    case DHT22_ERR_CKSUM:
      Serial.println("ERR CHECKSUM");
    break;

    case DHT22_ERR_TMOUT:
      Serial.println("ERR TIMEOUT");
    break;

  }

  delay(5000);
}
