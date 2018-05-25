DANGLESS_ROOT = ../..
DANGLESS_BIN_DIR = $(DANGLESS_ROOT)/build/rumprun

-include $(DANGLESS_BIN_DIR)/config.mk

ifndef RUMPRUN_ROOT
$(error RUMPRUN_ROOT is not set by $(DANGLESS_BIN_DIR)/config.mk)
endif

RUMPRUN_BAKE = $(RUMPRUN_BIN_DIR)/rumprun-bake
RUMPRUN_BAKE_CONFIG = hw_generic
RUMPRUN_RUN = $(RUMPRUN_BIN_DIR)/rumprun
RUMPRUN_RUN_PLATFORM = kvm

BIN = bin/dangless-rumprun-test
UNBAKED_BIN = $(BIN).raw

#INCLUDES = -I$(RUMPRUN_ROOT)/include -I$(RUMPRUN_ROOT)/src-netbsd/sys
INCLUDES = -I$(DANGLESS_ROOT)/include
CFLAGS = -std=gnu11 -Wall -Wextra -ggdb -O0 -DDEBUG
LDFLAGS =

SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,obj/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)

.PHONY: all directories clean run

all: $(BIN)

directories:
	mkdir -p bin
	mkdir -p obj

clean:
	rm -rf bin
	rm -rf obj

obj/%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# -Wl,-whole-archive -ldangless -Wl,-no-whole-archive ?? or something?
$(UNBAKED_BIN): directories $(OBJS) $(DANGLESS_BIN)
	$(CC) $(LDFLAGS) $(OBJS) -o $@ -L$(DANGLESS_BIN_DIR) -Wl,-whole-archive -ldangless -Wl,-no-whole-archive

$(BIN): $(UNBAKED_BIN)
	$(RUMPRUN_BAKE) $(RUMPRUN_BAKE_CONFIG) $@ $<

run: $(BIN)
	$(RUMPRUN_RUN) $(RUMPRUN_RUN_PLATFORM) -i $<

# debug
print-%  : ; @echo $* = $($*)

-include $(DEPS)
