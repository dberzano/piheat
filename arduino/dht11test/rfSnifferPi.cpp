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
  const proto_t *proto;
  size_t len;

  while (true) {
    if ( (len = rfRecv.recv(&buf, &proto)) ) {
      uint8_t synclen = proto->symbols[RFCSYM_SYNC].lo;
      printf("recv %u bytes - sync len %u>", len, synclen);
      for (size_t i=0; i<len; i++) {
        printf(" %3u", buf[i]);
      }
      puts("");

    }
  }

  return 0;

}

#endif // ARDUINO
