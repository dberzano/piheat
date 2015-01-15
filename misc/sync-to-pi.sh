#!/bin/bash

# Syncs all to Raspberry Pi.
# Not meant to be a production tool.

cd "$( dirname "$0" )/.."

exec rsync -a --delete "$PWD"/ raspy.local:piheat/
