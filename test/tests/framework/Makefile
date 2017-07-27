CC = gcc
CFLAGS = -std=gnu11 -O0 -ggdb -Wall -Wextra -Wno-unused-parameter -DDEBUG -Iinclude
LDFLAGS =
BIN = bin/metatest

SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c,obj/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)

.PHONY: all clean

all: $(BIN)

bin obj preproc:
	mkdir -p $@

obj/%.o: src/%.c obj
	$(CC) $(CFLAGS) -c $< -o $@

preproc/%.c: src/%.c preproc
	$(CC) $(CFLAGS) -E $< -o $@

$(BIN): $(OBJS) bin
	$(CC) $(LDFLAGS) $(OBJS) -o $@

clean:
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f $(BIN)
