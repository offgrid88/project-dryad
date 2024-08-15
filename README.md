### README for Project Dryad

---

#### Project Overview
Project Dryad leverages cutting-edge technology to monitor plant health, offering real-time insights and notifications. It is built on the robust ESP32 microcontroller unit (MCU) and integrates various sensors to assess soil moisture and overall plant condition. The system communicates via MQTT protocol to provide updates directly to your email, ensuring you are always informed about your plant's health.

#### Key Features
- **Advanced Sensing:** Utilizes soil moisture sensors along with other environmental sensors to track plant conditions.
- **MQTT Communication:** Sends sensor data through MQTT, allowing for efficient remote monitoring.
- **Battery Powered:** Designed for longevity with a battery that supports extended operational periods.
- **Indicator Lights:** Provides physical status updates via built-in lights, useful when the internet connection is unavailable.
- **Email Notifications:** Automatic email alerts to keep you informed about your plant’s needs and health.

#### Hardware Requirements
- ESP32 Microcontroller Unit
- Soil moisture sensor
- Environmental sensors (temperature, light, humidity)
- Battery pack
- LEDs for status indication

#### Software Setup
1. **Flash ESP32 with Firmware:** Download and install the firmware to the ESP32.
2. **Configure MQTT:** Set up MQTT credentials and connect to your network.
3. **Sensor Calibration:** Follow the steps to calibrate each sensor for accurate readings.

#### Installation
- Assemble the hardware components according to the schematic provided.
- Install the software dependencies.
- Deploy the code to the ESP32.

#### Usage
- Power on the device.
- The device will automatically connect to the configured MQTT broker.
- Check your email or MQTT client for real-time updates on plant health.

#### Troubleshooting
- Ensure all connections are secure if the device fails to send data.
- Verify battery levels if the indicator lights do not function as expected.
- Check network settings if MQTT messages are not being received.
