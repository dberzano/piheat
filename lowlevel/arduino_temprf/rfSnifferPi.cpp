#ifndef ARDUINO

/// \file rfSnifferPi.cpp
/// \brief Test executable for the RFComm class
///
/// Inspired by:
///
/// * http://code.google.com/p/rc-switch
/// * https://github.com/ninjablocks/433Utils
///
/// \author Dario Berzano

#include <iostream>
#include "RFComm.h"

/// See https://projects.drogon.net/raspberry-pi/wiringpi/pins/
#define PIN_RF 2

void call_me_maybe(size_t len, uint8_t *data, protoid_t protoid) {
  static const char *protos[] = { "RF_V1", "RF_V2", "RF_V3" };
  printf("recv %u bytes - proto %s>", len, protos[protoid]);
  for (size_t i=0; i<len; i++) {
    printf(" %3u", data[i]);
  }
  puts("");
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
    sleep(1000);
  }

  return 0;
}

#endif // ARDUINO
