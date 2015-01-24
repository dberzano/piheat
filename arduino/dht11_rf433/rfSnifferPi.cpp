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

int main(int argc, char *argv[]) {

  if (wiringPiSetup() == -1) {
    fprintf(stderr, "fatal: cannot setup wiringPi\n");
    return 1;
  }

  RFComm::init();

  RFComm rfRecv(PIN_RF);
  rfRecv.setupRecv();

  const uint8_t *buf;
  protoid_t protoid = rfProtoV1;
  const char *protos[] = { "RF_V1", "RF_V2", "RF_V3" };
  size_t len;

  while (true) {
    if ( (len = rfRecv.recv(&buf, &protoid)) ) {
      printf("recv %u bytes - proto %s>", len, protos[protoid]);
      for (size_t i=0; i<len; i++) {
        printf(" %3u", buf[i]);
      }
      puts("");
    }
  }

  return 0;

}

#endif // ARDUINO
