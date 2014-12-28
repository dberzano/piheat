## @file __init__.py
#  Main file of the Pi Heat application.

## This application's version
__version__ = '0.0.1'

import time, sys, os
import logging, logging.handlers
import json, requests
from daemon import Daemon

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
    ## Message expiration threshold
    self._messages_expire_after_s = None

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
      self._messages_expire_after_s = int( jsconf['messages_expire_after_s'] )
      self._thingid = str( jsconf['thingid'] )
    except (ValueError, KeyError) as e:
      logging.critical('invalid or missing value in configuration: %s' % e)
      return False

    return True

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

    while True:
      logging.info('new loop commenced')
      time.sleep(10)

    return 0
