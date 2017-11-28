#!/bin/bash -e

function mypip() {
  # Work around old pip still pointing to http instead of https
  printf "[easy_install]\nindex_url = https://pypi.python.org/simple/\n" > "$HOME"/.pydistutils.cfg
  for P in "$@"; do
    pip install --index-url https://pypi.python.org/simple --user $P
  done
}

cd "$(dirname $0)"/../lightlog

export PYTHONUSERBASE="$HOME/python-lightlog"
export PATH="$PYTHONUSERBASE/bin:$PATH"
REMOTE_STORE="lxplus.cern.ch:/eos/user/d/dberzano/www/mondata/read/"

if screen -ls | grep -q '\.lightlog\s'; then
  printf "Already running\n"
  exit 0
fi

# Check prerequisites. Pin Twisted and klein versions known to work
type sshpass &> /dev/null || { printf "Cannot find sshpass"; exit 1; }
lightlog --help &> /dev/null || mypip Twisted==16.0.0 klein==17.10.0 "-e ."

[[ -e ~/.lightlogcreds ]] || { printf "Cannot find SSH credentials under ~/.lightlogcreds\n"; exit 1; }

# Download data from remote persistent store
mkdir -p /var/tmp/lightlog
rsync -av \
      --rsh="sshpass -p `cat ~/.lightlogcreds|cut -d: -f2-` ssh -oUserKnownHostsFile=/dev/null -oStrictHostKeyChecking=no -l `cat ~/.lightlogcreds|cut -d: -f1`" \
      --delete \
      --delete-before \
      --include '*/' \
      --include "*/$(date -u +%Y/%m/%d.json)" \
      --exclude '*' \
      "$REMOTE_STORE" \
      /var/tmp/lightlog/ || true

screen -dmS lightlog \
            lightlog --after-dump-cmd 'rsync -av --rsh="sshpass -p `cat ~/.lightlogcreds|cut -d: -f2-` ssh -oUserKnownHostsFile=/dev/null -oStrictHostKeyChecking=no -l `cat ~/.lightlogcreds|cut -d: -f1`" {store_prefix}/ '"$REMOTE_STORE" \
                     --dump-every 60 \
                     --store /var/tmp/lightlog \
                     --host 0.0.0.0
