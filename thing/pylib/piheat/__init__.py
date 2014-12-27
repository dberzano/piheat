## @package piheat
#  Remote heating control for Raspberry Pi: server application

## This application's version
__version__ = '0.0.1'

import time
from daemon import Daemon

## @class PiHeat
#  The PiHeat application, daemonized.
class PiHeat(Daemon):

  def run(self):
    print 'this daemon runs for 60 seconds'
    time.sleep(60)
