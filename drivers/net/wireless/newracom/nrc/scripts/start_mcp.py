#!/usr/bin/python

"""
NRC MCP Module Standalone Startup Script
Installs only MCP frontend module with firmware download capability
Can be used independently or after WLAN module installation
"""

import sys, os, time, subprocess

# Default Configuration
model = 7394         # 7292/7394
fw_name = 'uni_s1g.bin'
bd_name = 'nrc7394_bd.dat'
mcp_priority = 0  # 0=disabled, 1=enabled
use_eeprom_config = 0  # 0 (Flash Memory) or 1 (EEPROM)

def copyFirmware():
    """Copy firmware and board data files to /lib/firmware"""
    print("[*] Copying firmware and board data to /lib/firmware")
    os.system("sudo /home/pi/nrc_pkg/sw/firmware/copy.sh " + str(model) + " " + bd_name + " " + str(use_eeprom_config))

def check_module_loaded(module_name):
    """Check if a kernel module is loaded"""
    lsmod_cmd = "lsmod | grep " + module_name
    try:
        result = subprocess.check_output(lsmod_cmd, shell=True, stderr=subprocess.DEVNULL)
        return len(result) > 0
    except subprocess.CalledProcessError:
        return False

def load_backend_if_needed():
    """Load SPI backend module if not already loaded"""
    if check_module_loaded("nrc_spi"):
        print("[*] SPI backend module (nrc_spi.ko) already loaded")
        return True

    print("[*] Loading SPI backend module (nrc_spi.ko)")
    # Use default SPI parameters
    spi_param = " hifspeed=20000000 spi_bus_num=0 spi_cs_num=0 spi_gpio_irq=5 spi_polling_interval=0"
    spi_insmod_cmd = "sudo insmod /home/pi/nrc_pkg/sw/driver/nrc_spi.ko" + spi_param
    print(spi_insmod_cmd)
    ret = os.system(spi_insmod_cmd)
    if ret != 0:
        print("ERROR: Failed to load SPI module (nrc_spi.ko)")
        return False
    time.sleep(2)
    return True

def load_hal_if_needed():
    """Load HAL core module if not already loaded"""
    if check_module_loaded("nrc_core"):
        print("[*] HAL core module (nrc_core.ko) already loaded")
        return True

    print("[*] Loading HAL core module (nrc_core.ko)")
    core_insmod_cmd = "sudo insmod /home/pi/nrc_pkg/sw/driver/nrc_core.ko"
    print(core_insmod_cmd)
    ret = os.system(core_insmod_cmd)
    if ret != 0:
        print("ERROR: Failed to load HAL core module (nrc_core.ko)")
        os.system("sudo rmmod nrc_spi 2>/dev/null")
        return False
    time.sleep(2)
    return True

def unload_mcp_if_loaded():
    """Unload MCP module if already loaded"""
    if check_module_loaded("nrc_mcp"):
        print("[*] Unloading existing MCP module (nrc-mcp.ko)")
        os.system("sudo rmmod nrc_mcp 2>/dev/null")
        time.sleep(1)

def load_mcp_module():
    """Load MCP frontend module with specified parameters"""
    # Always unload MCP first if it exists
    unload_mcp_if_loaded()

    print("[*] Loading MCP frontend module (nrc-mcp.ko)")
    # MCP module parameters with firmware, board data, and priority
    mcp_param = " fw_name=" + fw_name + " bd_name=" + bd_name + " mcp_priority=" + str(mcp_priority)
    mcp_insmod_cmd = "sudo insmod /home/pi/nrc_pkg/sw/driver/nrc-mcp.ko" + mcp_param
    print(mcp_insmod_cmd)
    ret = os.system(mcp_insmod_cmd)
    if ret != 0:
        print("ERROR: Failed to load MCP module (nrc-mcp.ko)")
        return False
    time.sleep(2)
    return True

def usage_print():
    print("Usage:")
    print("  ./start_mcp.py [fw_name] [bd_name] [mcp_priority] [model] [use_eeprom]")
    print("")
    print("Arguments:")
    print("  fw_name       - Firmware file name (default: uni_s1g.bin)")
    print("  bd_name       - Board data file name (default: nrc7394_bd.dat)")
    print("  mcp_priority  - MCP priority mode: 0=disabled, 1=enabled (default: 0)")
    print("  model         - Chip model: 7292 or 7394 (default: 7394)")
    print("  use_eeprom    - Config location: 0=Flash, 1=EEPROM (default: 0)")
    print("")
    print("Examples:")
    print("  ./start_mcp.py")
    print("  ./start_mcp.py uni_s1g.bin nrc7394_bd.dat")
    print("  ./start_mcp.py uni_s1g.bin nrc7394_bd.dat 1  # Enable MCP priority")
    print("  ./start_mcp.py uni_s1g.bin nrc7292_bd.dat 0 7292  # For 7292 chip")
    print("  ./start_mcp.py uni_s1g.bin nrc7394_bd.dat 0 7394 1  # Use EEPROM")
    print("")
    print("MCP Priority Mode:")
    print("  - 0 (default): WLAN and MCP transmit in parallel")
    print("  - 1 (enabled): WLAN TX is suspended when MCP is transmitting")
    print("")
    print("Note:")
    print("  - This script can be run independently (will load SPI and HAL modules)")
    print("  - Can also be run after WLAN module is already loaded")
    print("  - If WLAN module loaded first, MCP will use WLAN's fw_name and bd_name")
    print("  - Firmware files are automatically copied to /lib/firmware")
    exit()

def main():
    global fw_name, bd_name, mcp_priority, model, use_eeprom_config

    # Parse command line arguments
    if len(sys.argv) > 1:
        if sys.argv[1] == "help" or sys.argv[1] == "-h" or sys.argv[1] == "--help":
            usage_print()
        fw_name = sys.argv[1]

    if len(sys.argv) > 2:
        bd_name = sys.argv[2]

    if len(sys.argv) > 3:
        mcp_priority = int(sys.argv[3])

    if len(sys.argv) > 4:
        model = int(sys.argv[4])

    if len(sys.argv) > 5:
        use_eeprom_config = int(sys.argv[5])

    print("=" * 70)
    print("NRC MCP Module Standalone Startup")
    print("=" * 70)
    print("Model:          " + str(model))
    print("Firmware file:  " + fw_name)
    print("Board data:     " + bd_name)
    print("MCP priority:   " + str(mcp_priority) + " (" + ("enabled" if mcp_priority else "disabled") + ")")
    print("Config location:" + (" EEPROM" if use_eeprom_config else " FLASH"))
    print("-" * 70)

    # Check if WLAN module is already loaded
    if check_module_loaded("nrc_wlan"):
        print("[!] WARNING: WLAN module already loaded")
        print("[!] MCP will use firmware/board data from first loaded frontend (WLAN)")
        print("[!] Your specified fw_name and bd_name may be ignored")
        print("-" * 70)
    else:
        # Copy firmware files only if WLAN module is not loaded
        # (If WLAN is loaded, it already copied the firmware)
        copyFirmware()

    # Step 1: Load SPI backend if needed
    if not load_backend_if_needed():
        sys.exit(1)

    # Step 2: Load HAL core if needed
    if not load_hal_if_needed():
        sys.exit(1)

    # Step 3: Load MCP frontend
    if not load_mcp_module():
        sys.exit(1)

    print("=" * 70)
    print("NRC MCP Module loaded successfully!")
    print("=" * 70)

    # Show current loaded modules
    print("\nCurrently loaded NRC modules:")
    os.system("lsmod | grep nrc")

    print("\nDone.")

if __name__ == '__main__':
    main()
