#!/bin/bash -ex
cd "$(dirname "$0")/.."
rsync -av \
      --exclude '.*' \
      --exclude '.*/' \
      --exclude 'priv*' \
      --delete \
      --delete-excluded \
      client/ dberzano@lxplus.cern.ch:www/ph/
echo Sync to lxplus done
