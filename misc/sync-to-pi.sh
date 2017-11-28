#!/bin/bash

# Syncs all to my Raspberry Pi.
# Not meant to be a production tool.

cd "$(dirname "$0")/.."

if [[ $1 == '--soft' ]] ; then
  flag=''
else
  flag='--delete'
fi

exec rsync -av $flag "$PWD"/ raspy:piheat/
