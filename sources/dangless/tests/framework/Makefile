CC = gcc
AR = ar
RANLIB = ranlib

CFLAGS = -std=gnu11 -O0 -ggdb -Wall -Wextra -Wno-unused-parameter -DDEBUG
INCLUDES = -Iinclude

BIN = bin/libtestfx.a
MKCONFIG = bin/testfx.mk

SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c,obj/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)

.PHONY: all clean

all: $(BIN)

bin obj:
	mkdir -p $@

obj/%.o: src/%.c obj
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BIN): $(OBJS) $(MKCONFIG) bin
	$(AR) rcs $@ $(OBJS)
	$(RANLIB) $@

define MKCONFIG_DATA :=
TESTFX_ROOT := $(CURDIR)
TESTFX_BIN_DIR := $$(TESTFX_ROOT)/bin
TESTFX_INCLUDE_DIR := $$(TESTFX_ROOT)/include

TESTFX_USER_CC = gcc
TESTFX_USER_CFLAGS = -std=gnu11 -O0 -ggdb -DDEBUG -Wno-unused-parameter -I$$(TESTFX_ROOT)/include
TESTFX_USER_LDFLAGS = -L$$(TESTFX_ROOT)/bin -ltestfx
endef

$(MKCONFIG): bin
	$(file > $@,$(MKCONFIG_DATA))

clean:
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f $(BIN) $(MKCONFIG)
