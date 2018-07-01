# optional configuration variables that the user can supply

CONFIG_OVERRIDE_SYMBOLS ?= 1
CONFIG_REGISTER_PREINIT ?= 1
CONFIG_COLLECT_STATISTICS ?= 1
CONFIG_SUPPORT_MULTITHREADING ?= 0
CONFIG_ALLOW_SYSMALLOC_FALLBACK ?= 1

CONFIG_AUTODEDICATE_PML4ES ?= 1
CONFIG_CALLOC_SPECIAL_BUFSIZE ?= 32
CONFIG_SYSCALLMETA_HAS_INFO ?= 1
CONFIG_PRINTF_NOMALLOC_BUFFER_SIZE ?= 4096

CONFIG_DEBUG_DGLMALLOC ?= 0
CONFIG_DEBUG_VIRTMEM ?= 0
CONFIG_DEBUG_VIRTMEMALLOC ?= 0
CONFIG_DEBUG_VIRTREMAP ?= 0
CONFIG_DEBUG_SYSMALLOC ?= 0
CONFIG_DEBUG_INIT ?= 0
CONFIG_DEBUG_PHYSMEM_ALLOC ?= 0

CONFIG_TRACE_SYSCALLS ?= 0
CONFIG_TRACE_HOOKS ?= 0
CONFIG_TRACE_HOOKS_BACKTRACE ?= 0
CONFIG_TRACE_HOOKS_BACKTRACE_BUFSIZE ?= 64
