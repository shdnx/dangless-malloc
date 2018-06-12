.SUFFIXES:

CC := gcc
AR := ar
RANLIB := ranlib

CFLAGS := -std=gnu11 -O0 -ggdb -Wall -Wextra -Wno-unused-parameter -DDEBUG -MMD -MP
INCLUDES := -Iinclude

LIBNAME := ctestfx
BIN := build/lib$(LIBNAME).a
MKCONFIG = build/ctestfx.mk

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
CTESTFX_ROOT := $(CURDIR)

CTESTFX_BIN_LIBNAME := $(LIBNAME)
CTESTFX_BIN := $(CURDIR)/$(BIN)
CTESTFX_BIN_DIR := $(CURDIR)/build
CTESTFX_INCLUDE_DIR := $(CURDIR)/include

CTESTFX_USER_CC = gcc
CTESTFX_USER_CFLAGS = -std=gnu11 -O0 -ggdb -DDEBUG -Wno-unused-parameter -I$$(CTESTFX_INCLUDE_DIR)
CTESTFX_USER_LDFLAGS = -L$$(CTESTFX_BIN_DIR) -l$$(CTESTFX_BIN_LIBNAME)
endef

clean:
	rm -rf build

-include $(DEPS)
