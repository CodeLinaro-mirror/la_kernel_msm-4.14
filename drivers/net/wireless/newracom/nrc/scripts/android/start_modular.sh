#!/system/bin/sh
# NRC Modular Driver Start Script for Android
# Manual wpa_supplicant control version (no Android WiFi Service dependency)

# Default paths
DRIVER_PATH="/data/nrc_pkg/sw/driver"
FW_PATH="/data/nrc_pkg/sw/firmware"
SCRIPT_PATH="/data/nrc_pkg/script"
CONF_BASE="/data/nrc_pkg/script/conf"

# Configuration file paths (will be selected based on country)
WPA_SUPPLICANT_CONF=""
HOSTAPD_CONF=""
TEMP_WPA_CONF="/data/nrc_pkg/temp_wpa_supplicant.conf"
TEMP_HOSTAPD_CONF="/data/nrc_pkg/temp_hostapd.conf"

# wpa_supplicant control paths
WPA_CTRL_DIR="/data/local/tmp/wpa_supplicant"
WPA_PID_FILE="/data/local/tmp/wpa_supplicant.pid"

# Default Configuration
##################################################################################
# Firmware Conf.
MODEL=7394
FW_DOWNLOAD=1
FW_NAME="uni_s1g.bin"
##################################################################################
# DEBUG Conf.
DRIVER_DEBUG=0
DBG_FLOW_CONTROL=0
##################################################################################
# CSPI Conf.
SPI_CLOCK=20000000
SPI_BUS_NUM=0
SPI_CS_NUM=0
SPI_GPIO_IRQ=5
SPI_POLLING_INTERVAL=0
FT232H_USB_SPI=0
##################################################################################
# RF Conf.
MAX_TXPWR=24
BD_NAME=""  # Empty means auto-select: nrc7394_bd.dat
##################################################################################
# PHY Conf.
GUARD_INT="auto"  # 'auto' or 'long' or 'short'
##################################################################################
# MAC Conf.
SHORT_BCN_ENABLE=0
LEGACY_ACK_ENABLE=0
AUTH_CONTROL_ENABLE=0
AUTH_CONTROL_SLOT=100
AUTH_CONTROL_SCALE=10
AUTH_CONTROL_TI_MIN=8
AUTH_CONTROL_TI_MAX=64
BEACON_BYPASS_ENABLE=0
AMPDU_ENABLE=2  # 0(disable) 1(manual) 2(auto)
NDP_ACK_1M=0
NDP_PREQ=0
CQM_ENABLE=1
##################################################################################
# Power Save (STA Only)
POWER_SAVE=0       # 0(Always on) 2(Deep_Sleep TIM) 3(Deep_Sleep nonTIM)
PS_TIMEOUT="3s"
SLEEP_DURATION="3s"
LISTEN_INTERVAL=1000
IDLE_MODE=0
##################################################################################
# BSS MAX IDLE PERIOD (AP Only)
BSS_MAX_IDLE_ENABLE=1
BSS_MAX_IDLE=1800
##################################################################################
# SW encryption/decryption
SW_ENC=0  # 0(HW) 1(SW) 2(HYBRID)
##################################################################################
# Self configuration (AP Only)
SELF_CONFIG=0
PREFER_BW=0
DWELL_TIME=100
##################################################################################
# Misc
DISCARD_DEAUTH=0
BITMAP_ENCODING=1
REVERSE_SCRAMBLER=1
SUPPORT_CH_WIDTH=1  # 0(1/2MHz) 1(1/2/4MHz)
POWER_SAVE_PRETEND=0
DUTY_CYCLE_ENABLE=0
DUTY_CYCLE_WINDOW=0
DUTY_CYCLE_DURATION=0
CCA_THRESHOLD=-75
##################################################################################
# TWT
TWT_INT=0
TWT_NUM=0
TWT_SP=0
TWT_FORCE_SLEEP=0
TWT_NUM_IN_GROUP=1
TWT_ALGO=0
##################################################################################
# EEPROM
USE_EEPROM_CONFIG=0
##################################################################################
# Network Configuration (STA mode)
STATIC_IP="192.168.100.10"
STATIC_NETMASK="24"
STATIC_GATEWAY="192.168.100.1"
STATIC_DNS1="8.8.8.8"
STATIC_DNS2="8.8.4.4"
##################################################################################

# Usage
usage() {
    echo "Usage:"
    echo "  $0 [sta_type] [security_mode] [country] [channel] [params...]"
    echo ""
    echo "Arguments:"
    echo "  sta_type      [0:STA | 1:AP]"
    echo "  security_mode [0:Open | 1:WPA2-PSK | 3:WPA3-SAE]"
    echo "  country       [US:USA | JP:Japan | TW:Taiwan | AU:Australia | NZ:New Zealand |"
    echo "                 K1:Korea-USN | K2:Korea-MIC | SG:Singapore |"
    echo "                 EU countries (AT,BE,BG,CY,CZ,DE,DK,EE,ES,FI,FR,GR,HR,HU,IE,IT,LT,LU,LV,MT,NL,PL,PT,RO,SE,SI,SK,GB,SA)]"
    echo "  channel       [S1G Channel Number] * Only for AP mode"
    echo ""
    echo "Optional Parameters (key=value format):"
    echo "  power_save=N, ps_timeout=Ns, listen_interval=N, max_txpwr=N, etc."
    echo ""
    echo "Examples:"
    echo "  $0 0 0 US                    # STA Open mode"
    echo "  $0 0 1 US                    # STA WPA2 mode"
    echo "  $0 0 3 US                    # STA WPA3-SAE mode"
    echo "  $0 1 1 US 40                 # AP WPA2 mode on channel 40"
    echo "  $0 0 1 US power_save=2       # STA WPA2 with power save"
    echo "  $0 1 1 US 40 max_txpwr=20    # AP WPA2 with custom TX power"
    exit 1
}

# Helper function to check and load cfg80211
ensure_cfg80211_loaded() {
    echo "Checking cfg80211 module..."

    # Check if cfg80211 is loaded (as module or built-in)
    if lsmod | grep -q cfg80211 || [ -d /sys/module/cfg80211 ]; then
        if [ -d /sys/module/cfg80211 ]; then
            echo "  cfg80211 is built-in to kernel"
        else
            echo "  cfg80211 module is loaded"
        fi
        return 0
    fi

    echo "  cfg80211 not loaded, attempting to load..."

    # Try to find and load cfg80211
    local cfg80211_path=""
    for path in /system/lib/modules /vendor/lib/modules /lib/modules/$(uname -r) /data/nrc_pkg/sw/driver; do
        if [ -f "$path/cfg80211.ko" ]; then
            cfg80211_path="$path/cfg80211.ko"
            break
        fi
    done

    if [ -n "$cfg80211_path" ]; then
        echo "  Found cfg80211 at: $cfg80211_path"
        insmod "$cfg80211_path" 2>/dev/null
        if [ $? -eq 0 ]; then
            echo "  cfg80211 loaded successfully"
            sleep 2
            return 0
        else
            echo "  Failed to load cfg80211"
            return 1
        fi
    else
        echo "  WARNING: cfg80211.ko not found as module"
        echo "  Checking if built-in to kernel..."
        if [ -d /sys/module/cfg80211 ]; then
            echo "  cfg80211 is built-in to kernel (OK)"
            return 0
        fi
        echo "  ERROR: cfg80211 not available!"
        return 1
    fi
}

# Helper function to set and verify regulatory domain
set_regulatory_domain() {
    local country=$1

    echo "Setting regulatory domain to: $country"

    # Set country code (single attempt)
    iw reg set $country 2>/dev/null
    sleep 1

    # Verify if country was set
    local current_cc=$(iw reg get | grep "^country" | head -1 | awk '{print $2}' | tr -d ':')

    if [ "$current_cc" = "$country" ]; then
        echo "  SUCCESS: Regulatory domain set to '$current_cc'"
        return 0
    elif [ "$current_cc" = "00" ]; then
        echo "  ERROR: Country still '00' - regulatory domain not set"
    else
        echo "  WARNING: Regulatory domain is '$current_cc' (requested: $country)"
        if [ "$current_cc" != "" ]; then
            return 0
        fi
    fi

    echo "  Current regulatory domain:"
    iw reg get | head -5
    return 1
}

# Check arguments
if [ $# -lt 3 ]; then
    usage
fi

STA_TYPE=$1
SECURITY_MODE=$2
COUNTRY=$3
CHANNEL=""

# Convert sta_type number to mode string
case "$STA_TYPE" in
    0)
        MODE="sta"
        ;;
    1)
        MODE="ap"
        ;;
    *)
        echo "Error: Invalid sta_type '$STA_TYPE'"
        echo "Use: 0(STA) or 1(AP)"
        usage
        ;;
esac

# Convert security_mode number to security string
case "$SECURITY_MODE" in
    0)
        SECURITY="open"
        ;;
    1)
        SECURITY="wpa2"
        ;;
    2)
        SECURITY="owe"
        ;;
    3)
        SECURITY="wpa3"
        ;;
    4)
        SECURITY="pbc"
        ;;
    *)
        echo "Error: Invalid security_mode '$SECURITY_MODE'"
        echo "Use: 0(Open), 1(WPA2-PSK), 2(WPA3-OWE), 3(WPA3-SAE), 4(WPS-PBC)"
        usage
        ;;
esac

# Parse positional and optional arguments
shift 3
while [ $# -gt 0 ]; do
    # Check if argument is a number (channel) or key=value pair
    if echo "$1" | grep -qE '^[0-9]+$'; then
        # It's a number - treat as channel if not set yet
        if [ -z "$CHANNEL" ]; then
            CHANNEL=$1
        fi
    elif echo "$1" | grep -q '='; then
        # Parse key=value parameters
        KEY="${1%%=*}"
        VALUE="${1#*=}"
        case "$KEY" in
            power_save) POWER_SAVE=$VALUE ;;
            ps_timeout) PS_TIMEOUT=$VALUE ;;
            sleep_duration) SLEEP_DURATION=$VALUE ;;
            listen_interval) LISTEN_INTERVAL=$VALUE ;;
            idle_mode) IDLE_MODE=$VALUE ;;
            max_txpwr) MAX_TXPWR=$VALUE ;;
            ampdu_enable) AMPDU_ENABLE=$VALUE ;;
            cqm_enable) CQM_ENABLE=$VALUE ;;
            ndp_preq) NDP_PREQ=$VALUE ;;
            ndp_ack_1m) NDP_ACK_1M=$VALUE ;;
            sw_enc) SW_ENC=$VALUE ;;
            bss_max_idle) BSS_MAX_IDLE=$VALUE ;;
            bss_max_idle_enable) BSS_MAX_IDLE_ENABLE=$VALUE ;;
            guard_int) GUARD_INT=$VALUE ;;
            support_ch_width) SUPPORT_CH_WIDTH=$VALUE ;;
            driver_debug) DRIVER_DEBUG=$VALUE ;;
            dbg_flow_control) DBG_FLOW_CONTROL=$VALUE ;;
            spi_clock) SPI_CLOCK=$VALUE ;;
            spi_gpio_irq) SPI_GPIO_IRQ=$VALUE ;;
            spi_polling_interval) SPI_POLLING_INTERVAL=$VALUE ;;
            short_bcn_enable) SHORT_BCN_ENABLE=$VALUE ;;
            legacy_ack_enable) LEGACY_ACK_ENABLE=$VALUE ;;
            beacon_bypass_enable) BEACON_BYPASS_ENABLE=$VALUE ;;
            discard_deauth) DISCARD_DEAUTH=$VALUE ;;
            bitmap_encoding) BITMAP_ENCODING=$VALUE ;;
            reverse_scrambler) REVERSE_SCRAMBLER=$VALUE ;;
            power_save_pretend) POWER_SAVE_PRETEND=$VALUE ;;
            duty_cycle_enable) DUTY_CYCLE_ENABLE=$VALUE ;;
            duty_cycle_window) DUTY_CYCLE_WINDOW=$VALUE ;;
            duty_cycle_duration) DUTY_CYCLE_DURATION=$VALUE ;;
            cca_threshold) CCA_THRESHOLD=$VALUE ;;
            twt_int) TWT_INT=$VALUE ;;
            twt_num) TWT_NUM=$VALUE ;;
            twt_sp) TWT_SP=$VALUE ;;
            bd_name) BD_NAME=$VALUE ;;
            fw_name) FW_NAME=$VALUE ;;
            *)
                echo "Warning: Unknown parameter '$KEY'"
                ;;
        esac
    else
        echo "Warning: Unrecognized argument '$1'"
    fi
    shift
done

# Validate mode
case "$MODE" in
    sta|STA)
        MODE="sta"
        ;;
    ap|AP)
        MODE="ap"
        ;;
    *)
        echo "Error: Invalid mode '$MODE'"
        usage
        ;;
esac

# Validate security
case "$SECURITY" in
    open|OPEN)
        SECURITY="open"
        ;;
    wpa2|WPA2)
        SECURITY="wpa2"
        ;;
    wpa3|WPA3|sae|SAE)
        SECURITY="wpa3"
        ;;
    *)
        echo "Error: Invalid security mode '$SECURITY'"
        usage
        ;;
esac

# Set BD name if not specified
if [ -z "$BD_NAME" ]; then
    BD_NAME="nrc${MODEL}_bd.dat"
fi

# Mode-specific parameter adjustments
if [ "$MODE" = "ap" ]; then
    NDP_PREQ=1
fi

echo "=================================="
echo "NRC Modular Driver for Android"
echo "=================================="
echo "Model:             $MODEL"
echo "STA Type:          $STA_TYPE ($MODE)"
echo "Security Mode:     $SECURITY_MODE ($SECURITY)"
echo "Country:           $COUNTRY"
[ -n "$CHANNEL" ] && echo "Channel:           $CHANNEL"
echo "FW Name:           $FW_NAME"
echo "BD Name:           $BD_NAME"
echo "Max TX Power:      ${MAX_TXPWR} dBm"
echo "AMPDU:             $AMPDU_ENABLE (0:off,1:manual,2:auto)"
echo "Guard Interval:    $GUARD_INT"
if [ "$MODE" = "sta" ]; then
    echo "Power Save:        $POWER_SAVE (0:off,2:TIM,3:nonTIM)"
    if [ $POWER_SAVE -gt 0 ]; then
        echo "  PS Timeout:      $PS_TIMEOUT"
        if [ $POWER_SAVE -eq 3 ]; then
            echo "  Sleep Duration:  $SLEEP_DURATION"
        fi
        echo "  Listen Interval: $LISTEN_INTERVAL"
    fi
    echo "Idle Mode:         $IDLE_MODE"
    echo "CQM Enable:        $CQM_ENABLE"
    echo "Support CH Width:  $SUPPORT_CH_WIDTH (0:1/2MHz,1:1/2/4MHz)"
fi
if [ "$MODE" = "ap" ] || [ "$MODE" = "sta" ]; then
    echo "BSS Max Idle:      $BSS_MAX_IDLE_ENABLE"
    if [ $BSS_MAX_IDLE_ENABLE -eq 1 ]; then
        echo "  Max Idle Period: $BSS_MAX_IDLE"
    fi
fi
echo "SW Encryption:     $SW_ENC (0:HW,1:SW,2:HYBRID)"
echo "=================================="

# Step 1: Clean up
echo "[1] Cleaning up..."

# Kill any existing wpa_supplicant/hostapd
killall wpa_supplicant 2>/dev/null
killall hostapd 2>/dev/null
rm -f "$WPA_PID_FILE" 2>/dev/null
rm -f /data/misc/wifi/wpa_supplicant.pid 2>/dev/null
rm -f /data/misc/wifi/hostapd.pid 2>/dev/null

# Remove kernel modules
rmmod nrc_wlan 2>/dev/null
rmmod nrc_core 2>/dev/null
rmmod nrc_spi 2>/dev/null

# Bring down interface
ip link set wlan0 down 2>/dev/null
sleep 1

# Step 2: Prepare regulatory domain
echo "[2] Preparing regulatory domain..."

# Ensure cfg80211 is loaded
if ! ensure_cfg80211_loaded; then
    echo ""
    echo "========================================="
    echo "CRITICAL ERROR: cfg80211 not available"
    echo "========================================="
    echo "cfg80211 module is required for regulatory domain support."
    echo ""
    echo "Solutions:"
    echo "1. Copy cfg80211.ko to /data/nrc_pkg/sw/driver/"
    echo "2. Ensure cfg80211.ko matches kernel version: $(uname -r)"
    echo "3. Check kernel config: CONFIG_CFG80211=m"
    echo ""
    echo "Cannot proceed without cfg80211!"
    echo "========================================="
    exit 1
fi

# Pre-set regulatory domain (attempt to set before driver load)
echo "[2.1] Pre-setting regulatory domain..."
if ! set_regulatory_domain $COUNTRY; then
    echo ""
    echo "========================================="
    echo "WARNING: Regulatory domain setup failed"
    echo "========================================="
    echo "Country code could not be set via iw reg set: $COUNTRY"
    echo ""
    echo "This is normal on Android devices without regulatory.db support."
    echo "Will use driver parameter nrc_country_code=$COUNTRY instead."
    echo "========================================="
    echo ""
fi

# AP mode parameter adjustments (matching start_modular.py behavior)
if [ "$MODE" = "ap" ]; then
    echo "[2.2] Adjusting parameters for AP mode..."
    # Enable NDP probe request for AP mode (required for proper AP operation)
    # This matches the setAPParam() function in start_modular.py (line 561-564)
    NDP_PREQ=1
    echo "  - Set NDP_PREQ=1 (NDP Probe Request enabled for AP)"
fi

# Step 2.5: Copy firmware and BD files to /lib/firmware/
echo "[2.5] Copying firmware and BD files..."
LIB_FW_PATH="/lib/firmware"

# Create /lib/firmware directory if it doesn't exist
if [ ! -d "$LIB_FW_PATH" ]; then
    echo "  Creating $LIB_FW_PATH directory..."
    mkdir -p "$LIB_FW_PATH"
fi

# Remount /lib/firmware as read-write (Android specific)
echo "  Remounting $LIB_FW_PATH as read-write..."
mount -o remount,rw "$LIB_FW_PATH" 2>/dev/null
if [ $? -ne 0 ]; then
    echo "  Attempting to remount root partition..."
    mount -o remount,rw / 2>/dev/null
fi

# Copy firmware file
if [ -f "$FW_PATH/$FW_NAME" ]; then
    cp "$FW_PATH/$FW_NAME" "$LIB_FW_PATH/" 2>/dev/null
    if [ $? -eq 0 ]; then
        echo "  Copied $FW_NAME to $LIB_FW_PATH/"
    else
        echo "  WARNING: Failed to copy $FW_NAME"
    fi
else
    echo "  WARNING: Firmware file $FW_PATH/$FW_NAME not found"
fi

# Copy BD file
if [ -f "$FW_PATH/$BD_NAME" ]; then
    cp "$FW_PATH/$BD_NAME" "$LIB_FW_PATH/" 2>/dev/null
    if [ $? -eq 0 ]; then
        echo "  Successfully copied $BD_NAME to $LIB_FW_PATH/"
        ls -l "$LIB_FW_PATH/$BD_NAME"
    else
        echo "  ERROR: Failed to copy BD file (permission denied?)"
        echo "  Trying alternative method with cat..."
        cat "$FW_PATH/$BD_NAME" > "$LIB_FW_PATH/$BD_NAME" 2>/dev/null
        if [ $? -eq 0 ]; then
            echo "  Successfully copied $BD_NAME using cat"
        else
            echo "  ERROR: All copy methods failed"
            exit 1
        fi
    fi
else
    echo "  ERROR: BD file $FW_PATH/$BD_NAME not found"
    echo "  Available files in $FW_PATH:"
    ls -l "$FW_PATH/"*.dat 2>/dev/null
    exit 1
fi

# Verify BD file exists and has content
if [ -f "$LIB_FW_PATH/$BD_NAME" ] && [ -s "$LIB_FW_PATH/$BD_NAME" ]; then
    BD_SIZE=$(stat -c%s "$LIB_FW_PATH/$BD_NAME" 2>/dev/null || wc -c < "$LIB_FW_PATH/$BD_NAME")
    echo "  BD file verified: $BD_NAME (size: $BD_SIZE bytes)"
else
    echo "  ERROR: BD file copy verification failed"
    exit 1
fi

# Optionally remount back to read-only for security
# mount -o remount,ro "$LIB_FW_PATH" 2>/dev/null

# Step 3: Load modules
echo "[3] Loading NRC modular driver..."

# Load mac80211 if not already loaded (required for nrc_wlan)
if ! lsmod | grep -q mac80211; then
    echo "[3.0] Loading mac80211.ko..."
    modprobe mac80211 2>/dev/null
    if [ $? -eq 0 ]; then
        echo "  mac80211 loaded successfully"
        sleep 1
    else
        echo "  WARNING: Failed to load mac80211"
    fi
fi

# Configure power save GPIO if needed
PS_GPIO_ARG=""
if [ "$MODE" = "sta" ] && [ \( $POWER_SAVE -gt 0 \) -o \( $IDLE_MODE -eq 1 \) ]; then
    if [ "$MODEL" = "7394" ]; then
        # Check kernel version
        KERNEL_VER=$(uname -r | cut -d'.' -f1,2)
        if [ "${KERNEL_VER#*.}" -ge 6 ]; then
            if [ $USE_EEPROM_CONFIG -eq 1 ]; then
                PS_GPIO_ARG="power_save_gpio=527,14,1"
            else
                PS_GPIO_ARG="power_save_gpio=529,14,1"
            fi
        else
            if [ $USE_EEPROM_CONFIG -eq 1 ]; then
                PS_GPIO_ARG="power_save_gpio=15,12,1"
            else
                PS_GPIO_ARG="power_save_gpio=17,14,1"
            fi
        fi
    fi
fi

# Load SPI backend module
echo "[3.1] Loading nrc_spi.ko..."
SPI_PARAMS="hifspeed=$SPI_CLOCK spi_bus_num=$SPI_BUS_NUM spi_cs_num=$SPI_CS_NUM spi_gpio_irq=$SPI_GPIO_IRQ spi_polling_interval=$SPI_POLLING_INTERVAL"
if [ -n "$PS_GPIO_ARG" ]; then
    SPI_PARAMS="$SPI_PARAMS $PS_GPIO_ARG"
fi
echo "  Parameters: $SPI_PARAMS"
insmod $DRIVER_PATH/nrc_spi.ko $SPI_PARAMS
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to load nrc_spi.ko"
    exit 1
fi
sleep 2

# Load HAL core module
echo "[3.2] Loading nrc_core.ko..."
insmod $DRIVER_PATH/nrc_core.ko
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to load nrc_core.ko"
    rmmod nrc_spi
    exit 1
fi
sleep 2

# Build WLAN module parameters
WLAN_PARAMS=""

# Firmware download
if [ $FW_DOWNLOAD -eq 1 ]; then
    WLAN_PARAMS="$WLAN_PARAMS fw_name=$FW_NAME"
fi

# Board data
WLAN_PARAMS="$WLAN_PARAMS bd_name=$BD_NAME"

# Power save parameters (STA only)
if [ "$MODE" = "sta" ] && [ $POWER_SAVE -gt 0 ]; then
    WLAN_PARAMS="$WLAN_PARAMS power_save=$POWER_SAVE"
    if [ $POWER_SAVE -eq 3 ]; then
        # Extract number and unit from sleep_duration
        SLEEP_NUM=$(echo $SLEEP_DURATION | sed 's/[^0-9]*//g')
        SLEEP_UNIT=$(echo $SLEEP_DURATION | sed 's/[0-9]*//g')
        if [ "$SLEEP_UNIT" = "m" ]; then
            WLAN_PARAMS="$WLAN_PARAMS sleep_duration=${SLEEP_NUM},0"
        else
            WLAN_PARAMS="$WLAN_PARAMS sleep_duration=${SLEEP_NUM},1"
        fi
    fi
fi

# Idle mode (STA only)
if [ "$MODE" = "sta" ] && [ $IDLE_MODE -eq 1 ]; then
    WLAN_PARAMS="$WLAN_PARAMS idle_mode=1"
fi

# BSS max idle
if [ $BSS_MAX_IDLE_ENABLE -eq 1 ]; then
    if [ "$MODE" = "ap" ] || [ "$MODE" = "sta" ]; then
        WLAN_PARAMS="$WLAN_PARAMS bss_max_idle=$BSS_MAX_IDLE"
    fi
fi

# NDP probe request
if [ $NDP_PREQ -eq 1 ]; then
    WLAN_PARAMS="$WLAN_PARAMS ndp_preq=1"
fi

# 1MHz NDP ACK
if [ $NDP_ACK_1M -eq 1 ]; then
    WLAN_PARAMS="$WLAN_PARAMS ndp_ack_1m=1"
fi

# AMPDU mode
if [ $AMPDU_ENABLE -ne 2 ]; then
    WLAN_PARAMS="$WLAN_PARAMS ampdu_mode=$AMPDU_ENABLE"
fi

# SW encryption
if [ $SW_ENC -gt 0 ]; then
    WLAN_PARAMS="$WLAN_PARAMS sw_enc=$SW_ENC"
fi

# CQM (STA only)
if [ "$MODE" = "sta" ] && [ $CQM_ENABLE -eq 0 ]; then
    WLAN_PARAMS="$WLAN_PARAMS disable_cqm=1"
fi

# Short beacon
if [ $SHORT_BCN_ENABLE -eq 0 ]; then
    WLAN_PARAMS="$WLAN_PARAMS enable_short_bi=0"
fi

# Legacy ACK
if [ $LEGACY_ACK_ENABLE -eq 1 ]; then
    WLAN_PARAMS="$WLAN_PARAMS enable_legacy_ack=1"
fi

# Authentication control (AP only)
if [ "$MODE" = "ap" ] && [ $AUTH_CONTROL_ENABLE -eq 1 ]; then
    WLAN_PARAMS="$WLAN_PARAMS set_auth_control=${AUTH_CONTROL_ENABLE},${AUTH_CONTROL_SLOT},${AUTH_CONTROL_TI_MIN},${AUTH_CONTROL_TI_MAX},${AUTH_CONTROL_SCALE}"
fi

# Beacon bypass (STA only)
if [ "$MODE" = "sta" ] && [ $BEACON_BYPASS_ENABLE -eq 1 ]; then
    WLAN_PARAMS="$WLAN_PARAMS enable_beacon_bypass=1"
fi

# Listen interval (STA only)
if [ "$MODE" = "sta" ] && [ $LISTEN_INTERVAL -gt 0 ]; then
    WLAN_PARAMS="$WLAN_PARAMS listen_interval=$LISTEN_INTERVAL"
fi

# Korea band
if [ "$COUNTRY" = "K1" ]; then
    WLAN_PARAMS="$WLAN_PARAMS kr_band=1"
elif [ "$COUNTRY" = "K2" ]; then
    WLAN_PARAMS="$WLAN_PARAMS kr_band=2"
fi

# Discard deauth (test only)
if [ $DISCARD_DEAUTH -eq 1 ]; then
    WLAN_PARAMS="$WLAN_PARAMS discard_deauth=1"
fi

# Driver debug
if [ $DRIVER_DEBUG -eq 1 ]; then
    WLAN_PARAMS="$WLAN_PARAMS debug_level_all=1"
fi

# Flow control debug
if [ $DBG_FLOW_CONTROL -eq 1 ]; then
    WLAN_PARAMS="$WLAN_PARAMS debug_level_all=1 dbg_flow_control=1"
fi

# Bitmap encoding
if [ $BITMAP_ENCODING -eq 0 ]; then
    WLAN_PARAMS="$WLAN_PARAMS bitmap_encoding=0"
fi

# Reverse scrambler
if [ $REVERSE_SCRAMBLER -eq 0 ]; then
    WLAN_PARAMS="$WLAN_PARAMS reverse_scrambler=0"
fi

# Supported channel width (STA only)
if [ "$MODE" = "sta" ] && [ $SUPPORT_CH_WIDTH -eq 0 ]; then
    WLAN_PARAMS="$WLAN_PARAMS support_ch_width=0"
fi

# Power save pretend
if [ $POWER_SAVE_PRETEND -eq 1 ]; then
    WLAN_PARAMS="$WLAN_PARAMS ps_pretend=1"
fi

# Duty cycle
if [ $DUTY_CYCLE_ENABLE -eq 1 ]; then
    WLAN_PARAMS="$WLAN_PARAMS set_duty_cycle=${DUTY_CYCLE_ENABLE},${DUTY_CYCLE_WINDOW},${DUTY_CYCLE_DURATION}"
fi

# CCA threshold
WLAN_PARAMS="$WLAN_PARAMS set_cca_threshold=$CCA_THRESHOLD"

# TWT
if [ $TWT_NUM -gt 0 ] || [ $TWT_SP -gt 0 ] || [ $TWT_INT -gt 0 ]; then
    WLAN_PARAMS="$WLAN_PARAMS twt_num=$TWT_NUM twt_sp=$TWT_SP twt_int=$TWT_INT twt_force_sleep=$TWT_FORCE_SLEEP twt_num_in_group=$TWT_NUM_IN_GROUP twt_algo=$TWT_ALGO"
fi

# Country code - Add nrc_country_code parameter for Android regulatory domain
WLAN_PARAMS="$WLAN_PARAMS nrc_country_code=$COUNTRY"

# Load WLAN frontend module
echo "[3.3] Loading nrc_wlan.ko..."
echo "  Parameters: $WLAN_PARAMS"
insmod $DRIVER_PATH/nrc_wlan.ko $WLAN_PARAMS
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to load nrc_wlan.ko"
    rmmod nrc_core
    rmmod nrc_spi
    exit 1
fi
sleep 5

# Step 4: Bring up interface
echo "[4] Bringing up wlan0..."
ip link set wlan0 up
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to bring up wlan0"
    rmmod nrc_wlan
    rmmod nrc_core
    rmmod nrc_spi
    exit 1
fi
sleep 2

# Step 5: Verify regulatory domain after driver load
echo "[5] Verifying regulatory domain after driver load..."
# Note: On Android without regulatory.db, iw reg get may still show "country 00"
# but the driver will use the nrc_country_code parameter internally
set_regulatory_domain $COUNTRY || echo "  Note: Using driver parameter nrc_country_code=$COUNTRY"

# Step 6: Set TX power and configure TX power vector table
echo "[6] Setting TX power: ${MAX_TXPWR} dBm..."

# Set TX power limit via iw (this may not work on all Android devices)
iw phy phy0 set txpower limit $((MAX_TXPWR * 100)) 2>/dev/null || echo "  Warning: iw set txpower not supported"

# Configure TX power vector table via sysfs (Android workaround for missing cli_app)
# This ensures TX power is properly configured even without regulatory domain support
SYSFS_TXPWR="/sys/kernel/debug/ieee80211/phy0/nrc80211/txpwr_limit"
if [ -f "$SYSFS_TXPWR" ]; then
    echo "  Setting TX power via sysfs..."
    echo $MAX_TXPWR > "$SYSFS_TXPWR" 2>/dev/null
    if [ $? -eq 0 ]; then
        echo "  ✓ TX power configured: $MAX_TXPWR dBm (via sysfs)"
    else
        echo "  ✗ Failed to set TX power via sysfs (permission denied?)"
    fi
else
    # Fallback: Use debugfs direct write for TX power vector configuration
    # This is Android-specific workaround when regulatory domain is not available
    echo "  Configuring TX power vector table for country: $COUNTRY"

    # Map country code to internal CC value
    case "$COUNTRY" in
        US) CC_VALUE=1 ;;
        JP) CC_VALUE=2 ;;
        K1) CC_VALUE=3 ;;
        EU|AT|BE|BG|CY|CZ|DE|DK|EE|ES|FI|FR|GR|HR|HU|IE|IT|LT|LU|LV|MT|NL|PL|PT|RO|SE|SI|SK|GB|SA)
            CC_VALUE=5 ;;
        NZ) CC_VALUE=7 ;;
        AU) CC_VALUE=8 ;;
        K2) CC_VALUE=9 ;;
        TW) CC_VALUE=13 ;;
        SG) CC_VALUE=14 ;;
        *)
            CC_VALUE=1  # Default to US
            echo "  Warning: Unknown country $COUNTRY, defaulting to US (CC=1)"
            ;;
    esac

    # Try to configure via debugfs
    DEBUGFS_BASE="/sys/kernel/debug/ieee80211/phy0/nrc80211"
    if [ -d "$DEBUGFS_BASE" ]; then
        # Set country code
        if [ -f "$DEBUGFS_BASE/country" ]; then
            echo "$COUNTRY" > "$DEBUGFS_BASE/country" 2>/dev/null
            echo "  Set country code: $COUNTRY (CC=$CC_VALUE)"
        fi

        # Set TX power limit
        if [ -f "$DEBUGFS_BASE/txpwr_limit" ]; then
            echo $MAX_TXPWR > "$DEBUGFS_BASE/txpwr_limit" 2>/dev/null
            echo "  ✓ TX power limit set: $MAX_TXPWR dBm"
        fi

        # Trigger TX power reconfiguration
        if [ -f "$DEBUGFS_BASE/recalc_txpwr" ]; then
            echo 1 > "$DEBUGFS_BASE/recalc_txpwr" 2>/dev/null
            echo "  Triggered TX power recalculation"
        fi
    else
        echo "  Warning: debugfs not available, TX power vector may not be configured"
        echo "  This may result in reduced TX power or no transmission"
    fi
fi

# Step 7: Set guard interval
echo "[7] Setting guard interval: $GUARD_INT..."
# Try to set guard interval via sysfs if available
SYSFS_GI="/sys/kernel/debug/ieee80211/phy0/nrc80211/gi"
if [ -f "$SYSFS_GI" ]; then
    case "$GUARD_INT" in
        auto) GI_VALUE=0 ;;
        long|lgi) GI_VALUE=1 ;;
        short|sgi) GI_VALUE=2 ;;
        *) GI_VALUE=0 ;;
    esac
    echo $GI_VALUE > "$SYSFS_GI" 2>/dev/null
    if [ $? -eq 0 ]; then
        echo "  ✓ Guard interval set: $GUARD_INT (value: $GI_VALUE)"
    fi
else
    echo "  Note: Guard interval configuration via sysfs not available"
fi

# Step 8: Set power save timeout (STA only)
if [ "$MODE" = "sta" ] && [ $POWER_SAVE -gt 0 ]; then
    echo "[8] Setting power save timeout: $PS_TIMEOUT..."
    # Extract number from PS_TIMEOUT (e.g., "3s" -> "3")
    PS_TIMEOUT_NUM=$(echo $PS_TIMEOUT | sed 's/[^0-9]*//g')
    iwconfig wlan0 power timeout ${PS_TIMEOUT_NUM}s 2>/dev/null || echo "  Warning: iwconfig power not supported"
fi

echo ""
echo "=================================="
echo "NRC driver loaded successfully!"
echo "=================================="
echo "Interface: wlan0"
echo "Status:"
ip link show wlan0
echo ""
echo "Regulatory domain:"
iw reg get | head -5
echo ""
echo "Loaded modules:"
lsmod | grep nrc
echo ""

# Step 9: Start STA mode using manual wpa_supplicant
if [ "$MODE" = "sta" ]; then
    echo "=================================="
    echo "[9] Starting STA mode (Manual wpa_supplicant)"
    echo "=================================="

    # Determine conf directory (check for EU countries)
    CONF_DIR="$CONF_BASE/$COUNTRY"
    if [ ! -d "$CONF_DIR" ]; then
        # Check if it's an EU country
        case "$COUNTRY" in
            AT|BE|BG|CY|CZ|DE|DK|EE|ES|FI|FR|GR|HR|HU|IE|IT|LT|LU|LV|MT|NL|PL|PT|RO|SE|SI|SK|GB|SA)
                CONF_DIR="$CONF_BASE/EU"
                ;;
            *)
                echo "ERROR: Configuration directory not found for country: $COUNTRY"
                echo "Available countries:"
                ls -d $CONF_BASE/*/ 2>/dev/null | xargs -n1 basename
                exit 1
                ;;
        esac
    fi

    # Select wpa_supplicant configuration file
    case "$SECURITY" in
        open)
            WPA_SUPPLICANT_CONF="$CONF_DIR/sta_halow_open.conf"
            ;;
        wpa2)
            WPA_SUPPLICANT_CONF="$CONF_DIR/sta_halow_wpa2.conf"
            ;;
        wpa3)
            WPA_SUPPLICANT_CONF="$CONF_DIR/sta_halow_sae.conf"
            ;;
        owe)
            WPA_SUPPLICANT_CONF="$CONF_DIR/sta_halow_owe.conf"
            ;;
        *)
            echo "ERROR: Unsupported security mode: $SECURITY"
            exit 1
            ;;
    esac

    if [ ! -f "$WPA_SUPPLICANT_CONF" ]; then
        echo "ERROR: Configuration file not found: $WPA_SUPPLICANT_CONF"
        exit 1
    fi

    echo "Using configuration: $WPA_SUPPLICANT_CONF"

    # Extract SSID from config file (exclude commented lines)
    SSID=$(sed -n '/^[ \t]*#/d; /ssid=/ {s/.*ssid="\([^"]*\)".*/\1/p; q}' "$WPA_SUPPLICANT_CONF")

    if [ -z "$SSID" ]; then
        echo "ERROR: Could not extract SSID from configuration"
        exit 1
    fi

    # Extract passphrase for WPA2/WPA3
    PASSPHRASE=""
    if [ "$SECURITY" = "wpa2" ] || [ "$SECURITY" = "wpa3" ]; then
        PASSPHRASE=$(sed -n '/^[ \t]*#/d; /psk=/ {s/.*psk="\([^"]*\)".*/\1/p; q}' "$WPA_SUPPLICANT_CONF")
        if [ -z "$PASSPHRASE" ]; then
            echo "ERROR: Could not extract passphrase from configuration"
            exit 1
        fi
    fi

    echo ""
    echo "Network Configuration:"
    echo "  SSID: $SSID"
    echo "  Security: $SECURITY"
    [ -n "$PASSPHRASE" ] && echo "  Passphrase: ****"
    echo ""

    # Find wpa_supplicant binary
    echo ""
    echo "Using iw-based connection (no wpa_supplicant)"
    echo ""

    echo "Scanning for AP: $SSID"
    echo ""

    # Scan and connect logic
    MAX_SCAN_ATTEMPTS=10
    SCAN_COUNT=0
    AP_FOUND=0
    CONNECTED=0

    while [ $SCAN_COUNT -lt $MAX_SCAN_ATTEMPTS ] && [ $CONNECTED -eq 0 ]; do
        SCAN_COUNT=$((SCAN_COUNT + 1))
        echo "Scan attempt $SCAN_COUNT/$MAX_SCAN_ATTEMPTS..."

        # Trigger scan
        iw dev wlan0 scan trigger 2>/dev/null
        sleep 2

        # Check scan results for our SSID
        SCAN_RESULT=$(iw dev wlan0 scan 2>/dev/null | grep -B5 -A8 "SSID: $SSID" | head -15)

        if [ -n "$SCAN_RESULT" ]; then
            AP_FOUND=1
            echo "  ✓ Found AP: $SSID"

            # Extract frequency/channel info
            FREQ=$(echo "$SCAN_RESULT" | grep "freq:" | head -1 | awk '{print $2}')
            SIGNAL=$(echo "$SCAN_RESULT" | grep "signal:" | head -1 | awk '{print $2, $3}')

            [ -n "$FREQ" ] && echo "    Frequency: $FREQ MHz"
            [ -n "$SIGNAL" ] && echo "    Signal: $SIGNAL"

            # Try to connect
            echo "  Connecting to $SSID..."

            if [ "$SECURITY" = "open" ]; then
                iw dev wlan0 connect "$SSID" 2>/dev/null
            elif [ "$SECURITY" = "wpa2" ] || [ "$SECURITY" = "wpa3" ]; then
                # For WPA2/WPA3, use key
                if [ -n "$PASSPHRASE" ]; then
                    iw dev wlan0 connect "$SSID" key 0:"$PASSPHRASE" 2>/dev/null
                else
                    iw dev wlan0 connect "$SSID" 2>/dev/null
                fi
            else
                iw dev wlan0 connect "$SSID" 2>/dev/null
            fi

            # Wait for connection to establish
            sleep 3

            # Check if connected
            if iw dev wlan0 link 2>/dev/null | grep -q "Connected"; then
                CONNECTED=1
                echo ""
                echo "=================================================="
                echo "Connected to AP!"
                echo "=================================================="
                iw dev wlan0 link | head -10
                break
            else
                echo "  Connection attempt failed, retrying..."
                sleep 2
            fi
        else
            echo "  AP not found in scan results"
            sleep 1
        fi
    done

    echo ""

    if [ $CONNECTED -eq 0 ]; then
        if [ $AP_FOUND -eq 0 ]; then
            echo "ERROR: AP '$SSID' not found after $MAX_SCAN_ATTEMPTS scan attempts"
            echo ""
            echo "Available networks:"
            iw dev wlan0 scan 2>/dev/null | grep "SSID:" | head -10
        else
            echo "ERROR: Found AP but failed to connect after $SCAN_COUNT attempts"
            echo ""
            echo "Connection status:"
            iw dev wlan0 link
        fi
        echo ""
        echo "Troubleshooting:"
        echo "  1. Check if AP is broadcasting: iw dev wlan0 scan | grep \"$SSID\""
        echo "  2. Try manual connection: iw dev wlan0 connect \"$SSID\""
        echo "  3. Check signal strength and channel"
        echo ""
        exit 1
    else
        echo ""
        echo "Requesting IP address via DHCP..."

        # Use our custom DHCP client
        DHCP_CLIENT="$DRIVER_PATH/../bin/dhcpc"
        DHCP_SUCCESS=1
        DHCP_GATEWAY=""

        if [ -x "$DHCP_CLIENT" ]; then
            echo "  Using custom DHCP client: $DHCP_CLIENT"

            # Try DHCP up to 3 times
            DHCP_RETRY=0
            MAX_DHCP_RETRY=3
            OFFERED_IP=""
            OFFERED_GATEWAY=""

            while [ $DHCP_RETRY -lt $MAX_DHCP_RETRY ] && [ $DHCP_SUCCESS -ne 0 ]; do
                if [ $DHCP_RETRY -gt 0 ]; then
                    echo "  Retry attempt $DHCP_RETRY/$MAX_DHCP_RETRY..."
                    sleep 2
                fi

                # Run DHCP client and capture output
                DHCP_OUTPUT=$("$DHCP_CLIENT" wlan0 2>&1)
                DHCP_SUCCESS=$?

                # Display DHCP output
                echo "$DHCP_OUTPUT"

                # Parse offered IP and gateway from DHCP OFFER
                OFFERED_IP=$(echo "$DHCP_OUTPUT" | grep "^Offered IP:" | awk '{print $3}')
                OFFERED_GATEWAY=$(echo "$DHCP_OUTPUT" | grep "^Gateway:" | awk '{print $2}')

                # Parse gateway from output (for backward compatibility)
                if [ -z "$OFFERED_GATEWAY" ]; then
                    DHCP_GATEWAY=$(echo "$DHCP_OUTPUT" | grep "^Gateway:" | awk '{print $2}')
                else
                    DHCP_GATEWAY="$OFFERED_GATEWAY"
                fi

                # Check if IP was actually assigned
                sleep 1
                CURRENT_IP=$(ip addr show wlan0 2>/dev/null | grep 'inet ' | awk '{print $2}')
                if [ -n "$CURRENT_IP" ]; then
                    DHCP_SUCCESS=0
                    echo "  ✓ DHCP configuration complete!"
                    break
                fi

                DHCP_RETRY=$((DHCP_RETRY + 1))
            done

            # If custom DHCP client failed but we got an offered IP, use it
            if [ $DHCP_SUCCESS -ne 0 ] && [ -n "$OFFERED_IP" ]; then
                echo ""
                echo "  DHCP ACK not received, but OFFER was received"
                echo "  Configuring interface with offered IP: $OFFERED_IP"

                # Determine netmask (default /24 for typical networks)
                NETMASK="24"

                # Add IP address
                ip addr add "$OFFERED_IP/$NETMASK" dev wlan0 2>/dev/null

                if [ $? -eq 0 ]; then
                    echo "  ✓ IP address configured: $OFFERED_IP/$NETMASK"

                    # Infer gateway if not provided (assume .1 in the same subnet)
                    if [ -z "$OFFERED_GATEWAY" ]; then
                        # Extract first 3 octets and append .1
                        OFFERED_GATEWAY=$(echo "$OFFERED_IP" | awk -F. '{print $1"."$2"."$3".1"}')
                        echo "  Inferring gateway from IP: $OFFERED_GATEWAY"
                    fi

                    # Set gateway variable for later use (don't add to main table yet)
                    # The ping test logic will configure routing if needed
                    if [ -n "$OFFERED_GATEWAY" ]; then
                        DHCP_GATEWAY="$OFFERED_GATEWAY"
                        echo "  Gateway will be configured: $OFFERED_GATEWAY"
                    fi

                    DHCP_SUCCESS=0
                else
                    echo "  ✗ Failed to configure IP address"
                fi
            fi

            # If still failed, try system DHCP clients
            if [ $DHCP_SUCCESS -ne 0 ]; then
                echo ""
                echo "  Custom DHCP client failed, trying system DHCP client..."

                # Try udhcpc (busybox)
                if command -v udhcpc >/dev/null 2>&1; then
                    echo "  Using udhcpc..."
                    udhcpc -i wlan0 -n -q -t 10 2>&1
                    DHCP_SUCCESS=$?
                # Try dhcpcd
                elif command -v dhcpcd >/dev/null 2>&1; then
                    echo "  Using dhcpcd..."
                    dhcpcd -4 -t 10 wlan0 2>&1
                    DHCP_SUCCESS=$?
                fi
            fi
        else
            # Fallback to system DHCP clients
            echo "  Custom DHCP client not found, using system DHCP client..."

            # Try udhcpc (busybox)
            if command -v udhcpc >/dev/null 2>&1; then
                echo "  Using udhcpc..."
                udhcpc -i wlan0 -n -q -t 10 2>&1
                DHCP_SUCCESS=$?
            # Try dhcpcd
            elif command -v dhcpcd >/dev/null 2>&1; then
                echo "  Using dhcpcd..."
                dhcpcd -4 -t 10 wlan0 2>&1
                DHCP_SUCCESS=$?
            else
                echo "  WARNING: No DHCP client found"
                DHCP_SUCCESS=1
            fi
        fi

        sleep 2

        # Check if we got an IP
        CURRENT_IP=$(ip addr show wlan0 2>/dev/null | grep 'inet ' | awk '{print $2}')

        if [ -n "$CURRENT_IP" ]; then
            echo ""
            echo "=================================================="
            echo "Network configuration complete!"
            echo "=================================================="
            echo "IP Address: $CURRENT_IP"

            # Extract IP address without netmask
            IP_ADDR=$(echo "$CURRENT_IP" | cut -d'/' -f1)
            NETMASK=$(echo "$CURRENT_IP" | cut -d'/' -f2)

            # Get gateway (try from DHCP output first, then from route table)
            GATEWAY="$DHCP_GATEWAY"
            if [ -z "$GATEWAY" ]; then
                GATEWAY=$(ip route show dev wlan0 2>/dev/null | grep default | awk '{print $3}')
            fi

            # Initialize ping success flag
            PING_SUCCESS=0

            if [ -n "$GATEWAY" ]; then
                echo "Gateway: $GATEWAY"

                # Test connectivity to gateway
                echo ""
                echo "Testing connectivity to gateway..."

                # Try ping (3 packets, 2 second timeout)
                if ping -c 3 -W 2 "$GATEWAY" >/dev/null 2>&1; then
                    PING_SUCCESS=1
                    echo "  ✓ Gateway is reachable"
                else
                    echo "  ✗ Gateway not reachable, checking routing..."

                    # Check which interface the system would use for the gateway
                    ROUTE_CHECK=$(ip route get "$GATEWAY" 2>/dev/null)
                    ROUTE_DEV=$(echo "$ROUTE_CHECK" | grep -o "dev [^ ]*" | awk '{print $2}')

                    echo "  Current route to gateway: $ROUTE_CHECK"

                    if [ "$ROUTE_DEV" != "wlan0" ]; then
                        echo "  ⚠ Traffic to gateway ($GATEWAY) is using $ROUTE_DEV instead of wlan0"
                        echo "  Configuring routing policy for wlan0..."

                        # Use table 200 for wlan0
                        WLAN_TABLE=200

                        # Get subnet and network address
                        SUBNET=$(ip -o -f inet addr show wlan0 | awk '{print $4}')
                        NETWORK=$(echo "$SUBNET" | cut -d'/' -f1 | awk -F. '{print $1"."$2"."$3".0"}')
                        NETMASK_BITS=$(echo "$SUBNET" | cut -d'/' -f2)

                        # Add subnet route to wlan0 table
                        if [ -n "$NETWORK" ] && [ -n "$NETMASK_BITS" ]; then
                            echo "  Adding subnet route ($NETWORK/$NETMASK_BITS) to table $WLAN_TABLE..."
                            ip route add "$NETWORK/$NETMASK_BITS" dev wlan0 src "$IP_ADDR" table $WLAN_TABLE 2>&1 | grep -v "File exists" || true
                        fi

                        # Add default route to wlan0 table
                        echo "  Adding default route via $GATEWAY to table $WLAN_TABLE..."
                        ip route add default via "$GATEWAY" dev wlan0 table $WLAN_TABLE 2>&1 | grep -v "File exists" || true

                        # Add policy rules
                        echo "  Adding policy rules..."
                        # Rule 1: packets from wlan0 IP use wlan0 table
                        ip rule add from "$IP_ADDR" table $WLAN_TABLE priority 100 2>&1 | grep -v "File exists" || true

                        # Rule 2: packets to wlan0 subnet use wlan0 table
                        if [ -n "$NETWORK" ] && [ -n "$NETMASK_BITS" ]; then
                            ip rule add to "$NETWORK/$NETMASK_BITS" table $WLAN_TABLE priority 101 2>&1 | grep -v "File exists" || true
                        fi

                        # Flush route cache
                        ip route flush cache 2>/dev/null || true

                        echo "  ✓ Routing policy configured"

                        # Test again after fixing routes
                        echo ""
                        echo "Testing connectivity after route configuration..."
                        sleep 1

                        if ping -c 3 -W 2 "$GATEWAY" >/dev/null 2>&1; then
                            PING_SUCCESS=1
                            echo "  ✓ Gateway is now reachable"
                        else
                            echo "  ✗ Gateway still not reachable"
                            echo ""
                            echo "  Routing table (wlan0):"
                            ip route show table $WLAN_TABLE
                            echo ""
                            echo "  Policy rules:"
                            ip rule show | grep -E "100:|101:" | head -5
                        fi
                    else
                        echo "  Route is using wlan0 correctly, but gateway not responding"
                        echo "  This may indicate an AP or network issue"
                    fi
                fi
            else
                echo "No gateway configured"
            fi

            echo ""
            echo "=================================================="
            echo "Network configuration summary"
            echo "=================================================="
            echo "Interface status:"
            ip addr show wlan0 | grep -E "inet |state "
            echo ""
            echo "Route table (wlan0):"
            ip route show dev wlan0

            # Show routing policy only if custom rules were added
            CUSTOM_RULES=$(ip rule show | grep -E "100:|101:" 2>/dev/null | wc -l)
            if [ "$CUSTOM_RULES" -gt 0 ]; then
                echo ""
                echo "Custom routing policy:"
                ip rule show | grep -E "100:|101:" 2>/dev/null | head -5
            fi

            # Show connectivity status
            echo ""
            if [ -n "$GATEWAY" ]; then
                if [ "$PING_SUCCESS" -eq 1 ]; then
                    echo "Connectivity: ✓ Gateway reachable"
                else
                    echo "Connectivity: ✗ Gateway not reachable - check AP configuration"
                fi
            else
                echo "Connectivity: No gateway configured"
            fi
            echo "=================================================="
        else
            echo ""
            echo "=================================================="
            echo "ERROR: DHCP configuration failed"
            echo "=================================================="
            echo "Connected to AP but failed to obtain IP address"
            echo ""
            echo "Possible causes:"
            echo "  1. DHCP server not responding"
            echo "  2. Network configuration issue"
            echo "  3. DHCP client timeout or error"
            echo ""
            echo "Please check:"
            echo "  - AP DHCP server is running and configured correctly"
            echo "  - Network connectivity: iw dev wlan0 link"
            echo "  - Try reconnecting to the AP"
            echo "=================================================="
        fi
    fi

    echo ""
    echo "Network management:"
    echo "  Scan: iw dev wlan0 scan"
    echo "  Disconnect: iw dev wlan0 disconnect"
    echo "  Reconnect: iw dev wlan0 connect \"$SSID\""
    echo "  DHCP: $DRIVER_PATH/../bin/dhcpc wlan0"
    echo ""

elif [ "$MODE" = "ap" ]; then
    echo "=================================="
    echo "[9] Starting AP mode (Manual hostapd)"
    echo "=================================="

    # Determine conf directory
    CONF_DIR="$CONF_BASE/$COUNTRY"
    if [ ! -d "$CONF_DIR" ]; then
        # Check if it's an EU country
        case "$COUNTRY" in
            AT|BE|BG|CY|CZ|DE|DK|EE|ES|FI|FR|GR|HR|HU|IE|IT|LT|LU|LV|MT|NL|PL|PT|RO|SE|SI|SK|GB|SA)
                CONF_DIR="$CONF_BASE/EU"
                ;;
            *)
                echo "ERROR: Configuration directory not found for country: $COUNTRY"
                exit 1
                ;;
        esac
    fi

    # Select hostapd configuration file
    case "$SECURITY" in
        open)
            HOSTAPD_CONF="$CONF_DIR/ap_halow_open.conf"
            ;;
        wpa2)
            HOSTAPD_CONF="$CONF_DIR/ap_halow_wpa2.conf"
            ;;
        wpa3)
            HOSTAPD_CONF="$CONF_DIR/ap_halow_sae.conf"
            ;;
        owe)
            HOSTAPD_CONF="$CONF_DIR/ap_halow_owe.conf"
            ;;
        pbc)
            HOSTAPD_CONF="$CONF_DIR/ap_halow_pbc.conf"
            ;;
    esac

    if [ ! -f "$HOSTAPD_CONF" ]; then
        echo "ERROR: Configuration file not found: $HOSTAPD_CONF"
        exit 1
    fi

    echo "Using configuration: $HOSTAPD_CONF"

    # Extract SSID from hostapd config
    AP_SSID=$(sed -n '/^[ \t]*#/d; /^ssid=/ {s/.*ssid=\(.*\)/\1/p; q}' "$HOSTAPD_CONF")

    # Extract passphrase if WPA2/WPA3
    AP_PASSPHRASE=""
    if [ "$SECURITY" = "wpa2" ] || [ "$SECURITY" = "wpa3" ]; then
        AP_PASSPHRASE=$(sed -n '/^[ \t]*#/d; /^wpa_passphrase=/ {s/.*wpa_passphrase=\(.*\)/\1/p; q}' "$HOSTAPD_CONF")
    fi

    echo ""
    echo "AP Configuration:"
    echo "  SSID: $AP_SSID"
    echo "  Security: $SECURITY"
    [ -n "$CHANNEL" ] && echo "  Channel: $CHANNEL"
    [ -n "$AP_PASSPHRASE" ] && echo "  Passphrase: ****"
    echo ""

    # Find hostapd binary
    echo "Searching for hostapd binary..."
    HOSTAPD_BIN=""
    for bin_path in \
        "$DRIVER_PATH/hostapd" \
        /vendor/bin/hw/hostapd \
        /vendor/bin/hostapd \
        /system/bin/hostapd; do
        if [ -x "$bin_path" ]; then
            HOSTAPD_BIN="$bin_path"
            echo "  Found: $HOSTAPD_BIN"
            break
        fi
    done

    if [ -z "$HOSTAPD_BIN" ]; then
        echo "ERROR: hostapd binary not found"
        echo "Searched locations:"
        echo "  $DRIVER_PATH/hostapd"
        echo "  /vendor/bin/hw/hostapd"
        echo "  /vendor/bin/hostapd"
        echo "  /system/bin/hostapd"
        exit 1
    fi

    # Prepare hostapd directories
    HOSTAPD_CTRL_DIR="/data/local/tmp/hostapd"
    HOSTAPD_PID_FILE="/data/local/tmp/hostapd.pid"

    rm -rf "$HOSTAPD_CTRL_DIR"
    mkdir -p "$HOSTAPD_CTRL_DIR"
    chmod 777 "$HOSTAPD_CTRL_DIR"

    # Create temporary hostapd conf with interface and channel settings
    cp "$HOSTAPD_CONF" "$TEMP_HOSTAPD_CONF"

    # Update control interface
    sed -i "s|^ctrl_interface=.*|ctrl_interface=$HOSTAPD_CTRL_DIR|" "$TEMP_HOSTAPD_CONF"

    # Update interface name
    sed -i "s/^interface=.*/interface=wlan0/" "$TEMP_HOSTAPD_CONF"

    # Update country code if it's an EU country
    if [ "$CONF_DIR" = "$CONF_BASE/EU" ]; then
        sed -i "s/^country_code=.*/country_code=$COUNTRY/" "$TEMP_HOSTAPD_CONF"
    fi

    # Update channel if specified
    # NOTE: Channel number in config file should be 5GHz channel (e.g., 161)
    # FW will internally map it to S1G channel based on country code
    # DO NOT modify hw_mode - it should remain as configured in the base config file
    if [ -n "$CHANNEL" ]; then
        sed -i "s/^channel=.*/channel=$CHANNEL/" "$TEMP_HOSTAPD_CONF"
        echo "  Using channel: $CHANNEL (FW will map to S1G internally)"
    fi

    echo "Starting hostapd..."

    # Start hostapd in background using nohup (Android vendor hostapd has issues with -B flag)
    # The -B flag causes AIDL interface problems, so we use nohup instead
    nohup "$HOSTAPD_BIN" "$TEMP_HOSTAPD_CONF" > /dev/null 2>&1 &
    HOSTAPD_PID=$!
    echo $HOSTAPD_PID > "$HOSTAPD_PID_FILE"

    # Wait for hostapd to initialize
    sleep 2

    # Check if hostapd is still running
    if kill -0 $HOSTAPD_PID 2>/dev/null; then
        HOSTAPD_RET=0
    else
        HOSTAPD_RET=1
        HOSTAPD_ERR="hostapd process died immediately after start"
    fi

    if [ $HOSTAPD_RET -eq 0 ]; then
        echo ""
        echo "=================================================="
        echo "hostapd started successfully"
        echo "=================================================="
        echo "  Binary: $HOSTAPD_BIN"
        echo "  Control interface: $HOSTAPD_CTRL_DIR"
        echo "  PID file: $HOSTAPD_PID_FILE"
        echo "  Config: $TEMP_HOSTAPD_CONF"
        echo ""
        sleep 3

        # Configure AP interface IP address
        echo "Configuring AP interface..."
        ip addr flush dev wlan0 2>/dev/null
        ip addr add 192.168.200.1/24 dev wlan0 2>/dev/null
        ip link set wlan0 up 2>/dev/null

        sleep 1

        # Start DHCP server
        echo "Starting DHCP server..."

        # Check for available DHCP server
        DHCP_SERVER=""
        DNSMASQ_CONF="/data/nrc_pkg/etc/dnsmasq/dnsmasq.conf"

        if [ -x "/system/bin/dnsmasq" ] && [ -f "$DNSMASQ_CONF" ]; then
            # Use dnsmasq if available
            echo "  Using dnsmasq for DHCP server"

            # Kill any existing dnsmasq
            killall dnsmasq 2>/dev/null
            sleep 1

            # Create PID directory if it doesn't exist
            mkdir -p /var/run 2>/dev/null || mkdir -p /data/local/tmp 2>/dev/null

            # Start dnsmasq (use --no-poll to avoid pid file issue)
            /system/bin/dnsmasq -C "$DNSMASQ_CONF" -i wlan0 -x /data/local/tmp/dnsmasq.pid &
            DHCP_PID=$!
            DHCP_SERVER="dnsmasq"
            sleep 1
            echo "  ✓ dnsmasq started (PID: $DHCP_PID)"
        else
            echo "  WARNING: dnsmasq not found or config missing"
            echo "  DHCP server not started - STAs will need static IP configuration"
        fi

        # Enable IP forwarding for NAT
        echo "Enabling NAT and routing..."
        echo 1 > /proc/sys/net/ipv4/ip_forward 2>/dev/null

        # Configure routing policy for wlan0 (similar to STA mode)
        # Use table 200 for wlan0 to ensure AP traffic uses wlan0 interface
        WLAN_TABLE=200
        AP_SUBNET="192.168.200.0/24"
        AP_IP="192.168.200.1"

        # Clear any existing rules for table 200
        ip route flush table $WLAN_TABLE 2>/dev/null
        ip rule del from $AP_IP table $WLAN_TABLE 2>/dev/null
        ip rule del to $AP_SUBNET table $WLAN_TABLE 2>/dev/null

        # Add subnet route to wlan0 table
        echo "  Adding wlan0 routing policy..."
        ip route add $AP_SUBNET dev wlan0 src $AP_IP table $WLAN_TABLE 2>&1 | grep -v "File exists" || true

        # Add policy rules
        # Rule 1: packets from AP IP use wlan0 table
        ip rule add from $AP_IP table $WLAN_TABLE priority 100 2>&1 | grep -v "File exists" || true

        # Rule 2: packets to AP subnet use wlan0 table (CRITICAL for DHCP!)
        ip rule add to $AP_SUBNET table $WLAN_TABLE priority 101 2>&1 | grep -v "File exists" || true

        # Flush route cache
        ip route flush cache 2>/dev/null || true

        echo "  ✓ wlan0 routing policy configured"

        # Set up iptables for NAT (if eth0 or other interface exists)
        # This allows STAs to access internet through AP
        if ip link show eth0 >/dev/null 2>&1; then
            echo "  Setting up NAT with eth0"
            iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE 2>/dev/null
            iptables -A FORWARD -i eth0 -o wlan0 -m state --state RELATED,ESTABLISHED -j ACCEPT 2>/dev/null
            iptables -A FORWARD -i wlan0 -o eth0 -j ACCEPT 2>/dev/null
        elif ip link show wlan1 >/dev/null 2>&1; then
            echo "  Setting up NAT with wlan1"
            iptables -t nat -A POSTROUTING -o wlan1 -j MASQUERADE 2>/dev/null
            iptables -A FORWARD -i wlan1 -o wlan0 -m state --state RELATED,ESTABLISHED -j ACCEPT 2>/dev/null
            iptables -A FORWARD -i wlan0 -o wlan1 -j ACCEPT 2>/dev/null
        else
            echo "  No upstream interface found - NAT not configured"
        fi

        sleep 1

        # Show AP status
        echo ""
        echo "AP Status:"
        ip addr show wlan0 | grep -E "inet |state "
        echo ""

        # Try to get hostapd status via hostapd_cli
        HOSTAPD_CLI=""
        for cli_path in \
            "$DRIVER_PATH/hostapd_cli" \
            /vendor/bin/hostapd_cli \
            /system/bin/hostapd_cli; do
            if [ -x "$cli_path" ]; then
                HOSTAPD_CLI="$cli_path"
                break
            fi
        done

        if [ -n "$HOSTAPD_CLI" ]; then
            echo "Checking hostapd status..."
            "$HOSTAPD_CLI" -p "$HOSTAPD_CTRL_DIR" status 2>/dev/null | head -10
            echo ""
            echo "Management commands:"
            echo "  $HOSTAPD_CLI -p $HOSTAPD_CTRL_DIR status"
            echo "  $HOSTAPD_CLI -p $HOSTAPD_CTRL_DIR all_sta"
            echo "  Stop: kill \$(cat $HOSTAPD_PID_FILE)"
        else
            echo "hostapd_cli not found, cannot query status"
            echo "Stop hostapd: kill \$(cat $HOSTAPD_PID_FILE)"
        fi
    else
        echo ""
        echo "=================================================="
        echo "ERROR: Failed to start hostapd (exit code: $HOSTAPD_RET)"
        echo "=================================================="
        echo ""
        if [ -n "$HOSTAPD_ERR" ]; then
            echo "Error output:"
            echo "$HOSTAPD_ERR"
            echo ""
        fi
        echo "Troubleshooting:"
        echo "  1. Check hostapd config: cat $TEMP_HOSTAPD_CONF"
        echo "  2. Check hostapd binary: $HOSTAPD_BIN"
        echo "  3. Check kernel messages: dmesg | tail -20"
        echo "  4. Try manual start in foreground:"
        echo "     $HOSTAPD_BIN -dd $TEMP_HOSTAPD_CONF"
        echo ""
        echo "NOTE: Keeping temp config for debugging (not deleting)"
        # Don't delete the temp config so we can debug it
        # rm -f "$TEMP_HOSTAPD_CONF"
        exit 1
    fi
fi

echo ""
echo "=================================="
echo "Startup complete!"
echo "=================================="
