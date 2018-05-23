#!/bin/bash
RUMPRUN_OBJ_ROOT=../rumprun/obj-amd64-hw
SYMBOL_NAME=$1
PATTERN=$2

for lib in `find "$RUMPRUN_OBJ_ROOT" -name $PATTERN`; do
  nm --defined-only "$lib" | grep "$SYMBOL_NAME"
  if [[ $? == 0 ]]; then
    echo "$lib"
  fi
done