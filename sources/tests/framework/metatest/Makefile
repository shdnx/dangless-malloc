# NOTE: this is currently broken, since everything else was converted to use CMake instead of plan Makefiles. It won't be fixed until ctestfx is moved into a separate repository.

include ../build/ctestfx.mk

CC ?= gcc
CFLAGS := $(CTESTFX_USER_CFLAGS) -MMD -MP
LDFLAGS := $(CTESTFX_USER_LDFLAGS)

BIN := bin/metatest

#SRCS := $(wildcard *.c)
SRCS := metatest.c
OBJS := $(patsubst %.c,obj/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all clean

all: $(BIN)

bin obj preproc:
	mkdir -p $@

obj/%.o: %.c obj
	$(CC) $(CFLAGS) -c $< -o $@

preproc/%.c: %.c preproc
	$(CC) $(CFLAGS) -E $< -o $@

$(BIN): $(OBJS) bin $(CTESTFX_BIN)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

clean:
	rm -f $(OBJS)
	rm -f $(BIN)
	rm -rf preproc

-include $(DEPS)
