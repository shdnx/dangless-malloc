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

CC = gcc
AR = ar
RANLIB = ranlib

INCLUDES = -Iinclude
CFLAGS = -std=gnu11 -Wall -Wextra -ggdb -O0 -DDEBUG
LDFLAGS =
BIN = $(BIN_DIR)/libdangless.a

define MKLASTCONFIG_DATA :=
-include $(realpath $(MKCONFIG))
endef

OBJ_DIR = obj
BASE_OBJS_DIR = $(OBJ_DIR)/$(PLATFORM)

CORE_SRCS_DIR = src
CORE_SRCS = $(wildcard src/*.c)
CORE_OBJS_DIR = $(BASE_OBJS_DIR)/core
CORE_OBJS = $(patsubst $(CORE_SRCS_DIR)/%.c,$(CORE_OBJS_DIR)/%.o,$(CORE_SRCS))

PLATFORM_SRCS_DIR = platform/$(PLATFORM)
PLATFORM_SRCS = $(wildcard $(PLATFORM_SRCS_DIR)/*.c)
PLATFORM_OBJS_DIR = $(BASE_OBJS_DIR)/platform/$(PLATFORM)
PLATFORM_OBJS = $(patsubst $(PLATFORM_SRCS_DIR)/%.c,$(PLATFORM_OBJS_DIR)/%.o,$(PLATFORM_SRCS))

SRCS = $(CORE_SRCS) $(PLATFORM_SRCS)
OBJS = $(CORE_OBJS) $(PLATFORM_OBJS)
DEPS = $(OBJS:.o=.d)

.PHONY: all clean directories platform_bin platform_clean always

all: $(BIN)

always:;

directories:
	mkdir -p $(CORE_OBJS_DIR)
	mkdir -p $(PLATFORM_OBJS_DIR)
	mkdir -p $(BIN_DIR)

-include $(PLATFORM_SRCS_DIR)/Makefile.frag.mk

define MKCONFIG_DATA :=
PLATFORM ?= $(PLATFORM)
DANGLESS_BIN := $(realpath $(BIN))

DANGLESS_USER_CFLAGS := $(PLATFORM_USER_CFLAGS)
DANGLESS_USER_LDFLAGS := -L$$(dir $$(DANGLESS_BIN)) -Wl,-whole-archive -ldangless -Wl,-no-whole-archive -lpthread $(PLATFORM_USER_LDFLAGS)

$(PLATFORM_MKCONFIG_DATA)
endef

$(MKLASTCONFIG): always
	$(file > $@,$(MKLASTCONFIG_DATA))

$(MKCONFIG): always
	$(file > $@,$(MKCONFIG_DATA))

$(CORE_OBJS_DIR)/%.o: src/%.c
	$(CC) $(CFLAGS) $(PLATFORM_CFLAGS) $(INCLUDES) $(PLATFORM_INCLUDES) -c $< -o $@

$(PLATFORM_OBJS_DIR)/%.o: platform/$(PLATFORM)/%.c
	$(CC) $(CFLAGS) $(PLATFORM_CFLAGS) $(INCLUDES) $(PLATFORM_INCLUDES) -c $< -o $@

$(BIN): directories $(MKCONFIG) $(MKLASTCONFIG) platform_bin

clean: platform_clean
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f $(BIN)
	rm -f $(MKCONFIG) $(MKLASTCONFIG)

clean_all:
	rm -rf $(BIN_DIR)
	rm -rf $(OBJ_DIR)

print-%  : ; @echo $* = $($*)

-include $(DEPS)
