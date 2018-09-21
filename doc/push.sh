#!/usr/bin/env bash
set -eu

REPO_ROOT=$(cd $(pwd)/$(dirname $0)/.. && pwd)

"$REPO_ROOT"/doc/build.sh

if ! git diff-index --quiet HEAD --; then 
  echo Uncommitted changes. Will not push docs.
  exit -1
fi

TMP_DIR=$(mktemp -d)

SHA1=$(git rev-parse HEAD)

(cd "$TMP_DIR" 
git clone --recursive "$REPO_ROOT"
cd polymorphic_value
git checkout gh-pages
git remote set-url origin	git@github.com:jbcoe/polymorphic_value.git
rm -rf *
cp -Raf "$REPO_ROOT"/doc/html/* .
git add -A
git commit -m "Update docs to $SHA1"
git push -f)
