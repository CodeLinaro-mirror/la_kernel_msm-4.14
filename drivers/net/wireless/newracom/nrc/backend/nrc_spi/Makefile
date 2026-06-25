# SPDX-License-Identifier: GPL-2.0
# NRC SPI Backend Module Makefile
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

M_SRC = nrc_spi

# Support both module (obj-m) and built-in (obj-y) compilation
# For builtin: CONFIG_NRC7394=y, for module: CONFIG_NRC7394=m
ifneq ($(CONFIG_NRC7394),)
obj-$(CONFIG_NRC7394) += $(M_SRC).o
else
obj-m += $(M_SRC).o
endif

nrc_spi-objs = \
		nrc-hif-cspi.o \
		nrc-spi-hif-ops.o \
		nrc-spi-gpio.o \
		nrc-debug.o \
		nrc-spi-callback.o \
		nrc-spi-device.o \
		nrc-spi-params.o \
		nrc-spi-init.o

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
    ccflags-y += -DCONFIG_NRC_HIF_CSPI -Wall -Werror
endif

# Add SPI-specific feature flags passed from main Makefile or set defaults
ifdef SPI_FEATURES
    ccflags-y += $(SPI_FEATURES)
else
    # Default SPI features for built-in compilation
    ccflags-y += -DENABLE_HW_RESET
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

checkstyle:
	@$(CHECKPATCH) --root=$(KDIR) -file -terse --ignore=LINUX_VERSION_CODE -no-tree *.c *.h
