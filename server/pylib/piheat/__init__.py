# -*- coding: utf-8 -*- 

## @file __init__.py
#  Main file of the Pi Heat application.

## This application's version
__version__ = '0.0.1'

import time, sys, os
import logging, logging.handlers
import json, requests
import base64
from Crypto.Hash import SHA256
from Crypto.Cipher import AES
from Crypto import Random
from daemon import Daemon
from timestamp import TimeStamp

## @class PiHeat
#  Daemon class for the Pi Heat application (inherits from Daemon).
#
#  The Pi Heat application is a server-side application supposed to run on a Raspberry Pi or any
#  other device to control your heating.
#
#  The web service [dweet.io](http://dweet.io) is used as intermediate "message board" for Pi Heat:
#
#   * a client requests to turn on or off the heating by sending a dweet
#   * the dweet is picked up by this application and elaborated properly
#
#  All communications are RESTful: the [requests](http://docs.python-requests.org/en/latest/) module
#  is used for this purpose as it is [the most straightforward
#  one](http://isbullsh.it/2012/06/Rest-api-in-python/).
#
#  @todo Keep track of nonces
class PiHeat(Daemon):

  ## Constructor.
  #
  #  @param name     Arbitrary nickname for the daemon
  #  @param pidfile  Full path to PID file. Path must exist
  #  @param conffile Full path to the configuration file
  def __init__(self, name, pidfile, conffile):
    super(PiHeat, self).__init__(name, pidfile)
    ## Full path to the configuration file in JSON format
    self._conffile = conffile
    ## Thing identifier, as used on dweet.io
    self._thingid = None
    ## Friendly name of the thing
    self._thingname = None
    ## Messages expiration threshold: not used by server, only sent to client for config
    self._msg_expiry_s = None
    ## Commands expiration threshold: honored while retrieving commands
    self._cmd_expiry_s = None
    ## Query for commands timeout
    self._get_commands_every_s = None
    ## Send heating status timeout
    self._send_status_every_s = None
    ## Current heating status (boolean)
    self._heating_status = False
    ## True if current heating status has been just updated
    self._heating_status_updated = False
    ## SHA256 hash of encryption password
    self._password_hash = None
    ## Cleartext password
    self._password = None
    ## Tolerance (in msec) between message's declared timestamp and server's timestamp
    self._tolerance_ms = 15000
    ## Control file for the heating switch (we write 0 or 1 to this file)
    self._switch_file = None
    ## Turn on/turn off programs
    self._program = []
    ## Override current program
    self._override_program = None
    ## Never touched heating status?
    self._heating_status_firsttime = True
    ## Last command unique ID (string)
    self._lastcmd_id = 'saved_configuration'
    ## Timeout (connect timeout, read timeout) in seconds for Python requests
    self._requests_timeout = (15,15)
    try:
      try: requests.get("https://dweet.io", timeout=(1,1))
      except ValueError: self._requests_timeout = 15
    except requests.exceptions: pass

  ## Initializes log facility. Logs both on stderr and syslog. Works on OS X and Linux.
  def init_log(self):

    log_level = logging.DEBUG
    msg_format = self.name + '(%(process)d): %(asctime)s %(levelname)s ' \
      '[%(module)s.%(funcName)s] %(message)s'
    date_format = '%Y-%m-%d %H:%M:%S'

    # increase debug level of the requests module
    logging.getLogger('requests').setLevel(logging.DEBUG)

    # logs on stderr
    logging.basicConfig(level=log_level, format=msg_format, datefmt=date_format, stream=sys.stderr)

    # logs on syslog
    syslog_handler = self._get_syslog_handler()
    if syslog_handler:
      syslog_handler.setLevel(log_level)
      syslog_handler.setFormatter( logging.Formatter(msg_format, date_format) )
      logging.getLogger('').addHandler(syslog_handler)
    else:
      logging.error('cannot log to syslog')


  ## Reads daemon configuration from JSON format.
  #
  #  @return True on success, False on configuration error (missing file, invalid JSON, missing
  #          required variables, invalid variables type, etc.)
  def read_conf(self):

    try:
      with open(self._conffile, 'r') as cfp:
        jsconf = json.load(cfp)
    except IOError as e:
      logging.critical('cannot read configuration file %s: %s' % (self._conffile, e))
      return False
    except ValueError as e:
      logging.critical('malformed configuration file %s: %s' % (self._conffile, e))
      return False

    # Read variables
    try:
      self._msg_expiry_s = int( jsconf['msg_expiry_s'] )
      self._cmd_expiry_s = int( jsconf['cmd_expiry_s'] )
      self._thingid = str( jsconf['thingid'] )
      self._thingname = str( jsconf['thingname'] )
      self._get_commands_every_s = int( jsconf['get_commands_every_s'] )
      self._send_status_every_s = int( jsconf['send_status_every_s'] )
      self._switch_file = str( jsconf['switch_file'] )
      self._program = jsconf.get('program', [])
      self._override_program = jsconf.get('override_program', None)
      self._password = str(jsconf['password'])
      hashfunc = SHA256.new()
      hashfunc.update( self._password )
      self._password_hash = hashfunc.digest()
    except (ValueError, KeyError) as e:
      logging.critical('invalid or missing value in configuration: %s' % e)
      return False

    return True


  ## Save configuration, if possible.
  #
  #  @return True on success.
  def save_conf(self):
    try:
      open(self._conffile, 'w').write(json.dumps({
        'msg_expiry_s': self._msg_expiry_s,
        'cmd_expiry_s': self._cmd_expiry_s,
        'thingid': self._thingid,
        'thingname': self._thingname,
        'get_commands_every_s': self._get_commands_every_s,
        'send_status_every_s': self._send_status_every_s,
        'switch_file': self._switch_file,
        'program': self._program,
        'override_program': self._override_program,
        'password': self._password,
        'last_saved': str(TimeStamp())
      }, indent=2))
    except IOError as e:
      logging.critical('cannot save configuration, check permissions: %s' % e)
      return False
    return True


  ## Gets status of heating.
  @property
  def heating_status(self):
    return self._heating_status


  ## Sets status of heating.
  #
  #  @param status True for on, False for off
  @heating_status.setter
  def heating_status(self, status):
    if status == self._heating_status and not self._heating_status_firsttime:
      logging.debug('heating status unchanged')
      return True
    self._heating_status_firsttime = False
    if status:
      status_str = 'on'
      status_file = '1'
    else:
      status_str = 'off'
      status_file = '0'
    logging.info('turning heating %s' % status_str)
    try:
      with open(self._switch_file, 'w') as fp:
        fp.write(status_file)
    except IOError as e:
      logging.error('cannot change status: %s' % e)
      return False
    self._heating_status = status
    self._heating_status_updated = True
    return True


  ## Returns True if heating status was just updated, and immediately set the
  #  value to false.
  #
  #  @return True if heating status was just updated, false after first call
  @property 
  def heating_status_updated(self):
    x = self._heating_status_updated
    self._heating_status_updated = False
    return x


  ## Get latest command via dweet.io.
  def get_latest_command(self):

    logging.debug('checking for commands')
    skipped = 0

    try:
      r = requests.get("https://dweet.io/get/dweets/for/%s" % self._thingid,
                       timeout=self._requests_timeout)
    except requests.exceptions.RequestException as e:
      logging.error('failed to get latest commands: %s' % e)
      return False

    if r.status_code != 200:
      logging.error('invalid status code received: %d' % r.status_code)
      return False

    try:

      for idx,item in enumerate( r.json()['with'] ):

        try:

          now_ts = TimeStamp()
          msg_ts = TimeStamp.from_iso_str( item['created'] )

          if (now_ts-msg_ts).total_seconds() <= self._cmd_expiry_s:
            # consider this message: it is still valid

            # nonce must be unique
            nonce = item['content']['nonce']
            nonce_is_unique = True

            for ridx, ritem in reversed( list(enumerate(r.json()['with']))[idx+1:] ):
              if 'nonce' in ritem['content'] and nonce == ritem['content']['nonce']:
                nonce_is_unique = False
                break

            if nonce_is_unique:
              msg = self.decrypt_msg(item['content'])

            if not nonce_is_unique:
              logging.warning('duplicated nonce (%s): possible replay attack, ignoring' % nonce)

            elif msg is None:
              logging.warning('cannot decrypt message, check password')

            else:

              msg_real_ts = TimeStamp.from_iso_str( msg['timestamp'] )
              msg_delta = msg_ts - msg_real_ts

              if abs(msg_delta.total_seconds())*1000 > self._tolerance_ms:
                logging.warning('message timestamps mismatch, ignoring')

              elif msg['type'].lower() == 'command':

                logging.debug('found valid command')
                logging.debug(json.dumps(msg, indent=2))

                save = False
                if msg['override_program'] != self._override_program:
                  save = True
                if msg['program'] != self._program:
                  save = True
                lid = msg['id']

                self._override_program = msg['override_program']
                self._program = msg['program']
                self._lastcmd_id = lid

                if save:
                  if self.save_conf():
                    logging.info('new configuration saved')
                  else:
                    logging.error('cannot save new configuration')

                break

          else:
            # messages from now on are too old, no need to parse the rest
            logging.debug('found first outdated message, skipping the rest')
            break

        except (KeyError, TypeError) as e:
          logging.debug('error parsing, skipped: %s' % e)
          skipped = skipped+1
          pass

    except Exception as e:
      logging.error('error parsing response: %s' % e)
      return False

    if skipped > 0:
      logging.warning('invalid messages skipped: %d' % skipped)
    return True


  ## Sends status update.
  #
  #  @return True on success, False if it fails
  def send_status_update(self):

    if self.heating_status:
      status_str = 'on'
    else:
      status_str = 'off'
    logging.debug('sending status update (status is %s)' % status_str)

    now = TimeStamp()
    try:
      raw = {
        'type': 'status',
        'timestamp': str(now),  # ISO UTC
        'status': self.heating_status,
        'msgexp_s': self._msg_expiry_s,
        'msgupd_s': self._send_status_every_s,
        'program': self._program,
        'override_program': self._override_program,
        'name': self._thingname,
        'lastcmd_id': self._lastcmd_id
      }
      logging.debug('message: ' + json.dumps(raw, indent=2))
      payload = self.encrypt_msg(raw)
      r = requests.post( 'https://dweet.io/dweet/for/%s' % self._thingid, params=payload )
    except requests.exceptions.RequestException as e:
      logging.error('failed to send update: %s' % e)
      return False

    if r.status_code != 200:
      logging.error('invalid status code received: %d' % r.status_code)
      return False

    logging.info('status update sent (status is %s)' % status_str)
    return True


  ## Sends a command to itself.
  #
  #  @return True on success, False if it fails
  def send_command(self):
    raw = { "type": "command",
            "id": base64.b64encode(Random.new().read(30)),
            "timestamp": str(TimeStamp()),
            "program": self._program,
            "from": "myself",
            "override_program": self._override_program }
    logging.debug("sending command: %s" % json.dumps(raw, indent=2))
    payload = self.encrypt_msg(raw)
    try:
      r = requests.post("https://dweet.io/dweet/for/%s" % self._thingid,
                        params=payload)
    except requests.exceptions.RequestException as e:
      logging.error("failed to send command: %s" % e)
      return False
    if r.status_code != 200:
      logging.error("invalid status code received while sending command: %d" % r.status_code)
      return False
    logging.info("command sent")
    self._lastcmd_id = raw["id"]
    return True


  ## Encrypts an object using AES-CBC.
  #
  #  @param obj Source object
  #
  #  @return The encrypted object, with a `nonce` and a `payload`
  def encrypt_msg(self, obj):

    # random IV
    rndfp = Random.new()
    iv = rndfp.read(16)

    # convert to JSON (output is always ASCII)
    obj_json = str( json.dumps(obj) )

    # pad to blocks of 16 bytes
    pad_len = 16 - len(obj_json) % 16
    obj_json = obj_json + chr(pad_len)*pad_len

    # encrypt
    aes = AES.new(self._password_hash, AES.MODE_CBC, iv)
    obj_enc = aes.encrypt(obj_json)

    # convert to base64
    obj_enc_b64 = base64.b64encode(obj_enc)
    iv_b64 = base64.b64encode(iv)

    # assemble final object
    return {
      'nonce': iv_b64,
      'payload': obj_enc_b64
    }


  ## Decrypts an object using AES-CBC.
  #
  #  @param obj Encrypted object (must have a `nonce` and `payload` key)
  #
  #  @return The decrypted object
  def decrypt_msg(self, obj):

    if not 'nonce' in obj or not 'payload' in obj:
      return None

    try:
      iv = base64.b64decode( obj['nonce'] )
      enc = base64.b64decode( obj['payload'] )
    except TypeError as e:
      logging.debug('cannot decode base64: %s' % e)
      return None

    # decrypt
    # note: does not check if password is ok! JSON will fail in case of garbage
    try:
      aes = AES.new(self._password_hash, AES.MODE_CBC, iv)
    except ValueError as e:
      logging.debug('cannot decrypt: %s' % e)

    dec = aes.decrypt(enc)

    # last byte contains how many bytes were used for padding
    # padding is constituted by the last byte repeated (last byte) times
    pad_len = ord(dec[-1])
    dec = dec[0:-pad_len]

    # decode from JSON
    try:
      dec = json.loads(dec)
    except ValueError as e:
      logging.debug('cannot decode JSON: %s' % e)
      return None

    return dec


  ## Exit handler, overridden from the base Daemon class.
  #
  #  @return Always True to satisfy the exit request
  def onexit(self):
    return True


  ## True if the given "time" (hours, minutes) is included within the interval.
  #  If begin < end assumes different days. If begin or end are < 0, assume we
  #  are always in the interval.
  #
  #  @return A boolean
  @staticmethod
  def tminc(hm, beg, end):
    if beg < 0 or end < 0:
      return True
    if beg < end:
      return beg <= hm and hm < end
    else:
      return hm >= beg or hm < end


  ## Program's entry point, overridden from the base Daemon class.
  #
  #  @return Always zero
  def run(self):

    self.init_log()
    if self.read_conf() == False:
      return 1

    last_command_check_ts = 0
    last_status_update_ts = 0
    last_status_change_ts = 0
    while True:

      # Retrieve latest command
      if int(time.time())-last_command_check_ts > self._get_commands_every_s:
        if self.get_latest_command():
          last_command_check_ts = int(time.time())

      # Change status according to programs and overrides
      if int(time.time())-last_status_change_ts > 20:
        hm = int(TimeStamp().get_formatted_str("%H%M"))

        if self._override_program:
          logging.debug("An override is set: " + json.dumps(self._override_program))
          if self.tminc(hm, self._override_program["begin"], self._override_program["end"]):
            self.heating_status = self._override_program["status"]
          elif hm > self._override_program["end"]:
            # Overrides are < 24 h
            logging.debug("Override has expired, deleting")
            self._override_program = None
            self.send_command()
            self.save_conf()
        else:
          logging.debug("No override set")

        if not self._override_program:
          if self._program:
            logging.debug("Programs are set: " + json.dumps(self._program))
            self.heating_status = len([1 for p in self._program if self.tminc(hm, p["begin"], p["end"])]) > 0
          else:
            logging.debug("No program set")
            self.heating_status = False

        last_status_change_ts = int(time.time())

      # Update status
      if self.heating_status_updated or \
         int(time.time())-last_status_update_ts > self._send_status_every_s:
        if self.send_status_update():
          last_status_update_ts = int(time.time())

      time.sleep(1)

    return 0
