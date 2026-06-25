# SPDX-License-Identifier: GPL-2.0
# NRC MCP Module Makefile
KDIR ?= /lib/modules/$(shell uname -r)/build
CHECKPATCH ?= $(KDIR)/scripts/checkpatch.pl

# Only pass ARCH and CROSS_COMPILE if they are set (for cross-compilation)
ifdef ARCH
    export ARCH
endif
ifdef CROSS_COMPILE
    export CROSS_COMPILE
endif

# Include kernel config only for out-of-tree builds
ifneq ($(KERNELRELEASE),)
    # In-tree build: CONFIG_* already defined
else
    -include $(KDIR)/.config
endif

M_SRC = nrc-mcp

# Support both module (obj-m) and built-in (obj-y) compilation
# MCP uses separate Kconfig option NRC7394_MCP
ifneq ($(CONFIG_NRC7394_MCP),)
obj-$(CONFIG_NRC7394_MCP) += $(M_SRC).o
else
obj-m += $(M_SRC).o
endif

nrc-mcp-objs = \
		nrc-netlink-driver.o \
		nrc-log.o \
		nrc-debug.o \
		nrc-mcp-callback.o \
		nrc-mcp-params.o \
		nrc-mcp-transmit.o \
		nrc-mcp-init.o

# Base compilation flags
ccflags-y := -I$(src)

# Add common include path (fallback to relative path if not provided)
ifdef COMMON_INCLUDE
    ccflags-y += -I$(COMMON_INCLUDE)
else ifeq ($(KBUILD_EXTMOD),)
    ccflags-y += -I$(src)/../../common -I$(srctree)/$(src)/../../common
else
    ccflags-y += -I$(src)/../../common
endif

# Add common flags passed from main Makefile or set defaults for built-in
ifdef COMMON_CFLAGS
    ccflags-y += $(COMMON_CFLAGS)
else
    # Default flags for built-in compilation
    ccflags-y += -Wall -Werror
endif

# Debug support from Kconfig or make variable
ifdef CONFIG_NRC7394_DEBUG
    ccflags-y += -DDEBUG -g
endif
ifdef DEBUG
    ccflags-y += -DDEBUG -g
endif

all: modules

modules:
	@$(MAKE) -C $(KDIR) M=$$PWD modules

modules_install:
	@$(MAKE) -C $(KDIR) M=$$PWD modules_install

install: modules
	@sudo insmod $(M_SRC).ko

test: modules
	@sudo insmod $(M_SRC).ko
	@sudo rmmod -f $(M_SRC)

clean:
	@$(MAKE) -C $(KDIR) M=$$PWD clean
	@rm -f *.sym *.dis

checkstyle:
	@$(CHECKPATCH) --root=$(KDIR) -file -terse --ignore=LINUX_VERSION_CODE -no-tree *.c *.h
