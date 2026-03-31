# Module Loading Guide — modprobe vs insmod

This guide explains how to load NRC7394 modular driver modules using
`modprobe` (system-managed) or `insmod` (manual path-based).

Two usage scenarios are covered:

- **Scenario A**: Using `start_modular.py` (automated — handles everything)
- **Scenario B**: Manual `modprobe` / `insmod` (without `start_modular.py`)

---

## Module Architecture

```
nrc_spi.ko   →   nrc_core.ko   →   nrc_wlan.ko   [→ nrc_mcp.ko]
(SPI Backend)     (HAL)             (WLAN Frontend)    (MCP Frontend)
```

Dependencies are resolved automatically by `modprobe`.
Loading `nrc_wlan` automatically loads `nrc_core` and `nrc_spi` first.

---

## Scenario A: Using start_modular.py

`start_modular.py` supports both `modprobe` and `insmod` via the `load=`
option. It **automatically manages** conf files, depmod, firmware, and
network configuration.

### modprobe mode (default)

```bash
# Prerequisites: run once
cd host/linux/driver/nrc_modular
make && sudo make install

# STA mode
sudo ./start_modular.py 0 0 US

# AP mode
sudo ./start_modular.py 1 0 US

# Stop
sudo ./stop_modular.py
```

When using modprobe mode, `start_modular.py` automatically:
1. Generates `/etc/modprobe.d/nrc_*.conf` files with current parameters
2. If `.ko` files exist in `nrc_pkg/sw/driver/`, copies them to the system
   module path and runs `depmod -a` (skipped if already up-to-date)
3. Loads all modules with a single `modprobe nrc_wlan`
4. Loads MCP module separately via `modprobe nrc_mcp`

**Conf files are regenerated on every run**, so parameters like `ps=2`,
`debug_level=3` etc. are always reflected in the conf files.

### insmod mode (legacy)

```bash
# No 'make install' needed — loads directly from nrc_pkg/sw/driver/
sudo ./start_modular.py 0 0 US load=insmod
```

### Additional parameters

```bash
# Power save
sudo ./start_modular.py 0 0 US ps=2          # TIM deep sleep
sudo ./start_modular.py 0 0 US ps=3 idle=1   # nonTIM + idle

# Debug
sudo ./start_modular.py 0 0 US debug_level=3
sudo ./start_modular.py 0 0 US "dbg=TX|RX"   # level=3 + TX,RX mask
sudo ./start_modular.py 0 0 US "vbs=ALL"      # level=4 + all mask

# Recovery
sudo ./start_modular.py 0 0 US recovery=1

# RAW (AP only)
sudo ./start_modular.py 1 0 US raw=1

# Combined
sudo ./start_modular.py 0 0 US ps=2 recovery=1 debug_level=3 load=insmod
```

---

## Scenario B: Manual modprobe (without start_modular.py)

For users who want to load modules manually without the startup script.

### Step 1: Build and install

```bash
cd host/linux/driver/nrc_modular
make
sudo make install
```

This copies `.ko` to `/lib/modules/$(uname -r)/extra/nrc/`, firmware to
`/lib/firmware/`, and runs `depmod -a`.

### Step 2: Configure parameters

Copy the template conf files and edit for your setup:

```bash
sudo cp extras/modprobe.d/nrc_spi.conf  /etc/modprobe.d/
sudo cp extras/modprobe.d/nrc_core.conf /etc/modprobe.d/
sudo cp extras/modprobe.d/nrc_wlan.conf /etc/modprobe.d/
sudo cp extras/modprobe.d/nrc_mcp.conf  /etc/modprobe.d/
```

**Each template conf file contains ALL available parameters** with
descriptions, grouped by category. Required parameters are active;
optional ones are commented out. Uncomment and edit as needed.

#### Minimum required edits

**`/etc/modprobe.d/nrc_spi.conf`** — Verify SPI hardware settings:
```
options nrc_spi hifspeed=20000000 spi_bus_num=0 spi_cs_num=0 spi_gpio_irq=5
```

**`/etc/modprobe.d/nrc_wlan.conf`** — Firmware and mode-specific params:
```
# Basic STA:
options nrc_wlan fw_name=uni_s1g.bin bd_name=nrc7394_bd.dat

# STA with Power Save:
options nrc_wlan fw_name=uni_s1g.bin bd_name=nrc7394_bd.dat \
    power_save=2 listen_interval=1000 bss_max_idle=1800

# AP with NDP probe:
options nrc_wlan fw_name=uni_s1g.bin bd_name=nrc7394_bd.dat \
    ndp_preq=1 bss_max_idle=1800
```

> **Note**: You must unload and reload modules for conf changes to take
> effect. `modprobe` reads conf files only at load time.

### Step 3: Copy firmware

Ensure firmware files are in `/lib/firmware/`:
```bash
ls /lib/firmware/uni_s1g.bin /lib/firmware/nrc7394_bd.dat
```
(`make install` handles this, but verify after kernel upgrades.)

### Step 4: Load modules

```bash
# Load WLAN stack (auto-loads nrc_spi → nrc_core → nrc_wlan)
sudo modprobe nrc_wlan

# Load MCP frontend (optional)
sudo modprobe nrc_mcp
```

### Step 5: Unload modules

```bash
sudo rmmod nrc_mcp   2>/dev/null
sudo rmmod nrc_wlan
sudo rmmod nrc_core
sudo rmmod nrc_spi
```

### Step 6: Network configuration (manual)

After loading modules, configure the network interface manually:

```bash
# STA — connect via wpa_supplicant
sudo ip link set wlan0 up
sudo wpa_supplicant -D nl80211 -i wlan0 -c /path/to/wpa_supplicant.conf -B
sudo dhclient wlan0

# AP — start hostapd
sudo ip addr add 192.168.200.1/24 dev wlan0
sudo ip link set wlan0 up
sudo hostapd /path/to/hostapd.conf -B
```

---

## Boot-Time Loading (Kernel Integration)

For systems that need the NRC driver to load automatically at boot
(e.g., embedded Linux, Ubuntu Core), use the standard kernel module
loading mechanism.

The `nrc_spi` module internally synchronizes with the SPI device probe
before returning from `module_init()`, so `modprobe nrc_wlan` reliably
loads the entire dependency chain (nrc_spi → nrc_core → nrc_wlan) in
a single command — no special sequencing or delays required.

### Step 1: Install modules and firmware

```bash
cd host/linux/driver/nrc_modular
make
sudo make install    # installs .ko + firmware + runs depmod
```

This places:
- Modules in `/lib/modules/$(uname -r)/extra/nrc/`
- Firmware (`uni_s1g.bin`, `nrc7394_bd.dat`) in `/lib/firmware/`

### Step 2: Configure parameters

Copy conf templates and edit for your NRC7394 setup:

```bash
sudo cp extras/modprobe.d/nrc_spi.conf  /etc/modprobe.d/
sudo cp extras/modprobe.d/nrc_wlan.conf /etc/modprobe.d/
```

`/etc/modprobe.d/nrc_spi.conf`:
```
options nrc_spi hifspeed=20000000 spi_bus_num=0 spi_cs_num=0 \
    spi_gpio_irq=5 spi_polling_interval=0
```

`/etc/modprobe.d/nrc_wlan.conf`:
```
options nrc_wlan fw_name=uni_s1g.bin bd_name=nrc7394_bd.dat
```

### Step 3: Add to boot

Add a single line to `/etc/modules`:

```bash
echo "nrc_wlan" | sudo tee -a /etc/modules
```

That's it. On boot, `systemd-modules-load` runs `modprobe nrc_wlan`,
which automatically resolves dependencies and loads all three modules
with parameters from the conf files.

### Step 4: Reboot and verify

```bash
sudo reboot

# After reboot:
lsmod | grep nrc                       # all 3 modules loaded
ip link show wlan0                     # interface exists
```

### Network Services (post-boot)

Module loading only creates the `wlan0` interface. Network services
(hostapd, wpa_supplicant) are configured separately — for example,
via systemd:

```ini
# Example: /etc/systemd/system/halow-network.service
[Unit]
Description=HaLow Network Service
After=systemd-modules-load.service

[Service]
Type=simple
ExecStartPre=/bin/bash -c "while [ ! -d /sys/class/net/wlan0 ]; do sleep 1; done"
# For STA:
ExecStart=/sbin/wpa_supplicant -iwlan0 -c /etc/wpa_supplicant/halow.conf
# For AP:
# ExecStart=/usr/sbin/hostapd /etc/hostapd/halow.conf
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

### Boot-Time Setup Checklist

1. ☐ `sudo make install` — modules + firmware + depmod
2. ☐ Conf files in `/etc/modprobe.d/`: `nrc_spi.conf`, `nrc_wlan.conf`
3. ☐ `nrc_wlan` added to `/etc/modules`

---

## Module File Priority

When `start_modular.py` uses modprobe mode:

| `.ko` in `nrc_pkg/sw/driver/` | System path | Behavior |
|-------------------------------|-------------|----------|
| ✅ Present | Any | Auto-copied to system → modprobe uses latest |
| ❌ Absent | ✅ Installed | modprobe uses system-installed version |
| ❌ Absent | ❌ Empty | Error — run `make install` or `load=insmod` |

> **Tip**: For the cleanest setup, run `make install` once and then delete
> `.ko` files from `nrc_pkg/sw/driver/`:
> ```bash
> rm -f /home/pi/nrc_pkg/sw/driver/nrc_spi.ko
> rm -f /home/pi/nrc_pkg/sw/driver/nrc_core.ko
> rm -f /home/pi/nrc_pkg/sw/driver/nrc_wlan.ko
> rm -f /home/pi/nrc_pkg/sw/driver/nrc-mcp.ko
> ```

---

## Method: insmod (Development)

`insmod` loads modules directly from `.ko` file paths without system
installation. Useful during active development.

```bash
# Load in dependency order with explicit parameters
sudo insmod /path/to/nrc_spi.ko  hifspeed=20000000 spi_bus_num=0 \
    spi_cs_num=0 spi_gpio_irq=5
sudo insmod /path/to/nrc_core.ko
sudo insmod /path/to/nrc_wlan.ko fw_name=uni_s1g.bin bd_name=nrc7394_bd.dat \
    bss_max_idle=1800 listen_interval=1000 set_cca_threshold=-75

# Optional: MCP module
sudo insmod /path/to/nrc-mcp.ko mcp_priority=0 bd_name=nrc7394_bd.dat \
    fw_name=uni_s1g.bin
```

> **Note**: With `insmod`, you must load modules in the correct dependency
> order and pass all parameters on the command line.

---

## Configuration Files Reference

All template conf files are in `extras/modprobe.d/`. Each file contains:
- **Active parameters** (uncommented `options` line) — essential defaults
- **All optional parameters** — commented out with descriptions
- **Quick examples** — common configurations for STA, AP, debug

| File | Module | Key Parameters |
|------|--------|----------------|
| `nrc_spi.conf` | nrc_spi | SPI bus/CS/IRQ, speed, polling, GPIO |
| `nrc_core.conf` | nrc_core | debug_level, debug_mask |
| `nrc_wlan.conf` | nrc_wlan | FW, PS, BSS idle, AMPDU, NDP, CCA, TWT, RAW, recovery, debug |
| `nrc_mcp.conf` | nrc_mcp | MCP priority, FW/BD names |

---

## File Locations Summary

| File | Path | Purpose |
|------|------|---------|
| Kernel modules | `/lib/modules/$(uname -r)/extra/nrc/*.ko` | System-installed modules |
| Firmware | `/lib/firmware/uni_s1g.bin` | Firmware binary |
| Board data | `/lib/firmware/nrc7394_bd.dat` | Board calibration data |
| SPI config | `/etc/modprobe.d/nrc_spi.conf` | SPI backend parameters |
| Core config | `/etc/modprobe.d/nrc_core.conf` | HAL parameters |
| WLAN config | `/etc/modprobe.d/nrc_wlan.conf` | WLAN frontend parameters |
| MCP config | `/etc/modprobe.d/nrc_mcp.conf` | MCP frontend parameters |
| Module deps | `/lib/modules/$(uname -r)/modules.dep` | Auto-generated by depmod |
| Default confs | `extras/modprobe.d/*.conf` | Templates to copy |

---

## Troubleshooting

### modprobe: FATAL: Module nrc_wlan not found

Modules are not installed or depmod hasn't been run:

```bash
sudo make install    # installs .ko files + runs depmod
```

### modprobe succeeds but driver fails to initialize

Check that conf files have correct parameters for your hardware:

```bash
cat /etc/modprobe.d/nrc_spi.conf    # verify SPI settings
cat /etc/modprobe.d/nrc_wlan.conf   # verify firmware name
ls /lib/firmware/uni_s1g.bin        # verify firmware exists
```

### Verify module dependencies

```bash
# Show dependency chain
modinfo nrc_wlan | grep depends
# Expected: depends: nrc_core

modinfo nrc_core | grep depends
# Expected: depends: nrc_spi

# Show full dependency tree
modprobe --show-depends nrc_wlan
```

### Verify loaded modules and parameters

```bash
# List loaded NRC modules
lsmod | grep nrc

# Check current parameter values
cat /sys/module/nrc_wlan/parameters/power_save
cat /sys/module/nrc_wlan/parameters/debug_level
cat /sys/module/nrc_spi/parameters/hifspeed
```

### After kernel upgrade

Module paths are kernel-version specific. After a kernel upgrade:

```bash
cd host/linux/driver/nrc_modular
make clean && make
sudo make install
```

The conf files in `/etc/modprobe.d/` persist across kernel upgrades.
