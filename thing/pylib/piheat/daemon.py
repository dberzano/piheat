## @package piheat
#  Generic linux daemon base class for Python 3.x.
#  Source: http://www.jejik.com/files/examples/daemon3x.py

import sys, os, time, atexit, signal

## A generic daemon class.
#
#  Usage: subclass the daemon class and override the `run()` method.
class Daemon:

  def __init__(self, pidfile):
    self._pidfile = pidfile
    self.pid = None

  ## Write pidfile.
  def write_pid(self):
    with open(self._pidfile, 'w') as pf:
      pf.write( str(self.pid) + '\n' )

  ## Read pidfile.
  def read_pid(self):
    try:
      with open(self._pidfile, 'r') as pf:
        self.pid = int( pf.read().strip() )
    except (IOError, ValueError):
      self.pid = None

  ## Daemonize class. Unix double-fork mechanism.
  #
  #  @return False on failure, True on success
  def daemonize(self):

    try:
      pid = os.fork()
      if pid > 0:
        # exit from first parent
        sys.exit(0)
    except OSError as e:
      sys.stderr.write('Fork #1 failed: %s\n' % e)
      sys.exit(1)

    # decouple from parent environment
    os.chdir('/')
    os.setsid()
    os.umask(0)

    # do second fork
    try:
      pid = os.fork()
      if pid > 0:
        # exit from second parent
        sys.exit(0)
    except OSError as e:
      sys.stderr.write('Fork #2 failed: %s\n' % e)
      sys.exit(1)

    # we are in the daemon: know our pid
    self.pid = os.getpid()

    # redirect standard file descriptors
    sys.stdout.flush()
    sys.stderr.flush()
    si = open(os.devnull, 'r')
    so = open(os.devnull, 'a+')
    se = open(os.devnull, 'a+')

    os.dup2(si.fileno(), sys.stdin.fileno())
    os.dup2(so.fileno(), sys.stdout.fileno())
    os.dup2(se.fileno(), sys.stderr.fileno())

    # write pidfile and schedule deletion
    atexit.register(self.del_pid)
    self.write_pid()

  ## Delete pidfile.
  def del_pid(self):
    if os.path.isfile(self._pidfile):
      os.remove(self._pidfile)

  ## Start the daemon.
  def start(self):

    # Check for a pidfile to see if the daemon already runs
    self.read_pid()

    if self.pid:
      sys.stderr.write( 'PID file %s already exists: daemon already running?\n' % self._pidfile )
      sys.exit(1)

    # Start the daemon
    self.daemonize()
    self.run()

  ## Stop the daemon.
  def stop(self):

    # Get the pid from the pidfile
    self.read_pid()

    if not self.pid:
      sys.stderr.write( 'PID file %s does not exist: daemon not running?\n' % self._pidfile )
      return # not an error in a restart

    # Try killing the daemon process
    try:
      while True:
        os.kill(self.pid, signal.SIGTERM)
        time.sleep(0.1)
    except OSError as err:
      e = str(err.args)
      if e.find("No such process") > 0:
        self.del_pid()
      else:
        print (str(err.args))
        sys.exit(1)

  ## You should override this method when you subclass Daemon.
  #
  # It will be called after the process has been daemonized by `start()` or `restart()`.
  def run(self):
    pass
