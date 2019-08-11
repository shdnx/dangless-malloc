include ../common.mk

CFLAGS := $(DANGLESS_USER_CFLAGS) $(DANGLESS_INCLUDES) -I$(DUNE_ROOT)
LDFLAGS := $(DANGLESS_USER_LDFLAGS)

BIN := bin/dune-mem-layout
SRCS := $(wildcard *.c)

.PHONY: clean run

all: $(BIN)

bin:
	mkdir $@

$(BIN): bin $(SRCS) $(DANGLESS_BIN)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRCS) -o $@

clean:
	rm -rf bin

run: $(BIN)
	$(BIN)

print-%  : ; @echo $* = $($*)
