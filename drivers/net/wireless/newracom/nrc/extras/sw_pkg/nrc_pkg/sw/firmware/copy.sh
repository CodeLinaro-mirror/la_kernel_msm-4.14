#!/bin/bash

# Check if at least 3 arguments are provided
if [ $# -lt 3 ]; then
    echo "Usage: $0 <version> <board_file> <use_eeprom_config>"
    exit 1
fi

VERSION=$1
BOARD_FILE=$2
USE_EEPROM_CONFIG=$3

# Auto-detect firmware path based on script location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FIRMWARE_PATH="${SCRIPT_DIR}"

LIB_FIRMWARE_PATH="/lib/firmware"
UNIVERSAL_FILE="uni_s1g.bin"
SOURCE_FILE=""
EEPROM_FILE="nrc${VERSION}_cspi_eeprom.bin"
BINARY_FILE="nrc${VERSION}_cspi.bin"

echo "[*] Firmware path: ${FIRMWARE_PATH}"

# Decide which file to use for uni_s1g.bin based on $3
if [ "$USE_EEPROM_CONFIG" -eq 1 ]; then
    # If $3 is 1, use the EEPROM file
    SOURCE_FILE="${EEPROM_FILE}"
    echo "[*] Using EEPROM config: ${SOURCE_FILE}"
else
    # If $3 is 0, use the normal binary file
    SOURCE_FILE="${BINARY_FILE}"
    echo "[*] Using normal binary: ${SOURCE_FILE}"
fi

# Check if the source file exists before copying
if [ ! -f "${FIRMWARE_PATH}/${SOURCE_FILE}" ]; then
    echo "Error: Source file ${FIRMWARE_PATH}/${SOURCE_FILE} does not exist."
    echo "Available files:"
    ls -la "${FIRMWARE_PATH}"/*.bin 2>/dev/null || echo "No .bin files found"
    exit 2
fi

# Copy the selected source file to uni_s1g.bin
echo "[*] Creating ${UNIVERSAL_FILE} from ${SOURCE_FILE}"
cp "${FIRMWARE_PATH}/${SOURCE_FILE}" "${FIRMWARE_PATH}/${UNIVERSAL_FILE}"
if [ $? -ne 0 ]; then
    echo "Error: Failed to copy ${SOURCE_FILE} to ${UNIVERSAL_FILE}."
    exit 3
fi

# Check if the universal file exists before copying to /lib/firmware
if [ -f "${FIRMWARE_PATH}/${UNIVERSAL_FILE}" ]; then
    echo "[*] Copying ${UNIVERSAL_FILE} to ${LIB_FIRMWARE_PATH}"
    sudo cp "${FIRMWARE_PATH}/${UNIVERSAL_FILE}" "${LIB_FIRMWARE_PATH}"
    if [ $? -ne 0 ]; then
        echo "Error: Failed to copy ${UNIVERSAL_FILE} to ${LIB_FIRMWARE_PATH}."
        exit 4
    fi
    echo "[*] ${UNIVERSAL_FILE} copied successfully"
else
    echo "Error: Universal file ${UNIVERSAL_FILE} does not exist."
    exit 5
fi

# Check if the board file exists before copying
if [ -f "${FIRMWARE_PATH}/${BOARD_FILE}" ]; then
    echo "[*] Copying ${BOARD_FILE} to ${LIB_FIRMWARE_PATH}"
    sudo cp "${FIRMWARE_PATH}/${BOARD_FILE}" "${LIB_FIRMWARE_PATH}"
    if [ $? -ne 0 ]; then
        echo "Error: Failed to copy ${BOARD_FILE} to ${LIB_FIRMWARE_PATH}."
        exit 6
    fi
    echo "[*] ${BOARD_FILE} copied successfully"
else
    echo "Error: Board file ${FIRMWARE_PATH}/${BOARD_FILE} does not exist."
    echo "Available files:"
    ls -la "${FIRMWARE_PATH}"/*.dat 2>/dev/null || echo "No .dat files found"
    exit 7
fi

# List files in destination directory
echo "[*] Files in ${LIB_FIRMWARE_PATH}:"
ls -al "${LIB_FIRMWARE_PATH}"/uni_s1g* "${LIB_FIRMWARE_PATH}"/*.dat 2>/dev/null || echo "No firmware files found in ${LIB_FIRMWARE_PATH}"

echo "[*] Firmware copy completed successfully"
