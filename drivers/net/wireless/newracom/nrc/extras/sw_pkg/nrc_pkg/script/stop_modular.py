#!/usr/bin/python

import sys, os, time, subprocess, re
import signal

# Auto-detect script path based on script location
SCRIPT_PATH = os.path.abspath(__file__)
SCRIPT_DIR = os.path.dirname(SCRIPT_PATH)
script_path = SCRIPT_DIR + "/"

def usage_print():
    print("Usage: \n\tstop_modular.py [optional_mode]")
    print("Argument:    \n\toptional_mode [clean: Clean all configurations | modules: Stop modules only]")
    print("Example:  \n\tStop all services and modules          : ./stop_modular.py \
                      \n\tStop modules only                      : ./stop_modular.py modules \
                      \n\tClean all configs and stop everything  : ./stop_modular.py clean")
    print("Note: \n\tDefault behavior stops all services and unloads modules in proper order \
                  \n\t'modules' mode only unloads kernel modules without stopping services \
                  \n\t'clean' mode removes all temporary configs and resets network settings")
    exit()

def stopNAT():
    print("[*] Stopping NAT configuration")
    os.system('sudo sh -c "echo 0 > /proc/sys/net/ipv4/ip_forward"')
    os.system("sudo iptables -t nat --flush")
    os.system("sudo iptables --flush")

def stopDHCPCD():
    print("[*] Stopping DHCPCD service")
    # Only release wlan interfaces — do NOT stop the global dhcpcd service
    # which would also kill eth0's DHCP lease and cause SSH disconnection.
    os.system("sudo dhcpcd -k wlan0 2>/dev/null")
    os.system("sudo dhcpcd -k wlan1 2>/dev/null")

def stopDNSMASQ():
    print("[*] Stopping DNSMASQ service")
    os.system("sudo systemctl stop dnsmasq")

def stopWPASupplicant():
    print("[*] Stopping WPA Supplicant")
    os.system("sudo wpa_cli disable wlan0 2>/dev/null")
    os.system("sudo wpa_cli disable wlan1 2>/dev/null")
    os.system("sudo killall -9 wpa_supplicant 2>/dev/null")

def stopHostAPD():
    print("[*] Stopping HostAPD")
    os.system("sudo hostapd_cli disable 2>/dev/null")
    os.system("sudo killall -9 hostapd 2>/dev/null")

def stopWireshark():
    print("[*] Stopping Wireshark")
    os.system("sudo killall -9 wireshark 2>/dev/null")

RECOVERYD_PID_FILE = "/tmp/nrc_recoveryd.pid"

def stopRecoveryDaemon():
    """Stop recovery daemon (recoveryd.py) if running."""
    if not os.path.exists(RECOVERYD_PID_FILE):
        return
    pid = None
    try:
        with open(RECOVERYD_PID_FILE, 'r') as f:
            pid = int(f.read().strip())
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
    if pid is not None:
        try:
            with open(RECOVERYD_PID_FILE, 'r') as f:
                current_pid = int(f.read().strip())
            if current_pid == pid:
                os.remove(RECOVERYD_PID_FILE)
        except (IOError, OSError, ValueError):
            pass

def stopBridgeSetup():
    print("[*] Removing bridge configurations")
    # Remove bridge interfaces if they exist
    os.system("sudo ifconfig br0 down 2>/dev/null")
    os.system("sudo brctl delbr br0 2>/dev/null")
    
    # Reset 4addr mode on wireless interfaces only if they exist
    # Check if wlan0 exists before trying to modify it
    if os.system("iw dev wlan0 info >/dev/null 2>&1") == 0:
        os.system("sudo iw wlan0 set 4addr off 2>/dev/null")
    
    # Check if wlan1 exists before trying to modify it  
    if os.system("iw dev wlan1 info >/dev/null 2>&1") == 0:
        os.system("sudo iw wlan1 set 4addr off 2>/dev/null")

def removeWLANInterfaces():
    print("[*] Removing additional WLAN interfaces")
    # Remove wlan1 if it was created for concurrent mode
    os.system("sudo iw dev wlan1 del 2>/dev/null")
    # Remove mesh0 if it was created for mesh mode
    os.system("sudo iw dev mesh0 del 2>/dev/null")

def unloadNRCModules():
    """
    Unload NRC modular driver modules in reverse dependency order
    Critical: Must unload in reverse order - Frontend → HAL → Backend
    """
    print("[*] Unloading NRC Modular Driver modules")

    print("[*] Unloading WLAN frontend module (nrc_wlan.ko)")
    ret = os.system("sudo rmmod nrc_wlan 2>/dev/null")
    if ret == 0:
        print("    - nrc_wlan.ko successfully unloaded")
    else:
        print("    - nrc_wlan.ko was not loaded or failed to unload")

    print("[*] Unloading MCP frontend module (nrc-mcp.ko)")
    ret = os.system("sudo rmmod nrc_mcp 2>/dev/null")
    if ret == 0:
        print("    - nrc-mcp.ko successfully unloaded")
    else:
        print("    - nrc-mcp.ko was not loaded or failed to unload")

    print("[*] Unloading HAL core module (nrc_core.ko)")
    ret = os.system("sudo rmmod nrc_core 2>/dev/null")
    if ret == 0:
        print("    - nrc_core.ko successfully unloaded")
    else:
        print("    - nrc_core.ko was not loaded or failed to unload")

    print("[*] Unloading SPI backend module (nrc_spi.ko)")
    ret = os.system("sudo rmmod nrc_spi 2>/dev/null")
    if ret == 0:
        print("    - nrc_spi.ko successfully unloaded")
    else:
        print("    - nrc_spi.ko was not loaded or failed to unload")

    # Also try to remove legacy driver if it exists
    print("[*] Removing legacy driver (nrc.ko) if present")
    ret = os.system("sudo rmmod nrc 2>/dev/null")
    if ret == 0:
        print("    - nrc.ko successfully unloaded")
    else:
        print("    - nrc.ko was not loaded")

def cleanupTempFiles():
    print("[*] Cleaning up temporary configuration files")
    os.system("sudo rm " + script_path + "conf/temp_self_config.conf 2>/dev/null")
    os.system("sudo rm " + script_path + "conf/temp_hostapd_config.conf 2>/dev/null")

def resetSystemSettings():
    print("[*] Resetting system settings")
    os.system("sudo sh -c '[ -e /proc/sys/kernel/sysrq ] && echo 0 > /proc/sys/kernel/sysrq'")

def restartNetworkManager():
    print("[*] Restarting NetworkManager and WPA Supplicant")
    os.system("sudo systemctl start NetworkManager 2>/dev/null")
    os.system("sudo systemctl start wpa_supplicant 2>/dev/null")

def stopModulesOnly():
    """
    Stop only the kernel modules without affecting services
    """
    print("=== NRC Modular Driver - Modules Only Stop ===")
    stopRecoveryDaemon()
    removeWLANInterfaces()
    unloadNRCModules()
    time.sleep(1)
    print("=== NRC Modules Successfully Stopped ===")

def stopAll():
    """
    Stop all services and modules (default behavior)
    """
    print("=== NRC Modular Driver - Full Stop ===")
    
    # Stop recovery daemon first (before modules are unloaded)
    stopRecoveryDaemon()
    
    # Stop all network services
    stopWPASupplicant()
    stopHostAPD()
    stopWireshark()
    
    # Stop system services
    stopNAT()
    stopDHCPCD()
    stopDNSMASQ()
    
    # Remove network configurations
    stopBridgeSetup()
    removeWLANInterfaces()
    
    # Unload driver modules
    unloadNRCModules()
    
    # System cleanup
    resetSystemSettings()
    
    time.sleep(1)
    print("=== NRC Modular Driver Successfully Stopped ===")

def cleanAll():
    """
    Clean all configurations and stop everything
    """
    print("=== NRC Modular Driver - Clean Stop ===")
    
    # Stop everything
    stopAll()
    
    # Additional cleanup
    cleanupTempFiles()
    
    # Restart network services for normal operation
    restartNetworkManager()
    
    print("=== NRC Modular Driver Cleaned and Stopped ===")

def checkModuleStatus():
    """
    Check which NRC modules are currently loaded
    """
    print("[*] Checking current NRC module status:")
    
    # Check lsmod output for NRC modules
    lsmod_cmd = "lsmod | grep nrc"
    try:
        result = subprocess.check_output(lsmod_cmd, shell=True)
        if result:
            print("    Currently loaded NRC modules:")
            modules = result.decode().strip().split('\n')
            for module in modules:
                print(f"    - {module}")
        else:
            print("    No NRC modules currently loaded")
    except subprocess.CalledProcessError:
        print("    No NRC modules currently loaded")
    
    # Check for network interfaces
    print("\n[*] Checking NRC network interfaces:")
    try:
        ifconfig_result = subprocess.check_output("ifconfig | grep wlan", shell=True)
        if ifconfig_result:
            print("    Active WLAN interfaces:")
            interfaces = ifconfig_result.decode().strip().split('\n')
            for interface in interfaces:
                print(f"    - {interface.split(':')[0]}")
        else:
            print("    No WLAN interfaces found")
    except subprocess.CalledProcessError:
        print("    No WLAN interfaces found")

if __name__ == '__main__':
    # Parse command line arguments
    mode = "default"
    if len(sys.argv) > 1:
        if sys.argv[1] == "modules":
            mode = "modules"
        elif sys.argv[1] == "clean":
            mode = "clean"
        elif sys.argv[1] == "help" or sys.argv[1] == "-h" or sys.argv[1] == "--help":
            usage_print()
        else:
            print("Unknown mode: " + sys.argv[1])
            usage_print()
    
    # Show current status before stopping
    checkModuleStatus()
    print("")
    
    # Execute based on mode
    if mode == "modules":
        stopModulesOnly()
    elif mode == "clean":
        cleanAll()
    else:
        stopAll()
    
    print("\nDone.")