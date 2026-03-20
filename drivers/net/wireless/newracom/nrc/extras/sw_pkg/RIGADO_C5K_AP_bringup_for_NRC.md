
# Rigado C5K AP – NRC Driver Bring‑Up Guide

This document provides the complete procedure for setting up and bringing up the NRC wireless driver on a Rigado C5K access point running Ubuntu.

## 1. Connect to the Device

SSH into the C5K system:

```bash
ssh ubuntu@<device-ip>
# username: ubuntu
# password: rigado
```

## 2. Install Required Linux Packages

Install all essential components for mDNS broadcasting, DHCP/DNS services, hostapd, wireless tools, Python 3 environment, and build toolchains.

```bash
sudo apt update

# mDNS service
sudo apt install avahi-daemon

# Network services & wireless utilities
sudo apt install dnsmasq hostapd iw dhcpcd iperf3

# Ensure `python` command maps to Python 3
sudo apt install python-is-python3

# Build toolchain for NRC driver compilation
sudo apt install make gcc-13 g++-13
```

### Package Details

| Package | Purpose |
|--------|---------|
| `avahi-daemon` | Provides mDNS/Bonjour discovery (`*.local`) |
| `dnsmasq` | Lightweight DNS/DHCP server for AP mode |
| `hostapd` | Required for Wi‑Fi AP functionality |
| `iw` | nl80211-based wireless management tool |
| `dhcpcd` | DHCP client daemon |
| `iperf3` | Network throughput performance testing |
| `python-is-python3` | Ensures `python` invokes Python 3 |
| `make`, `gcc-13`, `g++-13` | Build tools for compiling NRC kernel modules |

## 3. Prepare the System for NRC Modular Operation

Add the `mac80211` module to the module auto‑load configuration:

```bash
echo mac80211 | sudo tee -a /etc/modules-load.d/modules.conf
cat /etc/modules-load.d/modules.conf
sudo reboot
```

A reboot is required for the module to load correctly.

## 4. Copy the NRC Package

Copy the NRC package directory into the home folder:

```
/home/ubuntu/nrc_pkg
```

## 5. Build the NRC Kernel Driver

Build the NRC driver modules directly on the C5K platform. After building, copy all generated `.ko` files into:

```
/home/ubuntu/nrc_pkg/sw/driver
```

Ensure the directory contains the required kernel objects before proceeding.

## 6. Run the Modular Driver Bring‑Up Script

Ensure all `.py` and `.sh` scripts inside the `script` directory have executable permissions:

```bash
cd /home/ubuntu/nrc_pkg/script
chmod +x *.py *.sh
```

Start the NRC modular bring‑up:

```bash
./start_modular 1 0 US
```

## Notes

- Ensure your kernel version matches the driver build environment.
- mDNS hostname discovery allows accessing the device using: `ssh ubuntu@<hostname>.local`
- If AP mode is used, verify that `hostapd` and `dnsmasq` are not masked or disabled.
