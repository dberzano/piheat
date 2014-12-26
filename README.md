Pi Heat
=======

Remote heating control via Raspberry Pi.

Uses [dweet.io](http://dweet.io/) for asynchronous communication.


Allowed JSON messages
---------------------

All messages are JSON and have a `type` field indicating the type.


### Status messages

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
