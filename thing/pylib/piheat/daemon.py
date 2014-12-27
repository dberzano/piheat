## @package piheat
#  Generic linux daemon base class. Works with Python 3 too.
#  Inspired by http://www.jejik.com/files/examples/daemon3x.py but heavily modified.

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

  ## Are we running?
  #
  #  @return True or False
  def is_running(self):
    if self.pid is None:
      return False
    try:
      os.kill(self.pid, 0)
    except OSError:
      return False
    return True

  ## Start the daemon.
  def start(self):

    # Check for a pidfile to see if the daemon already runs
    self.read_pid()

    if self.is_running():
      sys.stderr.write( 'Daemon already running with PID %d\n' % self.pid );
      sys.exit(0)

    # Start the daemon
    self.daemonize()
    self.run()

  ## Stop the daemon.
  def stop(self):

    # Get the pid from the pidfile
    self.read_pid()

    if not self.is_running():
      sys.stderr.write('Daemon not running\n')
      sys.exit(0)

    # Try killing the daemon process gracefully
    kill_count = 0
    kill_count_threshold = 30
    try:

      while kill_count < kill_count_threshold:
        os.kill(self.pid, signal.SIGTERM)
        time.sleep(1)
        kill_count = kill_count + 1

      # force-kill
      os.kill(self.pid, signal.SIGKILL)
      time.sleep(2)

    except OSError:
      sys.stderr.write('Daemon exited gracefully\n')
      sys.exit(0)

    if self.is_running():
      sys.stderr.write('Could not terminate daemon!\n')
      sys.exit(1)
    else:
      sys.stderr.write('Daemon force-killed\n')

    sys.exit(0)

  ## You should override this method when you subclass Daemon.
  #
  # It will be called after the process has been daemonized by `start()` or `restart()`.
  def run(self):
    pass
