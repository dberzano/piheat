#!/bin/bash

# Syncs all to Raspberry Pi.
# Not meant to be a production tool.

cd "$( dirname "$0" )/.."

if [[ $1 == '--soft' ]] ; then
  flag=''
else
  flag='--delete'
fi

exec rsync -a $flag "$PWD"/ raspy.local:piheat/
