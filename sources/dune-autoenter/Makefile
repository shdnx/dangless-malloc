.SUFFIXES:

BUILD_DIR := build
BIN := $(BUILD_DIR)/libdune-autoenter.a

CC := gcc
AR := ar
RANLIB := ranlib

CFLAGS += -std=gnu11 -Wall -Wextra -pthread -MMD -MP -I$(DUNE_ROOT)/libdune

ifeq ($(PROFILE),debug)
  CFLAGS += -DDEBUG -ggdb -O0
else
  CFLAGS += -DNDEBUG -Ofast
endif

SRCS := $(wildcard *.c)
OBJS_DIR := $(BUILD_DIR)/obj
OBJS := $(patsubst %.c,$(OBJS_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all info clean clean-all

all: $(BIN)

$(BUILD_DIR) $(OBJS_DIR):
	mkdir -p $@

$(OBJS_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJS_DIR) $(OBJS)
	$(AR) rcs $@ $(OBJS)
	$(RANLIB) $@

	@echo ""
	@$(MAKE) -s --no-print-directory info

info:
	@echo "-- To link target application to this library and to Dune:"
	@echo ""
	@echo "LDFLAGS += -L$(realpath $(BUILD_DIR)) -ldune-autoenter -L$(DUNE_ROOT)/libdune -ldune -ldl -pthread"

clean:
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f $(BIN)

clean-all:
	rm -rf $(BUILD_DIR)

# can be used to dump Makefile variables for debugging purposes
print-%  : ; @echo $* = $($*)

-include $(DEPS)
