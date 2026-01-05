# NRC7394 Driver - Kernel Integration Guide

This guide shows how to integrate the NRC7394 modular driver into the Linux kernel tree.

## Quick Start for Kernel Integration

### Prerequisites

- Linux kernel source tree (tested on 5.10+)
- Required kernel options enabled:
  - `CONFIG_MAC80211=y/m` (MAC80211 subsystem)
  - `CONFIG_SPI=y` (SPI bus support)
  - `CONFIG_FW_LOADER=y` (Firmware loading support)

### Integration Steps

#### 1. Copy Driver to Kernel Tree

```bash
# Copy driver to kernel wireless directory
cp -r nrc_modular /path/to/kernel/drivers/net/wireless/newracom/
```

#### 2. Add to Kernel Build System

**Edit `drivers/net/wireless/Kconfig`:**
```kconfig
source "drivers/net/wireless/newracom/Kconfig"
```

**Edit `drivers/net/wireless/Makefile`:**
```makefile
obj-$(CONFIG_NRC7394) += newracom/
```

#### 3. Configure and Build

```bash
# Configure kernel
cd /path/to/kernel
make menuconfig

# Navigate to:
# Device Drivers → Network device support → Wireless LAN →
# [M/*] Newracom NRC7394 802.11ah HaLow driver (modular)

# Build kernel (builtin) or modules
make -j$(nproc)

# Or build modules only
make modules

# Install modules
make modules_install
```

## Configuration Menu Structure

```
[M/*] Newracom NRC7394 802.11ah HaLow driver (modular)
      [ ] Enable debug support
      [M/*] NRC7394 MCP Frontend (nrc-mcp.ko)
```

## Configuration Options

| Option | Default | Purpose |
|--------|---------|---------|
| `CONFIG_NRC7394` | - | Main driver (enables all core modules: nrc_spi, nrc_core, nrc_wlan) |
| `CONFIG_NRC7394_DEBUG` | N | Enable debug messages and runtime checks |
| `CONFIG_NRC7394_MCP` | Y | Optional MCP frontend for ESL applications |

### Build Types

- **Built-in (`=y`)**: Driver compiled into kernel image (zImage/Image)
- **Module (`=m`)**: Driver built as loadable modules (.ko files)

### Module Structure

When built as modules, the driver produces:
- `nrc_spi.ko` - SPI backend layer
- `nrc_core.ko` - HAL/Core layer
- `nrc_wlan.ko` - WLAN frontend (MAC80211 interface)
- `nrc-mcp.ko` - MCP frontend (optional, if CONFIG_NRC7394_MCP=m)

## Module Dependencies

### Module Load Order

```
nrc_spi.ko     (Backend - SPI hardware interface)
    ↓
nrc_core.ko    (HAL - Hardware abstraction)
    ↓
nrc_wlan.ko    (Frontend - MAC80211 interface)
    ↓
nrc-mcp.ko     (Optional - MCP interface)
```

**Kernel Dependencies:**
- `mac80211.ko` - MAC layer (required)
- `cfg80211.ko` - Wireless configuration (required, loaded by mac80211)
- SPI subsystem (built-in) - Required

## Device Tree Configuration

For Raspberry Pi with SPI overlay:

```dts
&spi0 {
    nrc80211: nrc80211@0 {
        compatible = "newracom,nrc7394";
        reg = <0>;
        spi-max-frequency = <20000000>;
        interrupt-parent = <&gpio>;
        interrupts = <5 IRQ_TYPE_EDGE_RISING>;
    };
};
```

## Testing Integration

### Docker-based Builtin Build Test

Use the provided Docker tooling to test kernel builtin builds:

```bash
# Navigate to docker directory
cd docker/

# Test builtin build for specific kernel
./build-builtin.sh rpi-6.12.47

# Keep container running for inspection
./build-builtin.sh rpi-6.12.47 --keep

# Access container to inspect .config and binaries
docker exec -it nrc-builtin-test bash
```

### Verify Module Build (Modules)

```bash
# Check modules were built
ls -l drivers/net/wireless/newracom/*/*.ko

# Expected output:
# backend/nrc_spi/nrc_spi.ko
# hal/nrc_core/nrc_core.ko
# frontend/nrc_wlan/nrc_wlan.ko
# frontend/nrc_mcp/nrc-mcp.ko (if CONFIG_NRC7394_MCP=m)
```

### Load Modules

```bash
# Load in dependency order (automatic with modprobe)
modprobe nrc_wlan

# Or manual loading
insmod nrc_spi.ko
insmod nrc_core.ko
insmod nrc_wlan.ko
insmod nrc-mcp.ko  # Optional
```

### Verify Loading

```bash
# Check modules loaded
lsmod | grep nrc

# Check interfaces
iw dev

# Check kernel log
dmesg | grep nrc
```

## Common Issues

### Issue: Missing Dependencies

**Solution:** Enable required kernel options:
```bash
CONFIG_MAC80211=y  # or =m
CONFIG_SPI=y
CONFIG_FW_LOADER=y
```

### Issue: Module Load Fails

**Solution:** Check dependencies:
```bash
modinfo nrc_wlan | grep depends
modprobe --show-depends nrc_wlan
```

### Issue: Firmware Not Found

**Solution:** Place firmware files in `/lib/firmware/`:
```bash
cp uni_s1g.bin /lib/firmware/
cp nrc7394_bd.dat /lib/firmware/
```

## Out-of-Tree Build (Module Development)

For development without kernel integration, use Docker build:

```bash
# Build all modules using Docker
cd docker/
./build-docker.sh all rpi-6.12.47

# Or build specific modules
./build-docker.sh backend rpi-6.12.47
./build-docker.sh hal rpi-6.12.47
./build-docker.sh frontend rpi-6.12.47
```

## Kernel Versions

**Tested:**
- Linux 5.10.x (Raspberry Pi OS)
- Linux 6.1.x (Raspberry Pi OS)
- Linux 6.6.x (Android RPi)
- Linux 6.12.x (Raspberry Pi OS)

**Minimum:** Linux 5.10

## Reference Files

- `Kconfig` - Main configuration options
- `Makefile` - Top-level build rules
- `backend/Kconfig`, `backend/Makefile` - SPI backend
- `hal/Kconfig`, `hal/Makefile` - HAL/Core layer
- `frontend/Kconfig`, `frontend/Makefile` - Frontend layers
- `docker/build-builtin.sh` - Docker builtin build test
- `docker/build-docker.sh` - Docker module build

## Summary

1. Copy driver to `drivers/net/wireless/newracom/`
2. Add Kconfig source and Makefile entry to parent directory
3. Configure with `make menuconfig`:
   - Enable `CONFIG_NRC7394` (=y for builtin, =m for modules)
   - Enable `CONFIG_NRC7394_MCP` if MCP functionality needed
   - Enable `CONFIG_NRC7394_DEBUG` for development
4. Build with `make` or `make modules`
5. Install firmware files to `/lib/firmware/`

The driver uses a unified Kconfig that automatically handles all sub-modules.
