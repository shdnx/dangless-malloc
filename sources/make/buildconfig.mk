define CBUILDCONFIG_DATA :=
#ifndef DANGLESS_BUILDCONFIG_H
#define DANGLESS_BUILDCONFIG_H

#define DANGLESS_CONFIG_PLATFORM $(PLATFORM)

#define DANGLESS_CONFIG_OVERRIDE_SYMBOLS $(CONFIG_OVERRIDE_SYMBOLS)
#define DANGLESS_CONFIG_AUTO_DEDICATE_MAX_PML4ES $(CONFIG_AUTODEDICATE_PML4ES)
#define DANGLESS_CONFIG_CALLOC_SPECIAL_BUFSIZE $(CONFIG_CALLOC_SPECIAL_BUFSIZE)
#define DANGLESS_CONFIG_REGISTER_PREINIT $(CONFIG_REGISTER_PREINIT)

#endif
endef

define MKCONFIG_DATA :=
PLATFORM ?= $(PLATFORM)
DANGLESS_BIN := $(realpath $(BIN_DIR))/libdangless.a
DANGLESS_INCLUDE_DIR := $(realpath include)
DANGLESS_BUILD_INCLUDE_DIR := $(realpath $(BUILDCONFIG_INCLUDE_PATH))

CONFIG_OVERRIDE_SYMBOLS ?= $(CONFIG_OVERRIDE_SYMBOLS)
CONFIG_AUTODEDICATE_PML4ES ?= $(CONFIG_AUTODEDICATE_PML4ES)
CONFIG_CALLOC_SPECIAL_BUFSIZE ?= $(CONFIG_CALLOC_SPECIAL_BUFSIZE)
CONFIG_REGISTER_PREINIT ?= $(CONFIG_REGISTER_PREINIT)

DANGLESS_USER_CFLAGS := -pthread $(PLATFORM_USER_CFLAGS)
DANGLESS_USER_LDFLAGS := -L$$(dir $$(DANGLESS_BIN)) -Wl,-whole-archive -ldangless -Wl,-no-whole-archive -pthread $(PLATFORM_USER_LDFLAGS)

$(PLATFORM_MKCONFIG_DATA)
endef

define MKLASTCONFIG_DATA :=
-include $(realpath $(MKCONFIG))
endef

define generate_mklastconfig
	$(file > $(1),$(MKLASTCONFIG_DATA))
endef

$(MKLASTCONFIG): $(BUILD_DIR) always
	$(call generate_mklastconfig,$@)

$(MKCONFIG): $(BIN_DIR) always
	$(file > $@,$(MKCONFIG_DATA))

$(BUILDCONFIG_HEADER_FILE): $(BUILDCONFIG_HEADER_DIR) always
	$(file > $@,$(CBUILDCONFIG_DATA))

BUILDCONFIG_FILES = $(MKCONFIG) $(MKLASTCONFIG) $(BUILDCONFIG_HEADER_FILE)

# it's important to have the directories BIN_DIR and BUILDCONFIG_HEADER_DIR created first, otherwise realpath will fail while generating the various files
config: $(BIN_DIR) $(BUILDCONFIG_HEADER_DIR) $(BUILDCONFIG_FILES) always
	@echo Build configuration files written for platform $(PLATFORM)

# always re-generate the mklastconfig file if PLATFORM was specified
ifeq ($(EXPLICIT_PLATFORM),1)
$(call generate_mklastconfig,$(MKLASTCONFIG))
endif
