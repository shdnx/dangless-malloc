.SUFFIXES:

DANGLESS_ROOT := $(realpath $(CURDIR)/../..)

include $(DANGLESS_ROOT)/build/last-config.mk
DANGLESS_BIN_DIR := $(DANGLESS_ROOT)/build/$(PLATFORM)_$(PROFILE)
include $(DANGLESS_BIN_DIR)/user.mk
