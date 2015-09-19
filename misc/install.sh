#!/bin/bash -ex
[[ $(whoami) == root ]] 
PIUSER=piheat
PIHOME=/opt/piheat
PICONF=$PIHOME/.piheat/piheat.json
groupadd $PIUSER || true
id $PIUSER || useradd -d $PIHOME -g $PIUSER $PIUSER
crontab -u $PIUSER -r || true
mkdir -p $PIHOME/.piheat
[[ -d $PIHOME/piheat/.git ]] || git clone https://github.com/dberzano/piheat $PIHOME/piheat
if [[ ! -e $PICONF ]]; then
  cp $PIHOME/piheat/server/etc/piheat.json.example $PICONF
  set +x
  read -p "Name: " THING_NAME
  read -p "ID: " THING_ID
  read -sp "Password: " THING_PASSWORD
  export THING_NAME THING_ID THING_PASSWORD
  set -x
  PY=$(mktemp)
  cat > $PY <<-EOF
from os import environ
from json import loads,dumps
c=loads(open("$PICONF").read())
c["password"]=environ["THING_PASSWORD"]
c["thingid"]=environ["THING_ID"]
c["thingname"]=environ["THING_NAME"]
open("$PICONF", "w").write(dumps(c, indent=2))
EOF
  python $PY
  rm -f $PY
fi
chown -R $PIUSER:$PIUSER $PIHOME/
crontab -u $PIUSER - <<EOF
@reboot $PIHOME/piheat/misc/run.sh > /dev/null 2>&1
* * * * * $PIHOME/piheat/misc/run.sh > /dev/null 2>&1
EOF
(
  (crontab -l 2> /dev/null || true) | grep -v run-switch.sh
  echo "@reboot $PIHOME/piheat/misc/run-switch.sh > /dev/null 2>&1"
  echo "* * * * * $PIHOME/piheat/misc/run-switch.sh > /dev/null 2>&1"
) | crontab
set +x
echo Installation complete
