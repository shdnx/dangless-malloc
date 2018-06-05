define CBUILDCONFIG_DATA :=
#ifndef DANGLESS_BUILDCONFIG_H
#define DANGLESS_BUILDCONFIG_H

#define DANGLESS_CONFIG_PROFILE $(PROFILE)
#define DANGLESS_CONFIG_PLATFORM $(PLATFORM)

#define DANGLESS_CONFIG_OVERRIDE_SYMBOLS $(CONFIG_OVERRIDE_SYMBOLS)
#define DANGLESS_CONFIG_REGISTER_PREINIT $(CONFIG_REGISTER_PREINIT)

#define DANGLESS_CONFIG_AUTO_DEDICATE_MAX_PML4ES $(CONFIG_AUTODEDICATE_PML4ES)
#define DANGLESS_CONFIG_CALLOC_SPECIAL_BUFSIZE $(CONFIG_CALLOC_SPECIAL_BUFSIZE)
#define DANGLESS_CONFIG_VIRTMEMALLOC_STATS $(CONFIG_VIRTMEMALLOC_STATS)
#define DANGLESS_CONFIG_SYSCALLMETA_HAS_INFO $(CONFIG_SYSCALLMETA_HAS_INFO)
#define DANGLESS_CONFIG_PRINTF_NOMALLOC_BUFFER_SIZE $(CONFIG_PRINTF_NOMALLOC_BUFFER_SIZE)

#define DANGLESS_CONFIG_DEBUG_DGLMALLOC $(CONFIG_DEBUG_DGLMALLOC)
#define DANGLESS_CONFIG_DEBUG_VIRTMEM $(CONFIG_DEBUG_VIRTMEM)
#define DANGLESS_CONFIG_DEBUG_VIRTMEMALLOC $(CONFIG_DEBUG_VIRTMEMALLOC)
#define DANGLESS_CONFIG_DEBUG_VIRTREMAP $(CONFIG_DEBUG_VIRTREMAP)
#define DANGLESS_CONFIG_DEBUG_SYSMALLOC $(CONFIG_DEBUG_SYSMALLOC)
#define DANGLESS_CONFIG_DEBUG_INIT $(CONFIG_DEBUG_INIT)
#define DANGLESS_CONFIG_DEBUG_PHYSMEM_ALLOC $(CONFIG_DEBUG_PHYSMEM_ALLOC)

#define DANGLESS_CONFIG_TRACE_SYSCALLS $(CONFIG_TRACE_SYSCALLS)

#endif
endef

define MKCONFIG_DATA :=
CONFIG_OVERRIDE_SYMBOLS ?= $(CONFIG_OVERRIDE_SYMBOLS)
CONFIG_REGISTER_PREINIT ?= $(CONFIG_REGISTER_PREINIT)

CONFIG_AUTODEDICATE_PML4ES ?= $(CONFIG_AUTODEDICATE_PML4ES)
CONFIG_CALLOC_SPECIAL_BUFSIZE ?= $(CONFIG_CALLOC_SPECIAL_BUFSIZE)
CONFIG_VIRTMEMALLOC_STATS ?= $(CONFIG_VIRTMEMALLOC_STATS)
CONFIG_SYSCALLMETA_HAS_INFO ?= $(CONFIG_SYSCALLMETA_HAS_INFO)
CONFIG_PRINTF_NOMALLOC_BUFFER_SIZE ?= $(CONFIG_PRINTF_NOMALLOC_BUFFER_SIZE)

CONFIG_DEBUG_DGLMALLOC ?= $(CONFIG_DEBUG_DGLMALLOC)
CONFIG_DEBUG_VIRTMEM ?= $(CONFIG_DEBUG_VIRTMEM)
CONFIG_DEBUG_VIRTMEMALLOC ?= $(CONFIG_DEBUG_VIRTMEMALLOC)
CONFIG_DEBUG_VIRTREMAP ?= $(CONFIG_DEBUG_VIRTREMAP)
CONFIG_DEBUG_SYSMALLOC ?= $(CONFIG_DEBUG_SYSMALLOC)
CONFIG_DEBUG_INIT ?= $(CONFIG_DEBUG_INIT)
CONFIG_DEBUG_PHYSMEM_ALLOC ?= $(CONFIG_DEBUG_PHYSMEM_ALLOC)

CONFIG_TRACE_SYSCALLS ?= $(CONFIG_TRACE_SYSCALLS)

$(PLATFORM_MKCONFIG_DATA)
endef

define MKLASTCONFIG_DATA :=
PROFILE ?= $(PROFILE)
PLATFORM ?= $(PLATFORM)

$(PLATFORM_MKCONFIG_DATA)
endef

define generate_mklastconfig
	$(file > $(MKLASTCONFIG),$(MKLASTCONFIG_DATA))
endef

define MKUSER_DATA :=
DANGLESS_ROOT := $(ROOT)

DANGLESS_BUILD_DIR := $(ROOT)/$(BUILD_DIR)
DANGLESS_BIN_DIR := $(ROOT)/$(BIN_DIR)
DANGLESS_BIN_LIBNAME := dangless
DANGLESS_BIN := $(ROOT)/$(BIN)

DANGLESS_INCLUDES := -I$(ROOT)/$(BUILDCONFIG_SPECIALIZED_INCLUDE_PATH) -I$(ROOT)/$(BUILDCONFIG_COMMON_INCLUDE_PATH) -I$(ROOT)/include

DANGLESS_USER_CFLAGS := -pthread $(PLATFORM_USER_CFLAGS)
DANGLESS_USER_LDFLAGS := -L$$(DANGLESS_BIN_DIR) -Wl,-whole-archive -l$$(DANGLESS_BIN_LIBNAME) -Wl,-no-whole-archive -pthread $(PLATFORM_USER_LDFLAGS)
endef

config: $(BIN_DIR) $(dir $(BUILDCONFIG_HEADER_FILE))
	$(file > $(BUILDCONFIG_HEADER_FILE),$(CBUILDCONFIG_DATA))
	$(file > $(MKCONFIG),$(MKCONFIG_DATA))
	$(call generate_mklastconfig)
	$(file > $(MKUSER),$(MKUSER_DATA))

	@$(MAKE) -s --no-print-directory show-config

show-config:
	@echo "-- Current build ($(MKLASTCONFIG)):"
	@echo ""
	@cat $(MKLASTCONFIG)
	@echo ""
	@echo "-- Configuration ($(MKCONFIG)):"
	@echo ""
	@cat $(MKCONFIG)
