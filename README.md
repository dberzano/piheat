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


Client configuration
--------------------

Web client does not require a web server: everything runs in the browser. On the
server, though, a configuration file must be provided.

It must be placed in `js/private-config.js` and it contains a JSON-formatted
configuration like the following:

```json
piheat_config = {
  "thingid": "<thingid>",
  "messages_expire_after_s": 12345,
};
```

All the configuration variables are **mandatory**: the client will stop and
issue a warning if they are not configured.

* **thingid**: the "thing" identifier as on [dweet.io](http://dweet.io). As all
  dweets are public, and there is no registration needed, everybody can "dweet"
  in place of your thing, so set a difficult-to-guess name, like a UUID. It must
  be the same on the client and on the server.
* **messages_expire_after_s**: all dweets older than the specified number of
  seconds will be considered expired, and therefore ignored.


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
