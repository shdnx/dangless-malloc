#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

doSync () {
  local path_relative_to_root="sources/dangless"

  rsync \
    --verbose \
    --archive \
    --delete \
    --compress \
    --progress \
    --partial \
    --exclude=build \
    --exclude=bin \
    --exclude=obj \
    $@ \
    "$DIR/" \
    "homelan:~/remote/thesis/${path_relative_to_root}"
}

autoSync () {
  doSync
}

if [ $# -ge 1 ] && [ $1 == "--watch" ]; then
  echo "Watching $DIR for changes and performing sync automatically"

  fswatch -o $DIR | while read -r event; do autoSync; done
else
  doSync
fi
