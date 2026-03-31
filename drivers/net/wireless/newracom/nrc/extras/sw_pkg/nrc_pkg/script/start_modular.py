#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys, os, time, subprocess, re
import threading
import signal
from mesh import *

# Auto-detect nrc_pkg path based on script location
# This script should be located in nrc_pkg/script/ or nrc_pkg/scripts/
SCRIPT_PATH = os.path.abspath(__file__)
SCRIPT_DIR = os.path.dirname(SCRIPT_PATH)
NRC_PKG_PATH = os.path.dirname(SCRIPT_DIR)  # Parent directory of script dir

def validate_nrc_pkg_path():
    """Validate nrc_pkg path and prompt user if not found"""
    global NRC_PKG_PATH

    # First, check if auto-detected path is valid
    if os.path.isdir(NRC_PKG_PATH):
        required_dirs = ["script", "sw/driver", "sw/firmware"]
        missing_dirs = [d for d in required_dirs if not os.path.isdir(os.path.join(NRC_PKG_PATH, d))]

        if not missing_dirs:
            print("[*] nrc_pkg path auto-detected: %s" % NRC_PKG_PATH)
            return NRC_PKG_PATH

    # If auto-detection failed, try fallback paths
    fallback_paths = [
        "/home/pi/nrc_pkg",
        "/home/ubuntu/nrc_pkg",
        os.path.expanduser("~/nrc_pkg")
    ]

    for fallback in fallback_paths:
        if os.path.isdir(fallback):
            required_dirs = ["script", "sw/driver", "sw/firmware"]
            missing_dirs = [d for d in required_dirs if not os.path.isdir(os.path.join(fallback, d))]
            if not missing_dirs:
                NRC_PKG_PATH = fallback
                print("[*] nrc_pkg path found: %s" % NRC_PKG_PATH)
                return NRC_PKG_PATH

    # If all auto-detection failed, prompt user
    while True:
        if os.path.isdir(NRC_PKG_PATH):
            required_dirs = ["script", "sw/driver", "sw/firmware"]
            missing_dirs = [d for d in required_dirs if not os.path.isdir(os.path.join(NRC_PKG_PATH, d))]

            if not missing_dirs:
                print("[*] nrc_pkg path validated: %s" % NRC_PKG_PATH)
                return NRC_PKG_PATH
            else:
                print("[!] WARNING: nrc_pkg path '%s' is missing required directories: %s" % (NRC_PKG_PATH, ", ".join(missing_dirs)))
        else:
            print("[!] WARNING: nrc_pkg path '%s' does not exist" % NRC_PKG_PATH)

        # Prompt user for correct path
        print("[?] Please enter the correct path to nrc_pkg (or 'q' to quit):")
        user_input = input(">>> ").strip()

        if user_input.lower() == 'q':
            print("[!] Exiting...")
            sys.exit(1)

        # Remove trailing slash if present
        NRC_PKG_PATH = user_input.rstrip('/')

# Validate path at startup
validate_nrc_pkg_path()

def check_and_fix_permissions(script_path):
    """Check if script has execute permission, if not, add it"""
    if not os.path.exists(script_path):
        return False

    if not os.access(script_path, os.X_OK):
        print("[*] Adding execute permission to %s" % script_path)
        try:
            os.chmod(script_path, 0o755)
            return True
        except PermissionError:
            # Try with sudo
            print("[*] Trying with sudo...")
            ret = os.system("sudo chmod +x %s" % script_path)
            return ret == 0
    return True

def check_and_install_tool(tool_name, package_name=None):
    """Check if a command-line tool exists, if not, try to install it"""
    if package_name is None:
        package_name = tool_name

    # Check if tool exists (Python 2/3 compatible)
    try:
        # Try using which command
        subprocess.check_output(['which', tool_name], stderr=subprocess.STDOUT)
        return True
    except subprocess.CalledProcessError:
        # Tool not found, try to install
        print("[!] %s not found, attempting to install..." % tool_name)
        install_cmd = "sudo apt-get update -qq && sudo apt-get install -y %s" % package_name
        ret = os.system(install_cmd)
        if ret == 0:
            print("[*] %s installed successfully" % tool_name)
            return True
        else:
            print("[!] Failed to install %s" % tool_name)
            return False
    except OSError:
        # which command doesn't exist, assume tool is not available
        print("[!] %s not found, attempting to install..." % tool_name)
        install_cmd = "sudo apt-get update -qq && sudo apt-get install -y %s" % package_name
        ret = os.system(install_cmd)
        if ret == 0:
            print("[*] %s installed successfully" % tool_name)
            return True
        else:
            print("[!] Failed to install %s" % tool_name)
            return False

def ensure_script_executable(script_path):
    """Ensure script exists and is executable"""
    if not os.path.exists(script_path):
        print("[!] WARNING: Script not found: %s" % script_path)
        return False

    if not check_and_fix_permissions(script_path):
        print("[!] WARNING: Could not set execute permission for: %s" % script_path)
        return False

    return True

def check_system_dependencies():
    """Check and install required system tools"""
    print("[*] Checking system dependencies...")
    required_tools = {
        'iw': 'iw',
        'rfkill': 'rfkill',
        'ifconfig': 'net-tools',
        'brctl': 'bridge-utils'
    }

    missing_tools = []
    for tool, package in required_tools.items():
        if not check_and_install_tool(tool, package):
            missing_tools.append(tool)

    if missing_tools:
        print("[!] WARNING: Some tools could not be installed: %s" % ", ".join(missing_tools))
        print("[!] You may need to install them manually")
    else:
        print("[*] All system dependencies are available")

script_path = NRC_PKG_PATH + "/script/"
s1g_ch_support_country_list = ["US", "JP", "TW", "AU", "NZ", "K1", "K2", "SG", "T2"]
eu_ch_support_country_list = ["AT", "BE", "BG", "CY", "CZ", "DE", "DK", "EE", "ES", "FI", "FR", "GR", "HR", "HU", "IE", "IT", "LT", "LU", "LV", "MT", "NL", "PL", "PT", "RO", "SE", "SI", "SK", "GB", "SA"]

# Debug parameters (parsed from command line)
# dbg= sets both debug_level=DBG and debug_mask
debug_level_param = None
debug_mask_param = None
ps_param = None
idle_param = None
recovery_param = 0             # 0: disabled, 1: enable recovery daemon
load_method = 'modprobe'       # 'modprobe' (default) or 'insmod'

# Default Configuration (you can change value you want here)
##################################################################################
# Raspbery Pi Conf.
max_cpuclock      = 1         # Set Max CPU Clock : 0(off) or 1(on)
##################################################################################
# Firmware Conf.
model             = 7394      # 7292/7394
fw_download       = 1         # 0(FW Download off) or 1(FW Download on)
fw_name           = 'uni_s1g.bin'
auto_fw_update    = 0
fw_update_name    = 'uni_s1g_update.bin'
##################################################################################
# DEBUG Conf.
# NRC Driver log
driver_debug      = 0         # NRC Driver debug option : 0(off) or 1(on)
dbg_flow_control  = 0         # Print TRX slot and credit status in real-time
#--------------------------------------------------------------------------------#
# WPA Supplicant Log (STA Only)
supplicant_debug  = 0         # WPA Supplicant debug option : 0(off) or 1(on)
#--------------------------------------------------------------------------------#
# HOSTAPD Log (AP Only)
hostapd_debug     = 0         # Hostapd debug option    : 0(off) or 1(on)
#################################################################################
# CSPI Conf. (Default)
spi_clock    = 20000000       # SPI Master Clock Frequency
spi_bus_num  = 0              # SPI Master Bus Number
spi_cs_num   = 0              # SPI Master Chipselect Number
spi_gpio_irq = 5              # NRC-CSPI EIRQ GPIO Number
                              # BBB is 60 recommanded.
spi_polling_interval = 0      # NRC-CSPI Polling Interval (msec)

#
# NOTE:
#  - NRC-CSPI EIRQ Input Interrupt: spi_gpio_irq >= 0 and spi_polling_interval <= 0
#  - NRC-CSPI EIRQ Input Polling  : spi_gpio_irq >= 0 and spi_polling_interval > 0
#  - NRC-CSPI Registers Polling   : spi_gpio_irq < 0 and spi_polling_interval > 0
#
#--------------------------------------------------------------------------------#
# FT232H USB-SPI Conf. (FT232H CSPI Conf)
ft232h_usb_spi = 0            # FTDI FT232H USB-SPI bridge
                              # 0 : Unused
                              # 1 : NRC-CSPI_EIRQ Input Polling
                              # 2 : NRC-CSPI Registers Polling
#################################################################################
# RF Conf.
max_txpwr         = 24       # Maximum TX Power (in dBm)
bd_name           = ''       # board data name (bd defines max TX Power per CH/MCS/CC)
                             # specify your bd name here. If not, follow naming rules in strBDName()
##################################################################################
# PHY Conf.
guard_int         = 'auto'   # Guard Interval ('auto' (adaptive) or 'long'(LGI) or 'short'(SGI))
##################################################################################
# MAC Conf.
macaddr           = ''       # MAC Address (e.g. '00:11:22:33:44:55')
# S1G Short Beacon (AP & MESH Only)
#  If disabled, AP sends only S1G long beacon every BI
#  Recommend using S1G short beacon for network efficiency (Default: enabled)
short_bcn_enable  = 0        # 0 (disable) or 1 (enable)
#--------------------------------------------------------------------------------#
# Legacy ACK enable (AP & STA)
#  If disabled, AP/STA sends only NDP ack frame
#  Recommend using NDP ack mode  (Default: disable)
legacy_ack_enable  = 0        # 0 (NDP ack mode) or 1 (legacy ack mode)
#--------------------------------------------------------------------------------#
# DAC (Distributed Authentication Control) (AP only)
#  If enabled, AP sends beaon/probe response frame including authentication control IE
auth_control_enable   = 0	# 0 (disable) or 1 (enable)
auth_control_slot     = 100	# slot duration (in TU) (1~127)
auth_control_scale    = 10	# scale (1 or 10)
auth_control_ti_min   = 8	# mininum time interval (in scale * BI) (1~127)
auth_control_ti_max   = 64 	# maximum time interval (in scale * BI) (1~255)
#--------------------------------------------------------------------------------#
# Beacon Bypass enable (STA only)
#  If enabled, STA receives beacon frame from other APs even connected
#  Recommend that STA only receives beacon frame from AP connected while connecting  (Default: disable)
beacon_bypass_enable  = 0        # 0 (Receive beacon frame from only AP connected while connecting)
                                 # 1 (Receive beacon frame from all APs even while connecting)
#--------------------------------------------------------------------------------#
# Enable sched scan
#  If enabled, STA use scheduled scan
sched_scan_enable = 0            # 0 (disable) 1 (enable)
#--------------------------------------------------------------------------------#
# AMPDU (Aggregated MPDU)
#  Enable AMPDU for full channel utilization and throughput enhancement (Default: auto)
#  disable(0): AMPDU is deactivated
#   manual(1): AMPDU is activated and BA(Block ACK) session is made by manual
#     auto(2): AMPDU is activated and BA(Block ACK) session is made automatically
ampdu_enable      = 2        # 0 (disable) 1 (manual) 2 (auto)
#--------------------------------------------------------------------------------#
# 1M NDP (Block) ACK
#  Enable 1M NDP ACK on 2/4MHz BW instead of 2M NDP ACK
#  Note: if enabled, throughput can be decreased on high MCS
ndp_ack_1m        = 0        # 0 (disable) or 1 (enable)
#--------------------------------------------------------------------------------#
# NDP Probe Request
#  For STA, "scan_ssid=1" in wpa_supplicant's conf should be set to use
ndp_preq          = 0        # 0 (Legacy Probe Req) 1 (NDP Probe Req)
#--------------------------------------------------------------------------------#
# CQM (Channel Quality Manager) (STA Only)
#  STA can disconnect according to Channel Quality (Beacon Loss or Poor Signal)
#  Note: if disabled, STA keeps connection regardless of Channel Quality
cqm_enable        = 1        # 0 (disable) or 1 (enable)
#--------------------------------------------------------------------------------#
# RELAY (Do NOT use! it will be deprecated)
relay_type        = 1        # 0 (wlan0: STA, wlan1: AP) 1 (wlan0: AP, wlan1: STA)
relay_nat         = 1        # 0 (not use NAT) 1 (use NAT - no need to add routing table)
#--------------------------------------------------------------------------------#
# Power Save (STA Only)
#  3-types PS: (0)Always on (2)Deep_Sleep(TIM) (3)Deep_Sleep(nonTIM)
#     TIM Mode : check beacons during PS to receive BU from AP
#  nonTIM Mode : Not check beacons during PS (just wake up by TX or EXT INT)
power_save        = 0        # STA (power save type 0~3)
ps_timeout        = '3s'     # STA (timeout before going to sleep) (min:1000ms)
sleep_duration    = '3s'     # STA (sleep duration only for nonTIM deepsleep) (min:1000ms)
listen_interval   = 1000     # STA (listen interval in BI unit) (max:65530)
                             # Listen Interval time should be less than bss_max_idle time to avoid association reject
#--------------------------------------------------------------------------------#
# Idle Mode (STA Only)
#  When The device is running, but idle, The device enters Deep_Sleep(nonTIM)
idle_mode = 0      # 0 (disable) or 1 (enable)
#--------------------------------------------------------------------------------#
# BSS MAX IDLE PERIOD (aka. keep alive) (AP Only)
#  STA should follow (i.e STA should send any frame before period),if enabled on AP
#  Period is in unit of 1000TU(1024ms, 1TU=1024us)
#  Note: if disabled, AP removes STAs' info only with explicit disconnection like deauth
bss_max_idle_enable = 1      # 0 (disable) or 1 (enable)
bss_max_idle        = 1800   # time interval (e.g. 1800: 1843.2 sec) (1 ~ 163,830,000)
#--------------------------------------------------------------------------------#
#  SW encryption/decryption (default HW offload)
sw_enc              = 0     # 0 (HW), 1 (SW), 2 (HYBRID: SW GTK HW PTK)
#--------------------------------------------------------------------------------#
# Mesh Options (Mesh Only)
#  Manual Peering & Static IP
peer                = 0     # 0 (disable) or Peer MAC Address
static_ip           = 0     # 0 (disable) or Static IP Address
batman              = 0     # 0 (disable) or 'bat0' (B.A.T.M.A.N routing protocol)
#--------------------------------------------------------------------------------#
# Self configuration (AP Only)
#  AP scans the clearest CH and then starts with it
self_config       = 0        # 0 (disable)  or 1 (enable)
prefer_bw         = 0        # 0: no preferred bandwidth, 1: 1M, 2: 2M, 4: 4M
dwell_time        = 100      # max dwell is 1000 (ms), min: 10ms, default: 100ms
#--------------------------------------------------------------------------------#
# Filter tx deauth frame for Multi Connection Test (STA Only) (Test only)
discard_deauth    = 0         # 1: discard TX deauth frame on STA
#--------------------------------------------------------------------------------#
# Use bitmap encoding for NDP (block) ack operation (NRC7292 only)
bitmap_encoding   = 1         # 0 (disable) or 1 (enable)
#--------------------------------------------------------------------------------#
# User scrambler reversely (NRC7292 only)
reverse_scrambler = 1         # 0 (disable) or 1 (enable)
#--------------------------------------------------------------------------------#
# Use bridge setup in br0 interface
use_bridge_setup  = 0         # AP & STA : 0 (not use bridge setup) or n (use bridge setup with eth(n-1))
                              # RELAY : 0 (not use bridge setup) or 1 (use bridge setup with wlan0,wlan1)
bridge_ip_mode = 1            # 0i (static ip) 1 (dhcp client) 2 (dhcp server)
#--------------------------------------------------------------------------------#
# Supported CH Width (STA Only)
support_ch_width  = 1         # 0 (1/2MHz Support) or 1 (1/2/4MHz Support)
#--------------------------------------------------------------------------------#
# Use Power save pretend operation for no response STA
power_save_pretend  = 0      # 0 (disable) or 1 (enable)
#--------------------------------------------------------------------------------#
# Duty cycle configuration
duty_cycle_enable = 0      # 0 (disable) or 1 (enable)
duty_cycle_window = 0
duty_cycle_duration = 0
#--------------------------------------------------------------------------------#
# CCA Threshold
cca_threshold = -75
#--------------------------------------------------------------------------------#
# TWT Wake Interval, Service Number, Service Period
# Two out of three values should be set, and the remaining value will be automatically calculated.
twt_int = 0					# 0 (disable) or Wake Interval (usec)
twt_num = 0					# 0 (disable) or Service Number
twt_sp = 0					# 0 (disable) or Service Period (usec)
twt_force_sleep = 0         # force sleep at the end of service
twt_num_in_group = 1        # Max STA Number in one Slot (default 1)
twt_algo = 0                # 0 (Balanced) or 1 (FCFS)
#--------------------------------------------------------------------------------#
# Restricted Access Window (RAW) configuration
raw = 0                     # (AP only) 0 (disable) or 1 (enable)
#--------------------------------------------------------------------------------#
# Use EEPROM for sysconfig & RFCAL
use_eeprom_config  = 0         # 0 (Flash Memory) or 1 (EEPROM)
#--------------------------------------------------------------------------------#
# Set sub-XTAL bypass
sub_xtal_bypass = 0         # 0 (External XTAL is used ) or 1 (Enable sub-XTAL bypass)
#--------------------------------------------------------------------------------#
# MCP Priority (Enable MCP priority control)
mcp_priority       = 0         # 0 (disabled) or 1 (enabled: suspend WLAN TX when MCP transmits)
##################################################################################

def parse_debug_level(level_str):
    """
    Parse debug level string to integer value
    ERR=0, WARN=1, INFO=2, DBG=3
    """
    level_map = {
        'ERR': 0, 'ERROR': 0,
        'WARN': 1, 'WARNING': 1,
        'INFO': 2,
        'DBG': 3, 'DEBUG': 3,
        'VBS': 4, 'VERBOSE': 4
    }
    level_upper = level_str.upper()
    if level_upper in level_map:
        return level_map[level_upper]
    # Try parsing as integer
    try:
        level = int(level_str)
        if 0 <= level <= 4:
            return level
    except ValueError:
        pass
    print("[!] WARNING: Invalid debug level '%s', using default" % level_str)
    return None

def parse_debug_mask(mask_str):
    """
    Parse debug mask string to bitmask value
    Examples: "PS|FW" -> 0x480, "ALL" -> 0xFFFFFFFF
    Supports: BASIC, HIF, WIM, TX, RX, MAC, CAPI, PS, STATS, STATE, BD, FW, AMPDU, CREDIT, SLOT, BUS, ALL
    """
    mask_map = {
        'BASIC': 0x1,      # BIT(0)
        'HIF': 0x2,        # BIT(1)
        'WIM': 0x4,        # BIT(2)
        'TX': 0x8,         # BIT(3)
        'RX': 0x10,        # BIT(4)
        'MAC': 0x20,       # BIT(5)
        'CAPI': 0x40,      # BIT(6)
        'PS': 0x80,        # BIT(7)
        'STATS': 0x100,    # BIT(8)
        'STATE': 0x200,    # BIT(9)
        'BD': 0x400,       # BIT(10)
        'FW': 0x800,       # BIT(11)
        'AMPDU': 0x1000,   # BIT(12)
        'CREDIT': 0x2000,  # BIT(13)
        'SLOT': 0x4000,    # BIT(14)
        'BUS': 0x8000,     # BIT(15)
        'ALL': 0xFFFFFFFF
    }

    # Try parsing as hex/int first
    if mask_str.startswith('0x') or mask_str.startswith('0X'):
        try:
            return int(mask_str, 16)
        except ValueError:
            pass
    else:
        try:
            return int(mask_str)
        except ValueError:
            pass

    # Parse as category names with | separator
    categories = [cat.strip().upper() for cat in mask_str.split('|')]
    mask_value = 0
    invalid_cats = []

    for cat in categories:
        if cat in mask_map:
            mask_value |= mask_map[cat]
        else:
            invalid_cats.append(cat)

    if invalid_cats:
        print("[!] WARNING: Invalid debug categories: %s" % ', '.join(invalid_cats))

    if mask_value == 0:
        print("[!] WARNING: No valid debug categories found in '%s', using default" % mask_str)
        return None

    return mask_value

def parse_debug_args():
    """
    Parse command line arguments to extract debug, power save and recovery parameters
    dbg=MASK automatically sets debug_level=DBG (3) and debug_mask=MASK
    Returns: positional_args
    """
    global debug_level_param, debug_mask_param, ps_param, idle_param, raw
    global recovery_param, load_method

    positional_args = []

    for arg in sys.argv[1:]:
        if arg.startswith('dbg=') or arg.startswith('debug='):
            # dbg=/debug= sets debug_level to DBG (3) automatically
            mask_str = arg.split('=', 1)[1]
            debug_mask_param = parse_debug_mask(mask_str)
            if debug_mask_param is not None:
                debug_level_param = 3  # DBG level
                print("[*] Debug enabled: level=DBG(3), mask=0x%X (%s)" % (debug_mask_param, mask_str))
            else:
                print("[!] Invalid debug mask: %s" % mask_str)
        elif arg.startswith('vbs=') or arg.startswith('verbose='):
            # vbs=/verbose= sets debug_level to VBS (4) automatically
            mask_str = arg.split('=', 1)[1]
            debug_mask_param = parse_debug_mask(mask_str)
            if debug_mask_param is not None:
                debug_level_param = 4  # VBS level
                print("[*] Debug enabled: level=VBS(4), mask=0x%X (%s)" % (debug_mask_param, mask_str))
            else:
                print("[!] Invalid debug mask: %s" % mask_str)
        elif arg.startswith('ps='):
            ps_str = arg.split('=', 1)[1]
            try:
                ps_param = int(ps_str)
                print("[*] Power save set to %d" % ps_param)
            except ValueError:
                print("[!] Invalid ps value: %s (must be integer)" % ps_str)
        elif arg.startswith('idle='):
            idle_str = arg.split('=', 1)[1]
            try:
                idle_param = int(idle_str)
                print("[*] Idle mode set to %d" % idle_param)
            except ValueError:
                print("[!] Invalid idle value: %s (must be integer)" % idle_str)
        elif arg.startswith('recovery='):
            rec_str = arg.split('=', 1)[1]
            try:
                recovery_param = int(rec_str)
                print("[*] Recovery daemon set to %d" % recovery_param)
            except ValueError:
                print("[!] Invalid recovery value: %s (must be 0 or 1)" % rec_str)
        elif arg.startswith('raw='):
            raw_str = arg.split('=', 1)[1]
            try:
                raw = int(raw_str)
                print("[*] RAW set to %d" % raw)
            except ValueError:
                print("[!] Invalid raw value: %s (must be 0 or 1)" % raw_str)
        elif arg.startswith('debug_level='):
            lvl_str = arg.split('=', 1)[1]
            try:
                debug_level_param = int(lvl_str)
                print("[*] Debug level set to %d" % debug_level_param)
            except ValueError:
                print("[!] Invalid debug_level value: %s (must be 0-4)" % lvl_str)
        elif arg.startswith('debug_mask='):
            mask_str = arg.split('=', 1)[1]
            debug_mask_param = parse_debug_mask(mask_str)
            if debug_mask_param is not None:
                print("[*] Debug mask set to 0x%X (%s)" % (debug_mask_param, mask_str))
            else:
                print("[!] Invalid debug_mask value: %s" % mask_str)
        elif arg.startswith('load='):
            load_str = arg.split('=', 1)[1].lower()
            if load_str in ('modprobe', 'insmod'):
                load_method = load_str
                print("[*] Module load method set to '%s'" % load_method)
            else:
                print("[!] Invalid load value: %s (must be 'modprobe' or 'insmod')" % load_str)
                print("[!] Using default: modprobe")
        else:
            positional_args.append(arg)

    return positional_args

def check(interface):
    if int(use_bridge_setup) > 0 and int(bridge_ip_mode) == 1:
        os.system('sudo dhclient ' + interface +' -nw -v')

    ifconfig_cmd = "ifconfig " + interface
    ifconfig_process = subprocess.Popen(ifconfig_cmd.split(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    ifconfig_lines = ifconfig_process.communicate()[0]

    try:
        ifconfig_lines = ifconfig_lines.decode()
    except:
        pass

    ifconfig_lines = ifconfig_lines.split("\n")
    for line in ifconfig_lines:
        # if "inet 192.168" in line:
        if "inet " in line and "127.0.0.1" not in line and "inet6" not in line:
            return line
    return ''

def usage_print():
    print("Usage: \n\tstart_modular.py [sta_type] [security_mode] [country] [channel] [sniffer_mode] [dbg=MASK] [ps=VALUE] [idle=VALUE] [raw=VALUE] \
            \n\tstart_modular.py [sta_type] [security_mode] [country] [mesh_mode] [mesh_peering] [mesh_ip] [dbg=MASK] [ps=VALUE] [idle=VALUE] [raw=VALUE]")
    print("Argument:    \n\tsta_type      [0:STA   |  1:AP  |  2:SNIFFER  | 3:RELAY |  4:MESH] \
            \n\tsecurity_mode [0:Open  |  1:WPA2-PSK  |  2:WPA3-OWE  |  3:WPA3-SAE | 4:WPS-PBC] \
                         \n\tcountry       [US:USA  |  JP:Japan  |  TW:Taiwan  |  AU:Australia  |  NZ:New Zealand  | \
                         \n\t               K1:Korea-USN  |  K2:Korea-MIC  |  SG:Singapore | T2: Taiwan-nCC \
                         \n\t               and EU channel support countries(EU countries, GB and SA)] \
                         \n\t----------------------------------------------------------- \
                         \n\tchannel       [S1G Channel Number]   * Only for Sniffer & AP \
                         \n\tsniffer_mode  [0:Local | 1:Remote]   * Only for Sniffer \
                         \n\tmesh_mode     [0:MPP | 1:MP | 2:MAP] * Only for Mesh \
                         \n\tmesh_peering  [Peer MAC address]     * Only for Mesh \
                         \n\tmesh_ip       [Static IP address]    * Only for Mesh \
                         \n\t----------------------------------------------------------- \
                         \n\tdbg/debug     [BASIC|HIF|WIM|TX|RX|MAC|CAPI|PS|STATS|STATE|BD|FW|AMPDU|CREDIT|SLOT|BUS|ALL or hex] * Optional (auto sets level=DBG) \
                         \n\tvbs/verbose    [same categories as dbg] * Optional (auto sets level=VBS - verbose/trace) \
                         \n\tps            [0-3: Power save type] * Optional \
                         \n\tidle          [0:Disable | 1:Enable idle mode] * Optional \
                         \n\traw           [0:Disable | 1:Enable RAW (AP only)] * Optional \
                         \n\trecovery      [0:Disable | 1:Enable recovery daemon] * Optional \
                         \n\tload          [modprobe (default) | insmod] * Optional (module loading method)")
    print("Example:  \n\tOPEN mode STA for US                : ./start_modular.py 0 0 US \
                      \n\tSecurity mode AP for US                : ./start_modular.py 1 1 US \
                      \n\tLocal Sniffer mode on CH 40 for Japan  : ./start_modular.py 2 0 JP 40 0 \
                      \n\tSAE mode Mesh AP for US                : ./start_modular.py 4 3 US 2 \
                      \n\tMesh Point with static ip              : ./start_modular.py 4 3 US 1 192.168.222.1 \
                      \n\tMesh Point with manual peering         : ./start_modular.py 4 3 US 1 8c:0f:fa:00:29:46 \
                      \n\tMesh Point with manual peering & ip    : ./start_modular.py 4 3 US 1 8c:0f:fa:00:29:46 192.168.222.1 \
                      \n\t----------------------------------------------------------- \
                      \n\tWith optional parameters: \
                      \n\tSTA with PS and FW debug               : ./start_modular.py 0 0 US \"dbg=PS|FW\" \
                      \n\tSTA with verbose BUS trace             : ./start_modular.py 0 0 US \"vbs=BUS\" \
                      \n\tSTA with verbose TX+PS trace           : ./start_modular.py 0 0 US \"vbs=TX|PS\" \
                      \n\tAP with all debug messages             : ./start_modular.py 1 1 US dbg=ALL \
                      \n\tSTA with hex debug mask                : ./start_modular.py 0 0 US dbg=0x480 \
                      \n\tSTA with power save type 2             : ./start_modular.py 0 0 US ps=2 \
                      \n\tSTA with power save and idle mode      : ./start_modular.py 0 0 US ps=2 idle=1 \
                      \n\tSTA with all optional params           : ./start_modular.py 0 0 US ps=2 idle=1 \"dbg=PS|FW\" \
                      \n\tSTA with recovery daemon               : ./start_modular.py 0 1 US recovery=1 \
                      \n\t----------------------------------------------------------- \
                      \n\tModule loading method: \
                      \n\tSTA using modprobe (default)            : ./start_modular.py 0 0 US \
                      \n\tSTA using insmod (legacy)               : ./start_modular.py 0 0 US load=insmod")
    print("Note: \n\tsniffer_mode should be set as '1' when running sniffer on remote terminal \
                  \n\tMPP, MP mode support only Open, WPA3-SAE security mode \
                  \n\tOptional parameters (dbg/debug, vbs/verbose, ps, idle) can be placed anywhere in the command line \
                  \n\tUse quotes around dbg/vbs when using pipe operator: \"dbg=PS|FW\" or \"vbs=TX|PS\" \
                  \n\tdbg=/debug= sets debug level to DBG(3), vbs=/verbose= sets level to VBS(4) - verbose/trace \
                  \n\tload=modprobe (default): loads modules via modprobe from system path (requires 'make install') \
                  \n\tload=insmod: loads modules via insmod from nrc_pkg/sw/driver/ (legacy behavior)")
    exit()

def strSTA():
    if int(sys.argv[1]) == 0:
        return 'STA'
    elif int(sys.argv[1]) == 1:
        return 'AP'
    elif int(sys.argv[1]) == 2:
        return 'SNIFFER'
    elif int(sys.argv[1]) == 3:
        return 'RELAY'
    elif int(sys.argv[1]) == 4:
        return 'MESH'
    else:
        usage_print()

def checkEUCountry():
    if str(sys.argv[3]) in eu_ch_support_country_list:
        return True

def checkCountry():
    if checkEUCountry() or str(sys.argv[3]) in s1g_ch_support_country_list:
        return
    else:
        usage_print()

def checkMeshUsage():
    global sw_enc,relay_type, peer, static_ip
    if len(sys.argv) < 5:
        usage_print()
    # Use sw_enc=2 with Hybrid Security
    # sw_enc=2
    relay_type = int(sys.argv[4])
    if len(sys.argv) == 6:
        if isMacAddress(sys.argv[5]):
            peer = sys.argv[5]
        elif isIP(sys.argv[5]):
            static_ip = sys.argv[5]
        elif sys.argv[5] == 'nodhcp':
            static_ip = 'nodhcp'
        else:
            usage_print()
    elif len(sys.argv) == 7:
        if isMacAddress(sys.argv[5]):
            peer = sys.argv[5]
        else:
            usage_print()
        if isIP(sys.argv[6]):
            static_ip = sys.argv[6]
        elif sys.argv[6] == 'nodhcp':
            static_ip = 'nodhcp'
        else:
            usage_print()
    argv_print()

def checkParamValidity():
    if strSTA() == 'STA' and int(power_save) > 0 and int(listen_interval) > 65535:
        print("Max listen_interval is 65535!")
        exit()

def strSecurity():
    if int(sys.argv[2]) == 0:
        return 'OPEN'
    elif int(sys.argv[2]) == 1:
        return 'WPA2-PSK'
    elif int(sys.argv[2]) == 2:
        return 'WPA3-OWE'
    elif int(sys.argv[2]) == 3:
        return 'WPA3-SAE'
    elif int(sys.argv[2]) == 4:
        return 'WPA-PBC'
    else:
        usage_print()

def strPSType():
    if int(power_save) == 0:
        return 'Always On'
    elif int(power_save) == 2:
        return 'Deep Sleep (TIM)'
    elif int(power_save) == 3:
        return 'Deep Sleep (nonTIM)'
    else:
        return 'Invalid Type'

def strSnifferMode():
    if int(sys.argv[5]) == 0:
        return 'LOCAL'
    elif int(sys.argv[5]) == 1:
        return 'REMOTE'
    else:
        usage_print()

def strOnOff(param):
    if int(param) == 1:
        return 'ON'
    else:
        return 'OFF'

def strAMPDUMode(param):
    if int(param) == 0:
        return 'OFF'
    elif int(param) == 1:
        return 'MANUAL'
    else:
        return 'AUTO'

def strMeshMode():
    if int(sys.argv[4]) == 0:
        return 'Mesh Portal'
    elif int(sys.argv[4]) == 1:
        return 'Mesh Point'
    elif int(sys.argv[4]) == 2:
        return 'Mesh AP'
    else:
        usage_print()

def strOriCountry():
    if str(sys.argv[3]) == 'K1' or str(sys.argv[3]) == 'K2':
        return 'KR'
    elif str(sys.argv[3]) == 'T2':
        return 'TW'
    else:
        return str(sys.argv[3])

def isNumber(s):
    try:
        float(s)
        return True
    except ValueError:
        return False

def isMacAddress(s):
    if len(s) == 17 and ':' in s:
        return True
    else:
        return False

def isIP(s):
    if len(s) > 6 and len(s) < 16 and '.' in s:
        return True
    else:
        return False

def strBDName():
    if str(bd_name):
        return str(bd_name)
    else:
        return 'nrc' + str(model) + '_bd.dat'

def configure_power_save_gpio(use_eeprom_config):
    # Check kernel version
    import platform
    kernel_version = platform.release()
    # Parse major and minor version as integers for proper comparison
    version_parts = kernel_version.split('.')
    major = int(version_parts[0])
    minor = int(version_parts[1])

    # Compare as tuple (6, 12) >= (6, 6) = True
    is_kernel_6_6_or_later = (major, minor) >= (6, 6)

    if int(use_eeprom_config) == 1:
        if is_kernel_6_6_or_later:
            #7394 STA kernel 6.6+ (host_gpio_out(527) --> target_gpio_in(14))
            ps_gpio_arg = " power_save_gpio=527,14,1"
        else:
            #7394 STA using EEPROM B/D (host_gpio_out(15) --> target_gpio_in(12))
            ps_gpio_arg = " power_save_gpio=15,12,1"
    else:
        if is_kernel_6_6_or_later:
            #7394 STA kernel 6.6+ (host_gpio_out(529) --> target_gpio_in(14))
            ps_gpio_arg = " power_save_gpio=529,14,1"
        else:
            #7394 STA (host_gpio_out(17) --> target_gpio_in(14))
            ps_gpio_arg = " power_save_gpio=17,14,1"

    return ps_gpio_arg

def argv_print():
    print("------------------------------")
    print("Model            : " + str(model))
    print("STA Type         : " + strSTA())
    print("Country          : " + str(sys.argv[3]))
    print("Security Mode    : " + strSecurity())
    print("BD Name          : " + strBDName())
    print("AMPDU            : " + strAMPDUMode(ampdu_enable))
    if strSTA() == 'STA':
        print("CQM              : " + strOnOff(cqm_enable))
    if strSTA() == 'SNIFFER':
        print("Channel Selected : " + str(sys.argv[4]))
        print("Sniffer Mode     : " + strSnifferMode())
    if int(fw_download) == 1:
        print("Download FW      : " + fw_name)
    if int(auto_fw_update) == 1:
        print("Update FW        : " + fw_update_name)
    print ("MAX TX Power     : " + str(max_txpwr) + " dBm")
    if int(bss_max_idle_enable) == 1 :
        if strSTA() == 'AP' or strSTA() == 'RELAY' or strSTA() == 'STA':
            print("BSS MAX IDLE     : " + str(bss_max_idle))
    if strSTA() == 'STA':
        print("Power Save Type  : " + strPSType())
        if int(power_save) > 0:
            print("PS Timeout       : " + ps_timeout)
        if int(power_save) == 3:
            print("Sleep Duration   : " + sleep_duration)
        if int(listen_interval) > 0:
            print("Listen Interval  : " + str(listen_interval))
        if int(idle_mode) == 1:
            print("Idle Mode        : Enabled")
    if strSTA() == 'MESH':
        print("Mesh Mode        : " + strMeshMode())
    if int(use_eeprom_config) == 1:
        print("CONFIG_LOCATION  : EEPROM")
    else:
        print("CONFIG_LOCATION  : FLASH")
    # Print debug parameters if specified (dbg= sets both level and mask)
    if debug_level_param is not None or debug_mask_param is not None:
        if debug_mask_param is not None:
            print("Debug (dbg=)     : level=DBG, mask=0x%X" % debug_mask_param)
        else:
            level_names = {0: 'ERR', 1: 'WARN', 2: 'INFO', 3: 'DBG'}
            print("Debug Level      : " + str(debug_level_param) + " (" + level_names.get(debug_level_param, 'UNKNOWN') + ")")
    # Print power save parameters if overridden from command line
    if ps_param is not None:
        print("Power Save (CLI) : " + str(ps_param))
    if idle_param is not None:
        print("Idle Mode (CLI)  : " + str(idle_param))
    if recovery_param > 0:
        print("Recovery Daemon  : Enabled")
    print("Load Method      : " + load_method)
    print("------------------------------")

def copyConf():
    copy_script = NRC_PKG_PATH + "/sw/firmware/copy.sh"
    if ensure_script_executable(copy_script):
        os.system("sudo " + copy_script + " " + str(model) + " " + strBDName() + " " + str(use_eeprom_config))
    else:
        print("[!] Skipping firmware copy (script not available)")

    ip_config_script = NRC_PKG_PATH + "/script/conf/etc/ip_config.sh"
    if ensure_script_executable(ip_config_script):
        os.system("sudo " + ip_config_script + " " + strSTA() + " " +  str(relay_type) + " " + str(static_ip) + " " + str(batman))
    else:
        print("[!] Skipping IP config (script not available)")

    if int(use_bridge_setup) > 0:
        bridge_script = NRC_PKG_PATH + "/script/conf/etc/ip_config_bridge.sh"
        if ensure_script_executable(bridge_script):
            os.system("sudo " + bridge_script + " " + strSTA() + " " + str(use_bridge_setup - 1) + " " + str(bridge_ip_mode))
        else:
            print("[!] Skipping bridge config (script not available)")

# ---------------------------------------------------------------------------
# Module loading helpers (modprobe / insmod)
# ---------------------------------------------------------------------------

def get_system_module_dir():
    """Return the system module directory for NRC modules."""
    import platform
    kernel_version = platform.release()
    return "/lib/modules/%s/extra/nrc" % kernel_version

def check_nrc_pkg_modules():
    """
    Check if .ko files exist in nrc_pkg/sw/driver/.
    Returns list of (filename, full_path) for found .ko files.
    """
    driver_dir = NRC_PKG_PATH + "/sw/driver"
    ko_files = [
        ("nrc_spi.ko", os.path.join(driver_dir, "nrc_spi.ko")),
        ("nrc_core.ko", os.path.join(driver_dir, "nrc_core.ko")),
        ("nrc_wlan.ko", os.path.join(driver_dir, "nrc_wlan.ko")),
        ("nrc-mcp.ko", os.path.join(driver_dir, "nrc-mcp.ko")),
    ]
    found = []
    for name, path in ko_files:
        if os.path.isfile(path):
            found.append((name, path))
    return found

def _modules_need_sync(pkg_modules, mod_dir):
    """
    Compare .ko files in nrc_pkg/sw/driver/ with system module path.
    Returns True if any file is missing or has a different size/mtime,
    meaning sync + depmod is required.
    """
    for name, src_path in pkg_modules:
        dst_path = os.path.join(mod_dir, name)
        if not os.path.isfile(dst_path):
            return True
        try:
            src_stat = os.stat(src_path)
            dst_stat = os.stat(dst_path)
            if src_stat.st_size != dst_stat.st_size:
                return True
            if src_stat.st_mtime > dst_stat.st_mtime:
                return True
        except OSError:
            return True
    return False

def sync_modules_to_system():
    """
    If .ko files exist in nrc_pkg/sw/driver/, copy them to the system
    module directory and run depmod.  This keeps the existing build→deploy
    workflow (remote-build.sh → nrc_pkg/sw/driver/) compatible with modprobe.

    Skips copy + depmod if the system files are already up-to-date
    (same size and not older than nrc_pkg copies).

    Returns True if modules are available in the system path after sync.
    """
    pkg_modules = check_nrc_pkg_modules()
    if not pkg_modules:
        return check_system_modules_installed()

    mod_dir = get_system_module_dir()

    if not _modules_need_sync(pkg_modules, mod_dir):
        ret = os.system("sudo modprobe --dry-run --quiet nrc_spi 2>/dev/null")
        if ret == 0:
            print("[*] System modules are up-to-date — skipping sync/depmod")
            return True

    print("[*] Found .ko files in %s/sw/driver/ — syncing to system path" % NRC_PKG_PATH)

    os.system("sudo mkdir -p %s" % mod_dir)

    for name, src_path in pkg_modules:
        dst_path = os.path.join(mod_dir, name)
        ret = os.system("sudo cp %s %s" % (src_path, dst_path))
        if ret == 0:
            print("[*]   %s -> %s" % (name, mod_dir))
        else:
            print("[!]   Failed to copy %s" % name)
            return False

    print("[*] Running depmod...")
    ret = os.system("sudo depmod -a")
    if ret != 0:
        print("[!] WARNING: depmod failed")
        return False

    print("[*] System modules synced and depmod completed")
    return True

def check_system_modules_installed():
    """
    Check if NRC modules are installed in the system module path.
    Returns True if at least the three core modules are present.
    """
    mod_dir = get_system_module_dir()
    required = ["nrc_spi.ko", "nrc_core.ko", "nrc_wlan.ko"]
    if not os.path.isdir(mod_dir):
        return False
    for ko in required:
        if not os.path.isfile(os.path.join(mod_dir, ko)):
            return False
    return True

def ensure_depmod():
    """
    Ensure modules are available for modprobe.
    1. If .ko files exist in nrc_pkg/sw/driver/, sync them to system path
    2. Otherwise check system path directly
    3. Run depmod if needed
    Returns True if modprobe can resolve modules after this call.
    """
    # First, sync from nrc_pkg if .ko files are present there
    if check_nrc_pkg_modules():
        return sync_modules_to_system()

    # No .ko in nrc_pkg — check if already installed in system
    ret = os.system("sudo modprobe --dry-run --quiet nrc_spi 2>/dev/null")
    if ret == 0:
        return True

    # Modules installed but depmod not run?
    if check_system_modules_installed():
        print("[*] Running depmod to update module dependencies...")
        ret = os.system("sudo depmod -a")
        if ret != 0:
            print("[!] WARNING: depmod failed (exit code %d)" % ret)
            return False
        ret = os.system("sudo modprobe --dry-run --quiet nrc_spi 2>/dev/null")
        if ret == 0:
            print("[*] depmod completed — modules are now available via modprobe")
            return True
        else:
            print("[!] WARNING: depmod ran but modprobe still cannot resolve modules")
            return False

    # Modules not installed at all
    return False

def load_module(module_name, ko_path, params):
    """
    Load a kernel module using the configured method (modprobe or insmod).

    Args:
        module_name: Module name for modprobe (e.g. 'nrc_spi')
        ko_path:     Full path to .ko file for insmod (e.g. '/path/nrc_spi.ko')
        params:      Module parameter string (e.g. 'hifspeed=20000000 ...')

    Returns:
        0 on success, non-zero on failure.
    """
    if load_method == 'modprobe':
        cmd = "sudo modprobe %s %s" % (module_name, params)
    else:
        cmd = "sudo insmod %s %s" % (ko_path, params)
    print(cmd)
    return os.system(cmd)

MODPROBE_CONF_DIR = "/etc/modprobe.d"

def _write_modprobe_conf(module_name, params):
    """
    Write a per-module conf file to /etc/modprobe.d/<module_name>.conf.
    Called every time start_modular.py runs so that parameter changes
    are always reflected.  The file is overwritten (not appended).
    """
    conf_path = "%s/%s.conf" % (MODPROBE_CONF_DIR, module_name)
    stripped = params.strip() if params else ""
    header = (
        "# NRC7394 Modular Driver — %s parameters\n"
        "# Auto-generated by start_modular.py — regenerated on every run\n"
    ) % module_name
    if stripped:
        content = header + "options %s %s\n" % (module_name, stripped)
    else:
        content = header + "# (no parameters)\n"

    import tempfile
    try:
        fd, tmp_path = tempfile.mkstemp(suffix='.conf', prefix=module_name + '_')
        with os.fdopen(fd, 'w') as f:
            f.write(content)
        ret = os.system("sudo cp %s %s && sudo chmod 644 %s" % (
            tmp_path, conf_path, conf_path))
        os.remove(tmp_path)
        if ret == 0:
            print("[*] %s written" % conf_path)
        else:
            print("[!] WARNING: Failed to write %s" % conf_path)
    except Exception as e:
        print("[!] WARNING: Failed to generate %s: %s" % (conf_path, str(e)))

def generate_modprobe_confs(spi_param, core_param, wlan_param):
    """
    Generate per-module conf files under /etc/modprobe.d/.
    Each file is overwritten so that the latest parameters from
    start_modular.py are always used by modprobe.

    Files created:
      /etc/modprobe.d/nrc_spi.conf
      /etc/modprobe.d/nrc_core.conf
      /etc/modprobe.d/nrc_wlan.conf
    """
    print("[*] Generating modprobe configuration files...")
    _write_modprobe_conf("nrc_spi", spi_param)
    _write_modprobe_conf("nrc_core", core_param)
    _write_modprobe_conf("nrc_wlan", wlan_param)

def generate_mcp_modprobe_conf(mcp_param):
    """Generate /etc/modprobe.d/nrc_mcp.conf for MCP module."""
    _write_modprobe_conf("nrc_mcp", mcp_param)

def startNAT():
    os.system('sudo sh -c "echo 1 > /proc/sys/net/ipv4/ip_forward"')
    if strSTA() == 'AP':
        os.system("sudo iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE")
        os.system("sudo iptables -A FORWARD -i eth0 -o wlan0 -m state --state RELATED,ESTABLISHED -j ACCEPT")
        os.system("sudo iptables -A FORWARD -i wlan0 -o eth0 -j ACCEPT")
    elif strSTA() == 'RELAY' and int(relay_nat) == 1:
        if int(relay_type) == 1:
            os.system("sudo iptables -t nat -A POSTROUTING -o wlan1 -j MASQUERADE")
            os.system("sudo iptables -A FORWARD -i wlan0 -o wlan1 -m state --state RELATED,ESTABLISHED -j ACCEPT")
            os.system("sudo iptables -A FORWARD -i wlan1 -o wlan0 -j ACCEPT")
        elif int(relay_type) == 0:
            os.system("sudo iptables -t nat -A POSTROUTING -o wlan0 -j MASQUERADE")
            os.system("sudo iptables -A FORWARD -i wlan1 -o wlan0 -m state --state RELATED,ESTABLISHED -j ACCEPT")
            os.system("sudo iptables -A FORWARD -i wlan0 -o wlan1 -j ACCEPT")
    else:
        print("fail to start NAT")

def stopNAT():
    os.system('sudo sh -c "echo 0 > /proc/sys/net/ipv4/ip_forward"')
    os.system("sudo iptables -t nat --flush")
    os.system("sudo iptables --flush")

def configure_ap_network(interface, static_ip="192.168.200.1", dhcp_range_start="192.168.200.10", dhcp_range_end="192.168.200.50"):
    """
    Configure network for AP mode (DHCP server + static IP)
    Works on Ubuntu/Debian systems
    """
    print("[*] Configuring AP network on %s" % interface)

    # Only unmanage the specific interface (wlan0), don't stop system-wide services
    # This prevents eth0 from losing its IP address
    os.system("sudo nmcli dev set %s managed no 2>/dev/null" % interface)
    os.system("sudo systemctl stop dnsmasq 2>/dev/null")

    # Set static IP on interface
    print("[*] Setting static IP %s/24 on %s" % (static_ip, interface))
    os.system("sudo ip addr flush dev %s" % interface)
    os.system("sudo ip addr add %s/24 dev %s" % (static_ip, interface))
    os.system("sudo ip link set %s up" % interface)

    # Configure dnsmasq for DHCP server
    dnsmasq_conf = "/tmp/dnsmasq_%s.conf" % interface
    with open(dnsmasq_conf, 'w') as f:
        f.write("interface=%s\n" % interface)
        f.write("bind-interfaces\n")
        f.write("server=8.8.8.8\n")
        f.write("domain-needed\n")
        f.write("bogus-priv\n")
        f.write("dhcp-range=%s,%s,255.255.255.0,24h\n" % (dhcp_range_start, dhcp_range_end))

    # Start dnsmasq with custom config
    print("[*] Starting dnsmasq DHCP server...")
    os.system("sudo dnsmasq -C %s --pid-file=/tmp/dnsmasq_%s.pid" % (dnsmasq_conf, interface))
    time.sleep(1)

    print("[*] AP network configured: %s/24" % static_ip)
    print("[*] DHCP range: %s - %s" % (dhcp_range_start, dhcp_range_end))

def configure_dhcpcd_static_ip(interface, static_ip="192.168.200.1"):
    """
    Alternative method: Configure dhcpcd with static IP
    Useful for systems that use dhcpcd as primary network manager
    """
    dhcpcd_conf = "/etc/dhcpcd.conf"
    interface_config = """
# Static IP for %s (AP mode)
interface %s
static ip_address=%s/24
""" % (interface, interface, static_ip)

    # Check if configuration already exists
    try:
        with open(dhcpcd_conf, 'r') as f:
            content = f.read()
            if "interface %s" % interface in content:
                print("[*] dhcpcd.conf already has %s configuration" % interface)
                return
    except:
        pass

    # Append configuration
    print("[*] Adding static IP configuration to %s" % dhcpcd_conf)
    os.system("echo '%s' | sudo tee -a %s" % (interface_config, dhcpcd_conf))

def startDHCPCD():
    # Use -b to background immediately so dhcpcd persists as a daemon
    # even when carrier is not yet available (wpa_supplicant starts later).
    # Without -b, dhcpcd exits(1) on timeout if no carrier → no DHCP after
    # wpa_supplicant connects. This is critical for recovery restarts where
    # the systemd dhcpcd.service was stopped by stop_modular.py.
    os.system("sudo dhcpcd -b wlan0")

def stopDHCPCD():
    os.system("sudo dhcpcd -k wlan0 2>/dev/null")

def startDNSMASQ():
    # Check if dnsmasq is already running (either as service or standalone)
    ret = os.system("pgrep -x dnsmasq > /dev/null 2>&1")
    if ret == 0:
        print("[*] dnsmasq is already running, skipping systemctl restart")
        return
    os.system("sudo systemctl restart dnsmasq")

def stopDNSMASQ():
    os.system("sudo systemctl stop dnsmasq")
    # Also kill any temporary dnsmasq instances
    os.system("sudo pkill -f 'dnsmasq -C /tmp/dnsmasq_wlan'")
    os.system("sudo rm -f /tmp/dnsmasq_wlan*.conf /tmp/dnsmasq_wlan*.pid 2>/dev/null")

def addWLANInterface(interface):
    if strSTA() == 'RELAY' and interface == "wlan1":
        print("[*] Create wlan1 for concurrent mode")
        os.system("sudo iw dev wlan0 interface add wlan1 type managed")
        os.system("sudo ifconfig wlan1 up")
        time.sleep(3)

def self_config_check():
    country = str(sys.argv[3])
    if checkEUCountry():
        country = "EU"
    conf_path = script_path + "conf/" + country
    conf_temp = script_path + "conf/temp_self_config.conf"
    conf_file = ""
    orig_channel = ""
    global dwell_time

    if strSecurity() == "OPEN" :
        conf_file+="/ap_halow_open.conf"
    elif strSecurity() == 'WPA2-PSK' :
        conf_file+="/ap_halow_wpa2.conf"
    elif strSecurity() == 'WPA3-OWE' :
        conf_file+="/ap_halow_owe.conf"
    elif strSecurity() == 'WPA3-SAE' :
        conf_file+="/ap_halow_sae.conf"
    elif strSecurity() == 'WPA-PBC' :
        conf_file+="/ap_halow_pbc.conf"

    print("country: " + country + ", prefer_bw: " + str(prefer_bw) + ", dwell_time: " + str(dwell_time))

    self_conf_cmd = script_path + 'cli_app show self_config ' + country + ' ' + str(prefer_bw) + ' ' + str(dwell_time) + ' '
    if int(dwell_time) > 1000:
        dwell_time = 1000
    elif int(dwell_time) < 10:
        dwell_time = 10
    # Max num of 1M channel is 26
    checkout_timeout = int(dwell_time)*26
    try:
        print("Start CCA scan.... It will take up to " + str(checkout_timeout/1000) + " sec to complete")
        result = subprocess.check_output('timeout ' + str(checkout_timeout) + ' ' + self_conf_cmd, shell=True)
        result = result.decode()
    except:
        sys.exit("[self_configuration] No return best channel within " + str(checkout_timeout/1000) + " seconds")

    if 'no_self_conf' in result:
        print("Target FW does NOT support self configuration. Please check FW")
        return 'Fail'
    else:
        print(result)
        best_channel = re.split(r'[:,\s,\t,\n]+', result)[-3]
        os.system("sudo cp " + conf_path + conf_file + " " + conf_temp)
        os.system("sed -i '/channel=/d' " + conf_temp)
        os.system("sed -i '/hw_mode=/d' " + conf_temp)
        os.system("sed -i '/#ssid=/d' " + conf_temp)
        if int(best_channel) < 36:
            os.system('sed -i "/ssid=.*/ahw_mode=' + 'g' +'" ' + conf_temp)
        else:
            os.system('sed -i "/ssid=.*/ahw_mode=' + 'a' +'" ' + conf_temp)
        os.system('sed -i "/hw_mode=.*/achannel=' + best_channel +'" ' + conf_temp)
        os.system("sed -i \"s/^country_code=.*/country_code=%s/g\" %s" % (strOriCountry(), conf_temp))
        print("Start with channel: " + best_channel)
        return 'Done'

def ft232h_usb():
    # Re-define SPI parameters for ft232h_usb_spi
    # ft232h_usb_spi
    global spi_clock, spi_bus_num, spi_gpio_irq, spi_cs_num, spi_polling_interval
    print("[*] use ft232h_usb_spi")
    spi_bus_num = 3
    spi_gpio_irq = 500
    if int(spi_clock) > 15000000:
        spi_clock = 15000000
    if int(spi_cs_num) != 0:
        spi_cs_num = 0
    if int(spi_polling_interval) <= 0:
        spi_polling_interval = 50
    if int(ft232h_usb_spi) != 1:
        spi_gpio_irq = -1

def setAPParam():
    # Re-define parameters for AP mode
    global ndp_preq
    ndp_preq=1

def setRelayParam():
    # Re-define parameters for RELAY mode
    global sw_enc, power_save, ndp_ack_1m, ndp_preq
    power_save=0; ndp_ack_1m=0; ndp_preq=0;
    # Use sw_enc=2 with Hybrid Security
    # sw_enc=2

def setSnifferParam():
    # Re-define parameters for Sniffer mode
    global sw_enc, ampdu_enable, bss_max_idle_enable, power_save, ndp_ack_1m, listen_interval
    sw_enc=0; ampdu_enable=0; bss_max_idle_enable=0; power_save=0; ndp_ack_1m=0; listen_interval=0;

def setMeshParam():
    # Re-define parameters for MESH mode
    global short_bcn_enable
    short_bcn_enable=0

def setSPIModuleParam():
    """
    Set module parameters for SPI backend module (nrc_spi.ko)
    Only SPI-specific parameters are included here
    """
    # Check ft232h_usb_spi
    if int(ft232h_usb_spi) > 0:
        ft232h_usb()

    # Configure power save GPIO for SPI module
    ps_gpio_arg = ""
    if strSTA() == 'STA' and (int(power_save) > 0 or int(idle_mode) == 1):
        if str(model) == "7394":
            ps_gpio_arg = configure_power_save_gpio(use_eeprom_config)

    # module param for spi setting
    # default:
    #  hifspeed(20000000) spi_bus_num(0) spi_cs_num(0) spi_gpio_irq(5) spi_polling_interval(0)
    spi_module_param = " hifspeed=" + str(spi_clock) + \
                      " spi_bus_num=" + str(spi_bus_num) + \
                      " spi_cs_num=" + str(spi_cs_num) + \
                      " spi_gpio_irq=" + str(spi_gpio_irq) + \
                      " spi_polling_interval=" + str(spi_polling_interval) + \
                      ps_gpio_arg

    # Add debug parameters if specified
    if debug_level_param is not None:
        spi_module_param += " debug_level=" + str(debug_level_param)
    if debug_mask_param is not None:
        spi_module_param += " debug_mask=" + str(debug_mask_param)

    return spi_module_param

def setCoreModuleParam():
    """
    Set module parameters for HAL core module (nrc_core.ko)
    Hardware interface parameters only
    """
    # HAL core module currently has no specific parameters
    # Board data is managed by WLAN module
    core_module_param = ""

    # Add debug parameters if specified
    if debug_level_param is not None:
        core_module_param += " debug_level=" + str(debug_level_param)
    if debug_mask_param is not None:
        core_module_param += " debug_mask=" + str(debug_mask_param)

    return core_module_param

def setWLANModuleParam():
    """
    Set module parameters for WLAN frontend module (nrc_wlan.ko)
    All WLAN-specific, power management, and feature parameters
    """
    # Initialize arguments for WLAN module params
    fw_arg = fw_update_arg = power_save_arg = sleep_duration_arg = idle_mode_arg = \
    bss_max_idle_arg = ndp_preq_arg = ndp_ack_1m_arg = auto_ba_arg =\
    sw_enc_arg =  cqm_arg = listen_int_arg = drv_dbg_arg = sched_scan_arg = tw_band_arg = \
    sbi_arg = discard_deauth_arg = dbg_fc_arg = kr_band_arg = legacy_ack_arg = \
    be_arg = rs_arg = beacon_bypass_arg = bd_name_arg = \
    support_ch_width_arg = ps_pretend_arg = duty_cycle_arg = cca_thresh_arg = \
    twt_arg = raw_arg = auth_control_arg = macaddr_arg = sub_xtal_bypass_arg = ""

    # Set parameters for AP (support NDP probing)
    if strSTA() == 'AP':
        setAPParam()

    # Set parameters for RELAY
    if strSTA() == 'RELAY':
        setRelayParam()

    # Set parameters for SNIFFER
    if strSTA() == 'SNIFFER':
        setSnifferParam()

    # Set parameters for MESH
    if strSTA() == 'MESH':
        setMeshParam()

    # module param for FW download from host
    # default: fw_name (NULL: no download)
    if int(fw_download) == 1:
        fw_arg= " fw_name=" + fw_name

    # module param for FW update from host to flash
    # default: fw_update_name (NULL: no update)
    if int(auto_fw_update) == 1:
        fw_update_arg = " auto_fw_update=1 fw_update_name=" + fw_update_name

    # module param for power_save
    # default: power_save(0: active mode) sleep_duration(0,0)
    if strSTA() == 'STA' and int(power_save) > 0:
        power_save_arg = " power_save=" + str(power_save)
        if int(power_save) == 3:
            sleep_duration_arg = " sleep_duration=" + re.sub(r'[^0-9]','',sleep_duration)
            unit = sleep_duration[-1]
            if unit == 'm':
                sleep_duration_arg += ",0"
            else:
                sleep_duration_arg += ",1"

    # module param for IDLE mode
    # default: idle_mode(0: disabled)
    if strSTA() == 'STA' and int(idle_mode) == 1:
        idle_mode_arg = " idle_mode=1"

    if int(duty_cycle_enable) == 1:
        duty_cycle_arg = " set_duty_cycle="+str(duty_cycle_enable)+","+str(duty_cycle_window)+","+str(duty_cycle_duration)

    cca_thresh_arg = " set_cca_threshold="+str(cca_threshold)

    # module param for bss_max_idle (keep alive)
    # default: bss_max_idle(0: disabled)
    if int(bss_max_idle_enable) == 1:
        if strSTA() == 'AP' or strSTA() == 'RELAY' or strSTA() == 'STA':
            bss_max_idle_arg = " bss_max_idle=" + str(bss_max_idle)

    # module param for NDP Prboe Request (NDP scan)
    # default: ndp_preq(0: disabled)
    if int(ndp_preq) == 1:
        ndp_preq_arg = " ndp_preq=1"

    # module param for 1MBW NDP ACK
    # default: ndp_ack_1m(0: disabled)
    if int(ndp_ack_1m) == 1:
        ndp_ack_1m_arg = " ndp_ack_1m=1"

    # module param for AMPDU
    # default: auto (0: disable 1: manual 2: auto)
    if int(ampdu_enable) != 2:
        auto_ba_arg = " ampdu_mode=" + str(ampdu_enable)

    # module param for SW-based ENC/DEC
    # default: sw_enc(0: HW-based ENC/DEC)
    if int(sw_enc) > 0:
        sw_enc_arg = " sw_enc=" + str(sw_enc)

    # module param for CQM
    # default: disable_cqm(0: CQM enabled)
    if int(cqm_enable) == 0:
        cqm_arg = " disable_cqm=1"

    # module param for short beacon
    # default: enable_short_bi(1: Short Beacon enabled)
    if int(short_bcn_enable) == 0:
        sbi_arg = " enable_short_bi=0"

    # module param for legacy ack mode
    # default: 0(Legacy ACK disabled)
    if int(legacy_ack_enable) == 1:
        legacy_ack_arg = " enable_legacy_ack=1"

    # module param for authentication control
    # default: 0(disabled)
    if int(auth_control_enable) == 1 and strSTA() == 'AP':
        auth_control_arg = " set_auth_control="+str(auth_control_enable)+","+str(auth_control_slot)+","+\
                         str(auth_control_ti_min)+","+str(auth_control_ti_max)+ ","+str(auth_control_scale)

    # module param for beacon bypass
    # default: 0(beacon bypass disabled)
    if int(beacon_bypass_enable) == 1:
        beacon_bypass_arg = " enable_beacon_bypass=1"

    # module param for sched scan
    # default: 0(sched scan disabled)
    if int(sched_scan_enable) == 1:
        sched_scan_arg = " enable_sched_scan=1"

    # module param for listen interval
    # default: listen_interval(100)
    if int(listen_interval) > 0:
        listen_int_arg = " listen_interval=" + str(listen_interval)

    # module param for KR Band (KR only)
    # default: not defined(-1) (1:K1(KR USN1), 2:K2(KR USN5))
    if str(sys.argv[3]) == 'K1':
        kr_band_arg = " kr_band=1"
    elif str(sys.argv[3]) == 'K2':
        kr_band_arg = " kr_band=2"

    # module param for TW Band (TW only)
    # default: not defined(TW:Taiwan) (2:T2(TW NCC))
    if str(sys.argv[3]) == 'T2':
        tw_band_arg = " tw_band=2"

    # module param for deauth-discard on STA (test only)
    # default: discard_deauth(0: disabled)
    if int(discard_deauth) == 1:
        discard_deauth_arg = " discard_deauth=1"

    # module param for driver debug (debug only)
    # default: debug_level_all(0: disabled)
    if int(driver_debug) == 1:
        drv_dbg_arg = " debug_level_all=1"

    # module param for flow control debug (debug only)
    # default: dbg_flow_control(0: disabled)
    if int(dbg_flow_control) == 1:
        dbg_fc_arg = " debug_level_all=1 dbg_flow_control=1"

    # module param for bitmap encoding
    # default: use bitmap encoding (1: enabled)
    if int(bitmap_encoding) == 0:
        be_arg = " bitmap_encoding=0"

    # module param for reverse scrambler
    # default: use reverse scrambler (1: enabled)
    if int(reverse_scrambler) == 0:
        rs_arg = " reverse_scrambler=0"

    # module param for power save pretend
    # default: use power save pretend (0: disabled)
    if int(power_save_pretend) == 1:
        ps_pretend_arg = " ps_pretend=1"

    # module param for TWT
    # default: disabled
    if int(twt_num) > 0 or int(twt_sp) > 0 or int(twt_int) > 0:
        twt_arg = " twt_num=" + str(twt_num) + " twt_sp=" + str(twt_sp) + " twt_int=" + str(twt_int) + \
                  " twt_force_sleep=" + str(twt_force_sleep) + " twt_num_in_group=" + str(twt_num_in_group) + \
                  " twt_algo=" + str(twt_algo)

    # module param for RAW (Restricted Access Window)
    # default: disabled (0: disabled, 1: enabled)
    if int(raw) == 1:
        raw_arg = " raw=1"

    # module param for Sub XTAL bypass
    # default: use sub_xtal_pass (0: false)
    if int(sub_xtal_bypass) == 1:
        sub_xtal_bypass_arg = " sub_xtal_bypass=1"

    # module param for board data file
    # default: bd.dat
    bd_name_arg = " bd_name=" + strBDName()

    # module param for supported channel width
    # default : support 1/2/4MHz (1: 1/2/4Mhz)
    if strSTA() == 'STA' and int(support_ch_width) == 0:
        support_ch_width_arg = " support_ch_width=0"

    # module param for MAC address
    # default : use MAC address from EEPROM/Flash
    if macaddr != '':
        macaddr_arg = " macaddr=" + macaddr

    # WLAN module parameter setting
    # Default value is used if arg is not defined
    wlan_module_param = fw_arg + fw_update_arg + bd_name_arg + \
                       power_save_arg + sleep_duration_arg + idle_mode_arg + bss_max_idle_arg + \
                       ndp_preq_arg + ndp_ack_1m_arg + auto_ba_arg + sw_enc_arg + \
                       cqm_arg + listen_int_arg + drv_dbg_arg + sched_scan_arg + tw_band_arg + \
                       sbi_arg + discard_deauth_arg + dbg_fc_arg + kr_band_arg + legacy_ack_arg + \
                       be_arg + rs_arg + beacon_bypass_arg + support_ch_width_arg + \
                       ps_pretend_arg + duty_cycle_arg + cca_thresh_arg + twt_arg + raw_arg + auth_control_arg + \
                       sub_xtal_bypass_arg + macaddr_arg

    # Add debug parameters if specified
    if debug_level_param is not None:
        wlan_module_param += " debug_level=" + str(debug_level_param)
    if debug_mask_param is not None:
        wlan_module_param += " debug_mask=" + str(debug_mask_param)

    # Add recovery parameter to kernel module
    if recovery_param > 0:
        wlan_module_param += " recovery=" + str(recovery_param)

    return wlan_module_param

def run_common():
    # Check and install system dependencies
    check_system_dependencies()

    # Ensure required scripts are executable
    scripts_to_check = [
        NRC_PKG_PATH + "/script/conf/etc/clock_config.sh",
        NRC_PKG_PATH + "/sw/firmware/copy.sh",
        NRC_PKG_PATH + "/script/conf/etc/ip_config.sh",
        NRC_PKG_PATH + "/script/conf/etc/ip_config_bridge.sh"
    ]

    for script in scripts_to_check:
        ensure_script_executable(script)

    if int(max_cpuclock) == 1:
        print("[*] Set Max CPU Clock on RPi")
        clock_script = NRC_PKG_PATH + "/script/conf/etc/clock_config.sh"
        if ensure_script_executable(clock_script):
            os.system("sudo " + clock_script)
        else:
            print("[!] Skipping clock configuration (script not available)")

    print("[0] Clear")
    # Stop recovery daemon if running
    stop_recovery_daemon()
    # NetworkManager 중지 (wlan0 충돌 방지)
    os.system("sudo systemctl stop NetworkManager 2>/dev/null")
    os.system("sudo systemctl stop wpa_supplicant 2>/dev/null")
    os.system("sudo hostapd_cli disable 2>/dev/null")
    os.system("sudo wpa_cli disable wlan0 2>/dev/null ")
    os.system("sudo wpa_cli disable wlan1 2>/dev/null")
    os.system("sudo killall -9 wpa_supplicant 2>/dev/null")
    os.system("sudo killall -9 hostapd 2>/dev/null")
    os.system("sudo killall -9 wireshark 2>/dev/null")

    # Remove modular driver modules in reverse dependency order
    os.system("sudo rmmod nrc_wlan 2>/dev/null")
    os.system("sudo rmmod nrc_mcp 2>/dev/null")
    os.system("sudo rmmod nrc_core 2>/dev/null")
    os.system("sudo rmmod nrc_spi 2>/dev/null")
    # Also remove legacy driver if exists
    os.system("sudo rmmod nrc 2>/dev/null")

    os.system("sudo rm "+script_path+"conf/temp_self_config.conf 2>/dev/null")
    os.system("sudo rm "+script_path+"conf/temp_hostapd_config.conf 2>/dev/null")
    os.system("sudo sh -c '[ -e /proc/sys/kernel/sysrq ] && echo 0 > /proc/sys/kernel/sysrq'")
    stopNAT()
    stopDHCPCD()
    stopDNSMASQ()
    time.sleep(1)

    print("[1] Copy and Set Module Parameters")
    copyConf()
    spi_param = setSPIModuleParam()
    core_param = setCoreModuleParam()
    wlan_param = setWLANModuleParam()

    # Generate /etc/modprobe.d/nrc.conf for modprobe mode
    if load_method == 'modprobe':
        generate_modprobe_confs(spi_param, core_param, wlan_param)

    print("[2] Set Initial Country")
    country_code = strOriCountry()
    ret = os.system("sudo iw reg set " + country_code)

    # Verify country code setting
    country_set_success = False
    time.sleep(1)
    try:
        iw_reg_result = subprocess.check_output("sudo iw reg get", shell=True).decode()
        if "country " + country_code in iw_reg_result:
            print("[*] Country code (%s) set successfully via iw reg" % country_code)
            country_set_success = True
        else:
            print("[!] WARNING: iw reg set failed to set country code")
    except:
        print("[!] ERROR: iw reg set failed, will retry with cli_app after loading nrc_wlan.ko")

    print("[3] Loading NRC Modular Driver modules")

    if load_method == 'modprobe':
        # --- modprobe path: single command loads all dependencies ---
        print("[*] Using modprobe to load modules (system-installed)")
        if not ensure_depmod():
            print("[!] ERROR: Modules are not installed in system path.")
            print("[!] Run 'make install' in the driver source directory first,")
            print("[!] or use 'load=insmod' to load from %s/sw/driver/" % NRC_PKG_PATH)
            sys.exit(1)

        # modprobe nrc_wlan → auto-loads nrc_spi → nrc_core → nrc_wlan
        # Per-module parameters are read from /etc/modprobe.d/<module>.conf
        print("[3.1] Loading all modules via modprobe (nrc_spi → nrc_core → nrc_wlan)")
        ret = os.system("sudo modprobe nrc_wlan")
        if ret != 0:
            print("ERROR: Failed to load modules via modprobe")
            print("[!] Check: 'make install' done? depmod run? conf files in /etc/modprobe.d/?")
            os.system("sudo rmmod nrc_wlan 2>/dev/null")
            os.system("sudo rmmod nrc_core 2>/dev/null")
            os.system("sudo rmmod nrc_spi 2>/dev/null")
            sys.exit(1)
    else:
        # --- insmod path: load each module individually with params ---
        print("[*] Using insmod to load modules from %s/sw/driver/" % NRC_PKG_PATH)

        print("[3.1] Loading SPI backend module (nrc_spi.ko)")
        spi_ko_path = NRC_PKG_PATH + "/sw/driver/nrc_spi.ko"
        ret = load_module("nrc_spi", spi_ko_path, spi_param)
        if ret != 0:
            print("ERROR: Failed to load SPI module (nrc_spi.ko)")
            sys.exit(1)
        time.sleep(2)

        print("[3.2] Loading HAL core module (nrc_core.ko)")
        core_ko_path = NRC_PKG_PATH + "/sw/driver/nrc_core.ko"
        ret = load_module("nrc_core", core_ko_path, core_param)
        if ret != 0:
            print("ERROR: Failed to load HAL core module (nrc_core.ko)")
            os.system("sudo rmmod nrc_spi")
            sys.exit(1)
        time.sleep(2)

        print("[3.3] Loading WLAN frontend module (nrc_wlan.ko)")
        wlan_ko_path = NRC_PKG_PATH + "/sw/driver/nrc_wlan.ko"
        ret = load_module("nrc_wlan", wlan_ko_path, wlan_param)
        if ret != 0:
            print("ERROR: Failed to load WLAN module (nrc_wlan.ko)")
            os.system("sudo rmmod nrc_core")
            os.system("sudo rmmod nrc_spi")
            sys.exit(1)

    if int(spi_polling_interval) <= 0:
        time.sleep(5)
    else:
        time.sleep(10)

    # Retry country code setting with cli_app if iw reg set failed
    if not country_set_success:
        print("[3.4] Retrying country code setting with cli_app")
        cli_output = subprocess.run(
            ["sudo", NRC_PKG_PATH + "/script/cli_app", "test", "country", country_code],
            capture_output=True,
            text=True
        )

        # Check both exit code and output for "FAIL"
        if cli_output.returncode == 0 and "FAIL" not in cli_output.stdout:
            print("[*] Country code (%s) set successfully via cli_app" % country_code)
        else:
            print("[!] WARNING: cli_app failed to set country code")
            if "FAIL" in cli_output.stdout or "NACK" in cli_output.stdout:
                print("[!] Kernel communication error (NACK) - driver may not be ready")
            print("[!] Note: iw reg set will be attempted again after interface is up")

    if strSTA() == 'RELAY' and int(relay_type) == 0:
        addWLANInterface('wlan1')
    elif strSTA() == 'MESH' and int(relay_type) == 2:
        addMeshInterface('mesh0')

    # Unblock RF-kill if blocked
    subprocess.call(["sudo", "rfkill", "unblock", "all"])
    ret = subprocess.call(["sudo", "ifconfig", "wlan0", "up"])
    if ret == 255:
        print("ERROR: Failed to bring up wlan0 interface")
        os.system('sudo rmmod nrc_wlan')
        os.system('sudo rmmod nrc_core')
        os.system('sudo rmmod nrc_spi')
        sys.exit()

    # Retry country code setting after interface is up if initial attempt failed
    if not country_set_success:
        print("[3.5] Retrying country code with iw reg after interface up")
        time.sleep(1)
        os.system("sudo iw reg set " + country_code)
        time.sleep(1)
        try:
            iw_reg_result = subprocess.check_output("sudo iw reg get", shell=True).decode()
            if "country " + country_code in iw_reg_result:
                print("[*] Country code (%s) successfully set after interface up" % country_code)
                country_set_success = True
            else:
                print("[!] WARNING: Country code setting still failed after interface up")
        except:
            print("[!] WARNING: Could not verify country code after interface up")

    print("[4] Set Maximum TX Power")
    os.system('sudo ' + NRC_PKG_PATH + '/script/cli_app set txpwr limit ' + str(max_txpwr))
    if strSTA() != 'SNIFFER':
        print("[*] Transmission Power Control(TPC) is activated")
        os.system('sudo iw phy nrc80211 set txpower limit ' + str(int(max_txpwr) * 100))

    print("[5] Set guard interval: " + guard_int)
    os.system('sudo ' + NRC_PKG_PATH + '/script/cli_app set gi ' + guard_int)

    # Start DHCPCD and DNSMASQ only for STA/RELAY modes
    # AP mode will configure its own DHCP server
    if strSTA() == 'STA' or strSTA() == 'RELAY':
        print("[*] Start DHCPCD (for STA/RELAY mode)")
        startDHCPCD()

    # DNSMASQ will be started by AP mode with custom config if needed

def load_mcp_module(skip_if_relay_sta=False):
    """
    Load MCP frontend module after WLAN initialization is complete.
    Called at the end of each run_* function to ensure WLAN is fully operational.

    Args:
        skip_if_relay_sta: Skip loading if this is RELAY mode STA (run_ap will load it)
    """
    # For RELAY mode, only load MCP once at the end of run_ap, not run_sta
    if skip_if_relay_sta and strSTA() == 'RELAY':
        return True

    print("=" * 70)
    print("[*] Loading MCP frontend module (nrc-mcp.ko)")
    print("=" * 70)

    # MCP module parameters
    mcp_param = " mcp_priority=" + str(mcp_priority)
    mcp_param += " bd_name=nrc7394_bd.dat fw_name=uni_s1g.bin"

    # Add debug parameters if specified
    if debug_level_param is not None:
        mcp_param += " debug_level=" + str(debug_level_param)
    if debug_mask_param is not None:
        mcp_param += " debug_mask=" + str(debug_mask_param)

    # Generate per-module conf for MCP (overwritten each run)
    if load_method == 'modprobe':
        generate_mcp_modprobe_conf(mcp_param)
        ret = os.system("sudo modprobe nrc_mcp")
    else:
        mcp_ko_path = NRC_PKG_PATH + "/sw/driver/nrc-mcp.ko"
        ret = load_module("nrc_mcp", mcp_ko_path, mcp_param)

    if ret != 0:
        print("WARNING: Failed to load MCP module (nrc-mcp.ko)")
        print("         WLAN functionality will continue normally")
        return False
    else:
        print("[*] MCP module loaded successfully")
        print("[*] MCP Priority: " + ("enabled" if int(mcp_priority) == 1 else "disabled"))
        time.sleep(1)
        return True

def run_sta(interface):
    country = str(sys.argv[3])
    if checkEUCountry():
        country="EU"
    conf_dir = script_path + "conf/" + country
    conf_file = ""
    os.system("sudo killall -9 wpa_supplicant")

    if int(use_bridge_setup) > 0:
        bridge = '-b br0 '
        print('[*] STA bridge configuration')
        if strSTA() == 'RELAY' :
            os.system('sudo brctl addbr br0; sudo ifconfig wlan1 up; sudo ifconfig wlan1 0.0.0.0; sudo ifconfig wlan0 0.0.0.0; sudo iw {w} set 4addr on; sudo brctl addif br0 {w}; sudo ifconfig br0 up'.format(w=interface))
        else :
            eth = 'eth' + str(int(use_bridge_setup) - 1)
            os.system('sudo brctl addbr br0; sudo ifconfig {e} up; sudo ifconfig {w} 0.0.0.0; sudo ifconfig {e} 0.0.0.0; sudo iw {w} set 4addr on; sudo brctl addif br0 {w}; sudo brctl addif br0 {e}; sudo ifconfig br0 up'.format(e=eth, w=interface))
            os.system('sudo brctl show')
    else:
        bridge = ''

    if int(supplicant_debug) == 1:
        debug = '-dddd'
    else:
        debug = ''

    if int(power_save) > 0:
        print("[*] Set default power save timeout for " + interface)
        os.system("sudo iwconfig " + interface + " power timeout " + ps_timeout)

    print("[6] Start wpa_supplicant on " + interface)
    if strSecurity() == 'OPEN':
        conf_file = "/sta_halow_open.conf "
    elif strSecurity() == 'WPA2-PSK':
        conf_file = "/sta_halow_wpa2.conf "
    elif strSecurity() == 'WPA3-OWE':
        conf_file = "/sta_halow_owe.conf "
    elif strSecurity() == 'WPA3-SAE':
        conf_file = "/sta_halow_sae.conf "
    elif strSecurity() == 'WPA-PBC':
        conf_file = "/sta_halow_pbc.conf "

    if conf_file != "":
        if country == "EU":
            os.system("sed -i \"s/^country=.*/country=%s/g\" %s" % ( str(sys.argv[3]), conf_dir + conf_file ) )
        os.system("sudo wpa_supplicant -i" + interface + " -c " + conf_dir + conf_file + bridge + debug + " &")
        if strSecurity() == 'WPA-PBC':
            time.sleep(1)
            os.system("sudo wpa_cli wps_pbc")
    time.sleep(3)

    # Ensure dhcpcd is still running after wpa_supplicant started.
    # dhcpcd may have exited if carrier was briefly acquired/lost during
    # module init (common after recovery restart where systemd dhcpcd.service
    # was stopped). Re-launch it now that wpa_supplicant is connecting.
    ret_dhcp = os.system("pgrep -x dhcpcd > /dev/null 2>&1")
    if ret_dhcp != 0:
        print("[*] dhcpcd not running, restarting for " + interface)
        os.system("sudo dhcpcd -b " + interface)

    print("[7] Connect and DHCP")
    if int(use_bridge_setup) > 0:
        interface = 'br0'
    ret = check(interface)
    while ret == '':
        print("Waiting for IP")
        time.sleep(5)
        ret = check(interface)

    print(ret)
    print("IP assigned. HaLow STA ready")
    print("--------------------------------------------------------------------")

    # Load MCP module after STA is fully operational
    # Skip if RELAY mode (run_ap will load it after both interfaces are ready)
    load_mcp_module(skip_if_relay_sta=True)

def launch_hostapd(interface, orig_hostapd_conf_file, country, debug, channel):
    print("[*] configure file copied from: %s" % (orig_hostapd_conf_file) )
    TEMP_HOSTAPD_CONF = script_path +  "conf/temp_hostapd_config.conf"
    os.system("sudo cp %s %s" % ( orig_hostapd_conf_file,  TEMP_HOSTAPD_CONF ) )
    os.system("sed -i \"4s/.*/interface=%s/g\" %s" % ( interface, TEMP_HOSTAPD_CONF ) )
    if country == "EU":
        os.system("sed -i \"s/^country_code=.*/country_code=%s/g\" %s" % ( str(sys.argv[3]), TEMP_HOSTAPD_CONF ) )
    if channel:
        os.system("sed -i \"s/^channel=.*/channel=%s/g\" %s" % ( channel, TEMP_HOSTAPD_CONF ) )

        # According to "UG-7292-001-EVK User Guide (Host Mode).pdf" page 40, the ``hw_mode`` needs to be changed to
        #  ``hw_mode=g`` instead of ``hw_mode=a`` in ``US`` country code.
        if country == "US" and 1 <= int(channel) and int(channel) <= 13:
            os.system("sed -i \"s/^hw_mode=.*/hw_mode=g/g\" %s" % ( TEMP_HOSTAPD_CONF ) )

    os.system("sudo hostapd %s %s &" % ( TEMP_HOSTAPD_CONF, debug ) )


def run_ap(interface):
    country = str(sys.argv[3])
    if checkEUCountry():
        country="EU"
    conf_path = script_path + "conf/" + country
    conf_file = ""
    global self_config
    channel = None

    if strSecurity() == "OPEN" :
        conf_file+="/ap_halow_open.conf"
    elif strSecurity() == 'WPA2-PSK' :
        conf_file+="/ap_halow_wpa2.conf"
    elif strSecurity() == 'WPA3-OWE' :
        conf_file+="/ap_halow_owe.conf"
    elif strSecurity() == 'WPA3-SAE' :
        conf_file+="/ap_halow_sae.conf"
    elif strSecurity() == 'WPA-PBC' :
        conf_file+="/ap_halow_pbc.conf"

    if int(use_bridge_setup) > 0:
        # Remove '#' before wds_sta, bridge in conf file
        os.system("sed -i /wds_sta/,/bridge/s/##*// " + conf_path + conf_file)
    else:
        # Add '#' before wds_sta, bridge in conf file
        os.system("sed -i /wds_sta/,/bridge/'s/^/#/;/wds_sta/,/bridge/s/##*/#/' " + conf_path + conf_file)

    if len(sys.argv) > 4 :
        channel = str(sys.argv[4])

    if int(hostapd_debug) == 1:
        debug = '-dddd'
    else:
        debug = ''

    if strSTA() == 'RELAY':
        self_config = 0
        print("[*] Selfconfig is not used in RELAY mode.")
    if (int(self_config)==1):
        print("[*] Self configuration start!")
        self_conf_result = self_config_check()
    elif(int(self_config)==0):
        print("[*] Self configuration off")
    else:
        print("[*] self_conf value should be 0 or 1..  Start with default mode(no self configuration)")

    print("[6] Start hostapd on " + interface)

    if(int(self_config)==1 and self_conf_result=='Done'):
        os.system("sed -i " + '"4s/.*/interface=' + interface + '/g" ' + script_path + "conf/temp_self_config.conf " )
        os.system("sudo hostapd " + script_path + "conf/temp_self_config.conf " + debug +" &")
        if strSecurity() == 'WPA-PBC':
            time.sleep(1)
            os.system("sudo hostapd_cli wps_pbc")
    else:
        launch_hostapd( interface, NRC_PKG_PATH + '/script/conf/' + country + conf_file, country, debug, channel )
        if strSecurity() == 'WPA-PBC':
            time.sleep(1)
            os.system("sudo hostapd_cli wps_pbc")
    time.sleep(3)

    if int(use_bridge_setup) > 0:
        print('[*] AP bridge configuration')
        if strSTA() == 'RELAY' :
            os.system('sudo brctl addif br0 {w}'.format(w=interface))
        else :
            eth = 'eth' + str(int(use_bridge_setup) - 1)
            os.system('sudo ifconfig {e} up; sudo ifconfig {w} 0.0.0.0; sudo ifconfig {e} 0.0.0.0; sudo brctl addif br0 {e}; sudo ifconfig br0 up '.format(e=eth, w=interface))
        os.system('sudo brctl show')
        time.sleep(3)

    print("[7] Start NAT")
    startNAT()

    print("[8] Configure AP Network (Static IP + DHCP Server)")
    if int(use_bridge_setup) > 0:
        configure_ap_network('br0')
    else:
        configure_ap_network(interface)

    time.sleep(3)
    print("[9] ifconfig")
    os.system('sudo ifconfig')
    print("HaLow AP ready")
    print("--------------------------------------------------------------------")

    # Load MCP module after AP is fully operational
    load_mcp_module()

def run_sniffer():
    print("[6] Setting Monitor Mode")
    time.sleep(3)
    os.system('sudo ifconfig wlan0 down; sudo iw dev wlan0 set type monitor; sudo ifconfig wlan0 up')
    print("[7] Setting Country: " + strOriCountry())
    os.system("sudo iw reg set " + strOriCountry())
    time.sleep(3)
    print("[8] Setting Channel: " + str(sys.argv[4]))
    os.system("sudo iw dev wlan0 set channel " + str(sys.argv[4]))
    time.sleep(3)
    print("[9] Start Sniffer")
    if strSnifferMode() == 'LOCAL':
        os.system('sudo wireshark -i wlan0 -k -S -l &')
    else:
        os.system('lxqt-sudo wireshark -i wlan0 -k -S -l &')
    print("HaLow SNIFFER ready")
    print("--------------------------------------------------------------------")

    # Load MCP module after Sniffer is fully operational
    load_mcp_module()

RECOVERYD_PID_FILE = "/tmp/nrc_recoveryd.pid"

def stop_recovery_daemon():
    """Stop existing recoveryd.py process if running."""
    if not os.path.exists(RECOVERYD_PID_FILE):
        return
    try:
        with open(RECOVERYD_PID_FILE, 'r') as f:
            pid = int(f.read().strip())
        # Check if process is alive
        os.kill(pid, 0)
        print("[*] Stopping recovery daemon (pid=%d)" % pid)
        os.kill(pid, signal.SIGTERM)
        # Wait and verify the daemon actually exited
        for _ in range(10):
            time.sleep(0.5)
            try:
                os.kill(pid, 0)
            except OSError:
                break  # process exited
    except (IOError, OSError, ValueError):
        pass
    # Only remove PID file if it still references the daemon we stopped
    try:
        with open(RECOVERYD_PID_FILE, 'r') as f:
            current_pid = int(f.read().strip())
        if current_pid == pid:
            os.remove(RECOVERYD_PID_FILE)
    except (IOError, OSError, ValueError, UnboundLocalError):
        pass

def start_recovery_daemon(original_argv):
    """
    Launch recoveryd.py as a background daemon process.
    Passes original command line arguments so the daemon can restart
    start_modular.py with the same parameters on recovery trigger.
    """
    import json

    script_dir = os.path.dirname(os.path.abspath(__file__))
    daemon_path = os.path.join(script_dir, "recoveryd.py")

    if not os.path.exists(daemon_path):
        print("[!] WARNING: recoveryd.py not found at %s" % daemon_path)
        print("[!] Recovery daemon will NOT be started")
        return

    # Pass original argv as JSON for safe transport of quoted/special args
    argv_json = json.dumps(original_argv)

    # Remove stale log file (may be owned by root from a previous run,
    # causing "Permission denied" if current shell redirect runs as non-root)
    os.system("sudo rm -f /tmp/recoveryd.log")

    cmd = 'sudo nohup python %s --script-dir "%s" --start-argv \'%s\' > /tmp/recoveryd.log 2>&1 &' % (
        daemon_path, script_dir, argv_json)

    print("[*] Starting recovery daemon (recoveryd.py)")
    print("[*]   Script dir : %s" % script_dir)
    print("[*]   Start argv : %s" % " ".join(original_argv))
    os.system(cmd)
    time.sleep(1)

    # Verify daemon started
    if os.path.exists("/tmp/nrc_recoveryd.pid"):
        try:
            with open("/tmp/nrc_recoveryd.pid", 'r') as f:
                pid = f.read().strip()
            print("[*] Recovery daemon running (pid=%s)" % pid)
        except IOError:
            print("[!] Recovery daemon PID file exists but unreadable")
    else:
        print("[!] WARNING: Recovery daemon may not have started (no PID file)")

if __name__ == '__main__':
    # Save original argv for recovery daemon (before parse_debug_args filters them)
    original_argv = sys.argv[1:]

    # Parse debug arguments and filter them out from positional arguments
    positional_args = parse_debug_args()

    # Apply power save parameters if provided
    if ps_param is not None:
        power_save = ps_param
    if idle_param is not None:
        idle_mode = idle_param

    # Replace sys.argv with filtered positional arguments (keeping script name)
    sys.argv = [sys.argv[0]] + positional_args

    if len(sys.argv) < 4 or not isNumber(sys.argv[1]) or not isNumber(sys.argv[2]):
        usage_print()
    elif strSTA() == 'SNIFFER' and len(sys.argv) < 6:
        usage_print()
    elif strSTA() == 'MESH':
        checkMeshUsage()
    else:
        argv_print()

    checkParamValidity()
    checkCountry()

    print("NRC Modular " + strSTA() + " setting for HaLow...")

    run_common()

    if strSTA() == 'STA':
        run_sta('wlan0')
    elif strSTA() == 'AP':
        run_ap('wlan0')
    elif strSTA() == 'SNIFFER':
        run_sniffer()
    elif strSTA() == 'RELAY':
        #start STA and then AP for Relay
        if int(relay_type) == 0:
            t = threading.Thread(target=run_sta, args=('wlan0',))
            t.start()
            run_ap('wlan1')
        else:
            addWLANInterface('wlan1')
            t = threading.Thread(target=run_sta, args=('wlan1',))
            t.start()
            run_ap('wlan0')
    elif strSTA() == 'MESH':
        if strMeshMode() == 'Mesh Portal':
            run_mpp('wlan0', str(sys.argv[3]), strSecurity(), supplicant_debug, peer, static_ip, batman)
        elif strMeshMode() == 'Mesh Point':
            run_mp('wlan0', str(sys.argv[3]), strSecurity(), supplicant_debug, peer, static_ip, batman)
        elif strMeshMode() == 'Mesh AP':
            run_map('wlan0', 'mesh0', str(sys.argv[3]), strSecurity(), supplicant_debug, peer, static_ip, batman)
        else:
            usage_print()
    else:
        usage_print()

    # Launch recovery daemon if recovery=1 was specified
    if recovery_param > 0:
        start_recovery_daemon(original_argv)

print("Done.")
