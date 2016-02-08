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
    ## Current actual heating status (boolean)
    self._actual_heating_on = False
    ## Heating ascending/descending (boolean)
    self._heating_ascending = True
    ## True if current heating status has been just updated
    self._heating_status_updated = False
    ## SHA256 hash of encryption password
    self._password_hash = None
    ## Cleartext password
    self._password = None
    ## Tolerance (in msec) between message's declared timestamp and server's timestamp
    self._tolerance_ms = 30000
    ## Control file for the heating switch (we write 0 or 1 to this file)
    self._switch_file = None
    ## File used for watchdog purposes. Create this file continuously, something else will delete
    ## it. The absence of this file is an indicator of a hung daemon. Might be None
    self._watchdog_file = None
    ## Turn on/turn off programs
    self._program = []
    ## Override current program
    self._override_program = None
    ## Never touched heating status?
    self._actual_heating_firsttime = True
    ## Last command unique ID (string)
    self._lastcmd_id = 'saved_configuration'
    ## Current temperature (Celsius degrees)
    self._temp = None
    ## Tolerance (in ms) for trustable temperature. Older temperatures are ignored
    self._temp_tolerance_ms = 5*60*1000
    ## Current target temperature (Celsius degrees)
    self._target_temp = 0;
    ## Hysteresis positive tolerance
    self._hysteresis_temp_pos = 0.2
    ## Hysteresis negative tolerance
    self._hysteresis_temp_neg = 0.1
    ## Current humidity (percentage)
    self._humi = None
    ## Number of last consecutive errors in reading sensor data
    self._sensors_errors = 0
    ## Threshold of consecutive sensor errors before resetting data to None
    ## (as opposed to keeping the last value)
    self._sensors_errors_tolerance = 5
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
      self._watchdog_file = jsconf.get('watchdog_file', None)
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
        'watchdog_file': self._watchdog_file,
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
    if self._heating_status != status:
      self._heating_status = status
      self._heating_status_updated = True
    self.update_temp_status()


  ## Gets temperature.
  @property
  def temp(self):
    return self._temp


  ## Sets temperature.
  #
  #  @param temp The temperature to set
  @temp.setter
  def temp(self, temp):
    if self._temp != temp:
      self._temp = temp
      self._heating_status_updated = True
    self.update_temp_status()


  ## Gets target temperature.
  @property
  def target_temp(self):
    return self._target_temp


  ## Sets target temperature.
  #
  #  @param target_temp The target temperature to set
  @target_temp.setter
  def target_temp(self, target_temp):
    if self._target_temp != target_temp:
      self._target_temp = target_temp
      self._heating_status_updated = True
    self.update_temp_status()


  ## Gets actual heating status.
  @property
  def actual_heating_on(self):
    return self._actual_heating_on


  ## Sets actual heating on or off.
  #
  #  @param status True if on, False if off
  @actual_heating_on.setter
  def actual_heating_on(self, status):
    if status == self._actual_heating_on and not self._actual_heating_firsttime:
      logging.debug("actual heating status unchanged (%s)" %
                    (self._actual_heating_on and "on" or "off"))
      return True
    self._actual_heating_firsttime = False
    # We change the status.
    if status:
      status_str = "on"
      status_file = "1"
    else:
      status_str = "off"
      status_file = "0"
    logging.info("turning actual heating %s" % status_str)
    try:
      with open(self._switch_file, "w") as fp:
        fp.write(status_file)
    except IOError as e:
      logging.error("cannot change status: %s" % e)
      return False
    # Set member only if write is successful.
    self._actual_heating_on = status
    self._heating_status_updated = True
    return True


  ## Updates status and temperature, and possibly changes actual status
  #  accordingly.
  def update_temp_status(self):
    status = self._heating_status
    temp = self._temp
    target_temp = self._target_temp
    logging.debug("updating temp (%s), target temp (%f) and status (%s)" %
                  (str(temp), target_temp, status))

    if status:
      # Heating set to be on. Check temp. Treats temp is None case.
      if temp is None or temp <= self.target_temp-self._hysteresis_temp_neg:
        self.actual_heating_on = True
        self._heating_ascending = True
      elif temp >= self.target_temp+self._hysteresis_temp_pos:
        self.actual_heating_on = False
        self._heating_ascending = False
      else:
        # Between hysteresis boundaries. Heating on if ascending, off if
        # descending. So, heating has same status of ascending.
        self.actual_heating_on = self._heating_ascending
      logging.debug("hysteresis: %s" %
                    (self._heating_ascending and "ascending" or "descending"))
    else:
      # Heating should be off. Ascending for the next time it goes on.
      self.actual_heating_on = False
      self._heating_ascending = True


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
        'temp': self.temp,
        'msgexp_s': self._msg_expiry_s,
        'msgupd_s': self._send_status_every_s,
        'program': self._program,
        'override_program': self._override_program,
        'name': self._thingname,
        'lastcmd_id': self._lastcmd_id
      }
      if self.heating_status:
        raw.update({
          'actual_status': self.actual_heating_on,
          'target_temp': self.target_temp,
        })
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


  ## Create the watchdog file with the current timestamp.
  def watchdog(self):
    if not self._watchdog_file: return
    try:
      with open(self._watchdog_file, "w") as fp:
        logging.debug("writing watchdog %s" % self._watchdog_file)
        fp.write(str(int(TimeStamp().get_timestamp_usec_utc())))
    except IOError as e:
      logging.error("cannot write watchdog file %s: %s" % (self._watchdog_file, str(e)))


  ## Get data from sensor
  def get_sensors(self):
    logging.debug("getting temperature and other sensor data")
    try:
      val = requests.get("https://dweet.io/get/latest/dweet/for/%s-sensors" % self._thingid,
                         timeout=self._requests_timeout).json()
      delta = (TimeStamp()-TimeStamp.from_iso_str(val["with"][0]["created"])).total_seconds()*1000
      if delta > self._temp_tolerance_ms:
        raise Exception("temperature data is too old (> %d ms)" % self._temp_tolerance_ms)
      self.temp = float(val["with"][0]["content"]["temp"]);
      self._humi = float(val["with"][0]["content"].get("humi", None));
      self._sensors_errors = 0
    except Exception as e:
      self._sensors_errors = self._sensors_errors + 1
      if self._sensors_errors >= self._sensors_errors_tolerance:
        logging.error("too many sensors reading errors (%d out of %d): resetting data: %s" %
                      (self._sensors_errors, self._sensors_errors_tolerance, e))
        self.temp = None
        self._humi = None
      else:
        logging.warning("sensor error, retaining old data: %s" % e)
    logging.debug("sensor data: temp=%s, humidity=%s" %
                  (self.temp  and "%.1f°C" % self.temp  or "n/a",
                   self._humi and "%.1f%%" % self._humi or "n/a"))

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

      # Retrieve latest command and temperature
      if int(time.time())-last_command_check_ts > self._get_commands_every_s:
        self.get_sensors()
        self.watchdog()
        if self.get_latest_command():
          last_command_check_ts = int(time.time())
      self.watchdog()

      # Change status according to programs and overrides
      # TODO: can be improved to speed up reaction time.
      if int(time.time())-last_status_change_ts > 20:
        hm = int(TimeStamp().get_formatted_str("%H%M"))

        if self._override_program:
          logging.debug("An override is set: " + json.dumps(self._override_program))
          if self.tminc(hm, self._override_program["begin"], self._override_program["end"]):
            self.heating_status = self._override_program["status"]
            if self.heating_status:
              self.target_temp = self.heating_status and \
                                 self._override_program.get("temp", 9999) or -9999
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
            temps = [p.get("temp", 9999) for p in self._program if self.tminc(hm, p["begin"], p["end"])] + [-9999]
            self.heating_status = len(temps) > 1
            self.target_temp = temps[0]
          else:
            logging.debug("No program set")
            self.heating_status = False

        last_status_change_ts = int(time.time())

      # Update status
      if self.heating_status_updated or \
         int(time.time())-last_status_update_ts > self._send_status_every_s:
        if self.send_status_update():
          last_status_update_ts = int(time.time())
      self.watchdog()

      time.sleep(1)

    return 0
