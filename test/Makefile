CC = x86_64-rumprun-netbsd-gcc
RUMPRUN_BAKE = rumprun-bake
RUMPRUN_CONFIG = hw_generic
RUMPRUN_RUN = rumprun
RUMPRUN_PLATFORM = kvm
RUMPRUN_ROOT = ../rumprun
RUMPRUN_OBJ_PATH = $(RUMPRUN_ROOT)/obj-amd64-hw
CFLAGS = -Iinclude -I$(RUMPRUN_ROOT)/include -I$(RUMPRUN_ROOT)/src-netbsd/sys
# $(RUMPRUN_OBJ_PATH)/lib/librumprun_base/librumprun_base.a $(RUMPRUN_OBJ_PATH)/lib/libbmk_core/libbmk_core.a $(RUMPRUN_OBJ_PATH)/lib/libbmk_rumpuser/libbmk_rumpuser.a
#LDFLAGS = -Wl,-whole-archive $(RUMPRUN_OBJ_PATH)/rumprun.o -Wl,-no-whole-archive
LDFLAGS = 
BIN = bin/testrump
IMG = bin/testrump.img

SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c,obj/%.o,$(SRCS))

.PHONY: clean directories run

all: directories $(IMG)

directories:
	mkdir -p obj
	mkdir -p bin

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

$(IMG): $(BIN)
	$(RUMPRUN_BAKE) $(RUMPRUN_CONFIG) $@ $<

run: $(IMG)
	$(RUMPRUN_RUN) $(RUMPRUN_PLATFORM) -i $<

clean:
	rm -f obj/*
	rm -f bin/*
