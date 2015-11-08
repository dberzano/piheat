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
  rf433.setupSend(rfProtoV1, 3);
  Serial.println("arduino_temprf init");
}

void loop() {
  Serial.println(">> loop");
  int chk = dht22.read22(DHT22_PIN);
  uint8_t rfdata[8];
  Serial.print("status: ");
  Serial.println(dht_err(chk));
  if (chk == DHTLIB_OK) {
    Serial.print("temp: ");
    Serial.println(dht22.temperature);
    Serial.print("humidity: ");
    Serial.println(dht22.humidity);
  }
  rfdata[0] = 1;
  rfdata[1] = 2;
  rfdata[2] = 3;
  rfdata[3] = 4;
  rfdata[4] = 5;
  rfdata[5] = 6;
  rfdata[6] = 7;
  rfdata[7] = 8;
  rf433.send(rfdata, 8);
  Serial.println("<< loop");
  delay(2000);
}
