#!/bin/bash

# NRC Modular Driver Deployment Script
# Deploys built modules to Raspberry Pi

set -e  # Exit on any error

# Change to project root directory (parent of scripts/)
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

# Default configuration
DEFAULT_PI_HOST="pi@192.168.0.6"
DEFAULT_PI_PASSWORD="raspberry"
PI_DEST_DIR="/home/pi/nrc_pkg/sw/driver"
ANDROID_DEST_DIR="/data/nrc_pkg/sw/driver"
DEPLOY_MODE="ssh"  # Can be "ssh" or "adb"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to show help
show_help() {
    echo -e "${BLUE}=== NRC Modular Driver Deploy Script ===${NC}"
    echo ""
    echo -e "${YELLOW}Usage:${NC}"
    echo "  $0 [SSH_URI] [SSH_PASSWORD]              # Deploy via SSH"
    echo "  $0 adb [DEST_PATH]                       # Deploy via ADB to Android device"
    echo "  $0 help"
    echo ""
    echo -e "${YELLOW}SSH Mode Arguments:${NC}"
    echo "  SSH_URI      SSH connection URI (default: $DEFAULT_PI_HOST)"
    echo "  SSH_PASSWORD SSH password (default: $DEFAULT_PI_PASSWORD)"
    echo ""
    echo -e "${YELLOW}ADB Mode Arguments:${NC}"
    echo "  adb          Use ADB to deploy to Android device"
    echo "  DEST_PATH    Destination path on Android (default: $ANDROID_DEST_DIR)"
    echo ""
    echo -e "${YELLOW}Examples:${NC}"
    echo "  $0                                       # SSH: Use default values"
    echo "  $0 pi@192.168.1.100                     # SSH: Custom URI, default password"
    echo "  $0 pi@192.168.1.100 mypassword          # SSH: Custom URI and password"
    echo "  $0 adb                                   # ADB: Deploy to $ANDROID_DEST_DIR"
    echo "  $0 adb /data/vendor/nrc                 # ADB: Deploy to custom path"
    echo "  $0 help                                  # Show this help"
    echo ""
    echo -e "${YELLOW}Default Configuration:${NC}"
    echo "  SSH URI:     $DEFAULT_PI_HOST"
    echo "  SSH Password: $DEFAULT_PI_PASSWORD"
    echo "  SSH Destination: $PI_DEST_DIR"
    echo "  ADB Destination: $ANDROID_DEST_DIR"
    echo ""
}

# Check for help argument
if [ "$1" = "help" ] || [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    show_help
    exit 0
fi

# Parse command line arguments
if [ "$1" = "adb" ]; then
    DEPLOY_MODE="adb"
    DEST_DIR="${2:-$ANDROID_DEST_DIR}"

    # Check if adb is installed
    if ! command -v adb &> /dev/null; then
        echo -e "${RED}Error: adb is not installed${NC}"
        echo "Please install Android Debug Bridge (adb)"
        echo "  Ubuntu/Debian: sudo apt-get install adb"
        echo "  Or download Android SDK Platform Tools"
        exit 1
    fi

    # Check if device is connected
    DEVICE_COUNT=$(adb devices | grep -c $'\tdevice$' || true)
    if [ "$DEVICE_COUNT" -eq 0 ]; then
        echo -e "${RED}Error: No Android device connected${NC}"
        echo "Please connect an Android device and enable USB debugging"
        echo ""
        echo "Connected devices:"
        adb devices
        exit 1
    fi

    # Detect if device is local (USB) or network (TCP/IP)
    DEVICE_ID=$(adb devices | grep $'\tdevice$' | awk '{print $1}')
    if [[ "$DEVICE_ID" =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+:[0-9]+$ ]]; then
        ADB_CONNECTION_TYPE="network"
    else
        ADB_CONNECTION_TYPE="local"
    fi
else
    DEPLOY_MODE="ssh"
    PI_HOST="${1:-$DEFAULT_PI_HOST}"
    PI_PASSWORD="${2:-$DEFAULT_PI_PASSWORD}"
    DEST_DIR="$PI_DEST_DIR"

    # Check if sshpass is installed
    if ! command -v sshpass &> /dev/null; then
        echo -e "${RED}Error: sshpass is not installed${NC}"
        echo "Please install sshpass: sudo apt-get install sshpass"
        exit 1
    fi
fi

echo -e "${BLUE}=== NRC Modular Driver Deploy ===${NC}"
echo -e "${YELLOW}Configuration:${NC}"
if [ "$DEPLOY_MODE" = "adb" ]; then
    echo "  Deploy Mode: ADB (Android Debug Bridge)"
    echo "  Device:      $(adb devices | grep $'\tdevice$' | awk '{print $1}')"
    if [ "$ADB_CONNECTION_TYPE" = "local" ]; then
        echo "  Connection:  Local USB device"
    else
        echo "  Connection:  Network device (TCP/IP)"
    fi
    echo "  Destination: $DEST_DIR"
else
    echo "  Deploy Mode: SSH"
    echo "  SSH URI:     $PI_HOST"
    echo "  SSH Password: [HIDDEN]"
    echo "  Destination: $DEST_DIR"
fi
echo ""

# Function to run scp with password
scp_with_password() {
    local src_file="$1"
    local dest_path="$2"

    sshpass -p "$PI_PASSWORD" scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "$src_file" "$PI_HOST:$dest_path"
}

# Function to run ssh with password
ssh_with_password() {
    local command="$1"

    sshpass -p "$PI_PASSWORD" ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "$PI_HOST" "$command"
}

# Function to push file via adb
adb_push() {
    local src_file="$1"
    local dest_path="$2"

    # For local USB devices, show detailed adb push output
    if [ "$ADB_CONNECTION_TYPE" = "local" ]; then
        adb push "$src_file" "$dest_path"
    else
        # For network devices, use silent mode for cleaner output
        adb push "$src_file" "$dest_path" > /dev/null 2>&1
    fi
}

# Function to run command via adb shell
adb_shell() {
    local command="$1"

    adb shell "$command"
}

# Check if sshpass is installed
if [ "$DEPLOY_MODE" = "ssh" ] && ! command -v sshpass &> /dev/null; then
    echo -e "${RED}Error: sshpass is not installed${NC}"
    echo "Please install sshpass: sudo apt-get install sshpass"
    exit 1
fi

# Step 1: Check if modules exist
echo -e "${YELLOW}Step 1: Checking built modules...${NC}"

declare -a ALL_MODULES=(
    "backend/nrc_spi/nrc_spi.ko"
    "hal/nrc_core/nrc_core.ko"
    "frontend/nrc_wlan/nrc_wlan.ko"
    "frontend/nrc_mcp/nrc-mcp.ko"
)

declare -a ALL_MODULE_NAMES=(
    "nrc_spi.ko"
    "nrc_core.ko"
    "nrc_wlan.ko"
    "nrc-mcp.ko"
)

# Arrays to store only existing modules
declare -a MODULES=()
declare -a MODULE_NAMES=()

for i in "${!ALL_MODULES[@]}"; do
    if [ -f "${ALL_MODULES[$i]}" ]; then
        # Get file size
        size=$(stat -c%s "${ALL_MODULES[$i]}")
        echo -e "${GREEN}✓${NC} ${ALL_MODULE_NAMES[$i]} found (${size} bytes)"

        # Add to deployment arrays
        MODULES+=("${ALL_MODULES[$i]}")
        MODULE_NAMES+=("${ALL_MODULE_NAMES[$i]}")
    else
        echo -e "${YELLOW}⚠${NC} ${ALL_MODULE_NAMES[$i]} not found - skipping"
    fi
done

# Check if at least one module is available
if [ ${#MODULES[@]} -eq 0 ]; then
    echo -e "${RED}Error: No .ko modules found to deploy!${NC}"
    echo "Please build modules first using: ./build-docker.sh"
    exit 1
fi

echo -e "${GREEN}Found ${#MODULES[@]} module(s) to deploy${NC}"

# Step 2: Create destination directory on Pi
echo -e "${YELLOW}Step 2: Creating destination directory...${NC}"

if [ "$DEPLOY_MODE" = "adb" ]; then
    adb_shell "mkdir -p $DEST_DIR"
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓${NC} Destination directory created: $DEST_DIR"
    else
        echo -e "${RED}Failed to create destination directory${NC}"
        exit 1
    fi
else
    ssh_with_password "mkdir -p $DEST_DIR" || true
    scp_with_password /dev/null "$PI_HOST:$DEST_DIR/.test" 2>/dev/null || {
        echo -e "${YELLOW}⚠${NC} Cannot verify SSH connection, but will attempt file transfer..."
    }
    echo -e "${GREEN}✓${NC} Proceeding with file transfer to: $DEST_DIR"
fi

# Step 3: Copy modules to Pi
echo -e "${YELLOW}Step 3: Copying modules...${NC}"

for i in "${!MODULES[@]}"; do
    if [ "$DEPLOY_MODE" = "adb" ] && [ "$ADB_CONNECTION_TYPE" = "local" ]; then
        # For local USB devices, show detailed adb push output
        echo -e "${BLUE}Pushing ${MODULE_NAMES[$i]}...${NC}"
        adb_push "${MODULES[$i]}" "$DEST_DIR/"
        COPY_STATUS=$?
    else
        # For SSH or network ADB, show traditional output
        echo -e "Copying ${MODULE_NAMES[$i]}..."
        if [ "$DEPLOY_MODE" = "adb" ]; then
            adb_push "${MODULES[$i]}" "$DEST_DIR/${MODULE_NAMES[$i]}"
            COPY_STATUS=$?
        else
            scp_with_password "${MODULES[$i]}" "$DEST_DIR/${MODULE_NAMES[$i]}"
            COPY_STATUS=$?
        fi

        if [ $COPY_STATUS -eq 0 ]; then
            echo -e "${GREEN}✓${NC} ${MODULE_NAMES[$i]} copied successfully"
        else
            echo -e "${RED}Failed to copy ${MODULE_NAMES[$i]}${NC}"
            exit 1
        fi
    fi

    if [ $COPY_STATUS -ne 0 ]; then
        echo -e "${RED}Failed to copy ${MODULE_NAMES[$i]}${NC}"
        exit 1
    fi
done

# Step 4: Verify copied files
echo -e "${YELLOW}Step 4: Verifying copied files...${NC}"

if [ "$DEPLOY_MODE" = "adb" ]; then
    adb_shell "ls -la $DEST_DIR/*.ko"
else
    ssh_with_password "ls -la $DEST_DIR/*.ko"
fi

# Summary
echo ""
echo -e "${BLUE}=== Deployment Summary ===${NC}"
echo -e "${GREEN}✓${NC} Successfully deployed ${#MODULES[@]} module(s)"

if [ "$DEPLOY_MODE" = "adb" ]; then
    DEVICE_ID=$(adb devices | grep $'\tdevice$' | awk '{print $1}')
    echo -e "${GREEN}✓${NC} All modules copied to Android device ($DEVICE_ID):$DEST_DIR"
    echo ""
    echo -e "${YELLOW}Deployed modules:${NC}"
    for module_name in "${MODULE_NAMES[@]}"; do
        echo -e "  - $module_name"
    done
else
    echo -e "${GREEN}✓${NC} All modules copied to $PI_HOST:$DEST_DIR"
    echo ""
    echo -e "${YELLOW}Deployed modules:${NC}"
    for module_name in "${MODULE_NAMES[@]}"; do
        echo -e "  - $module_name"
    done
fi

echo ""
echo -e "${YELLOW}Next steps:${NC}"
echo "  1. SSH/ADB into the target device"
echo "  2. Navigate to $DEST_DIR"
echo "  3. Use install-modules.sh script to load the modules"
