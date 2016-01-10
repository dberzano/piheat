#!/bin/bash -ex
cd "$(dirname "$0")/.."
DEST='www/ph'
[[ "$(git rev-parse --abbrev-ref HEAD 2> /dev/null || true)" == master ]] || DEST='www/ph-test'
rsync -av \
      --exclude '.*' \
      --exclude '.*/' \
      --exclude 'priv*' \
      --delete \
      --delete-excluded \
      client/ dberzano@lxplus.cern.ch:$DEST
echo Sync to lxplus done
