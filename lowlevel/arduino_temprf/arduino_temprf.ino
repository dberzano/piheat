// arduino_temprf: by Dario Berzano <dario.berzano@gmail.com>
// DHTlib: https://github.com/RobTillaart/Arduino/tree/master/libraries/DHTlib
//         (e475755)

#include "dht.h"
#include "RFComm.h"

#define DHT22_PIN 6
#define RF433_PIN 9
#define LED_PIN 13

dht dht22;
RFComm rf433 = RFComm(RF433_PIN, LED_PIN);

const char *dht_err(char n) {
  static const char *dht_err_txt[6] = { "DHTLIB_OK",
                                        "DHTLIB_ERROR_CHECKSUM",
                                        "DHTLIB_ERROR_TIMEOUT",
                                        "DHTLIB_ERROR_CONNECT",
                                        "DHTLIB_ERROR_ACK_L",
                                        "DHTLIB_ERROR_ACK_H" };
  static char dht_err_num[6] = { DHTLIB_OK,
                                 DHTLIB_ERROR_CHECKSUM,
                                 DHTLIB_ERROR_TIMEOUT,
                                 DHTLIB_ERROR_CONNECT,
                                 DHTLIB_ERROR_ACK_L,
                                 DHTLIB_ERROR_ACK_H };
  for (char i=0; i<6; i++) {
    if (dht_err_num[i] == n) return dht_err_txt[i];
  }
  return NULL;
}

void setup() {
  Serial.begin(115200);
  rf433.setupSend(rfProtoV2, 5);
  //pinMode(RF433_PIN, OUTPUT);
  Serial.println("arduino_temprf init");
}

void loop() {
  //Serial.println(">> loop");
  uint8_t rfdata[8];
  rfdata[0] = 1;
  rfdata[1] = 2;
  rfdata[2] = 3;
  rfdata[3] = 4;
  rfdata[4] = 5;
  rfdata[5] = 6;
  rfdata[6] = 7;
  rfdata[7] = 8;
  rf433.send(rfdata, 4);
  ////Serial.println("<< loop");
  delay(1000);

  //uint8_t hilen = 100;
  //uint8_t lolen = 200;
  //digitalWrite(RF433_PIN, HIGH);
  //delay(hilen);
  //digitalWrite(RF433_PIN, LOW);
  //delay(lolen);

}
