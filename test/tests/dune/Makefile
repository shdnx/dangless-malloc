DANGLESS_ROOT = ../..
FX_ROOT = ../framework

DANGLESS_BIN_DIR = $(DANGLESS_ROOT)/bin/dune
-include $(DANGLESS_BIN_DIR)/config.mk

ifndef DUNE_ROOT
$(error DUNE_ROOT is not set by $(DANGLESS_BIN_DIR)/config.mk)
endif

FX_BIN_DIR = $(FX_ROOT)/bin
-include $(FX_BIN_DIR)/testfx.mk

ifndef TESTFX_ROOT
$(error TESTFX_ROOT is not set by $(FX_BIN_DIR)/testfx.mk)
endif

BIN = bin/dangless-dune-test

CC = $(TESTFX_USER_CC)
INCLUDES = -I$(DUNE_ROOT) -I$(DANGLESS_INCLUDE_DIR) -I$(TESTFX_INCLUDE_DIR)
CFLAGS = $(TESTFX_USER_CFLAGS) $(DANGLESS_USER_CFLAGS)
LDFLAGS = $(TESTFX_USER_LDFLAGS) $(DANGLESS_USER_LDFLAGS)

SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,obj/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)

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
