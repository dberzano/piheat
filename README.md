Pi Heat
=======

Remote heating control via Raspberry Pi.

Uses [dweet.io](http://dweet.io/) for asynchronous communication.


Installation on Pi
------------------

As root:

```bash
bash -xe <(curl https://raw.githubusercontent.com/dberzano/piheat/master/misc/install.sh)
```

**Note:** do not forget `-xe` to bash.

You will be prompted for three pieces of information:

* **ID:** a unique ID for your heating control, same used in web application
* **Name:** a friendly name for your heating control (pick any)
* **Password:** encryption passwod (won't be displayed)

The daemon will start shortly afterwards.


### Updates

Updates occur automatically. Whenever a new change occurs, it is automatically
retrieved and applied.

Usage
-----

Connect [here](http://cern.ch/dberzano/ph/), this is the client: you are
prompted for the thing ID and the password. After entering them, you can
bookmark the generated URL for subsequent connections.

You can opt for not saving your password in the generated URL if you want.


Allowed JSON messages
---------------------

All messages are JSON and have two mandatory fields:

* a `type` field indicating the type
* a `timestamp` indicating when the message was created

The `timestamp` is an ISO-formatted UTC date.


### Status

Status messages are sent by the device to report current heating status.

```json
{
  "type": "status",
  "timestamp": "2014-07-30T21:06:15.600000Z",
  "status": true|false,
  "actual_status":  true|false,
  "temp": 19,
  "target_temp": 22,
  "msgexp_s": 123,
  "msgupd_s": 123,
  "name": "thing's friendly name",
  "program": [ { "begin": 1234, "end": 1234, "temp": 20 },
               { "begin": 1345, "end": 1345 } ],
  "override_program": { "begin": 1345, "end": 1345,
                        "status": true, "temp": 20 },
  "lastcmd_id": "abcdef"
}
```

* `status`: a `String` indicating the heating status. Can be **on** or **off**.
  Values are case-insensitive.
* `temp` *(optional)*: a `Decimal` value with the current measured temperature
  in °C.
* `target_temp` *(optional)*: a `Decimal` value with the current target
  temperature in °C.
* `msgexp_s` *(optional)*: an `Integer` value used to tell the client what is
  the maximum validity of a dweet.
* `msgupd_s` *(optional)*: an `Integer` value used to tell the client what is
  the status update rate of the server.
* `name` *(optional)*: a `String` with the thing's friendly name.
* `program`: current timespans when heating is on, and associated temperatures.
  Note that the `temp` field (in Celsius degrees) is optional.
* `override_program`: a single temporary timespan that takes precedence over
  current program and is deleted after it's elapsed. If `begin` and `end` are
  set to `-1`, it represents the "forever" override. The temperature (`temp`, in
  Celsius degrees) is optional.
* `lastcmd_id`: ID of the last command sent, used to understand whether the
  command has been received


### Commands

Commands are sent by clients to control heating.

```json
{
  "type": "command",
  "timestamp": "2014-07-30T21:06:15.600000Z",
  "program": [ { "begin": 1234, "end": 1234, "temp": 12 },
               { "begin": 1345, "end": 1345 } ],
  "override_program": { "begin": 1345, "end": 1345,
                        "status": true, "temp": 24 },
  "id": "abcdef"
}
```

* `program`: *see [Status](#status)*
* `override_program`: *see [Status](#status)*
* `id`: a unique ID


Client configuration
--------------------

The web client (in the `client` folder) can be hosted on any web server: it does
not require anything special to run as everything is performed in the browser.

It is not even needed to provide a configuration file. The first time you
connect to the page, you will be prompted with the **thing ID** and
**encryption password** to provide. They must be the same provided in the server
configuration.

Once set, they will be part of the URL (as anchor). The given URL can be
bookmarked for convenience and quick access.

If you want you can decide to save only the thing name in the anchor, and not
the password.

This method allows to use a single web page for controlling every device.


Temperature sensor
------------------

The `lowlevel` directory contains LUA sketches for a
[NodeMCU](http://nodemcu.com/index_en.html)-based temperature sensor. NodeMCU
**v0.9.6** is the firmware version we have tested on a
[ESP8266](http://esp8266.com) board (ESP-12 flavour, which is
breadboard-friendly). More [firmware versions](https://github.com/nodemcu/nodemcu-firmware/releases)
are available.

The temperature sensor is a DHT22, which is more precise than the DHT11. They
are both compatible with the given sketches. The data pin must be connected to
the GPIO2 (D4) connector on the NodeMCU.

An example pinout is provided:

![NodeMCU pinout](http://forum.makehackvoid.com/uploads/default/178/df994028721a8bdf.png)

The LUA sketches are heavily based on the instructions found on the
[Odd One Out](https://odd-one-out.serek.eu/esp8266-nodemcu-dht22-mqtt-deep-sleep/)
blog. These sketches use **deep sleep** to save power: this is useful *e.g.* if
the NodeMCU is connected to a battery and not on mains, as it
[consumes much less](http://sourceforge.net/p/nodemcu/wiki/Sleep/). A relevant
difference is that we use the public [dweet.io](https://dweet.io) platform
instead of a custom [MQTT](http://mqtt.org/) server.

This feature requires GPIO16 (D0) to be connected to the RST pin: the board will
send a short signal to that pin when the deep sleep timer is over, and we use it
to reset the board.

Bear in mind that the deep sleep turns the board off, so when it wakes up it
practically reboots with all the usual overhead (connecting to wifi, getting the
IP address via DHCP...)

Plenty of information is available on the Web concerning NodeMCU. For instance
[this video](https://www.youtube.com/watch?v=FWQ7D8zzYnk) *(in Spanish but look
at the pictures, they are very clear)* is very useful and covers all the
necessary steps to start.

Note that a [USB to UART driver](http://www.silabs.com/products/mcu/Pages/USBtoUARTBridgeVCPDrivers.aspx)
is required to connect the NodeMCU to the computer and communicate to it
serially.

### Configure the parameters

Based on the `private.lua.example` file, create a `private.lua` file and change:

* `wifi_SSID`: the Wi-Fi network name
* `wifi_password`: the Wi-Fi password *(if applicable)*
* `dweet_thing_id`: the "thing id" to use on [dweet.io](https://dweet.io). Note
  that it is not the same as the "thing id" used for controlling the heating,
  we are using a different (unencrypted) channel for it. The ID **must** be:
  `<main_thing_id>-sensors`, that is: the same thing ID used for heating status
  communication with `-sensors` appended. The Python server expects it to be
  called like this, so arbitrary names are not allowed.

### Data format

Dweets from the thing have only the `temp` field mandatory, which represents the
current temperature in Celsius degrees. The NodeMCU setup is configured to send
the relative humidity (%) in the `humi` field. Both values are floats.

The humidity is not used for the moment.

Other fields might be added at the user's discretion.


Encrypted messages
------------------

Encrypted JSON-serialized objects can be sent in place of regular cleartext
objects. Encryption uses the **AES-CBC-256** cipher, and binary data is encoded
using **base64**.

This is the format of an encrypted message:

```json
{
  "payload": "<base64-encoded-json-message>",
  "nonce": "<base64-encoded>"
}
```

* **payload**: contains the original JSON serialized object in base64 notation.
* **nonce**: the initialization vector used for encrypting the message, in
  base64 notation. This must be 16 bytes long.


Original payload is a JSON *status* or *command* object as described above.

Client-side JavaScript encryption uses
[Forge](https://github.com/digitalbazaar/forge).
