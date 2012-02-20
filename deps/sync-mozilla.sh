#!/bin/bash
# This script takes the root directory of the mozilla directory
# and syncs the relevant locations into the spidernode tree.
# Notes:
#  1. this script doesn't touch the mozilla tree
#  2. this script will add cruft in the mozilla tree

set -ex

test "x$1" != "x"
test -d $1

rsync -av --delete $1/nsprpub mozjs/
rsync -av --delete $1/mfbt mozjs/
rsync -av --delete $1/js mozjs
git add mozjs
git rm -r `git ls-files --deleted mozjs`
rev=`(cd $1 && git rev-parse HEAD)`
git commit -m "syncing mozilla dependencies from revision $rev"
