# This makefile fragment will get included by the global Makefile when PLATFORM=dune. It expects the DUNE_ROOT variable to be set.
ifndef DUNE_ROOT
$(error DUNE_ROOT is not set)
endif

PLATFORM_INCLUDES = -I$(DUNE_ROOT)/libdune
PLATOFORM_DEFINES =
PLATFORM_CFLAGS =

PLATFORM_USER_CFLAGS =
PLATFORM_USER_LDFLAGS = -L$(DUNE_ROOT)/libdune -ldune -ldl

define PLATFORM_MKCONFIG_DATA :=
DUNE_ROOT ?= $(realpath $(DUNE_ROOT))
endef

platform_bin: $(OBJS)
	$(AR) rcs $(BIN) $^
	$(RANLIB) $(BIN)

platform_clean:;
