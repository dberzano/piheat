#!/bin/bash -xe
[[ $(whoami) != root ]]
DEST=$HOME/piheat
cd /
[[ -d $DEST/.git ]] || git clone https://github.com/dberzano/piheat $DEST
cd $DEST
OLDHASH=$(git rev-parse HEAD)
git remote update -p || true  # non-fatal
BRANCH=$(git symbolic-ref HEAD 2>/dev/null) && BRANCH=${BRANCH##refs/heads/}
git branch -r | grep -qE "origin/$BRANCH\$"
if [[ $? == 0 ]]; then
  git clean -fdx
  git reset --hard origin/$BRANCH
fi
NEWHASH=$(git rev-parse HEAD)
export PATH=$DEST/server/usr/bin:$PATH
export PYTHONPATH=$DEST/server/pylib:$PYTHONPATH
export PYTHONDONTWRITEBYTECODE=1
WATCHDOG=$(cat /tmp/piheat_watchdog 2> /dev/null || echo 0)
NOW=$(date --utc +%s)
[[ $NEWHASH != $OLDHASH || "$((NOW-WATCHDOG))" -gt 100 ]] && piheat stop
piheat start
