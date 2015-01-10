/// \class dht11
/// \brief Read data from the DHT11 temperature/humidity sensor from an Arduino
///
/// Datasheet for DHT11 is [available here](http://www.micro4you.com/files/sensor/DHT11.pdf).
///
/// Original version is [available here](http://playground.arduino.cc/Main/DHT11Lib).
///
/// \authors George Hadjikyriacou, SimKard, Rob Tillaart, Dario Berzano

#ifndef dht11_h
#define dht11_h

#if defined(ARDUINO) && (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#define DHT11LIB_VERSION "0.4.1"

#define DHT11_OK 0
#define DHT11_ERR_CKSUM -1
#define DHT11_ERR_TMOUT -2

class dht11
{
  public:
    int read(int pin);
    int humidity;
    int temperature;
};

#endif  // dht11_h
