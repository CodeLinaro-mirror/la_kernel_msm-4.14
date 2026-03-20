#!/bin/bash

# Auto-detect nrc_pkg path based on script location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NRC_PKG_PATH="$(cd "${SCRIPT_DIR}/../.." && pwd)"

sudo killall -9 hostapd
sudo rmmod nrc
sleep 2
sudo cp "${NRC_PKG_PATH}/sw/firmware/nrc7292_cspi.bin" /lib/firmware/uni_s1g.bin
sudo insmod "${NRC_PKG_PATH}/sw/driver/nrc.ko" power_save=0 fw_name=uni_s1g.bin
sleep 5
sudo ifconfig wlan0 0.0.0.0
sudo ifconfig eth0 0.0.0.0
sleep 5
"${NRC_PKG_PATH}/script/cli_app" set txpwr 17
sudo hostapd ap_sdk_map.conf -ddddd | tee hostap.log
