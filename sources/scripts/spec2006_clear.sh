#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# source: https://stackoverflow.com/a/7126780/128240
dir_resolve() {
  cd "$1" 2>/dev/null || return $? # cd to desired directory; if fail, quell any error messages but return exit status
  echo "`pwd -P`" # output full, link-resolved path
}

spec_dir=`dir_resolve "$DIR/../../vendor/spec2006"`
base_dir="$spec_dir/benchspec/CPU2006"

for dir_name in `ls $base_dir`; do
  dir_path="$base_dir/$dir_name"
  if [ -d "$dir_path" ]; then
    echo "rm -rf \"$dir_path/run\""
    rm -rf "$dir_path/run"
  fi
done
