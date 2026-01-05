# NRC Linux Driver - Debug Interface Guide

This document provides essential information about the NRC Linux Driver debug interface.

## Quick Start

### debug.sh Script (Configuration & Control)

The `scripts/debug.sh` script provides comprehensive control over all debug features.

```bash
# Show help
./scripts/debug.sh help

# Show comprehensive status of all modules
sudo ./scripts/debug.sh status

# Debug level control (verbosity)
sudo ./scripts/debug.sh level status         # Show current debug level
sudo ./scripts/debug.sh level set DBG        # Enable all debug messages
sudo ./scripts/debug.sh level set INFO       # Default production level

# Debug mask control (categories)
sudo ./scripts/debug.sh mask status          # Show current debug masks
sudo ./scripts/debug.sh mask list            # List all debug bits
sudo ./scripts/debug.sh mask preset verbose  # Enable verbose debugging
sudo ./scripts/debug.sh mask set wlan TX RX

# SKB debug control
sudo ./scripts/debug.sh skb status           # Check SKB tracking status
sudo ./scripts/debug.sh skb enable           # Enable SKB tracking

# Device control
sudo ./scripts/debug.sh device status        # Show device status
sudo ./scripts/debug.sh loopback quick       # Run loopback throughput test

# Signal quality
sudo ./scripts/debug.sh signal               # Show RSSI, SNR, throughput

# Power management
sudo ./scripts/debug.sh pm wakeup            # Wakeup device
sudo ./scripts/debug.sh pm sleep             # Sleep device
```

## Debugfs Directory Structure

```
/sys/kernel/debug/
├── nrc_spi/                    # SPI Backend Module
│   ├── debug_mask              # SPI debug mask
│   ├── debug_level             # Debug level (0=ERR, 1=WARN, 2=INFO, 3=DBG)
│   ├── cspi                    # SPI status test
│   └── reset                   # Device reset trigger (HW level)
├── nrc_core/                   # Core HAL Module
│   ├── debug_mask              # Core debug mask
│   ├── debug_level             # Debug level (0=ERR, 1=WARN, 2=INFO, 3=DBG)
│   ├── skb_stats               # SKB statistics (read-only)
│   ├── skb_debug               # SKB tracking enable/disable (Y/N)
│   ├── credit                  # Credit queue status
│   ├── slot                    # Slot queue status (TX/RX)
│   ├── restart                 # Network restart trigger
│   ├── ps_control              # Power save control (status/wake/sleep/reset)
│   ├── ps_timing               # Power save timing history
│   ├── fw_control              # Firmware control (status/unload)
│   └── loopback/               # HIF Loopback Test
│       ├── test                # Run test (0=roundtrip, 1=TX only, 2=RX only)
│       ├── count               # Frame count for test (default: 100)
│       ├── sample              # Sample size in bytes (default: 1024, max: 1600)
│       ├── report              # Test results (throughput, timing)
│       └── hexdump             # Enable hex dump output (0/1)
├── ieee80211/nrc80211/         # WLAN Frontend Module
│   ├── debug_mask              # WLAN debug mask
│   ├── info                    # WLAN interface information (mode, channel, etc.)
│   ├── wakeup                  # Wakeup trigger
│   ├── sleep                   # Sleep trigger
│   ├── snr                     # Signal-to-Noise Ratio
│   ├── rssi                    # RSSI
│   ├── beacon_updated          # Beacon update status
│   ├── expected_tput           # Expected throughput
│   ├── twt/                    # TWT debugging
│   └── apf/                    # APF debugging
└── nrc_mcp/                    # MCP Frontend Module (Optional)
    └── debug_mask              # MCP debug mask
```

## Debug Levels and Masks

### Debug Level (Verbosity Control)

The debug level controls the verbosity of all debug messages globally. This is independent of category masks.

| Level | Name | Value | Description                                  |
|-------|------|-------|----------------------------------------------|
| ERR   | 0    | 0     | Errors only (ERR_* macros)                  |
| WARN  | 1    | 1     | Warnings and errors (WARn + ERR_*)          |
| INFO  | 2    | 2     | Info, warnings, errors (default production) |
| DBG   | 3    | 3     | All debug messages (default when DEBUG=y)   |

**Control Path:** `/sys/kernel/debug/nrc_core/debug_level`

**Examples:**
```bash
# Set to maximum verbosity (all debug messages)
sudo ./scripts/debug.sh level set DBG

# Set to production level (info and above)
sudo ./scripts/debug.sh level set INFO

# Show current level
sudo ./scripts/debug.sh level status
```

### Debug Mask Bits (Category Filtering)

Each module has an independent debug mask controlling which categories of messages are printed.
Note: ERR level messages are always shown regardless of mask (controlled only by level).

| Bit | Name   | Hex Value  | Description                    |
|-----|--------|------------|--------------------------------|
| 0   | BASIC  | 0x00000001 | Basic general debug messages   |
| 1   | HIF    | 0x00000002 | Host Interface operations      |
| 2   | WIM    | 0x00000004 | Wireless Interface Message     |
| 3   | TX     | 0x00000008 | TX operations                  |
| 4   | RX     | 0x00000010 | RX operations                  |
| 5   | MAC    | 0x00000020 | MAC layer operations           |
| 6   | CAPI   | 0x00000040 | C API operations               |
| 7   | PS     | 0x00000080 | Power save                     |
| 8   | STATS  | 0x00000100 | Statistics                     |
| 9   | STATE  | 0x00000200 | State machine                  |
| 10  | FW     | 0x00000400 | Firmware operations            |
| 11  | AMPDU  | 0x00000800 | A-MPDU aggregation monitoring  |
| 12  | CREDIT | 0x00001000 | Credit management and updates  |
| 13  | SLOT   | 0x00002000 | TX/RX slot management          |
| 14  | BUS    | 0x00004000 | Bus (SPI/SDIO) operations      |

**Module Paths:**
- SPI: `/sys/kernel/debug/nrc_spi/debug_mask`
- Core: `/sys/kernel/debug/nrc_core/debug_mask`
- WLAN: `/sys/kernel/debug/ieee80211/nrc80211/debug_mask`
- MCP: `/sys/kernel/debug/nrc_mcp/debug_mask`

## Common Debug Workflows

### Check Driver Status
```bash
# Display comprehensive status of all modules
sudo ./scripts/debug.sh status
```

### Enable Comprehensive Debugging
```bash
# Maximum verbosity: level + all categories
sudo ./scripts/debug.sh level set DBG
sudo ./scripts/debug.sh mask preset verbose
sudo ./scripts/debug.sh skb enable

# Check status
sudo ./scripts/debug.sh status
```

### Production Debugging (Less Verbose)
```bash
# Info level with specific categories
sudo ./scripts/debug.sh level set INFO
sudo ./scripts/debug.sh mask enable all STATE
dmesg -wH
```

### Errors and Warnings Only
```bash
# Minimal output for production monitoring
sudo ./scripts/debug.sh level set WARN
sudo ./scripts/debug.sh mask preset errors-only
```

### Debug TX/RX Issues
```bash
# Enable DBG level with TX/RX categories
sudo ./scripts/debug.sh level set DBG
sudo ./scripts/debug.sh mask enable all TX RX MAC
cat /sys/kernel/debug/nrc_core/credit  # Check credit queues
```

### Debug Credit and Slot Issues
```bash
# Enable DBG level with CREDIT and SLOT categories
sudo ./scripts/debug.sh level set DBG
sudo ./scripts/debug.sh mask enable all CREDIT SLOT

# Monitor credit updates in real-time
# CREDIT mask shows credit report events and AC credit values
# SLOT mask shows TX/RX slot head/tail changes
dmesg -wH | grep -E "Credit|Slot"

# Check current credit and slot status
cat /sys/kernel/debug/nrc_core/credit
cat /sys/kernel/debug/nrc_core/slot

# Or use status command to see all info
sudo ./scripts/debug.sh status
```

### Debug Memory Leaks
```bash
sudo ./scripts/debug.sh skb enable
cat /sys/kernel/debug/nrc_core/skb_stats  # Check for leaks
./scripts/memleak/monitor_memory.sh 2     # Auto-monitor
```

### Check WLAN Interface Status
```bash
# View WLAN interface information (mode, channel, connection status)
# Shows both 2.4/5GHz channel info and mapped S1G sub-1GHz channel info
cat /sys/kernel/debug/ieee80211/nrc80211/info

# Or use status command to see all info
sudo ./scripts/debug.sh status
```

### Test Device Connectivity
```bash
sudo ./scripts/debug.sh status
sudo ./scripts/debug.sh loopback quick   # Run loopback throughput test
sudo ./scripts/debug.sh signal
```

### HIF Loopback Testing

The loopback test measures HIF TX/RX throughput between host and target.

```bash
# Quick test with default settings (1024 bytes, 100 frames, round-trip)
sudo ./scripts/debug.sh loopback quick

# Or step-by-step:
# Step 1: Set sample size (bytes, max 1600, default: 1024)
echo 512 > /sys/kernel/debug/nrc_core/loopback/sample

# Step 2: Set frame count for test (default: 100)
echo 100 > /sys/kernel/debug/nrc_core/loopback/count

# Step 3: Enable hex dump (optional)
echo 1 > /sys/kernel/debug/nrc_core/loopback/hexdump

# Step 4: Run test
#   0 = Round-trip (TX + RX, default)
#   1 = TX only
#   2 = RX only
echo 0 > /sys/kernel/debug/nrc_core/loopback/test

# Step 5: View results
cat /sys/kernel/debug/nrc_core/loopback/report
```

**Note:** Sample data is generated dynamically during test execution.
No persistent template SKB is created.

**Loopback Test Modes:**
| Mode | Value | Description |
|------|-------|-------------|
| Round-trip | 0 | Host→Target→Host (measures full RTT) |
| TX only | 1 | Host→Target (measures TX throughput) |
| RX only | 2 | Target→Host (measures RX throughput) |

**Report Output:**
- Total frame counts and frame length
- TX/RX bytes transferred
- First/Last frame timing (microseconds)
- Round-trip time (RTT)
- Calculated throughput (kbps)

### Power Management Testing
```bash
# Enable PS debug with DBG level
sudo ./scripts/debug.sh level set DBG
sudo ./scripts/debug.sh mask enable all PS STATE
sudo ./scripts/debug.sh pm wakeup
sudo ./scripts/debug.sh pm sleep
```

### Device Recovery
```bash
sudo ./scripts/debug.sh device reset    # Hardware reset
sudo ./scripts/debug.sh device restart  # Network restart
```

## Manual Control (Advanced)

If you prefer direct debugfs access:

```bash
# Debug level control (all modules share this)
echo 3 > /sys/kernel/debug/nrc_core/debug_level    # Set to DBG level
echo 2 > /sys/kernel/debug/nrc_core/debug_level    # Set to INFO level
echo 1 > /sys/kernel/debug/nrc_core/debug_level    # Set to WARN level
echo 0 > /sys/kernel/debug/nrc_core/debug_level    # Set to ERR level
cat /sys/kernel/debug/nrc_core/debug_level         # Check current level

# Debug mask control (per-module category filtering)
echo 0x1007 > /sys/kernel/debug/nrc_core/debug_mask  # WIM+TX+RX
cat /sys/kernel/debug/nrc_core/debug_mask

# SKB statistics
echo "Y" > /sys/kernel/debug/nrc_core/skb_debug  # Enable
cat /sys/kernel/debug/nrc_core/skb_stats         # View stats
echo "N" > /sys/kernel/debug/nrc_core/skb_debug  # Disable

# Device control (SPI module)
echo 1 > /sys/kernel/debug/nrc_spi/reset         # Hardware reset
cat /sys/kernel/debug/nrc_spi/cspi               # SPI status test

# Device control (Core module)
cat /sys/kernel/debug/nrc_core/credit            # Credit status
cat /sys/kernel/debug/nrc_core/slot              # Slot status
echo 1 > /sys/kernel/debug/nrc_core/restart      # Network restart

# Loopback test (HIF throughput measurement)
echo 1024 > /sys/kernel/debug/nrc_core/loopback/sample  # Set sample size 1KB
echo 100 > /sys/kernel/debug/nrc_core/loopback/count    # 100 frames
echo 0 > /sys/kernel/debug/nrc_core/loopback/test       # Run round-trip test
cat /sys/kernel/debug/nrc_core/loopback/report          # View results

# Power save control
cat /sys/kernel/debug/nrc_core/ps_control               # View PS status
echo "wake 2000" > /sys/kernel/debug/nrc_core/ps_control    # Wake with 2s timeout
echo "sleep 2 1000" > /sys/kernel/debug/nrc_core/ps_control # Deepsleep TIM 1s

# Firmware control
cat /sys/kernel/debug/nrc_core/fw_control               # View FW status

# WLAN interface info
cat /sys/kernel/debug/ieee80211/nrc80211/info

# Signal quality
cat /sys/kernel/debug/ieee80211/nrc80211/rssi
cat /sys/kernel/debug/ieee80211/nrc80211/snr
```

## Architecture

### Two-Tier Debug System

The NRC driver uses a two-tier debug filtering system:

1. **Debug Level (Global Verbosity)**
   - Controls how verbose the output is
   - Applies to ALL modules
   - Priority-based: ERR (0) → WARN (1) → INFO (2) → DBG (3)
   - Set via `/sys/kernel/debug/nrc_core/debug_level`

2. **Debug Mask (Per-Module Categories)**
   - Controls which categories of messages are shown
   - Independent for each module (SPI, Core, WLAN, MCP)
   - Bitfield: each bit represents a category (HIF, WIM, TX, RX, etc.)
   - Set via `/sys/kernel/debug/<module>/debug_mask`

**Filtering Logic:**
```
Message shown if:
  (message_level <= current_debug_level) AND 
  (message_category_bit is set in debug_mask)

Exception: ERR level messages ignore mask check
```

**Example:**
```bash
# Set level to INFO (shows INFO, WARN, ERR)
sudo ./scripts/debug.sh level set INFO

# Enable only TX and RX categories
sudo ./scripts/debug.sh mask set core TX RX

# Result: Only TX/RX messages at INFO level or higher
# DBG_TX() messages will NOT show (DBG level > INFO level)
# INFO_TX() messages WILL show (INFO level == INFO level, TX bit set)
# ERR_MAC() messages WILL show (ERR always shown, mask ignored)
```

### Module Organization

```
┌─────────────────────────────────────┐
│ SPI Backend (nrc_spi.ko)            │
│ - SPI hardware interface            │
└──────────────────┬──────────────────┘
                   ↓
┌─────────────────────────────────────┐
│ Core HAL (nrc_core.ko)              │
│ - Hardware abstraction              │
│ - SKB statistics (shared)           │
│ - Device control                    │
└──────────────┬──────────────────────┘
               ↓
    ┌──────────┴──────────┐
    ↓                     ↓
┌─────────────┐   ┌─────────────┐
│ WLAN        │   │ MCP         │
│ (nrc_wlan)  │   │ (nrc_mcp)   │
└─────────────┘   └─────────────┘
```

### Key Features

- **Debug Level Control**: Global verbosity setting (ERR/WARN/INFO/DBG)
- **Modular Debug Masks**: Each module has independent category filtering
- **Centralized SKB Stats**: Shared by all frontends in nrc_core
- **User-Friendly Script**: debug.sh provides easy CLI interface
- **Real-Time Control**: Changes take effect immediately
- **Two-Tier Filtering**: Level (verbosity) + Mask (category) for fine control

## Notes

- Script must be run as root (use sudo)
- Debug messages appear in kernel log (dmesg)
- **Debug Level**: Controls verbosity globally (ERR < WARN < INFO < DBG)
- **Debug Mask**: Controls which categories are shown per module
- **Filtering Logic**: Message shown if (level <= current_level) AND (category bit set in mask)
- **ERR Messages**: Always shown regardless of mask, only level matters
- SKB tracking has minimal performance impact
- Not all modules may be loaded at runtime
- Use `level set WARN` and `mask preset errors-only` for production

## Reference

For detailed implementation information, see:
- `common/nrc-debug-common.h` - Debug bit definitions and macros
- `scripts/debug.sh` - Debug control script
- `backend/nrc_spi/nrc-debug.c` - SPI debug implementation
- `hal/nrc_core/nrc-debug.c` - Core debug implementation
- `frontend/nrc_wlan/nrc-debug.c` - WLAN debug implementation
- `frontend/nrc_mcp/nrc-debug.c` - MCP debug implementation
