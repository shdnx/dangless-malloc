include ../bin/testfx.mk

CC := $(TESTFX_USER_CC)
CFLAGS := $(TESTFX_USER_CFLAGS)
LDFLAGS := $(TESTFX_USER_LDFLAGS)

BIN := bin/metatest

SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c,obj/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all clean

all: $(BIN)

bin obj preproc:
	mkdir -p $@

obj/%.o: %.c obj
	$(CC) $(CFLAGS) -c $< -o $@

preproc/%.c: src/%.c preproc
	$(CC) $(CFLAGS) -E $< -o $@

$(BIN): $(OBJS) bin
	$(CC) $(LDFLAGS) $(OBJS) -o $@

clean:
	rm -f $(OBJS)
	rm -f $(BIN)
	rm -rf preproc
