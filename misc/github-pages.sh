#!/bin/bash -ex
cd "$(dirname "$0")/.."
DIR=$(mktemp -d /tmp/piheat-gh-pages-XXXXX)
git init $DIR
rsync -av \
      --exclude '.*' \
      --exclude '.*/' \
      --exclude 'priv*' \
      client/ $DIR/
cd $DIR
git checkout --orphan gh-pages
git add --all -v :/
git status
git commit -m "Automatic commit at $(LANG=C date -u)" --allow-empty
git remote add github https://github.com/dberzano/piheat
git push -f github gh-pages:gh-pages
cd /
rm -rf $DIR
echo "PiHeat client deployed"
