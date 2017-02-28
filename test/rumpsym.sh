#!/bin/bash

RUMPRUN_OBJ_ROOT=../rumprun/obj-amd64-hw
SYMNAME=$1

nm "$RUMPRUN_OBJ_ROOT/rumprun.o" | grep "$SYMNAME"