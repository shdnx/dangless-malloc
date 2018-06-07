#!/bin/bash

if [ -z ${SPEC+x} ]; then
  source ./shrc
fi

cfg=$1
sets=$2

runspec --config="dangless-$cfg" --action "build" --rebuild $sets
result=$?

if [[ $result != 0 ]]; then
  echo "Build failed, aborting"
  exit $result
fi

echo "--- Build complete, running... ---"

runspec --config="dangless-$cfg" --nobuild --iterations 1 --ignore-errors --size "test" $sets
exit $?
