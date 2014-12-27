## @package piheat
#  Remote heating control for Raspberry Pi: server application

## This application's version
__version__ = '0.0.1'

import time, signal, sys
from daemon import Daemon

## @class PiHeat
#  The PiHeat application, daemonized.
class PiHeat(Daemon):

  def exit_handler(self, signum, frame):
    signal.signal(signum, self.exit_handler_noop)
    print 'Delaying exit...'
    time.sleep(2)
    print 'Exiting...'
    sys.exit(0)

  def exit_handler_noop(self, signum, frame):
    pass

  def run(self):
    for sig in [signal.SIGTERM, signal.SIGINT, signal.SIGHUP, signal.SIGQUIT]:
      signal.signal(sig, self.exit_handler)
    while True:
      print 'this daemon runs for 10 seconds many times'
      time.sleep(10)
