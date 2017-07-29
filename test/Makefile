MKLASTCONFIG = bin/last-platform-config.mk

BIN_DIR = bin/$(PLATFORM)
MKCONFIG = $(BIN_DIR)/config.mk

# the PLATFORM variable should be supplied by the user
# if it's not specified, the last configuration is used, if any
ifdef PLATFORM
-include $(MKCONFIG)

else
-include $(MKLASTCONFIG)

ifndef PLATFORM
$(error PLATFORM is not set)
endif

endif

CONFIG_OVERRIDE_SYMBOLS ?= 1
CONFIG_AUTODEDICATE_PML4ES ?= 1
CONFIG_CALLOC_SPECIAL_BUFSIZE ?= 32

CC = gcc
AR = ar
RANLIB = ranlib

INCLUDES = -Iinclude
DEFINES = -DDANGLESS_OVERRIDE_SYMBOLS=$(CONFIG_OVERRIDE_SYMBOLS) -DDANGLESS_AUTO_DEDICATE_MAX_PML4ES=$(CONFIG_AUTODEDICATE_PML4ES) -DDANGLESS_CALLOC_SPECIAL_BUFSIZE=$(CONFIG_CALLOC_SPECIAL_BUFSIZE)
CFLAGS = -std=gnu11 -Wall -Wextra -ggdb -O0 -DDEBUG -pthread
BIN = $(BIN_DIR)/libdangless.a

define MKLASTCONFIG_DATA :=
-include $(realpath $(MKCONFIG))
endef

BASE_OBJS_DIR = obj/$(PLATFORM)

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

.PHONY: all clean always platform_bin platform_clean

all: $(BIN)

always:;

-include $(PLATFORM_SRCS_DIR)/Makefile.frag.mk

define MKCONFIG_DATA :=
PLATFORM ?= $(PLATFORM)
DANGLESS_BIN := $(realpath $(BIN))
DANGLESS_INCLUDE_DIR := $(realpath include)

CONFIG_OVERRIDE_SYMBOLS ?= $(CONFIG_OVERRIDE_SYMBOLS)
CONFIG_AUTODEDICATE_PML4ES ?= $(CONFIG_AUTODEDICATE_PML4ES)
CONFIG_CALLOC_SPECIAL_BUFSIZE ?= $(CONFIG_CALLOC_SPECIAL_BUFSIZE)

DANGLESS_USER_CFLAGS := -pthread $(PLATFORM_USER_CFLAGS)
DANGLESS_USER_LDFLAGS := -L$$(dir $$(DANGLESS_BIN)) -Wl,-whole-archive -ldangless -Wl,-no-whole-archive -pthread $(PLATFORM_USER_LDFLAGS)

$(PLATFORM_MKCONFIG_DATA)
endef

$(BIN_DIR) $(CORE_OBJS_DIR) $(PLATFORM_OBJS_DIR):
	mkdir -p $@

$(MKLASTCONFIG): $(BIN_DIR) always
	$(file > $@,$(MKLASTCONFIG_DATA))

$(MKCONFIG): $(BIN_DIR) always
	$(file > $@,$(MKCONFIG_DATA))

$(CORE_OBJS_DIR)/%.o: src/%.c $(CORE_OBJS_DIR)
	$(CC) $(CFLAGS) $(PLATFORM_CFLAGS) $(INCLUDES) $(PLATFORM_INCLUDES) $(DEFINES) $(PLATOFORM_DEFINES) -c $< -o $@

$(PLATFORM_OBJS_DIR)/%.o: src/platform/$(PLATFORM)/%.c $(PLATFORM_OBJS_DIR)
	$(CC) $(CFLAGS) $(PLATFORM_CFLAGS) $(INCLUDES) $(PLATFORM_INCLUDES) $(DEFINES) $(PLATOFORM_DEFINES) -c $< -o $@

$(BIN): $(BIN_DIR) $(MKCONFIG) $(MKLASTCONFIG) platform_bin

clean: platform_clean
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f $(BIN)
	rm -f $(MKCONFIG) $(MKLASTCONFIG)

clean_all:
	rm -rf bin
	rm -rf obj

print-%  : ; @echo $* = $($*)

-include $(DEPS)
