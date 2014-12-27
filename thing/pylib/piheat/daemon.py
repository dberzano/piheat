## @file daemon.py
#  Create a generic pythonic daemon.
#
#  Should work with Python 3 too.
#
#  Originally inspired by [this code](http://www.jejik.com/files/examples/daemon3x.py) but then
#  heavily improved.

import sys, os, time, atexit, signal

## @class Daemon
#  Abstract pytonic daemon class.
#
#  **Usage:** subclass it and override the `run()` method. Use `start()` to start it in background,
#  `stop()` to terminate it. Class must be initialized by providing a pidfile path.
#
#  Example code:
#
#  ~~~{.py}
#  class MyClass(Daemon):
#    def run(self):
#      # do things here
#
#  prog = MyClass('/tmp/myclass.pid')
#  prog.start()
#  ~~~
class Daemon:

  ## Constructor.
  #
  #  @param name    Arbitrary nickname for the daemon
  #  @param pidfile Full path to PID file. Path must exist
  def __init__(self, name, pidfile):
    ## Path to the file where to write the current PID
    self._pidfile = pidfile
    ## Daemon's nickname
    self.name = name
    ## PID of daemon
    self.pid = None

  ## Write PID to pidfile.
  def write_pid(self):
    with open(self._pidfile, 'w') as pf:
      pf.write( str(self.pid) + '\n' )

  ## Read PID from pidfile.
  def read_pid(self):
    try:
      with open(self._pidfile, 'r') as pf:
        self.pid = int( pf.read().strip() )
    except (IOError, ValueError):
      self.pid = None

  ## Daemonize class. Uses the [Unix double-fork technique](http://stackoverflow.com/questions/88138
  #8/what-is-the-reason-for-performing-a-double-fork-when-creating-a-daemon).
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

  ## Determines if a daemon with the current PID is running by sending a dummy signal.
  #
  #  @return True if running, False if not
  def is_running(self):
    if self.pid is None:
      return False
    try:
      os.kill(self.pid, 0)
    except OSError:
      return False
    return True

  ## Start the daemon. Daemon is sent to background then started.
  def start(self):

    # Check for a pidfile to see if the daemon already runs
    self.read_pid()

    if self.is_running():
      sys.stderr.write( 'Daemon already running with PID %d\n' % self.pid );
      sys.exit(0)

    # Start the daemon
    self.daemonize()
    self.run()

  ## Returns the status of the daemon. If daemon is running, its PID is printed.
  def status(self):
    self.read_pid()
    if self.is_running():
      sys.stderr.write('Daemon running with PID %d\n' % self.pid)
      sys.exit(0)
    else:
      sys.stderr.write('Daemon not running\n')
      sys.exit(1)

  ## Stop the daemon.
  #
  #  An attempt to kill the daemon is performed for 30 seconds sending **signal 15 (SIGTERM)**: if
  #  the daemon is implemented properly, it will perform its shutdown operations and it will exit
  #  gracefully.
  #
  #  If the daemon is still running after this termination attempt, **signal 9 (KILL)** is sent, and
  #  daemon is abruptly terminated.
  #
  #  Note that this attempt might fail as well.
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

  ## Dummy method to be overridden by subclasses.
  #
  #  It will be called after the process has been daemonized by `start()`.
  def run(self):
    pass
