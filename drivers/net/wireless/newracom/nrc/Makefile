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
COMMON_INCLUDE = $(CURDIR)/$(COMMON_DIR)

# Module install path (for modprobe support)
INSTALL_MOD_DIR ?= /lib/modules/$(shell uname -r)/extra/nrc

# Firmware
FIRMWARE_DIR    = firmware
FW_SRC          = $(FIRMWARE_DIR)/nrc7394_cspi.bin
FW_DST_NAME     = uni_s1g.bin
BD_SRC          = $(FIRMWARE_DIR)/nrc7394_bd.dat
LIB_FIRMWARE    = /lib/firmware

# Compilation flags
COMMON_CFLAGS := -I$(COMMON_DIR) -DCONFIG_NRC_HIF_CSPI -Wall -Werror
SPI_FEATURES := -DENABLE_HW_RESET

# Debug mode flags
ifeq ($(DEBUG),y)
    COMMON_CFLAGS += -DDEBUG -g
endif

.PHONY: all modules clean install uninstall firmware_install help
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
		KBUILD_EXTRA_SYMBOLS=$(CURDIR)/$(BACKEND_DIR)/Module.symvers modules

wlan: hal
	@echo "Building nrc_wlan.ko..."
	@$(MAKE) -C $(WLAN_DIR) \
		KDIR=$(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) \
		COMMON_INCLUDE=$(COMMON_INCLUDE) \
		COMMON_CFLAGS="$(COMMON_CFLAGS)" \
		KBUILD_MODPOST_WARN=$(KBUILD_MODPOST_WARN) \
		KBUILD_EXTRA_SYMBOLS="$(CURDIR)/$(BACKEND_DIR)/Module.symvers $(CURDIR)/$(HAL_DIR)/Module.symvers" modules

mcp: hal
	@echo "Building nrc-mcp.ko..."
	@$(MAKE) -C $(MCP_DIR) \
		KDIR=$(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) \
		COMMON_INCLUDE=$(COMMON_INCLUDE) \
		COMMON_CFLAGS="$(COMMON_CFLAGS)" \
		KBUILD_MODPOST_WARN=$(KBUILD_MODPOST_WARN) \
		KBUILD_EXTRA_SYMBOLS="$(CURDIR)/$(BACKEND_DIR)/Module.symvers $(CURDIR)/$(HAL_DIR)/Module.symvers" modules

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

firmware_install:
	@echo "Installing firmware to $(LIB_FIRMWARE)..."
	@if [ ! -f "$(FW_SRC)" ]; then \
		echo "Error: $(FW_SRC) not found"; exit 1; \
	fi
	@if [ ! -f "$(BD_SRC)" ]; then \
		echo "Error: $(BD_SRC) not found"; exit 1; \
	fi
	@sudo install -m 0644 $(FW_SRC) $(LIB_FIRMWARE)/$(FW_DST_NAME)
	@sudo install -m 0644 $(BD_SRC) $(LIB_FIRMWARE)/
	@echo "  $(FW_SRC) -> $(LIB_FIRMWARE)/$(FW_DST_NAME)"
	@echo "  $(BD_SRC) -> $(LIB_FIRMWARE)/$(notdir $(BD_SRC))"
	@echo "Firmware installed!"

install: modules firmware_install modules_install
	@echo "Running depmod..."
	@sudo depmod -a
	@echo "Install completed! Use 'modprobe nrc_spi / nrc_core / nrc_wlan / nrc-mcp' to load."

uninstall:
	@echo "Removing installed modules..."
	@sudo rm -f $(INSTALL_MOD_DIR)/nrc_spi.ko
	@sudo rm -f $(INSTALL_MOD_DIR)/nrc_core.ko
	@sudo rm -f $(INSTALL_MOD_DIR)/nrc_wlan.ko
	@sudo rm -f $(INSTALL_MOD_DIR)/nrc-mcp.ko
	@echo "Removing installed firmware..."
	@sudo rm -f $(LIB_FIRMWARE)/$(FW_DST_NAME)
	@sudo rm -f $(LIB_FIRMWARE)/$(notdir $(BD_SRC))
	@echo "Running depmod..."
	@sudo depmod -a
	@echo "Uninstall completed!"

modules_install:
	@echo "Installing kernel modules to $(INSTALL_MOD_DIR)..."
	@sudo install -d $(INSTALL_MOD_DIR)
	@for ko in $(BACKEND_DIR)/nrc_spi.ko $(HAL_DIR)/nrc_core.ko \
	            $(WLAN_DIR)/nrc_wlan.ko $(MCP_DIR)/nrc-mcp.ko; do \
		if [ -f "$$ko" ]; then \
			sudo install -m 0644 $$ko $(INSTALL_MOD_DIR)/; \
			echo "  $$ko -> $(INSTALL_MOD_DIR)/"; \
		fi; \
	done
	@echo "Kernel modules installed!"

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
	@echo "  install          - Build + install modules & firmware to system (requires sudo)"
	@echo "  firmware_install - Install firmware files to $(LIB_FIRMWARE) only"
	@echo "  modules_install  - Install .ko files to $(INSTALL_MOD_DIR) + firmware"
	@echo "  uninstall        - Remove installed modules & firmware from system"
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