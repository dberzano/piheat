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

The daemon will start shortly.


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
  "status": "on|off",
  "temp": 123,
  "msgexp_s": 123,
  "msgupd_s": 123,
  "name": "thing's friendly name",
  "program": [ { "begin": 1234, "end": 1234 },
               { "begin": 1345, "end": 1345 } ],
  "override_program": { "begin": 1345, "end": 1345, "status": true },
  "lastcmd_id": "abcdef"
}
```

* `status`: a `String` indicating the heating status. Can be **on** or **off**.
  Values are case-insensitive.
* `temp` *(optional)*: a `Decimal` value with the current temperature in °C.
* `msgexp_s` *(optional)*: an `Integer` value used to tell the client what is
  the maximum validity of a dweet.
* `msgupd_s` *(optional)*: an `Integer` value used to tell the client what is
  the status update rate of the server.
* `name` *(optional)*: a `String` with the thing's friendly name.
* `program`: current timespans when heating is on.
* `override_program`: a single temporary timespan that takes precedence over
  current program and is deleted after it's elapsed.
* `lastcmd_id`: ID of the last command sent, used to understand whether the
  command has been received


### Commands

Commands are sent by clients to control heating.

```json
{
  "type": "command",
  "timestamp": "2014-07-30T21:06:15.600000Z",
  "program": [ { "begin": 1234, "end": 1234 },
               { "begin": 1345, "end": 1345 } ],
  "override_program": { "begin": 1345, "end": 1345, "status": true },
  "id": "abcdef"
}
```

* `program`: *see [Status](#status)*
* `override_program`: *see [Status](#status)*
* `id`: a unique ID


Client configuration
--------------------

Web client does not require a web server: everything runs in the browser. On the
server, though, a configuration file must be provided.

It must be placed in `js/private-config.js` and it contains a JSON-formatted
configuration like the following:

```json
piheat_config = {
  "thingid": "<thingid>",
  "password": "<password>"
};
```

All the configuration variables are **mandatory**: the client will stop and
issue a warning if they are not configured.

* **thingid**: the "thing" identifier as on [dweet.io](http://dweet.io). As all
  dweets are public, and there is no registration needed, everybody can "dweet"
  in place of your thing, so set a difficult-to-guess name, like a UUID. It must
  be the same on the client and on the server.
* **password** *(optional)*: encryption password


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
