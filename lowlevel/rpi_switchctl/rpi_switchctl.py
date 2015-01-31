#!/usr/bin/env python

## @file rpi_switchctl.py
#  Controls turning on and off a relay connected to the Raspberry Pi's GPIO.
#
#  This program must be run as root, but it exposes a simple interface allowing non-root commands to
#  interact with the switch.

import RPi.GPIO as GPIO
import sys, signal, os
from getopt import getopt
from time import sleep
from pwd import getpwnam
from grp import getgrnam


## Exits gracefully from the program by cleaning up GPIO. Callback on various
#  quit signals.
#
#  @param signum signal number
#  @param frame frame
def graceful_exit(signum, frame):
  GPIO.cleanup()
  sys.exit(0)


## Main function: entry point.
#
#  @param argv list of arguments
def main(argv):

  conf = {
    'ctlfile': None,
    'user': None,
    'group': None,
    'pidfile': None,
    'pin': None
  }

  opt,_ = getopt(argv, '', ['file=', 'user=', 'group=', 'mode=', 'pidfile=', 'pin='])

  for k,v in opt:
    if k == '--file':
      conf['ctlfile'] = v
    elif k == '--user':
      conf['user'] = v
    elif k == '--group':
      conf['group'] = v
    elif k == '--mode':
      conf['mode'] = int(v, 8)
    elif k == '--pidfile':
      conf['pidfile'] = v
    elif k == '--pin':
      conf['pin'] = int(v)

  # Create the input file (~touch)
  with open(conf['ctlfile'], 'w') as fp:
    fp.write('0')  # off by default

  # Permissions for that file
  uid = -1
  gid = -1
  if conf['user'] is not None:
    uid = getpwnam( conf['user'] ).pw_uid
  if conf['group'] is not None:
    gid = getgrnam( conf['group'] ).gr_gid
  os.chown(conf['ctlfile'], uid, gid)
  if conf['mode'] is not None:
    os.chmod(conf['ctlfile'], conf['mode'])

  # Process ID to a file
  if conf['pidfile']:
    with open(conf['pidfile'], 'w') as fp:
      fp.write(os.getpid())

  # Using GPIO.BOARD numbering scheme, i.e.: number of PINs as on the board
  GPIO.setmode(GPIO.BOARD)
  GPIO.setup(conf['pin'], GPIO.OUT)
  GPIO.output(conf['pin'], GPIO.HIGH)

  pin_status = GPIO.HIGH

  while True:

    sleep(5)

    with open(conf['ctlfile'], 'r') as fp:
      newstatus = fp.read(1)

    if newstatus == '1':
      if pin_status == GPIO.HIGH:
        print 'turning on'
        pin_status = GPIO.LOW
        GPIO.output(conf['pin'], pin_status)
    elif newstatus == '0':
      if pin_status == GPIO.LOW:
        print 'turning off'
        pin_status = GPIO.HIGH
        GPIO.output(conf['pin'], pin_status)
    else:
      # Fix file content in case it contains garbage
      with open(conf['ctlfile'], 'w') as fp:
        if pin_status == GPIO.HIGH:
          fp.write('0')
        else:
          fp.write('1')

    # Interactive
    # print 'cmd> ',
    # cmd = sys.stdin.readline().strip()
    # print '\r',

    # if cmd == 'quit' or cmd == 'exit':
    #   print 'exiting'
    #   break
    # elif cmd == 'on':
    #   print 'turning on'
    #   GPIO.output(conf['pin'], GPIO.LOW)
    # elif cmd == 'off':
    #   print 'turning off'
    #   GPIO.output(conf['pin'], GPIO.HIGH)
    # elif cmd != '':
    #   print 'not understood'

  GPIO.cleanup()

if __name__ == '__main__':
  signal.signal(signal.SIGINT, graceful_exit)
  main(sys.argv[1:])
