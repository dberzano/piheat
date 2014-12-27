## @file __init__.py
#  Main file of the Pi Heat application.

## This application's version
__version__ = '0.0.1'

import time, signal, sys, os
import logging, logging.handlers
from daemon import Daemon

## @class PiHeat
#  Daemon class for the Pi Heat application (inherits from Daemon).
class PiHeat(Daemon):

  def exit_handler(self, signum, frame):
    signal.signal(signum, self.exit_handler_noop)
    logging.info('Delaying exit...')
    time.sleep(2)
    logging.info('Exiting...')
    sys.exit(0)

  def exit_handler_noop(self, signum, frame):
    pass

  ## Initializes log facility. Logs both on stderr and syslog. Works on OS X and Linux.
  def init_log(self):

    log_level = logging.DEBUG
    msg_format = self.name + '(%(process)d): %(asctime)s %(levelname)s ' \
      '[%(module)s.%(funcName)s] %(message)s'
    date_format = '%Y-%m-%d %H:%M:%S'

    # logs on stderr
    logging.basicConfig(level=log_level, format=msg_format, datefmt=date_format, stream=sys.stderr)

    # logs on syslog
    syslog_address = None
    for a in [ '/var/run/syslog', '/dev/log' ]:
      if os.path.exists(a):
        syslog_address = a
        break

    if syslog_address:
      syslog_handler = logging.handlers.SysLogHandler(address=syslog_address)
      syslog_handler.setLevel(log_level)
      syslog_handler.setFormatter( logging.Formatter(msg_format, date_format) )
      logging.getLogger('').addHandler(syslog_handler)
    else:
      logging.error('cannot log to syslog')

  def run(self):
    self.init_log()
    for sig in [signal.SIGTERM, signal.SIGINT, signal.SIGHUP, signal.SIGQUIT]:
      signal.signal(sig, self.exit_handler)
    while True:
      logging.info('this daemon runs for 10 seconds many times')
      time.sleep(10)
