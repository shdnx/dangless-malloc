.SUFFIXES:

CC := gcc
AR := ar
RANLIB := ranlib

CFLAGS := -std=gnu11 -O0 -ggdb -Wall -Wextra -Wno-unused-parameter -DDEBUG -MMD -MP
INCLUDES := -Iinclude

LIBNAME := testfx
BIN := build/lib$(LIBNAME).a
MKCONFIG = build/testfx.mk

SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c,build/obj/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)

.PHONY: all clean

all: $(BIN)

build build/obj:
	mkdir -p $@

build/obj/%.o: src/%.c build/obj
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BIN): $(OBJS) build
	$(file > $(MKCONFIG),$(MKCONFIG_DATA))

	$(AR) rcs $@ $(OBJS)
	$(RANLIB) $@

define MKCONFIG_DATA :=
TESTFX_ROOT := $(CURDIR)

TESTFX_BIN_LIBNAME := $(LIBNAME)
TESTFX_BIN := $(CURDIR)/$(BIN)
TESTFX_BIN_DIR := $(CURDIR)/build
TESTFX_INCLUDE_DIR := $(CURDIR)/include

TESTFX_USER_CC = gcc
TESTFX_USER_CFLAGS = -std=gnu11 -O0 -ggdb -DDEBUG -Wno-unused-parameter -I$$(TESTFX_INCLUDE_DIR)
TESTFX_USER_LDFLAGS = -L$$(TESTFX_BIN_DIR) -l$$(TESTFX_BIN_LIBNAME)
endef

clean:
	rm -rf build

-include $(DEPS)
