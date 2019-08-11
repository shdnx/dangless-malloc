#!/bin/bash

: "${REMOTE:=szfvar_thesis}"

REMOTE_ROOT="~/remote/thesis"
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )/.."

# source: https://stackoverflow.com/a/7126780/128240
dir_resolve() {
  cd "$1" 2>/dev/null || return $? # cd to desired directory; if fail, quell any error messages but return exit status
  echo "`pwd -P`" # output full, link-resolved path
}

DIR=`dir_resolve $DIR`

doSync () {
  local path_relative_to_root="sources"

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
    "${REMOTE}:${REMOTE_ROOT}/${path_relative_to_root}"
}

autoSync () {
  doSync
}

if [ $# -ge 1 ] && [ $1 == "--watch" ]; then
  autoSync

  echo "Watching $DIR for changes and performing sync automatically"

  fswatch -o $DIR | while read -r event; do autoSync; done
else
  # strange that we need that semicolon, otherwise Bash complains "unexpected token fi"
  doSync;
fi
