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
    ## Messages expiration threshold: honored while retrieving commands, sent to client for config
    self._msg_expiry_s = None
    ## Query for commands timeout
    self._get_commands_every_s = None
    ## Send heating status timeout
    self._send_status_every_s = None
    ## Current heating status (boolean)
    self._heating_status = False
    ## Latest command to process: `True` for turning on, `False` for turning off (default)
    self._desired_heating_status = False
    ## When latest command was issued (a `TimeStamp` instance)
    self._desired_heating_status_ts = None
    ## SHA256 hash of encryption password
    self._password_hash = None


  ## Initializes log facility. Logs both on stderr and syslog. Works on OS X and Linux.
  def init_log(self):

    log_level = logging.DEBUG
    msg_format = self.name + '(%(process)d): %(asctime)s %(levelname)s ' \
      '[%(module)s.%(funcName)s] %(message)s'
    date_format = '%Y-%m-%d %H:%M:%S'

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
      self._thingid = str( jsconf['thingid'] )
      self._thingname = str( jsconf['thingname'] )
      self._get_commands_every_s = int( jsconf['get_commands_every_s'] )
      self._send_status_every_s = int( jsconf['send_status_every_s'] )

      hashfunc = SHA256.new()
      hashfunc.update( str(jsconf['password']) )
      self._password_hash = hashfunc.digest()
    except (ValueError, KeyError) as e:
      logging.critical('invalid or missing value in configuration: %s' % e)
      return False

    return True


  ## Gets status of heating.
  @property
  def heating_status(self):
    return self._heating_status


  ## Sets status of heating.
  #
  #  @param status True for on, False for off
  #
  #  @todo To be fully implemented
  @heating_status.setter
  def heating_status(self, status):
    if status:
      status_str = 'on'
    else:
      status_str = 'off'
    logging.info('turning heating %s' % status_str)
    self._heating_status = status


  ## Gets the desired heating status.
  @property
  def desired_heating_status(self):
    return self._desired_heating_status


  ## Gets a `TimeStamp` object corresponding to when the desired heating status was set.
  @property
  def desired_heating_status_ts(self):
    return self._desired_heating_status_ts


  ## Sets the desired heating status.
  #
  #  @param status_with_timestamp A tuple with the status and the timestamp
  @desired_heating_status.setter
  def desired_heating_status(self, status_with_timestamp):
    self._desired_heating_status, self._desired_heating_status_ts = status_with_timestamp


  ## Get latest command via dweet.io.
  def get_latest_command(self):

    logging.debug('checking for commands')

    try:
      r = requests.get( 'https://dweet.io/get/dweets/for/%s' % self._thingid )
    except requests.exceptions.RequestException as e:
      logging.error('failed to get latest commands: %s' % e)
      return False

    if r.status_code != 200:
      logging.error('invalid status code received: %d' % r.status_code)
      return False

    try:

      for item in r.json()['with']:

        try:

          now_ts = TimeStamp()
          msg_ts = TimeStamp.from_iso_str( item['created'] )

          if (now_ts-msg_ts).total_seconds() <= self._msg_expiry_s:
            # consider this message: it is still valid

            msg = self.decrypt_msg(item['content'])
            if msg is None:
              logging.warning('cannot decrypt message, check password')

            elif msg['type'].lower() == 'command':

              cmd = msg['command'].lower()
              if cmd == 'turnon':
                self.desired_heating_status = True, msg_ts
                break
              elif cmd == 'turnoff':
                self.desired_heating_status = False, msg_ts
                break

          else:
            # messages from now on are too old, no need to parse the rest
            logging.debug('found first outdated message, skipping the rest')
            break

        except (KeyError, TypeError) as e:
          logging.debug('error parsing, skipped: %s' % e)
          pass

    except Exception as e:
      logging.error('error parsing response: %s' % e)
      return False

    return True


  ## Sends status update.
  #
  #  @return  True on success, False if it fails
  def send_status_update(self):

    if self.heating_status:
      status_str = 'on'
    else:
      status_str = 'off'
    logging.debug('sending status update (status is %s)' % status_str)

    try:
      payload = self.encrypt_msg({
        'type': 'status',
        'status': status_str,
        'msgexp_s': self._msg_expiry_s,
        'msgupd_s': self._send_status_every_s,
        'name': self._thingname
      })
      r = requests.post( 'https://dweet.io/dweet/for/%s' % self._thingid, params=payload )
    except requests.exceptions.RequestException as e:
      logging.error('failed to send update: %s' % e)
      return False

    if r.status_code != 200:
      logging.error('invalid status code received: %d' % r.status_code)
      return False

    logging.info('status update sent (status is %s)' % status_str)
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


  ## Program's entry point, overridden from the base Daemon class.
  #
  #  @return Always zero
  def run(self):

    self.init_log()
    if self.read_conf() == False:
      # configuration error: exit
      return 1

    last_command_check_ts = 0
    last_status_update_ts = 0
    while True:

      prev_desired_heating_status = self.desired_heating_status

      # retrieve current command
      if int(time.time())-last_command_check_ts > self._get_commands_every_s:
        if self.get_latest_command():
          last_command_check_ts = int(time.time())

      # check if heating status has expired (note: it is not superfluous, we must do it in case
      # requests for new commands fail)
      lastts = self.desired_heating_status_ts
      if lastts is not None:
        if (TimeStamp()-lastts).total_seconds() > self._msg_expiry_s:
          logging.warning('current heating command has expired: turning heating off')
          self.desired_heating_status = False, None

      # change status
      send_update_now = False
      if prev_desired_heating_status != self._desired_heating_status:
        if self.heating_status != self._desired_heating_status:
          self.heating_status = self._desired_heating_status
          send_update_now = True

      # update status
      if send_update_now or int(time.time())-last_status_update_ts > self._send_status_every_s:
        if self.send_status_update():
          last_status_update_ts = int(time.time())

      time.sleep(1)

    return 0
