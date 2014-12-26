Pi Heat
=======

Remote heating control via Raspberry Pi.

Uses [dweet.io](http://dweet.io/) for asynchronous communication.


Allowed JSON messages
---------------------

All messages are JSON and have a `type` field indicating the type.


### Status messages

Status messages are sent by the device to report current heating status.

```json
{
  "type": "status",
  "status": "on|off",
  "temp": 123,
}
```

* `status`: a `String` indicating the heating status. Can be **on** or **off**.
  Values are case-insensitive.
* `temp` *(optional)*: a `Decimal` value with the current temperature in °C.


### Commands

Commands are sent by clients to control heating.

```json
{
  "type": "command",
  "command": "turnon|turnoff|status"
}
```

Command is a case-insensitive string. Possible commands:

* **turnon**: requests to turn heating on
* **turnoff**: requests to turn heating off
* **status**: requests device to report status
