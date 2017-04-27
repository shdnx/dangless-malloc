CC = x86_64-rumprun-netbsd-gcc
RUMPRUN_BAKE = rumprun-bake
RUMPRUN_CONFIG = hw_generic
RUMPRUN_RUN = rumprun
RUMPRUN_PLATFORM = kvm
RUMPRUN_ROOT = ../rumprun
RUMPRUN_OBJ_PATH = $(RUMPRUN_ROOT)/obj-amd64-hw

INCLUDES = -Iinclude -I$(RUMPRUN_ROOT)/include -I$(RUMPRUN_ROOT)/src-netbsd/sys
CFLAGS = -std=gnu11 -Wall -Wextra -ggdb -O0 -DDEBUG
# $(RUMPRUN_OBJ_PATH)/lib/librumprun_base/librumprun_base.a $(RUMPRUN_OBJ_PATH)/lib/libbmk_core/libbmk_core.a $(RUMPRUN_OBJ_PATH)/lib/libbmk_rumpuser/libbmk_rumpuser.a
#LDFLAGS = -Wl,-whole-archive $(RUMPRUN_OBJ_PATH)/rumprun.o -Wl,-no-whole-archive
LDFLAGS = 
BIN = bin/testrump
IMG = bin/testrump.img

#SRCS = $(shell find src/ -type f -name '*.c')
SRCS = $(wildcard src/*.c) $(wildcard platform/rumprun/*.c)
OBJS = $(patsubst %.c,obj/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)

.PHONY: clean directories run

all: directories $(IMG)

directories:
	mkdir -p obj
	mkdir -p bin

obj/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

$(IMG): $(BIN)
	$(RUMPRUN_BAKE) $(RUMPRUN_CONFIG) $@ $<

run: $(IMG)
	$(RUMPRUN_RUN) $(RUMPRUN_PLATFORM) -i $<

clean:
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f $(BIN) $(IMG)

-include $(DEPS)
