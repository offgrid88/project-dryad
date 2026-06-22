# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Summary

**Project Dryad** is an IoT plant monitoring system for ESP32-C6/C3 microcontrollers that tracks soil moisture and environmental conditions, sending alerts via MQTT and email. The repository contains two parallel implementations:

1. **Main project** (`/main`) - Moisture monitoring focused, targets ESP32-C6
2. **LCD project** (`/esp32_lcd_project`) - Display and UI focused, targets ESP32-C3 with LVGL graphics library

## Repository Architecture

### High-Level Structure

The repository has a dual-project layout reflecting different use cases:

**ESP32-C6 Main Application** (`/main`)
- Simpler, production-oriented plant monitoring system
- Core task-based architecture using FreeRTOS with two main tasks:
  - `moisture_sensor_task`: Reads ADC on GPIO0 every 2 seconds, converts raw values to 0-100% moisture percentage
  - `display_task`: Placeholder for display updates (not yet implemented)
- Single ADC sensor component with curve-fitting calibration (ESP-IDF 5.5)
- Entry point: `app_main()` creates and manages FreeRTOS tasks

**ESP32-C3 LCD Display Project** (`/esp32_lcd_project`)
- More feature-rich with circular 240x240 LCD display (GC9A01 SPI controller)
- Reusable component architecture:
  - `lcd_driver/`: Low-level GC9A01 display control (SPI, drawing primitives, text)
  - `button_handler/`: Debounced button input with press/release/long-press events
  - `lvgl_port/`: Integration layer between LVGL graphics library and hardware (display rendering + button input mapping)
- LVGL 8.3.0 for advanced UI widgets and menu system
- More complex main.c with screen state machine and interactive demos
- Entry point: `app_main()` initializes components and runs LVGL event loop

### Technology Stack

| Layer | Technology | Version | Usage |
|-------|-----------|---------|-------|
| Build System | CMake + ESP-IDF | 5.5 | Project compilation and flashing |
| OS/Scheduler | FreeRTOS | Built-in | Task management and concurrency |
| Drivers | ESP-IDF HAL | 5.5 | ADC, SPI, GPIO interfaces |
| Graphics | LVGL | 8.3.0 | LCD project only - UI widgets and rendering |
| Configuration | idf.py | 5.5 | Environment setup, menuconfig |

## Build System & Common Commands

### Environment Setup (One-Time)
```bash
# Source ESP-IDF environment
source $IDF_PATH/export.sh

# Verify setup
idf.py --version
```

### Building

**Main Project (ESP32-C6)**
```bash
# Using the provided build script (recommended - handles setup automatically)
./build.sh                    # Full clean build for ESP32-C6
./build.sh esp32c3           # Build for ESP32-C3 if needed

# Or manual steps
idf.py set-target esp32c6
idf.py fullclean
idf.py build
```

**LCD Project (ESP32-C3)**
```bash
cd esp32_lcd_project
idf.py set-target esp32c3
idf.py build
```

### Flashing & Monitoring

```bash
# Flash main project
idf.py -p /dev/ttyUSB0 flash

# Flash and monitor serial output (recommended for debugging)
idf.py -p /dev/ttyUSB0 flash monitor

# Just monitor (if already flashed)
idf.py -p /dev/ttyUSB0 monitor
# Exit monitor: Ctrl+]

# Flash LCD project
cd esp32_lcd_project
idf.py -p /dev/ttyUSB0 flash monitor
```

### Configuration

```bash
# Interactive menu for build settings (WiFi, MQTT, ADC, etc.)
idf.py menuconfig

# Check current target
idf.py get-target

# Change target
idf.py set-target esp32c6
```

### Diagnostics

```bash
# Verbose build output
idf.py -v build

# Show component memory usage
idf.py size-components

# Show per-file memory usage
idf.py size-files

# Full clean build
idf.py fullclean
```

## Code Architecture Details

### Main Project ADC & Calibration

**File**: `main/moisture_sensor.c`

The ADC reads a capacitive soil moisture sensor on GPIO0. The conversion pipeline:
1. **Raw ADC read** (0-4095 on 12-bit ADC)
2. **Voltage conversion** via curve-fitting calibration (requires eFuse data on ESP32-C6)
3. **Moisture percentage** calculation using DRY_VALUE and WET_VALUE constants (inverse scale: higher voltage = wetter soil)
4. **Clamping** to 0-100% range

**Critical Configuration**:
```c
#define DRY_VALUE 1200   // Calibration constant - raw voltage in mV when soil is dry
#define WET_VALUE 3600   // Calibration constant - raw voltage in mV when soil is wet
```
These require per-sensor calibration by testing sensor output in dry and wet soil.

**ADC Setup**:
- ADC Unit 1, Channel 0 (ESP32-C6 GPIO0)
- 11dB attenuation (0-3.3V range)
- ESP-IDF 5.5 curve-fitting calibration scheme (replaces legacy eFuse/LUT methods)

**Logging**: All reads logged via `ESP_LOGI(TAG, "Raw: %d, Voltage: %dmV, Moisture: %d%%", ...)`

### LCD Project Component Architecture

**LCD Driver** (`esp32_lcd_project/components/lcd_driver/`)
- Implements GC9A01 controller communication over SPI
- Public API: `lcd_init()`, `lcd_clear()`, `lcd_draw_pixel()`, `lcd_draw_line()`, `lcd_draw_rectangle()`, `lcd_draw_filled_rectangle()`, `lcd_draw_circle()`, `lcd_draw_filled_circle()`, `lcd_draw_char()`, `lcd_draw_string()`, `lcd_set_rotation()`, `lcd_display_on()`, `lcd_set_brightness()`
- Uses RGB565 16-bit color format
- DMA-capable for fast transfers
- Pin configuration in lcd_driver.c (GPIO 4=RST, 5=BL, 6=CLK, 7=MOSI, 8=DC, 10=CS)

**Button Handler** (`esp32_lcd_project/components/button_handler/`)
- Event-driven GPIO input with queue-based delivery
- Supports three event types: BUTTON_PRESS, BUTTON_RELEASE, BUTTON_LONG_PRESS
- Configurable debounce and long-press duration per button
- Typical usage: Create queue, init handler, add buttons with config, read events from queue
- Default buttons: GPIO0 (navigation), GPIO2 (select)

**LVGL Port** (`esp32_lcd_project/components/lvgl_port/`)
- Display driver: `lvgl_port_display.c` - Renders LVGL frame buffers to GC9A01 via SPI
- Input driver: `lvgl_port_indev.c` - Maps button events to LVGL key inputs (LV_KEY_UP, LV_KEY_ENTER)
- Configuration: `lv_conf.h` - 32KB LVGL heap, 2x11KB display buffers, 30 FPS refresh, 20MHz SPI clock
- Thread-safe: Mutex protection for concurrent LVGL operations

**Main Application** (`esp32_lcd_project/main/main.c`)
- Screen state machine: Cycles between home menu (LVGL list), gauges demo, settings, system info
- Button navigation: BTN1 (GPIO0) = up/back, BTN2 (GPIO2) = enter/select
- LVGL widgets: List, gauge, slider, switch, label
- Two versions available: `main.c` (LVGL-based) and `main_basic.c` (simple drawing without LVGL)

### Build Configuration

**ESP32-C6 Config** (`sdkconfig.defaults`)
- Target: ESP32-C6 with DIO flash mode at 80MHz, 4MB flash
- Partition table: Custom OTA-capable table from `partitions.csv`
- FreeRTOS: Stats collection enabled for debugging
- Logging: INFO level, colored output
- Main task: 4KB stack
- ADC: OneShot control not in IRAM (default)

**ESP32-C3 Config** (`esp32_lcd_project/sdkconfig.defaults`)
- Similar settings but for ESP32-C3 target
- Configured for display and button use cases

## CI/CD Pipeline

**GitHub Actions** (`.github/workflows/build.yml`)
- Triggers: Push to main/dev, PRs, manual dispatch
- Environment: Ubuntu latest + ESP-IDF v5.5
- Builds ESP32-C6 firmware target
- Artifacts: `build/dryad.bin`, `build/dryad.elf`, partition table, bootloader, flash_args, flash_instructions.md
- Retention: 30 days
- Flash instructions auto-generated for esptool and idf.py

## Dependencies & External Libraries

**idf_component.yml** (`esp32_lcd_project/`)
- LVGL 8.3.0 (Light and Versatile Graphics Library) - via component manager
- Minimum IDF: 5.0+

**No additional dependencies** for main project - pure ESP-IDF.

## Key Files to Understand

- **main/main.c** - Task creation and entry point
- **main/moisture_sensor.c** - ADC reading and calibration logic
- **esp32_lcd_project/main/main.c** - LVGL UI application
- **.github/workflows/build.yml** - CI/CD build steps and artifacts
- **CMakeLists.txt** - Root project configuration
- **esp32_lcd_project/idf_component.yml** - Dependency declaration

## Known Status & TODOs

- ADC calibration values (DRY_VALUE, WET_VALUE) are placeholders - require per-sensor tuning
- Display task in main project is a stub (display.c not fully implemented)
- MQTT and email alert features mentioned in README are not yet implemented in main project
- Both projects tested with GitHub Actions on Ubuntu/ESP-IDF 5.5

## Serial Debugging

Enable `idf.py monitor` to view:
- **moisture_sensor**: Logs raw ADC, voltage (mV), and moisture percentage every 2 seconds
- **lcd_driver**: Logs SPI initialization, write commands
- **button_handler**: Logs button press/release/long-press events
- **Main**: Application startup and task creation

Common log tags for grep/filtering: "Main", "moisture_sensor", "lcd_driver", "button_handler", "lvgl_port"

## Troubleshooting Build Issues

| Issue | Fix |
|-------|-----|
| `IDF_PATH not set` | Run `source $IDF_PATH/export.sh` |
| `Error: No module named 'esp_idf_tools'` | Run `$IDF_PATH/install.sh` |
| Port permission denied | `sudo chmod 666 /dev/ttyUSB0` |
| Build cache corruption | `idf.py fullclean && idf.py build` |
| Target mismatch | Verify with `idf.py get-target`, correct with `idf.py set-target esp32c6` |
| LVGL build fails (LCD project) | Ensure `idf_component.yml` is present and `idf.py build` (not `cmake` directly) is used |

