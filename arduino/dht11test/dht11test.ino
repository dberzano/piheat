// dht11lib from here: http://playground.arduino.cc/main/DHT11Lib

#include "dht11.h"

#define PIN_DHT11 2
#define MAVG_SIZE 10
#define MAVG_IGNORE -9999

dht11 DHT11;
float temp_history[MAVG_SIZE];
float humi_history[MAVG_SIZE];
int history_idx = 0;
int count = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("BOOTING DHT11 TEST PROGRAM");
  for (int i=0; i<MAVG_SIZE; i++) {
    temp_history[i] = MAVG_IGNORE;
    humi_history[i] = MAVG_IGNORE;
  }
}

float mavg(float *vals, unsigned int size) {
  float sum = 0.;
  float n = 0;
  for (unsigned int i=0; i<size; i++) {
    if (vals[i] != MAVG_IGNORE) {
      sum += vals[i];
      n++;
    }
  }
  return sum/n;
}

void loop() {

  //Serial.println("ACQUIRING");
  int ans = DHT11.read(PIN_DHT11);

  float temp, humidity;

  switch (ans) {
    case DHT11_OK:

      temp_history[history_idx] = DHT11.temperature;  // °C
      humi_history[history_idx] = DHT11.humidity;  // %

      Serial.print("OK TEMP ");
      Serial.print( temp_history[history_idx], 0 );
      Serial.print("C HUMI ");
      Serial.print( humi_history[history_idx], 0 );
      Serial.println("%");

      history_idx = (history_idx + 1) % MAVG_SIZE;

    break;

    case DHT11_ERR_CKSUM:
      Serial.println("ERR CHECKSUM");
    break;

    case DHT11_ERR_TMOUT:
      Serial.println("ERR TIMEOUT");
    break;

    default:
      Serial.println("ERR UNKNOWN");
    break;
  }

  if (++count == MAVG_SIZE) {
    Serial.print("MAVG TEMP ");
    Serial.print( mavg(temp_history, MAVG_SIZE) );
    Serial.print("C HUMI ");
    Serial.print( mavg(humi_history, MAVG_SIZE) );
    Serial.println("%");

    count = 0;
  }

  delay(1000);
}
