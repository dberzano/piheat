## @file __init__.py
#  Main file of the Pi Heat application.

## This application's version
__version__ = '0.0.1'

import time, sys, os
import logging, logging.handlers
import random
from daemon import Daemon

## @class PiHeat
#  Daemon class for the Pi Heat application (inherits from Daemon).
class PiHeat(Daemon):

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

  def onexit(self):
    if random.random() > 0.7:
      logging.warning('cannot satisfy exit request!')
      return False
    else:
      logging.info('exiting soon, please wait')
      time.sleep(2)
      logging.info('bye!')
    return True

  def run(self):
    self.init_log()
    while True:
      logging.info('this daemon runs for 10 seconds many times')
      time.sleep(10)
