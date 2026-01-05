# SPDX-License-Identifier: GPL-2.0
#
# Newracom NRC7394 Modular Driver Build System
#

KDIR ?= /lib/modules/$(shell uname -r)/build

# Debug mode (default: n)
DEBUG ?= n

# Auto-detect architecture for native builds
ifeq ($(ARCH),)
    UNAME_ARCH := $(shell uname -m)
    ifeq ($(UNAME_ARCH),x86_64)
        ARCH := x86_64
    else ifeq ($(UNAME_ARCH),aarch64)
        ARCH := arm64
    else ifeq ($(UNAME_ARCH),armv7l)
        ARCH := arm
    else
        ARCH := $(UNAME_ARCH)
    endif
endif

# Module directories
BACKEND_DIR = backend/nrc_spi
HAL_DIR = hal/nrc_core
WLAN_DIR = frontend/nrc_wlan
MCP_DIR = frontend/nrc_mcp
COMMON_DIR = common

# Common paths
COMMON_INCLUDE = $(PWD)/$(COMMON_DIR)

# Compilation flags
COMMON_CFLAGS := -I$(COMMON_DIR) -DCONFIG_NRC_HIF_CSPI -Wall -Werror
SPI_FEATURES := -DCONFIG_NRC_SPI_TRACE -DENABLE_HW_RESET

# Debug mode flags
ifeq ($(DEBUG),y)
    COMMON_CFLAGS += -DDEBUG -g
endif

.PHONY: all modules clean install uninstall help
.PHONY: backend hal wlan mcp

all: modules

# Build all modules in dependency order (sequential)
modules:
	@echo "Building NRC7394 modular driver$(if $(filter y,$(DEBUG)), (DEBUG),)..."
	@$(MAKE) backend
	@$(MAKE) hal
	@$(MAKE) wlan
	@$(MAKE) mcp
	@echo "All modules built successfully!"

backend:
	@echo "Building nrc_spi.ko..."
	@$(MAKE) -C $(BACKEND_DIR) \
		KDIR=$(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) \
		COMMON_INCLUDE=$(COMMON_INCLUDE) \
		COMMON_CFLAGS="$(COMMON_CFLAGS)" \
		SPI_FEATURES="$(SPI_FEATURES)" \
		KBUILD_MODPOST_WARN=$(KBUILD_MODPOST_WARN) modules

hal: backend
	@echo "Building nrc_core.ko..."
	@$(MAKE) -C $(HAL_DIR) \
		KDIR=$(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) \
		COMMON_INCLUDE=$(COMMON_INCLUDE) \
		COMMON_CFLAGS="$(COMMON_CFLAGS)" \
		KBUILD_MODPOST_WARN=$(KBUILD_MODPOST_WARN) \
		KBUILD_EXTRA_SYMBOLS=$(PWD)/$(BACKEND_DIR)/Module.symvers modules

wlan: hal
	@echo "Building nrc_wlan.ko..."
	@$(MAKE) -C $(WLAN_DIR) \
		KDIR=$(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) \
		COMMON_INCLUDE=$(COMMON_INCLUDE) \
		COMMON_CFLAGS="$(COMMON_CFLAGS)" \
		KBUILD_MODPOST_WARN=$(KBUILD_MODPOST_WARN) \
		KBUILD_EXTRA_SYMBOLS="$(PWD)/$(BACKEND_DIR)/Module.symvers $(PWD)/$(HAL_DIR)/Module.symvers" modules

mcp: hal
	@echo "Building nrc-mcp.ko..."
	@$(MAKE) -C $(MCP_DIR) \
		KDIR=$(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) \
		COMMON_INCLUDE=$(COMMON_INCLUDE) \
		COMMON_CFLAGS="$(COMMON_CFLAGS)" \
		KBUILD_MODPOST_WARN=$(KBUILD_MODPOST_WARN) \
		KBUILD_EXTRA_SYMBOLS="$(PWD)/$(BACKEND_DIR)/Module.symvers $(PWD)/$(HAL_DIR)/Module.symvers" modules

clean:
	@$(MAKE) -C $(BACKEND_DIR) clean
	@$(MAKE) -C $(HAL_DIR) clean
	@$(MAKE) -C $(WLAN_DIR) clean
	@$(MAKE) -C $(MCP_DIR) clean
	@echo "Clean completed!"

# Aliases for Docker build script compatibility
clean-all: clean
clean-backend: 
	@$(MAKE) -C $(BACKEND_DIR) clean
clean-hal:
	@$(MAKE) -C $(HAL_DIR) clean
clean-frontend clean-wlan:
	@$(MAKE) -C $(WLAN_DIR) clean
clean-mcp:
	@$(MAKE) -C $(MCP_DIR) clean

install: modules
	@sudo insmod $(BACKEND_DIR)/nrc_spi.ko
	@sudo insmod $(HAL_DIR)/nrc_core.ko
	@sudo insmod $(WLAN_DIR)/nrc_wlan.ko
	@echo "Modules installed!"

uninstall:
	@sudo rmmod nrc_wlan 2>/dev/null || true
	@sudo rmmod nrc-mcp 2>/dev/null || true
	@sudo rmmod nrc_core 2>/dev/null || true
	@sudo rmmod nrc_spi 2>/dev/null || true
	@echo "Modules uninstalled!"

modules_install:
	@$(MAKE) -C $(BACKEND_DIR) modules_install
	@$(MAKE) -C $(HAL_DIR) modules_install
	@$(MAKE) -C $(WLAN_DIR) modules_install

help:
	@echo "NRC7394 Modular Driver Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all/modules  - Build all modules"
	@echo "  backend      - Build nrc_spi.ko only"
	@echo "  hal          - Build nrc_core.ko only"
	@echo "  wlan         - Build nrc_wlan.ko only"
	@echo "  mcp          - Build nrc-mcp.ko only"
	@echo "  clean        - Clean all modules"
	@echo "  install      - Load modules (requires sudo)"
	@echo "  uninstall    - Unload modules (requires sudo)"
	@echo ""
	@echo "Options:"
	@echo "  DEBUG=y      - Enable debug messages and symbols"
	@echo ""
	@echo "Cross-compilation:"
	@echo "  make KDIR=/path/to/kernel ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-"
	@echo ""
	@echo "Examples:"
	@echo "  make                    - Build release version"
	@echo "  make DEBUG=y            - Build debug version"
	@echo "  make clean && make      - Clean rebuild"