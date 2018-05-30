define CBUILDCONFIG_DATA :=
#ifndef DANGLESS_BUILDCONFIG_H
#define DANGLESS_BUILDCONFIG_H

#define DANGLESS_CONFIG_PLATFORM $(PLATFORM)

#define DANGLESS_CONFIG_OVERRIDE_SYMBOLS $(CONFIG_OVERRIDE_SYMBOLS)
#define DANGLESS_CONFIG_AUTO_DEDICATE_MAX_PML4ES $(CONFIG_AUTODEDICATE_PML4ES)
#define DANGLESS_CONFIG_CALLOC_SPECIAL_BUFSIZE $(CONFIG_CALLOC_SPECIAL_BUFSIZE)
#define DANGLESS_CONFIG_REGISTER_PREINIT $(CONFIG_REGISTER_PREINIT)

#define DANGLESS_CONFIG_VIRTMEMALLOC_STATS $(CONFIG_VIRTMEMALLOC_STATS)

#define DANGLESS_CONFIG_DEBUG_DGLMALLOC $(CONFIG_DEBUG_DGLMALLOC)
#define DANGLESS_CONFIG_DEBUG_VIRTMEM $(CONFIG_DEBUG_VIRTMEM)
#define DANGLESS_CONFIG_DEBUG_VIRTMEMALLOC $(CONFIG_DEBUG_VIRTMEMALLOC)
#define DANGLESS_CONFIG_DEBUG_VIRTREMAP $(CONFIG_DEBUG_VIRTREMAP)
#define DANGLESS_CONFIG_DEBUG_SYSMALLOC $(CONFIG_DEBUG_SYSMALLOC)
#define DANGLESS_CONFIG_DEBUG_INIT $(CONFIG_DEBUG_INIT)
#define DANGLESS_CONFIG_DEBUG_PHYSMEM_ALLOC $(CONFIG_DEBUG_PHYSMEM_ALLOC)

#endif
endef

define MKCONFIG_DATA :=
PLATFORM ?= $(PLATFORM)

DANGLESS_ROOT := $(ROOT)

DANGLESS_BUILD_DIR := $(ROOT)/$(BUILD_DIR)
DANGLESS_BIN_DIR := $(ROOT)/$(BIN_DIR)
DANGLESS_BIN_LIBNAME := dangless
DANGLESS_BIN := $(ROOT)/$(BIN)

DANGLESS_INCLUDE_DIR := $(ROOT)/include
DANGLESS_BUILD_INCLUDE_DIR := $(ROOT)/$(BUILDCONFIG_INCLUDE_PATH)

CONFIG_OVERRIDE_SYMBOLS ?= $(CONFIG_OVERRIDE_SYMBOLS)
CONFIG_AUTODEDICATE_PML4ES ?= $(CONFIG_AUTODEDICATE_PML4ES)
CONFIG_CALLOC_SPECIAL_BUFSIZE ?= $(CONFIG_CALLOC_SPECIAL_BUFSIZE)
CONFIG_REGISTER_PREINIT ?= $(CONFIG_REGISTER_PREINIT)

CONFIG_VIRTMEMALLOC_STATS ?= $(CONFIG_VIRTMEMALLOC_STATS)

CONFIG_DEBUG_DGLMALLOC ?= $(CONFIG_DEBUG_DGLMALLOC)
CONFIG_DEBUG_VIRTMEM ?= $(CONFIG_DEBUG_VIRTMEM)
CONFIG_DEBUG_VIRTMEMALLOC ?= $(CONFIG_DEBUG_VIRTMEMALLOC)
CONFIG_DEBUG_VIRTREMAP ?= $(CONFIG_DEBUG_VIRTREMAP)
CONFIG_DEBUG_SYSMALLOC ?= $(CONFIG_DEBUG_SYSMALLOC)
CONFIG_DEBUG_INIT ?= $(CONFIG_DEBUG_INIT)
CONFIG_DEBUG_PHYSMEM_ALLOC ?= $(CONFIG_DEBUG_PHYSMEM_ALLOC)

DANGLESS_USER_CFLAGS := -pthread $(PLATFORM_USER_CFLAGS)
DANGLESS_USER_LDFLAGS := -L$$(DANGLESS_BIN_DIR) -Wl,-whole-archive -l$$(DANGLESS_BIN_LIBNAME) -Wl,-no-whole-archive -pthread $(PLATFORM_USER_LDFLAGS)

$(PLATFORM_MKCONFIG_DATA)
endef

define MKLASTCONFIG_DATA :=
-include $(ROOT)/$(MKCONFIG)
endef

define generate_mklastconfig
	$(file > $(MKLASTCONFIG),$(MKLASTCONFIG_DATA))
endef

# it's important to have the directories BIN_DIR and BUILDCONFIG_HEADER_DIR created first, otherwise realpath will fail while generating the various files
config: $(BIN_DIR) $(BUILDCONFIG_HEADER_DIR) always
	$(file > $(BUILDCONFIG_HEADER_FILE),$(CBUILDCONFIG_DATA))
	$(file > $(MKCONFIG),$(MKCONFIG_DATA))
	$(call generate_mklastconfig)

	@echo Build configuration files written for platform $(PLATFORM) to $(MKCONFIG):
	@echo ""
	@cat $(MKCONFIG)
