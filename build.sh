#!/bin/bash

# ESP-IDF Build Script for Project Dryad

echo "ESP-IDF Build Script for Project Dryad"
echo "======================================"

# Check if IDF_PATH is set
if [ -z "$IDF_PATH" ]; then
    echo "Error: IDF_PATH is not set. Please set up ESP-IDF environment first."
    echo "Run: source /path/to/esp-idf/export.sh"
    exit 1
fi

# Default target
TARGET=${1:-esp32c6}

echo "Building for target: $TARGET"

# Clean previous build
echo "Cleaning previous build..."
idf.py fullclean

# Set target
echo "Setting target to $TARGET..."
idf.py set-target $TARGET

# Build the project
echo "Building project..."
idf.py build

if [ $? -eq 0 ]; then
    echo ""
    echo "Build successful!"
    echo "=================="
    echo "Firmware files:"
    echo "  - build/dryad.bin"
    echo "  - build/bootloader/bootloader.bin"
    echo "  - build/partition_table/partition-table.bin"
    echo ""
    echo "To flash: idf.py -p PORT flash"
    echo "To monitor: idf.py -p PORT monitor"
    echo "To flash and monitor: idf.py -p PORT flash monitor"
else
    echo "Build failed!"
    exit 1
fi