.SUFFIXES:

DANGLESS_ROOT := ../..
FX_ROOT := ../framework

include $(DANGLESS_ROOT)/build/last-config.mk

DANGLESS_BIN_DIR := $(DANGLESS_ROOT)/build/$(PLATFORM)_$(PROFILE)
include $(DANGLESS_BIN_DIR)/user.mk

# needed for DUNE_ROOT
include $(DANGLESS_BIN_DIR)/config.mk

FX_BIN_DIR = $(FX_ROOT)/build
include $(FX_BIN_DIR)/ctestfx.mk

BIN := build/dangless-dune-tests

CC ?= gcc

# DANGLESS_USER_CFLAGS doesn't include the include paths for dune, or dangless itself, because normal users don't need these things, they can just malloc() et al and not think about dangless or Dune
INCLUDES := -I$(DUNE_ROOT) $(DANGLESS_INCLUDES)
CFLAGS := $(CTESTFX_USER_CFLAGS) $(DANGLESS_USER_CFLAGS) -MMD -MP
LDFLAGS := $(CTESTFX_USER_LDFLAGS) $(DANGLESS_USER_LDFLAGS)

SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c,build/obj/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all clean run

all: $(BIN)

build build/obj:
	mkdir -p $@

clean:
	rm -rf build

build/obj/%.o: %.c build/obj
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BIN): $(OBJS) $(DANGLESS_BIN) build
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

run: $(BIN)
	$(BIN) $(ARGS)

# debug
print-%  : ; @echo $* = $($*)

-include $(DEPS)
