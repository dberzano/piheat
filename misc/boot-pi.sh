#!/bin/bash

# Quick and dirty boot script for RPi, to be included in /etc/rc.local

prefix="$( dirname "$0" )"/..
prefix="$( cd "$prefix" ; pwd )"

# Expose non-root interface for heating control
screen -dmS 'rpi_switchctl' \
  "${prefix}/lowlevel/rpi_switchctl/rpi_switchctl.py" \
    --file=/tmp/heat \
    --user=pi --group=pi --mode=0600 \
    --pidfile=/tmp/rpi_switchctl.pid \
    --pin=16

# Start the PiHeat server
screen -dmS 'piheat_server' \
  su pi -c "source ${prefix}/server/misc/env-test.sh ; piheat nodaemon"
