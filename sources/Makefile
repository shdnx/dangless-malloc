# Hack to disable implicit rules to VASTLY improve performance; see https://stackoverflow.com/a/4126617/128240
.SUFFIXES:

ROOT := $(CURDIR)
BUILD_DIR := build

MKLASTCONFIG := $(BUILD_DIR)/last-config.mk
-include $(MKLASTCONFIG)

PROFILE ?= debug
PLATFORM ?= dune

# have to defer evaluation on this, as it requires PLATFORM, but it may not be set yet!
BIN_DIR := $(BUILD_DIR)/$(PLATFORM)_$(PROFILE)
MKCONFIG := $(BIN_DIR)/config.mk
MKUSER := $(BIN_DIR)/user.mk

-include $(MKCONFIG)

BUILDCONFIG_COMMON_INCLUDE_PATH := $(BUILD_DIR)/include
BUILDCONFIG_SPECIALIZED_INCLUDE_PATH := $(BIN_DIR)/include

SYSCALLMETA_FILE := $(BUILDCONFIG_COMMON_INCLUDE_PATH)/dangless/build/common/syscallmeta.inc

BUILDCONFIG_HEADER_FILE := $(BUILDCONFIG_SPECIALIZED_INCLUDE_PATH)/dangless/build/spec/config.h

include make/buildconfig-defaults.mk

# TODO: shouldn't this be moved to the top? otherwise I can't see how the platform-specific makefile could override it
CC := gcc
AR := ar
RANLIB := ranlib

INCLUDES := -I$(BUILDCONFIG_SPECIALIZED_INCLUDE_PATH) -I$(BUILDCONFIG_COMMON_INCLUDE_PATH) -Iinclude
CFLAGS += -std=gnu11 -Wall -Wextra -Wno-format-zero-length -Wno-sign-compare -pthread -MMD -MP

ifeq ($(PROFILE),debug)
	CFLAGS += -DDEBUG -ggdb -O0
else
	CFLAGS += -DNDEBUG -Ofast
endif

BIN := $(BIN_DIR)/libdangless.a

BASE_OBJS_DIR := $(BIN_DIR)/obj

CORE_SRCS_DIR := src
CORE_SRCS := $(wildcard $(CORE_SRCS_DIR)/*.c) $(wildcard $(CORE_SRCS_DIR)/common/*.c)
CORE_OBJS_DIR := $(BASE_OBJS_DIR)/core
CORE_OBJS := $(patsubst $(CORE_SRCS_DIR)/%.c,$(CORE_OBJS_DIR)/%.o,$(CORE_SRCS))

PLATFORM_SRCS_DIR := src/platform/$(PLATFORM)
PLATFORM_SRCS := $(wildcard $(PLATFORM_SRCS_DIR)/*.c)
PLATFORM_OBJS_DIR := $(BASE_OBJS_DIR)/platform
PLATFORM_OBJS := $(patsubst $(PLATFORM_SRCS_DIR)/%.c,$(PLATFORM_OBJS_DIR)/%.o,$(PLATFORM_SRCS))

SRCS := $(CORE_SRCS) $(PLATFORM_SRCS)
OBJS := $(CORE_OBJS) $(PLATFORM_OBJS)
DEPS := $(OBJS:.o=.d)

.PHONY: all config show-config clean clean-all test always

all: $(BIN)

always:;

include $(PLATFORM_SRCS_DIR)/Makefile.frag.mk

include make/buildconfig.mk

$(BIN_DIR) $(dir $(SYSCALLMETA_FILE)) $(dir $(BUILDCONFIG_HEADER_FILE)) $(CORE_OBJS_DIR) $(CORE_OBJS_DIR)/common $(PLATFORM_OBJS_DIR):
	mkdir -p $@

$(SYSCALLMETA_FILE): $(dir $(SYSCALLMETA_FILE)) scripts/generate_syscallmeta.py
	python3 scripts/generate_syscallmeta.py /usr/src/linux-headers-`uname -r` > $@

$(CORE_OBJS_DIR)/%.o: src/%.c
	$(CC) $(CFLAGS) $(PLATFORM_CFLAGS) $(INCLUDES) $(PLATFORM_INCLUDES) -c $< -o $@

$(PLATFORM_OBJS_DIR)/%.o: src/platform/$(PLATFORM)/%.c
	$(CC) $(CFLAGS) $(PLATFORM_CFLAGS) $(INCLUDES) $(PLATFORM_INCLUDES) -c $< -o $@

$(BIN): $(BIN_DIR) $(CORE_OBJS_DIR) $(CORE_OBJS_DIR)/common $(PLATFORM_OBJS_DIR) $(SYSCALLMETA_FILE) $(OBJS)
	$(AR) rcs $@ $(OBJS)
	$(RANLIB) $@

clean:
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f $(BIN)

clean-all:
	rm -rf $(BUILD_DIR)

test: $(BIN)
	$(MAKE) -C tests/framework
	$(MAKE) -C tests/$(PLATFORM) run ARGS="$(ARGS)"

testapp: $(BIN)
	$(MAKE) -C testapps/$(APP) run

# can be used to dump Makefile variables for debugging purposes
print-%  : ; @echo $* = $($*)

-include $(DEPS)
