#!/system/bin/sh
# NRC Modular Driver Stop Script for Android
# Manual control version (no Android WiFi Service dependency)

TEMP_HOSTAPD_CONF="/data/nrc_pkg/temp_hostapd.conf"
TEMP_WPA_CONF="/data/nrc_pkg/temp_wpa_supplicant.conf"
WPA_CTRL_DIR="/data/local/tmp/wpa_supplicant"
WPA_PID_FILE="/data/local/tmp/wpa_supplicant.pid"
HOSTAPD_CTRL_DIR="/data/local/tmp/hostapd"
HOSTAPD_PID_FILE="/data/local/tmp/hostapd.pid"

echo "=================================="
echo "Stopping NRC Modular Driver"
echo "=================================="

# Check if modules are loaded
if ! lsmod | grep -q nrc; then
    echo "No NRC modules loaded"
    # Still try to stop services
fi

# Step 1: Stop services
echo "[1] Stopping services..."

# Stop wpa_supplicant
if [ -f "$WPA_PID_FILE" ]; then
    kill $(cat "$WPA_PID_FILE") 2>/dev/null
    rm -f "$WPA_PID_FILE"
fi
if [ -f /data/misc/wifi/wpa_supplicant.pid ]; then
    kill $(cat /data/misc/wifi/wpa_supplicant.pid) 2>/dev/null
    rm -f /data/misc/wifi/wpa_supplicant.pid
fi
killall wpa_supplicant 2>/dev/null

# Stop hostapd
if [ -f "$HOSTAPD_PID_FILE" ]; then
    kill $(cat "$HOSTAPD_PID_FILE") 2>/dev/null
    rm -f "$HOSTAPD_PID_FILE"
fi
if [ -f /data/misc/wifi/hostapd.pid ]; then
    kill $(cat /data/misc/wifi/hostapd.pid) 2>/dev/null
    rm -f /data/misc/wifi/hostapd.pid
fi
killall hostapd 2>/dev/null

sleep 1

# Clean up temporary files
rm -f "$TEMP_HOSTAPD_CONF" 2>/dev/null
rm -f "$TEMP_WPA_CONF" 2>/dev/null
rm -rf "$WPA_CTRL_DIR" 2>/dev/null
rm -rf "$HOSTAPD_CTRL_DIR" 2>/dev/null

# Step 2: Bring down interface
echo "[2] Bringing down wlan0..."
ip link set wlan0 down 2>/dev/null
sleep 1

# Step 3: Unload modules in reverse order
echo "[3] Unloading modules..."

echo "[3.1] Removing nrc_wlan.ko..."
rmmod nrc_wlan 2>/dev/null
if [ $? -ne 0 ]; then
    echo "Warning: Failed to remove nrc_wlan"
fi
sleep 1

echo "[3.2] Removing nrc_core.ko..."
rmmod nrc_core 2>/dev/null
if [ $? -ne 0 ]; then
    echo "Warning: Failed to remove nrc_core"
fi
sleep 1

echo "[3.3] Removing nrc_spi.ko..."
rmmod nrc_spi 2>/dev/null
if [ $? -ne 0 ]; then
    echo "Warning: Failed to remove nrc_spi"
fi
sleep 1

# Step 4: Verify cleanup
echo "[4] Verifying cleanup..."
if lsmod | grep -q nrc; then
    echo "Warning: Some NRC modules still loaded:"
    lsmod | grep nrc
else
    echo "All NRC modules unloaded successfully"
fi

echo "=================================="
echo "Done"
echo "=================================="
