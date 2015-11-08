#ifndef ARDUINO

/// \file rfTemp.cpp
/// \brief Get temperature and humidity from a remote source and write it periodically onto a file
///
/// \author Dario Berzano

#include <iostream>
#include <ctime>
#include "RFComm.h"

/// See https://projects.drogon.net/raspberry-pi/wiringpi/pins/
#define PIN_RF 2

/// Unique identifier to listen to
# define RF_UUID 34

void call_me_maybe(size_t len, uint8_t *data, protoid_t protoid) {

  static uint8_t temp;
  static uint8_t tempDec;
  static uint8_t humi;
  static uint8_t humiDec;

  if (len == 8 && data[0] == RF_UUID) {
    temp = data[1];
    tempDec = data[2];
    humi = data[3];
    humiDec = data[4];
    if (temp + humi == data[5] && tempDec + humiDec == data[6]) {
      printf("@%ld -- temp: %2u.%02u -- humi: %2u.%02u\n",
        time(NULL), temp, tempDec, humi, humiDec);
    }
  }

}

int main(int argc, char *argv[]) {

  if (wiringPiSetup() == -1) {
    fprintf(stderr, "fatal: cannot setup wiringPi\n");
    return 1;
  }

  RFComm::init();

  RFComm rfRecv(PIN_RF);
  rfRecv.setupRecv( &call_me_maybe );

  while (true) {
    sleep(10);
  }

  return 0;
}

#endif // ARDUINO
