# SPDX-License-Identifier: GPL-2.0
# NRC WLAN Frontend Module Makefile
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

M_SRC = nrc_wlan

# Support both module (obj-m) and built-in (obj-y) compilation
ifneq ($(CONFIG_NRC7394),)
obj-$(CONFIG_NRC7394) += $(M_SRC).o
else
obj-m += $(M_SRC).o
endif

nrc_wlan-objs = \
		nrc-mac80211.o \
		nrc-mac80211-twt.o \
		nrc-twt-sched.o \
		nrc-trx.o \
		mac80211-ext.o \
		nrc-netlink.o \
		nrc-s1g.o \
		nrc-stats.o \
		nrc-pm.o \
		nrc-ps.o \
		nrc-debug.o \
		nrc-wlan-callback.o \
		nrc-wlan-post-init.o \
		nrc-wlan-params.o \
		nrc-wlan-hal-init.o \
		nrc-wlan-init.o \
		nrc-apf.o \
		nrc-wim-wlan.o

# Base compilation flags
ccflags-y := -I$(src)

# Add common include path (fallback to relative path if not provided)
ifdef COMMON_INCLUDE
    ccflags-y += -I$(COMMON_INCLUDE)
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

# Add WLAN feature flags passed from main Makefile
ifdef WLAN_FEATURES
    ccflags-y += $(WLAN_FEATURES)
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
