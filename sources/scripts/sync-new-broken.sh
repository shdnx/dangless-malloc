#!/bin/bash

# Figure out the directory containing the script
# source: https://stackoverflow.com/a/7126780/128240
dir_resolve() {
  cd "$1" 2>/dev/null || return $? # cd to desired directory; if fail, quell any error messages but return exit status
  echo "`pwd -P`" # output full, link-resolved path
}

DANGLESS_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )/../.."
DANGLESS_ROOT=`dir_resolve $DANGLESS_ROOT`

REMOTE_ROOT="~/remote/thesis"

# default options
OPT_WATCH=0
OPT_LOCAL=0
OPT_REMOTE="homelan"
OPT_DIR="sources"

# we're gonna using enchanced getopt, see https://stackoverflow.com/a/29754866/128240
getopt --test > /dev/null
if [[ $? -ne 4 ]]; then
  echo "Error: getopt --test failed"
  exit 1
fi

PARSED=$(getopt --options="wl" --longoptions="watch,local,remote:" --name "$0" -- "$@")
if [[ $? -ne 0 ]]; then
  # e.g. $? == 1
  #  then getopt has complained about wrong arguments to stdout
  exit 2
fi

# read getopt’s output this way to handle the quoting right:
eval set -- "$PARSED"

while true; do
  case "$1" in
    -w|--watch)
      OPT_WATCH=1
      shift
      ;;

    -l|--local)
      OPT_LOCAL=1
      OPT_REMOTE="homelan-local"
      shift
      ;;

    --remote)
      OPT_REMOTE="$2"
      shift 2
      ;;

    --)
      shift
      break
      ;;

    *)
      echo "Programming error"
      exit 3
      ;;
  esac
done

if [[ $# -ge 1 ]]; then
  OPT_DIR="$@"
fi

doSync () {
  dir=$1

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
    "${DANGLESS_ROOT}/$dir" \
    "${OPT_REMOTE}:${REMOTE_ROOT}/$dir"
}

doSyncAll () {
  doSync $OPT_DIR
}

if [ "$OPT_WATCH" -eq 1 ]; then
  doSyncAll

  echo "Watching directory '${DANGLESS_ROOT}/${OPT_DIR}' for changes and performing sync to '${OPT_REMOTE}:${REMOTE_ROOT}' automatically..."

  fswatch -o "${DANGLESS_ROOT}/${OPT_DIR}" | while read -r event; do doSyncAll; done
else
  # strange that we need that semicolon, otherwise Bash complains "unexpected token fi"
  doSyncAll;
fi
