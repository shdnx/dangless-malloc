include ../common.mk

CXXFLAGS := $(DANGLESS_USER_CFLAGS)
LDFLAGS := $(DANGLESS_USER_LDFLAGS)

BIN := bin/cpp-ofstream
SRCS := $(wildcard *.cpp)

.PHONY: clean run

all: $(BIN)

bin:
	mkdir $@

$(BIN): bin $(SRCS) $(DANGLESS_BIN)
	$(CXX) -std=c++11 $(CXXFLAGS) $(LDFLAGS) $(SRCS) -o $@

clean:
	rm -rf bin

run: $(BIN)
	$(BIN)

print-%  : ; @echo $* = $($*)
