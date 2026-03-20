# NRC7394 Software Package (sw_pkg) -- Deployment Guide

> **Target Audience**: Rigado and integration partners deploying NRC7394 IEEE 802.11ah
> (Wi-Fi HaLow) on Linux-based embedded platforms.

## Overview

This software package (`sw_pkg`) provides everything needed to operate the Newracom
NRC7394 Wi-Fi HaLow chipset on production Linux hardware. It includes startup/shutdown
scripts, regional regulatory configurations, firmware binaries, network configuration
templates, and utility tools for mesh, sniffer, and power management.

### Package Origin

- **Base**: [newracom/nrc7394_sw_pkg](https://github.com/newracom/nrc7394_sw_pkg/tree/master/package/evk/sw_pkg) (SDK v1.3.1)
- **Enhancements**: Modular driver support (`start_modular.py`, `stop_modular.py`),
  additional country configs (T2, SG), updated scripts with auto-path detection,
  improved error handling, and NewraPeek sniffer versions.

### Location Within the Driver Source Tree

This `sw_pkg` is bundled inside the NRC driver source repository, **not** distributed
as a standalone package. Understanding this layout is important:

```
drivers/net/wireless/newracom/nrc/          <-- Driver source root
+-- Makefile                                <-- Build the kernel modules here
+-- firmware/                               <-- (*) Canonical firmware location
|   +-- nrc7394_cspi.bin                    <--   SPI firmware (latest, used by build)
|   +-- nrc7394_bd.dat                      <--   Board data (latest, used by build)
|
+-- extras/
|   +-- sw_pkg/                             <-- (*) This package (you are here)
|       +-- README.md                       <--   This guide
|       +-- nrc_pkg/                        <--   Runtime package to deploy
|           +-- sw/firmware/                <--   Firmware copies for deployment
|
+-- backend/nrc_spi/                        <-- SPI backend module source
+-- hal/nrc_core/                           <-- HAL core module source
+-- frontend/nrc_wlan/                      <-- WLAN frontend module source
+-- frontend/nrc_mcp/                       <-- MCP frontend module source (optional)
```

> **Key points:**
>
> - **Driver source** and **sw_pkg** live in the **same repository**. After cloning
>   the repo, both driver code and this deployment package are immediately available.
> - **Firmware files** exist in **two places**:
>   - `nrc/firmware/` -- The **canonical (authoritative)** copy. This is what the
>     kernel build system and `make install` use. Always the latest version.
>   - `nrc/extras/sw_pkg/nrc_pkg/sw/firmware/` -- A copy bundled for standalone
>     deployment to target devices. **If the canonical firmware is updated, you
>     should sync this copy** before deploying:
>     ```bash
>     cp nrc/firmware/nrc7394_cspi.bin  nrc/extras/sw_pkg/nrc_pkg/sw/firmware/
>     cp nrc/firmware/nrc7394_bd.dat    nrc/extras/sw_pkg/nrc_pkg/sw/firmware/
>     ```
> - **Kernel modules** are built from the driver source (`make` in `nrc/`), then
>   copied into `sw_pkg/nrc_pkg/sw/driver/` for deployment.

## Directory Structure

```
sw_pkg/
+-- README.md                              # This guide
+-- RIGADO_C5K_AP_bringup_for_NRC.md       # (*) Rigado C5K AP bring-up guide
+-- update.sh                              # Package updater (backup + install)
|
+-- nrc_pkg/               # Main package (deploy to target device)
    +-- VERSION-SDK        # SDK version info
    |
    +-- etc/                           # System configuration templates
    |   +-- dhcpcd/dhcpcd.conf         # DHCP client config (AP mode IP setup)
    |   +-- dnsmasq/dnsmasq.conf       # DNS/DHCP server config (AP mode)
    |
    +-- script/                        # Operational scripts
    |   +-- start_modular.py           # (*) Modular driver startup (recommended)
    |   +-- stop_modular.py            # (*) Modular driver shutdown (recommended)
    |   +-- start.py                   # Legacy monolithic driver startup
    |   +-- stop.py                    # Legacy monolithic driver shutdown
    |   +-- run_recovery.py            # Error recovery handler
    |   +-- mesh.py                    # Mesh networking utilities
    |   +-- mesh_add_peer.py           # Mesh peer management
    |   +-- ps_resume.sh               # Power-save resume
    |   +-- ps_suspend.sh              # Power-save suspend
    |   |
    |   +-- cli_app_src/               # (*) CLI application source code
    |   |   +-- Makefile               #     Build with 'make' on target device
    |   |   +-- main.c, cli_cmd.c ...  #     Source files
    |   |
    |   +-- conf/                      # Regional Wi-Fi configurations
    |   |   +-- US/                    # United States (FCC)
    |   |   +-- EU/                    # Europe (ETSI)
    |   |   +-- JP/                    # Japan (MIC)
    |   |   +-- AU/                    # Australia (ACMA)
    |   |   +-- NZ/                    # New Zealand
    |   |   +-- TW/                    # Taiwan (NCC)
    |   |   +-- SG/                    # Singapore (IMDA)
    |   |   +-- K1/                    # Korea Band 1
    |   |   +-- K2/                    # Korea Band 2
    |   |   +-- T2/                    # Thailand Band 2
    |   |   +-- etc/                   # IP config, routing, clock scripts
    |   |
    |   +-- airplane_mode/             # GPIO-based airplane mode sample
    |   +-- wps/                       # WPS Push-Button sample
    |   +-- mesh/                      # Mesh setup scripts
    |   +-- sniffer/                   # Wireshark / NewraPeek tools
    |
    +-- sw/                            # Software binaries
        +-- driver/                    # Kernel module placeholder
        |   +-- README.txt             # Build instructions
        +-- firmware/                  # Firmware images
            +-- nrc7394_cspi.bin       # SPI firmware
            +-- nrc7394_cspi_eeprom.bin# EEPROM-variant firmware
            +-- nrc7394_bd.dat         # Board data file
            +-- copy.sh               # Firmware install helper
```

## Prerequisites

### Hardware
- Linux SBC with SPI interface (Raspberry Pi 4, or custom embedded platform)
- NRC7394 EVK board or custom board with NRC7394 chipset
- SPI connection between host and NRC7394

### Software
- Linux kernel 4.19+ (tested on 5.10.17, 6.12.x)
- Python 3.x
- Required packages (auto-installed by `start_modular.py` if missing):

```bash
sudo apt-get install -y \
    iw \
    rfkill \
    bridge-utils \
    wpasupplicant \
    hostapd \
    dhcpcd5 \
    dnsmasq
```

### NRC Driver Modules

The modular scripts require the NRC modular driver kernel modules. Build them
from the driver source tree (the **same repository** that contains this sw_pkg):

```bash
# From the repository root
cd drivers/net/wireless/newracom/nrc
make

# Resulting modules:
#   backend/nrc_spi/nrc_spi.ko   -- SPI backend
#   hal/nrc_core/nrc_core.ko     -- HAL core
#   frontend/nrc_wlan/nrc_wlan.ko -- WLAN frontend (standard Wi-Fi)
#   frontend/nrc_mcp/nrc_mcp.ko  -- MCP frontend (optional, management)
```

After building, copy the modules into sw_pkg for deployment:

```bash
# Still inside drivers/net/wireless/newracom/nrc/
cp backend/nrc_spi/nrc_spi.ko    extras/sw_pkg/nrc_pkg/sw/driver/
cp hal/nrc_core/nrc_core.ko      extras/sw_pkg/nrc_pkg/sw/driver/
cp frontend/nrc_wlan/nrc_wlan.ko extras/sw_pkg/nrc_pkg/sw/driver/
```

### Firmware Sync (Important)

The **authoritative** firmware files live in `nrc/firmware/` (the driver source root).
The sw_pkg contains its own copies for standalone deployment. Before deploying,
**always sync** to ensure the sw_pkg has the latest firmware:

```bash
# From drivers/net/wireless/newracom/nrc/
cp firmware/nrc7394_cspi.bin  extras/sw_pkg/nrc_pkg/sw/firmware/
cp firmware/nrc7394_bd.dat    extras/sw_pkg/nrc_pkg/sw/firmware/
```

> WARNING: If you skip this step, the deployed package may contain older firmware
> than what the driver source tree provides.

## Quick Start

### 1. Build & Prepare the Deployment Package

```bash
# From the driver source root: drivers/net/wireless/newracom/nrc/
make

# Sync firmware from canonical location
cp firmware/nrc7394_cspi.bin  extras/sw_pkg/nrc_pkg/sw/firmware/
cp firmware/nrc7394_bd.dat    extras/sw_pkg/nrc_pkg/sw/firmware/

# Copy built kernel modules
cp backend/nrc_spi/nrc_spi.ko    extras/sw_pkg/nrc_pkg/sw/driver/
cp hal/nrc_core/nrc_core.ko      extras/sw_pkg/nrc_pkg/sw/driver/
cp frontend/nrc_wlan/nrc_wlan.ko extras/sw_pkg/nrc_pkg/sw/driver/
```

### 1a. Build cli_app on the Target Device (Required)

The `cli_app` binary is a userspace CLI tool for runtime driver control (e.g.,
setting TX power, querying stats). It is **not** provided as a pre-built binary
because it must be compiled natively on the target device's architecture.

After deploying nrc_pkg to the target device (Step 2), build it there:

```bash
# On the target device
cd ~/nrc_pkg/script/cli_app_src
make clean && make
cp cli_app ../          # Copy to script/ where start_modular.py expects it
```

> NOTE: The target device must have `gcc` and `make` installed.
> On Debian/Ubuntu: sudo apt install build-essential

### 2. Deploy to Target Device

```bash
# Copy the prepared nrc_pkg to the target device
scp -r extras/sw_pkg/nrc_pkg user@target:~/nrc_pkg

# On the target device, set permissions
ssh user@target "cd ~/nrc_pkg && chmod -R 755 *"
```

### 3. Install Firmware (on target device)

```bash
cd ~/nrc_pkg/sw/firmware
sudo ./copy.sh 7394 nrc7394_bd.dat 0
# Arguments: <chip_version> <board_data_file> <use_eeprom: 0=normal, 1=eeprom>
```

This copies firmware files to `/lib/firmware/` where the kernel driver expects them.

### 3. Start the Driver -- Modular (Recommended)

```bash
cd ~/nrc_pkg/script

# Start as STA (Station/Client) mode -- Open security -- US region
sudo python3 start_modular.py 0 0 US

# Start as AP (Access Point) mode -- WPA2-PSK -- US region
sudo python3 start_modular.py 1 1 US

# Start as AP -- WPA3-SAE -- US region
sudo python3 start_modular.py 1 3 US
```

### 4. Stop the Driver

```bash
# Full stop (stop services + unload modules)
sudo python3 stop_modular.py

# Stop modules only (keep network services running)
sudo python3 stop_modular.py modules

# Clean stop (reset all configs + restart NetworkManager)
sudo python3 stop_modular.py clean
```

## Script Reference

### start_modular.py -- Modular Driver Startup

This is the **recommended** startup script for the modular NRC driver architecture.

```
Usage:
    start_modular.py [sta_type] [security_mode] [country] [channel] [sniffer_mode]

Arguments:
    sta_type        0: STA (Station/Client)
                    1: AP (Access Point)
                    2: SNIFFER
                    3: RELAY
                    4: MESH (Mesh Point)
                    5: MESH Portal (MPP)
                    6: MESH AP (MAP)

    security_mode   0: Open (no security)
                    1: WPA2-PSK
                    2: WPA3-OWE (Opportunistic Wireless Encryption)
                    3: WPA3-SAE (Simultaneous Authentication of Equals)

    country         US  -- United States (FCC)
                    EU  -- Europe (ETSI)
                    JP  -- Japan (MIC)
                    AU  -- Australia
                    NZ  -- New Zealand
                    TW  -- Taiwan
                    SG  -- Singapore
                    K1  -- Korea Band 1
                    K2  -- Korea Band 2
                    T2  -- Thailand Band 2

    channel         S1G channel number (required for SNIFFER mode)
    sniffer_mode    0: Local  |  1: Remote (required for SNIFFER mode)
```

**Features over legacy `start.py`**:
- Auto-detects `nrc_pkg` path (no hardcoded `/home/pi/` dependency)
- Loads modular driver modules (nrc_spi -> nrc_core -> nrc_wlan)
- Improved error handling and dependency checking
- System dependency auto-installation

### stop_modular.py -- Modular Driver Shutdown

```
Usage:
    stop_modular.py [mode]

Modes:
    (default)   -- Full stop: stop all services and unload modules
    modules     -- Unload kernel modules only
    clean       -- Clean all configs, stop everything, restart NetworkManager
```

**Module unload order** (reverse of load): nrc_wlan -> nrc_mcp -> nrc_core -> nrc_spi

### start.py -- Legacy Driver Startup

Same interface as `start_modular.py` but uses the monolithic `nrc.ko` driver.
Use this only if you are running the legacy (non-modular) driver.

### stop.py -- Legacy Driver Shutdown

Simple shutdown script for the monolithic `nrc.ko` driver.

## Operating Modes

### Station (STA) Mode -- Connect to an AP

```bash
# Open security
sudo python3 start_modular.py 0 0 US

# WPA2-PSK
sudo python3 start_modular.py 0 1 US

# WPA3-SAE
sudo python3 start_modular.py 0 3 US
```

After starting, the device will scan and connect to a configured AP. Edit the
corresponding `conf/US/sta_halow_*.conf` file to set your target SSID and passphrase:

```ini
# Example: conf/US/sta_halow_wpa2.conf
ctrl_interface=/var/run/wpa_supplicant
country=US
network={
    ssid="your_network_ssid"
    psk="your_passphrase"
    key_mgmt=WPA-PSK
    proto=RSN
    pairwise=CCMP
}
```

### Access Point (AP) Mode -- Create a HaLow Network

```bash
# WPA2-PSK AP
sudo python3 start_modular.py 1 1 US
```

Edit `conf/US/ap_halow_wpa2.conf` to customize:

```ini
ctrl_interface=/var/run/hostapd
country_code=US
interface=wlan0
ssid=Rigado_HaLow
hw_mode=a
channel=37
beacon_int=100
wpa=2
wpa_key_mgmt=WPA-PSK
wpa_passphrase=your_secure_passphrase
wpa_pairwise=CCMP
```

### Sniffer Mode -- Monitor Wi-Fi HaLow Traffic

```bash
# Local sniffer on channel 40
sudo python3 start_modular.py 2 0 US 40 0

# Remote sniffer on channel 40
sudo python3 start_modular.py 2 0 US 40 1
```

Change channel at runtime:
```bash
cd ~/nrc_pkg/script/sniffer
python3 change_channel.py <new_channel>
```

### Mesh Networking

```bash
# Mesh Point (basic node)
sudo python3 start_modular.py 4 0 US

# Mesh Portal Point (gateway)
sudo python3 start_modular.py 5 0 US

# Mesh AP (mesh + concurrent AP)
sudo python3 start_modular.py 6 0 US
```

For advanced mesh setup, use the dedicated mesh scripts:
```bash
cd ~/nrc_pkg/script/mesh
./run_mesh.sh -m mpp -c 161 -s halow_mesh -p 12345678 -n US
```

## Configuration for US Region (Rigado)

Since Rigado operates in the United States, use the **US** country code for all
operations. The US configuration files are located in `script/conf/US/`:

| File | Mode | Security |
|------|------|----------|
| `sta_halow_open.conf` | STA | Open |
| `sta_halow_wpa2.conf` | STA | WPA2-PSK |
| `sta_halow_sae.conf` | STA | WPA3-SAE |
| `sta_halow_owe.conf` | STA | WPA3-OWE |
| `sta_halow_pbc.conf` | STA | WPS Push-Button |
| `ap_halow_open.conf` | AP | Open |
| `ap_halow_wpa2.conf` | AP | WPA2-PSK |
| `ap_halow_sae.conf` | AP | WPA3-SAE |
| `ap_halow_owe.conf` | AP | WPA3-OWE |
| `ap_halow_pbc.conf` | AP | WPS Push-Button |
| `mp_halow_open.conf` | Mesh Point | Open |
| `mp_halow_sae.conf` | Mesh Point | WPA3-SAE |
| `map_halow_*.conf` | Mesh AP | Various |
| `ibss_halow_*.conf` | Ad-Hoc | Open/WPA2 |

### US S1G Channel Map (IEEE 802.11ah)

The NRC7394 operates in the sub-1GHz band. US S1G channels map to specific
frequencies in the 902--928 MHz ISM band:

| Channel | Center Freq (MHz) | Bandwidth |
|---------|-------------------|-----------|
| 1--26    | 902.5 -- 927.5     | 1 MHz     |
| 27--39   | 903.0 -- 927.0     | 2 MHz     |
| 40--46   | 904.0 -- 926.0     | 4 MHz     |

## Network Configuration

### DHCP Client (STA mode)

The `etc/dhcpcd/dhcpcd.conf` template configures the wireless interface `wlan0`
for DHCP. To use a static IP in STA mode, uncomment and edit:

```ini
interface wlan0
metric 100
static ip_address=192.168.200.11/24
static routers=192.168.200.1
```

### DHCP Server (AP mode)

The `etc/dnsmasq/dnsmasq.conf` provides DHCP server configuration for AP mode.
Uncomment the wlan0 section:

```ini
interface=wlan0
dhcp-range=192.168.200.10,192.168.200.50,255.255.255.0,24h
```

### IP Configuration Scripts

The `script/conf/etc/` directory contains helper scripts:

| Script | Purpose |
|--------|---------|
| `ip_config.sh` | Automatic IP configuration for STA/AP modes |
| `ip_config_bridge.sh` | Bridge mode IP setup (relay/mesh) |
| `add_route.sh` | Add custom routes |
| `clock_config.sh` | NTP/clock configuration |
| `CONFIG_IP` | IP address configuration file |

## Power Management

```bash
# Suspend Wi-Fi (enter power-save)
cd ~/nrc_pkg/script
./ps_suspend.sh

# Resume Wi-Fi (exit power-save)
./ps_resume.sh
```

## Firmware Management

The firmware directory contains essential binary files:

| File | Description |
|------|-------------|
| `nrc7394_cspi.bin` | Main SPI firmware image |
| `nrc7394_cspi_eeprom.bin` | EEPROM-variant firmware |
| `nrc7394_bd.dat` | Board data (calibration, RF parameters) |

### Install / Update Firmware

```bash
cd ~/nrc_pkg/sw/firmware

# Normal mode (non-EEPROM)
sudo ./copy.sh 7394 nrc7394_bd.dat 0

# EEPROM mode
sudo ./copy.sh 7394 nrc7394_bd.dat 1
```

This copies firmware to `/lib/firmware/` as `uni_s1g.bin` (the name expected
by the kernel driver).

## Package Update

To update an existing installation on the target device:

```bash
# From the sw_pkg directory on the target
./update.sh
```

This script will:
1. Backup the current `~/nrc_pkg` with version suffix
2. Copy the new package
3. Rebuild `nrc.ko` and `cli_app` if kernel version differs
4. Set correct permissions

## Troubleshooting

### Module Load Failures
```bash
# Check if modules are loaded
lsmod | grep nrc

# Check kernel log for errors
dmesg | grep nrc

# Verify firmware is installed
ls -la /lib/firmware/uni_s1g.bin
ls -la /lib/firmware/nrc7394_bd.dat
```

### Wi-Fi Interface Not Appearing
```bash
# Check if wlan0 exists
ifconfig -a | grep wlan

# Check rfkill status
rfkill list

# If blocked, unblock
sudo rfkill unblock wifi
```

### Connection Issues
```bash
# Check wpa_supplicant status (STA mode)
sudo wpa_cli status

# Check hostapd status (AP mode)
sudo hostapd_cli status

# Scan for networks
sudo iw dev wlan0 scan
```

### Recovery
```bash
# Run the recovery script
cd ~/nrc_pkg/script
sudo python3 run_recovery.py
```

## Integration Notes for Rigado

### Rigado C5K AP Bring-Up Guide

If you are deploying on a **Rigado C5000K Access Point**, refer to the dedicated
bring-up guide included in this package:

**[RIGADO_C5K_AP_bringup_for_NRC.md](RIGADO_C5K_AP_bringup_for_NRC.md)**

This guide covers the complete procedure specific to the C5K platform (Ubuntu-based),
including:

- SSH access and initial device connection
- Required Linux package installation (avahi-daemon, dnsmasq, hostapd, iw,
  dhcpcd, gcc-13, etc.)
- mac80211 module loading configuration
- NRC kernel module on-device build and deployment
- Running start_modular.py for AP bring-up

Start here if you are working with the Rigado C5K hardware. The notes below
provide additional general-purpose reference that applies to all platforms.

### General Notes

1. **Modular vs Legacy**: Always use `start_modular.py` / `stop_modular.py` with
   the modular driver (nrc_spi + nrc_core + nrc_wlan). The legacy `start.py` /
   `stop.py` are provided only for backward compatibility with the monolithic
   `nrc.ko` driver.

2. **Path Auto-Detection**: `start_modular.py` automatically detects the `nrc_pkg`
   directory based on the script's location. No need to hardcode paths.

3. **Custom Board Data**: If Rigado uses a custom RF board design, replace
   `nrc7394_bd.dat` with your calibrated board data file.

4. **Systemd Integration**: For production deployment, create a systemd service
   to start the driver at boot:

   ```ini
   # /etc/systemd/system/nrc-halow.service
   [Unit]
   Description=NRC7394 Wi-Fi HaLow Driver
   After=network-pre.target
   Before=network.target

   [Service]
   Type=oneshot
   RemainAfterExit=yes
   ExecStart=/usr/bin/python3 /home/user/nrc_pkg/script/start_modular.py 1 1 US
   ExecStop=/usr/bin/python3 /home/user/nrc_pkg/script/stop_modular.py

   [Install]
   WantedBy=multi-user.target
   ```

5. **Security Recommendation**: For production deployments, use WPA3-SAE
   (security mode 3) for the strongest Wi-Fi HaLow security.

## Tips & Shell Aliases

For convenience on the target device, add these aliases to `~/.bashrc`:

```bash
alias nrc='cd ~/nrc_pkg'
alias script='cd ~/nrc_pkg/script/'
alias ipset='sudo vi /etc/dhcpcd.conf'
alias dhcpset='sudo vi /etc/dnsmasq.conf'
```

## License

This software package is distributed as part of the Newracom NRC7394 SDK.
Refer to the main driver license for terms of use.
