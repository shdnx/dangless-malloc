# This makefile fragment will get included by the global Makefile when PLATFORM=rumprun. It expects the RUMPRUN_ROOT variable to exist.
ifndef RUMPRUN_ROOT
$(error RUMPRUN_ROOT is not set)
endif

# override the compiler
RUMPRUN_BIN_DIR := $(RUMPRUN_ROOT)/rumprun/bin
CC := $(RUMPRUN_BIN_DIR)/x86_64-rumprun-netbsd-gcc
AR := $(RUMPRUN_BIN_DIR)/x86_64-rumprun-netbsd-ar
RANLIB := $(RUMPRUN_BIN_DIR)/x86_64-rumprun-netbsd-ranlib

# stuff used by the main Makefile
PLATFORM_INCLUDES := -I$(RUMPRUN_ROOT)/include -I$(RUMPRUN_ROOT)/src-netbsd/sys
PLATFORM_DEFINES :=
PLATFORM_CFLAGS :=

PLATFORM_USER_CFLAGS :=
PLATFORM_USER_LDFLAGS :=

# $(RUMPRUN_OBJ_DIR)/lib/librumprun_base/librumprun_base.a $(RUMPRUN_OBJ_DIR)/lib/libbmk_core/libbmk_core.a $(RUMPRUN_OBJ_DIR)/lib/libbmk_rumpuser/libbmk_rumpuser.a
#LDFLAGS = -Wl,-whole-archive $(RUMPRUN_OBJ_DIR)/rumprun.o -Wl,-no-whole-archive
PLATFORM_LDFLAGS :=

define PLATFORM_MKCONFIG_DATA :=
RUMPRUN_ROOT ?= $(realpath $(RUMPRUN_ROOT))
RUMPRUN_BIN_DIR ?= $(realpath $(RUMPRUN_BIN_DIR))
RUMPRUN_OBJ_DIR ?= $(realpath $(RUMPRUN_ROOT)/obj-amd64-hw)

CC = $(realpath $(CC))
AR = $(realpath $(AR))
RANLIB = $(realpath $(RANLIB))
endef
