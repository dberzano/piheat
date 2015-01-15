#ifndef ARDUINO

/// Hacked from http://code.google.com/p/rc-switch/
/// by @justy to provide a handy RF code sniffer

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
  size_t len;

  while (true) {
    if (len = rfRecv.recv(&buf)) {

      printf("recv %lu bytes>", len);
      for (size_t i=0; i<len; i++) {
        printf(" %3u", buf[i]);
      }
      puts("");

    }
  }

  return 0;

}

#endif // ARDUINO
