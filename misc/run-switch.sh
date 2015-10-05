#!/bin/bash -xe
[[ $(whoami) == root ]]
DEST=/opt/piheat/piheat
PIDFILE=/tmp/rpi_switchctl.pid
LOG=/tmp/rpi_switchctl.log
PID=$(cat $PIDFILE 2> /dev/null || echo 123456789)
kill -0 $PID && false
nohup $DEST/lowlevel/rpi_switchctl/rpi_switchctl.py \
  --file=/tmp/heat \
  --user=piheat \
  --group=piheat \
  --mode=0600 \
  --pidfile=$PIDFILE \
  --pin 16 \
  --on-is-high > $LOG 2>&1 &
