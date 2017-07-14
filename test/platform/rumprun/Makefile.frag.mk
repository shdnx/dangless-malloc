# This makefile fragment will get included by the global Makefile when PLATFORM=rumprun. It expects the RUMPRUN_ROOT variable to exist.
ifndef RUMPRUN_ROOT
$(error RUMPRUN_ROOT is not set)
endif

.PHONY: run

# override the compiler
CC = x86_64-rumprun-netbsd-gcc

# internal variables
UNBAKED_BIN := $(BIN).raw

# internal rumprun config
RUMPRUN_BAKE = rumprun-bake
RUMPRUN_CONFIG = hw_generic
RUMPRUN_RUN = rumprun
RUMPRUN_PLATFORM = kvm
RUMPRUN_OBJ_PATH = $(RUMPRUN_ROOT)/obj-amd64-hw

# stuff used by the main Makefile
PLATFORM_INCLUDES = -I$(RUMPRUN_ROOT)/include -I$(RUMPRUN_ROOT)/src-netbsd/sys
PLATFORM_CFLAGS =

# $(RUMPRUN_OBJ_PATH)/lib/librumprun_base/librumprun_base.a $(RUMPRUN_OBJ_PATH)/lib/libbmk_core/libbmk_core.a $(RUMPRUN_OBJ_PATH)/lib/libbmk_rumpuser/libbmk_rumpuser.a
#LDFLAGS = -Wl,-whole-archive $(RUMPRUN_OBJ_PATH)/rumprun.o -Wl,-no-whole-archive
PLATFORM_LDFLAGS =

$(UNBAKED_BIN): $(OBJS)
	$(CC) $(LDFLAGS) $(PLATFORM_LDFLAGS) $^ -o $@

platform_bin: $(UNBAKED_BIN)
	$(RUMPRUN_BAKE) $(RUMPRUN_CONFIG) $(BIN) $<

platform_clean:
	rm -f $(UNBAKED_BIN)

run: $(BIN)
	$(RUMPRUN_RUN) $(RUMPRUN_PLATFORM) -i $<