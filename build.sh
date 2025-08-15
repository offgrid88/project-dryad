#!/bin/bash

# Build script for ESP32-C6 project

echo "ESP32-C6 Project Build Script"
echo "============================="

# Check if PlatformIO is installed
if ! command -v pio &> /dev/null; then
    echo "PlatformIO is not installed. Installing..."
    pip install --user platformio
    export PATH=$PATH:~/.local/bin
fi

# Build for ESP32-C6
echo "Building for ESP32-C6..."
pio run -e esp32c6

# Build for ESP32-C3 (optional)
echo "Building for ESP32-C3..."
pio run -e esp32_c3

echo "Build complete!"
echo "Firmware files are located in .pio/build/"