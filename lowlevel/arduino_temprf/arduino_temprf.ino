// arduino_temprf: by Dario Berzano <dario.berzano@gmail.com>
// DHTlib: https://github.com/RobTillaart/Arduino/tree/master/libraries/DHTlib
//         (e475755)

#include "dht.h"
#define DHT22_PIN 6

dht dht22;

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
  Serial.println("arduino_temprf init");
}

void loop() {
  Serial.println(">> loop");
  int chk = dht22.read22(DHT22_PIN);
  Serial.print("status: ");
  Serial.println(dht_err(chk));
  if (chk == DHTLIB_OK) {
    Serial.print("temp: ");
    Serial.println(dht22.temperature);
    Serial.print("humidity: ");
    Serial.println(dht22.humidity);
  }
  Serial.println("<< loop");
  delay(2000);
}
