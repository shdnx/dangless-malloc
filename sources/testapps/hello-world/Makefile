include ../common.mk

CFLAGS := $(DANGLESS_USER_CFLAGS)
LDFLAGS := $(DANGLESS_USER_LDFLAGS)

BIN := ./hello-world
SRCS := $(wildcard *.c)

.PHONY: clean run

all: $(BIN)

$(BIN): $(SRCS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

clean:
	rm -f $(BIN)

run: $(BIN)
	$(BIN)

print-%  : ; @echo $* = $($*)
