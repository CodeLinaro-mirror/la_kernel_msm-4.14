#!/bin/bash
# Remote Target Build Script using rsync
# Sync local source to target device and build kernel modules
# Usage: ./remote-build.sh [target_ip] [target_user] [target_password]

set -e

# Default configuration
TARGET_IP="${1:-192.168.0.4}"
TARGET_USER="${2:-pi}"
TARGET_PASS="${3:-raspberry}"
LOCAL_SOURCE="$(cd "$(dirname "$0")/.." && pwd)"
REMOTE_SOURCE="/tmp/nrc_modular"

echo "NRC Modular Remote Build System"
echo "================================"
echo "Local source: $LOCAL_SOURCE"
echo "Target: $TARGET_USER@$TARGET_IP"
echo "Remote path: $REMOTE_SOURCE"
echo ""

# Check sshpass installation
if ! command -v sshpass &> /dev/null; then
    echo "[INFO] Installing sshpass..."
    sudo apt-get install -y sshpass
fi

# Test SSH connection
echo "[INFO] Testing SSH connection..."
if ! sshpass -p "$TARGET_PASS" ssh -o StrictHostKeyChecking=no -o ConnectTimeout=5 "$TARGET_USER@$TARGET_IP" "echo 'Connected'" 2>/dev/null; then
    echo "[ERROR] SSH connection failed: $TARGET_USER@$TARGET_IP"
    exit 1
fi
echo "[OK] SSH connection successful"

# Sync source to target using rsync
echo ""
echo "[INFO] Syncing source to target..."
sshpass -p "$TARGET_PASS" rsync -avz --delete \
    --exclude='.git' \
    --exclude='*.o' \
    --exclude='*.ko' \
    --exclude='*.mod' \
    --exclude='*.mod.c' \
    --exclude='*.mod.o' \
    --exclude='.tmp_versions' \
    --exclude='Module.symvers' \
    --exclude='modules.order' \
    --exclude='build_output' \
    -e "ssh -o StrictHostKeyChecking=no" \
    "$LOCAL_SOURCE/" "$TARGET_USER@$TARGET_IP:$REMOTE_SOURCE/"

echo "[OK] Source sync completed"

# Build on target
echo ""
echo "[INFO] Building on target..."
sshpass -p "$TARGET_PASS" ssh -o StrictHostKeyChecking=no "$TARGET_USER@$TARGET_IP" bash << REMOTE_BUILD
set -e
cd $REMOTE_SOURCE
make clean 2>/dev/null || true
make

echo ""
echo "[OK] Build completed!"
echo ""
echo "Built modules:"
find . -name "*.ko" -exec ls -lh {} \;
REMOTE_BUILD

# Fetch built modules back to local (same directory structure)
echo ""
echo "[INFO] Fetching built modules to local..."
sshpass -p "$TARGET_PASS" rsync -avz \
    --include='*/' \
    --include='*.ko' \
    --exclude='*' \
    -e "ssh -o StrictHostKeyChecking=no" \
    "$TARGET_USER@$TARGET_IP:$REMOTE_SOURCE/" "$LOCAL_SOURCE/"

echo ""
echo "================================"
echo "Remote build completed!"
echo ""
echo "Built modules (local):"
find "$LOCAL_SOURCE" -name "*.ko" -exec ls -lh {} \;
