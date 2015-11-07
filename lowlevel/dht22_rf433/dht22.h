/// \class dht22
/// \brief Read data from the DHT22 temperature and humidity sensor from an Arduino
///
/// Whenever a read from sensor ends successfully, the latest values are stored, and also placed in
/// an history: the history, of a user-configurable size, is used to smooth out fluctuations by
/// averaging its values.
///
/// Typical usage:
///
/// ~~~{.cpp}
/// #include "dht22.h"
///
/// dht22 myTempSensor(2 /* digi pin */, 10 /* hist size */);
///
/// void loop() {
///   if ( myTempSensor.read() == DHT22_OK ) {
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
/// - `===` is data sent by the DHT22
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
/// Datasheet for DHT22 is [available here](http://www.micro4you.com/files/sensor/DHT22.pdf).
///
/// Original version is [available here](http://playground.arduino.cc/Main/DHT22Lib).
///
/// \todo Use interrupts instead
///
/// \authors George Hadjikyriacou, SimKard, Rob Tillaart, Dario Berzano

#ifndef dht22_h
#define dht22_h

#if defined(ARDUINO) && (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#define DHT22LIB_VERSION "0.4.1"

#define DHT22_OK 0
#define DHT22_ERR_CKSUM -1
#define DHT22_ERR_TMOUT -2

#define DHT22_INVALID -9999

class dht22 {

  private:

    int mPin;
    int mHumidity;
    int mTemperature;
    size_t mHistSz;
    size_t mHistIdx;
    size_t mHistNelm;
    int *mHistTemp;
    int *mHistHumi;

    static int avg(int *vals, size_t n, int *avgDec);

  public:

    dht22(int pin, size_t histSz);
    ~dht22();
    int read();
    int get_weighted_temperature(int *avgDec);
    int get_weighted_humidity(int *avgDec);
    int get_last_temperature();
    int get_last_humidity();

};

#endif  // dht22_h
