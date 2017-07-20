DANGLESS_ROOT = ../..
DANGLESS_BIN_DIR = $(DANGLESS_ROOT)/bin/dune

-include $(DANGLESS_BIN_DIR)/config.mk

ifndef DUNE_ROOT
$(error DUNE_ROOT is not set by $(DANGLESS_BIN_DIR)/config.mk)
endif

BIN = bin/dangless-dune-test

INCLUDES = -I$(DUNE_ROOT) -I$(DANGLESS_ROOT)/include
CFLAGS = -std=gnu11 -Wall -Wextra -ggdb -O0 -DDEBUG
LDFLAGS =

SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,obj/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)

.PHONY: all directories clean run

all: directories $(BIN)

directories:
	mkdir -p bin
	mkdir -p obj

clean:
	rm -rf bin
	rm -rf obj

obj/%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# -Wl,-whole-archive -ldangless -Wl,-no-whole-archive ?? or something?
$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@ -L$(DANGLESS_BIN_DIR) -ldangless -L$(DUNE_ROOT)/libdune -ldune -ldl

# debug
print-%  : ; @echo $* = $($*)

-include $(DEPS)
