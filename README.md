### README for Project Dryad

---

#### Project Overview
Project Dryad leverages cutting-edge technology to monitor plant health, offering real-time insights and notifications. It is built on the robust ESP32 microcontroller unit (MCU) and integrates various sensors to assess soil moisture and overall plant condition. The system communicates via MQTT protocol to provide updates directly to your email, ensuring you are always informed about your plant's health.

#### Supported Hardware
- **ESP32-C3** (Adafruit QT Py ESP32-C3)
- **ESP32-C6** (ESP32-C6-DevKitC-1)

#### Key Features
- **Advanced Sensing:** Utilizes soil moisture sensors along with other environmental sensors to track plant conditions.
- **MQTT Communication:** Sends sensor data through MQTT, allowing for efficient remote monitoring.
- **Battery Powered:** Designed for longevity with a battery that supports extended operational periods.
- **Indicator Lights:** Provides physical status updates via built-in lights, useful when the internet connection is unavailable.
- **Email Notifications:** Automatic email alerts to keep you informed about your plant's needs and health.

#### Hardware Requirements
- ESP32 Microcontroller Unit (ESP32-C3 or ESP32-C6)
- Soil moisture sensor
- Environmental sensors (temperature, light, humidity)
- Battery pack
- LEDs for status indication
https://2btrading.tn/accueil/9488-module-d-affichage-circulaire-096-240x198-msp0963-st7789-pour-raspberry-stm32-.html

#### Software Setup
1. **Install PlatformIO:** Download and install [PlatformIO IDE](https://platformio.org/install) or use the CLI version.
2. **Clone the Repository:** 
   ```bash
   git clone <repository-url>
   cd <repository-folder>
   ```
3. **Configure MQTT:** Set up MQTT credentials and connect to your network.
4. **Sensor Calibration:** Follow the steps to calibrate each sensor for accurate readings.

#### Building the Project

##### Using PlatformIO CLI
```bash
# Build for ESP32-C6
pio run -e esp32c6

# Build for ESP32-C3
pio run -e esp32_c3

# Upload to ESP32-C6
pio run -e esp32c6 -t upload

# Upload to ESP32-C3
pio run -e esp32_c3 -t upload

# Monitor serial output
pio device monitor -e esp32c6
```

##### Using PlatformIO IDE
1. Open the project in PlatformIO IDE
2. Select the environment (esp32c6 or esp32_c3) from the bottom toolbar
3. Click the Build or Upload button

#### CI/CD with GitHub Actions
The project includes automated builds using GitHub Actions. Every push or pull request to the main branches will automatically build the firmware for both ESP32-C3 and ESP32-C6 targets. The built firmware files are available as artifacts in the Actions tab.

#### Installation
- Assemble the hardware components according to the schematic provided.
- Install the software dependencies using PlatformIO.
- Deploy the code to the ESP32 using the upload commands above.

#### Usage
- Power on the device.
- The device will automatically connect to the configured MQTT broker.
- Check your email or MQTT client for real-time updates on plant health.

#### Troubleshooting
- Ensure all connections are secure if the device fails to send data.
- Verify battery levels if the indicator lights do not function as expected.
- Check network settings if MQTT messages are not being received.
- For build issues, ensure you have the latest PlatformIO Core installed: `pio upgrade`
