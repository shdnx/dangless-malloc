# Hack to disable implicit rules to VASTLY improve performance; see https://stackoverflow.com/a/4126617/128240
.SUFFIXES:

BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/$(PLATFORM)

MKCONFIG = $(BIN_DIR)/config.mk
MKLASTCONFIG = $(BUILD_DIR)/last-platform-config.mk

BUILDCONFIG_INCLUDE_PATH = $(BUILD_DIR)/include
BUILDCONFIG_HEADER_DIR = $(BUILDCONFIG_INCLUDE_PATH)/dangless
BUILDCONFIG_HEADER_FILE = $(BUILDCONFIG_HEADER_DIR)/buildconfig.h

# the PLATFORM variable should be supplied by the user
# if it's not specified, the last configuration is used, if any
ifdef PLATFORM
-include $(MKCONFIG)

else
-include $(MKLASTCONFIG)

ifndef PLATFORM
$(error No existing build configuration: set PLATFORM and run 'make config' first.)
endif

endif # end ifdef PLATFORM

-include make/buildconfig-defaults.mk

# TODO: shouldn't this be moved to the top? otherwise I can't see how the platform-specific makefile could override it
CC = gcc
AR = ar
RANLIB = ranlib

INCLUDES = -Iinclude -I$(BUILDCONFIG_INCLUDE_PATH)
DEFINES = -DDEBUG
CFLAGS = -std=gnu11 -Wall -Wextra -Wno-format-zero-length -ggdb -O0 -pthread -MMD -MP
BIN = $(BIN_DIR)/libdangless.a

BASE_OBJS_DIR = $(BUILD_DIR)/obj/$(PLATFORM)

CORE_SRCS_DIR = src
CORE_SRCS = $(wildcard src/*.c)
CORE_OBJS_DIR = $(BASE_OBJS_DIR)/core
CORE_OBJS = $(patsubst $(CORE_SRCS_DIR)/%.c,$(CORE_OBJS_DIR)/%.o,$(CORE_SRCS))

PLATFORM_SRCS_DIR = src/platform/$(PLATFORM)
PLATFORM_SRCS = $(wildcard $(PLATFORM_SRCS_DIR)/*.c)
PLATFORM_OBJS_DIR = $(BASE_OBJS_DIR)/platform/$(PLATFORM)
PLATFORM_OBJS = $(patsubst $(PLATFORM_SRCS_DIR)/%.c,$(PLATFORM_OBJS_DIR)/%.o,$(PLATFORM_SRCS))

SRCS = $(CORE_SRCS) $(PLATFORM_SRCS)
OBJS = $(CORE_OBJS) $(PLATFORM_OBJS)
DEPS = $(OBJS:.o=.d)

.PHONY: all config clean clean_full always platform_bin platform_clean test

all: $(BIN)

always:;

-include $(PLATFORM_SRCS_DIR)/Makefile.frag.mk

-include make/buildconfig.mk

$(BIN_DIR) $(BUILDCONFIG_HEADER_DIR) $(CORE_OBJS_DIR) $(PLATFORM_OBJS_DIR):
	mkdir -p $@

$(CORE_OBJS_DIR)/%.o: src/%.c $(CORE_OBJS_DIR) $(BUILDCONFIG_HEADER_FILE)
	$(CC) $(CFLAGS) $(PLATFORM_CFLAGS) $(INCLUDES) $(PLATFORM_INCLUDES) $(DEFINES) $(PLATOFORM_DEFINES) -c $< -o $@

$(PLATFORM_OBJS_DIR)/%.o: src/platform/$(PLATFORM)/%.c $(PLATFORM_OBJS_DIR) $(BUILDCONFIG_HEADER_FILE)
	$(CC) $(CFLAGS) $(PLATFORM_CFLAGS) $(INCLUDES) $(PLATFORM_INCLUDES) $(DEFINES) $(PLATOFORM_DEFINES) -c $< -o $@

$(BIN): $(BIN_DIR) platform_bin

clean: platform_clean
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f $(BIN)
	rm -f $(BUILDCONFIG_FILES)

clean_full:
	rm -rf $(BUILD_DIR)

test: $(BIN)
	make -C tests/framework
	make -C tests/$(PLATFORM) run

# can be used to dump Makefile variables for debugging purposes
print-%  : ; @echo $* = $($*)

-include $(DEPS)
