/// \class dht11
/// \brief Read data from the DHT11 temperature and humidity sensor from an Arduino
///
/// Whenever a read from sensor ends successfully, the latest values are stored, and also placed in
/// an history: the history, of a user-configurable size, is used to smooth out fluctuations by
/// averaging its values.
///
/// Typical usage:
///
/// ~~~{.cpp}
/// #include "dht11.h"
///
/// dht11 myTempSensor(2 /* digi pin */, 10 /* hist size */);
///
/// void loop() {
///   if ( myTempSensor.read() == DHT11_OK ) {
///     Serial.println( myTempSensor.get_last_temperature() );
///     Serial.println( myTempSensor.get_last_humidity() );
///     Serial.println( myTempSensor.get_weighted_temperature(), 2 );
///     Serial.println( myTempSensor.get_weighted_humidity(), 2 );
///   }
///   delay(1000 /* msec */);
/// }
/// ~~~
///
/// Using `get_weighted_temperature()` and `get_weighted_humidity()` with a properly sized history
/// helps smoothing out small fluctuations.
///
/// The following cycle is used to request data:
///
/// ~~~{.txt}
///       ----        ====        ====        ======== ...
///      /    \      /    \      /    \      /
///     /      \    /      \    /      \    /
/// ----        ====        ====        ====           ...
/// ^^^^^^^^^^^^^^^^^^^^^^  ^^^^^^^^^^  ^^^^^^^^^^^^^^
///  Handshake: LOW+HIGH    Data Bit 0    Data Bit 1
///                         HIGH <40µs    HIGH >40µs
///
///                         ^^^^^^^^^^^^^^^^^^^^^^^^^^
///                              40 bits in total
/// ~~~
///
/// Where:
///
/// - `---` is data sent by the Arduino
/// - `===` is data sent by the DHT11
///
/// Breaking the cycle down:
///
/// - a `LOW` then `HIGH` is sent by the Arduino
/// - sensor replies with `LOW` then `HIGH`, then stards sending data
/// - bit 0 is a `LOW` followed by a "short" `HIGH` (< 40µs)
/// - bit 1 is a `LOW` followed by a "long" `HIGH` (> 40µs)
///
/// There are **40 bits (5 bytes)** in the response. Each byte is:
///
/// 1. humidity value, in %
/// 2. always zero
/// 3. temperature value, in °C
/// 4. always zero
/// 5. sum of humidity and temperature (a simple checksum)
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

#define DHT11_INVALID -9999

class dht11 {

  private:

    int pin;
    int humidity;
    int temperature;
    unsigned int hist_sz;
    unsigned int hist_idx;
    unsigned int hist_nelm;
    float *temp_hist;
    float *humi_hist;

    float mavg(float *vals, unsigned int n);

  public:

    dht11(int _pin, unsigned int _hist_sz);
    ~dht11();
    int read();
    float get_weighted_temperature();
    float get_weighted_humidity();
    int get_last_temperature();
    int get_last_humidity();

};

#endif  // dht11_h
