CC = x86_64-rumprun-netbsd-gcc
RUMPRUN_BAKE = rumprun-bake
RUMPRUN_CONFIG = hw_generic
RUMPRUN_RUN = rumprun
RUMPRUN_PLATFORM = kvm
RUMPRUN_ROOT = ../rumprun
RUMPRUN_OBJ_PATH = $(RUMPRUN_ROOT)/obj-amd64-hw
CFLAGS = -Iinclude -I$(RUMPRUN_ROOT)/include -I$(RUMPRUN_ROOT)/rumprun/src-netbsd/sys
LDFLAGS = -L$(RUMPRUN_OBJ_PATH)/lib/libbmk_core -Wl,-whole-archive -l:libbmk_core.a -Wl,-no-whole-archive
BIN = bin/testrump
IMG = bin/testrump.img

SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,obj/%.o,$(SRCS))

.PHONY: clean directories run

all: directories $(IMG)

directories:
	mkdir -p obj
	mkdir -p bin

obj/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) $(RUMPRUN_OBJ_PATH)/platform/arch/amd64/*.o $^ -o $@

$(IMG): $(BIN)
	$(RUMPRUN_BAKE) $(RUMPRUN_CONFIG) $@ $<

run: $(IMG)
	$(RUMPRUN_RUN) -D $(RUMPRUN_PLATFORM) -i $<

clean:
	rm -f obj/*
	rm -f bin/*
