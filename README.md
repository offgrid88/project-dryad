### README for Project Dryad

---

#### Project Overview
Project Dryad leverages cutting-edge technology to monitor plant health, offering real-time insights and notifications. It is built on the ESP32 microcontroller using the ESP-IDF framework and integrates various sensors to assess soil moisture and overall plant condition. The system communicates via MQTT protocol to provide updates directly to your email, ensuring you are always informed about your plant's health.

#### Supported Hardware
- **ESP32-C6** (ESP32-C6-DevKitC-1) - Primary target with WiFi 6 support
- **ESP32-C3** (Compatible with various ESP32-C3 boards)

#### Key Features
- **Advanced Sensing:** Utilizes soil moisture sensors along with other environmental sensors to track plant conditions.
- **MQTT Communication:** Sends sensor data through MQTT, allowing for efficient remote monitoring.
- **Battery Powered:** Designed for longevity with a battery that supports extended operational periods.
- **Indicator Lights:** Provides physical status updates via built-in lights, useful when the internet connection is unavailable.
- **Email Notifications:** Automatic email alerts to keep you informed about your plant's needs and health.

#### Hardware Requirements
- ESP32 Microcontroller Unit (ESP32-C6 or ESP32-C3)
- Soil moisture sensor (connected to GPIO0/ADC1_CH0)
- Environmental sensors (temperature, light, humidity)
- Battery pack
- LEDs for status indication
- Display module (I2C) - https://2btrading.tn/accueil/9488-module-d-affichage-circulaire-096-240x198-msp0963-st7789-pour-raspberry-stm32-.html

#### Software Prerequisites
1. **ESP-IDF v5.1 or v5.2:** Install from [ESP-IDF Getting Started](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
2. **Python 3.8+:** Required by ESP-IDF
3. **Git:** For cloning the repository

#### Project Setup

##### 1. Clone the Repository
```bash
git clone <repository-url>
cd <repository-folder>
```

##### 2. Set up ESP-IDF Environment
```bash
# Linux/macOS
source /path/to/esp-idf/export.sh

# Windows
/path/to/esp-idf/export.bat
```

##### 3. Configure the Project
```bash
# Set target to ESP32-C6
idf.py set-target esp32c6

# Or for ESP32-C3
idf.py set-target esp32c3

# Configure project settings (optional)
idf.py menuconfig
```

#### Building the Project

##### Using the Build Script
```bash
# Build for ESP32-C6 (default)
./build.sh

# Build for ESP32-C3
./build.sh esp32c3
```

##### Using ESP-IDF Commands Directly
```bash
# Build the project
idf.py build

# Flash to device
idf.py -p /dev/ttyUSB0 flash

# Monitor serial output
idf.py -p /dev/ttyUSB0 monitor

# Flash and monitor in one command
idf.py -p /dev/ttyUSB0 flash monitor
```

#### Project Structure
```
.
├── main/                    # Main application code
│   ├── CMakeLists.txt      # Component build configuration
│   ├── main.c              # Application entry point
│   ├── moisture_sensor.c   # Moisture sensor implementation
│   ├── moisture_sensor.h   # Moisture sensor interface
│   ├── display.c           # Display implementation
│   └── display.h           # Display interface
├── CMakeLists.txt          # Project build configuration
├── partitions.csv          # Custom partition table
├── sdkconfig.defaults      # Default configuration
└── README.md               # This file
```

#### Configuration
- **ADC Pin:** GPIO0 (ADC1_CH0) for moisture sensor
- **I2C Pins:** To be configured for display module
- **MQTT Settings:** Configure in menuconfig or sdkconfig

#### CI/CD with GitHub Actions
The project includes automated builds using GitHub Actions. Every push or pull request to the main branches will automatically build the firmware for both ESP32-C6 and ESP32-C3 targets using ESP-IDF v5.1 and v5.2. The built firmware files and flash instructions are available as artifacts in the Actions tab.

#### Flashing the Firmware

##### Using esptool.py
```bash
esptool.py -p /dev/ttyUSB0 -b 460800 --before default_reset --after hard_reset write_flash \
  0x0 build/bootloader/bootloader.bin \
  0x8000 build/partition_table/partition-table.bin \
  0x10000 build/dryad.bin
```

##### Using idf.py (Recommended)
```bash
idf.py -p /dev/ttyUSB0 flash
```

#### Troubleshooting
- **Build Issues:** Ensure ESP-IDF environment is properly set up: `source $IDF_PATH/export.sh`
- **Flash Issues:** Check the serial port permissions: `sudo chmod 666 /dev/ttyUSB0`
- **ADC Readings:** The moisture sensor values (DRY_VALUE and WET_VALUE) may need calibration for your specific sensor
- **Monitor Issues:** Exit monitor with `Ctrl+]`

#### Development Tips
- Use `idf.py menuconfig` to configure WiFi credentials, MQTT settings, and other parameters
- Enable verbose logging during development: Component config → Log output → Default log verbosity → Verbose
- The custom partition table supports OTA updates with two app slots
