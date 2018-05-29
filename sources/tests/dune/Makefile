.SUFFIXES:

DANGLESS_ROOT := ../..
FX_ROOT := ../framework

DANGLESS_BIN_DIR := $(DANGLESS_ROOT)/build/dune
include $(DANGLESS_BIN_DIR)/config.mk

FX_BIN_DIR = $(FX_ROOT)/bin
include $(FX_BIN_DIR)/testfx.mk

BIN := bin/dangless-dune-test

CC := $(TESTFX_USER_CC)

# DANGLESS_USER_CFLAGS doesn't include the include paths for dune, or dangless itself, because normal users don't need these things, they can just malloc() et al and not think about dangless or Dune
INCLUDES := -I$(DUNE_ROOT) -I$(DANGLESS_INCLUDE_DIR) -I$(DANGLESS_BUILD_INCLUDE_DIR)
CFLAGS := $(TESTFX_USER_CFLAGS) $(DANGLESS_USER_CFLAGS) -MMD -MP
LDFLAGS := $(TESTFX_USER_LDFLAGS) $(DANGLESS_USER_LDFLAGS)

SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c,obj/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all clean run

all: $(BIN)

bin obj:
	mkdir -p $@

clean:
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f $(BIN)

obj/%.o: %.c obj
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BIN): $(OBJS) $(DANGLESS_BIN) bin
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

run: $(BIN)
	sudo $(BIN)

# debug
print-%  : ; @echo $* = $($*)

-include $(DEPS)
