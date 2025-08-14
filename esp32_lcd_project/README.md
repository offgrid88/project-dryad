# ESP32-C3 Circular LCD Display Project

This is a boilerplate project for ESP32-C3 with a 1.28" 240x240 circular IPS LCD display (GC9A01 driver) and push button interface using ESP-IDF framework.

## Features

- **Display Driver**: Optimized GC9A01 driver with hardware SPI
- **Graphics Functions**: Basic drawing primitives (pixels, lines, rectangles, circles, text)
- **Button Handler**: Debounced input handling with support for press, release, and long-press events
- **Interactive Menu System**: Navigate through different demos using push buttons
- **Demo Screens**:
  - Color Test Pattern
  - Circle Drawing Demo
  - Text Rendering Demo
  - System Information Display

## Hardware Requirements

- ESP32-C3 development board
- 1.28" 240x240 IPS circular LCD with GC9A01 controller
- 2 push buttons (or more)
- Jumper wires for connections
- 3.3V power supply

## Pin Connections

### LCD Module Connections

| LCD Pin | ESP32-C3 GPIO | Description |
|---------|---------------|-------------|
| VCC     | 3.3V          | Power supply (3.3V) |
| GND     | GND           | Ground |
| SCL     | GPIO 6        | SPI Clock |
| SDA     | GPIO 7        | SPI MOSI (Data) |
| RES     | GPIO 4        | Reset |
| DC      | GPIO 8        | Data/Command |
| CS      | GPIO 10       | Chip Select |
| BLK     | GPIO 5        | Backlight (optional) |

### Button Connections

| Button | ESP32-C3 GPIO | Description |
|--------|---------------|-------------|
| BTN1   | GPIO 0        | Navigation/Up |
| BTN2   | GPIO 2        | Select/Enter |

**Note**: Buttons should be connected between the GPIO pin and GND. Internal pull-ups are enabled.

## Software Requirements

- ESP-IDF v5.0 or later
- Python 3.x
- Git

## Project Structure

```
esp32_lcd_project/
├── CMakeLists.txt              # Main CMake configuration
├── sdkconfig.defaults          # Default ESP32-C3 configuration
├── main/
│   ├── CMakeLists.txt
│   └── main.c                  # Main application code
├── components/
│   ├── lcd_driver/             # LCD driver component
│   │   ├── CMakeLists.txt
│   │   ├── lcd_driver.c
│   │   └── include/
│   │       ├── lcd_driver.h
│   │       └── font5x7.h       # Basic font data
│   └── button_handler/         # Button handler component
│       ├── CMakeLists.txt
│       ├── button_handler.c
│       └── include/
│           └── button_handler.h
└── README.md                   # This file
```

## Setup Instructions

### 1. Install ESP-IDF

Follow the official ESP-IDF installation guide:
```bash
# Clone ESP-IDF
git clone -b v5.0 --recursive https://github.com/espressif/esp-idf.git ~/esp/esp-idf

# Install prerequisites
cd ~/esp/esp-idf
./install.sh esp32c3

# Set up environment
. ~/esp/esp-idf/export.sh
```

### 2. Clone This Project

```bash
git clone <your-repo-url> esp32_lcd_project
cd esp32_lcd_project
```

### 3. Configure the Project

```bash
# Set target to ESP32-C3
idf.py set-target esp32c3

# Optional: Configure project settings
idf.py menuconfig
```

### 4. Build the Project

```bash
idf.py build
```

### 5. Flash to ESP32-C3

```bash
# Replace /dev/ttyUSB0 with your actual serial port
idf.py -p /dev/ttyUSB0 flash
```

### 6. Monitor Serial Output

```bash
idf.py -p /dev/ttyUSB0 monitor
```

To exit the monitor, press `Ctrl+]`.

## Customizing Pin Configuration

If you need to use different GPIO pins, modify the pin definitions in `components/lcd_driver/lcd_driver.c`:

```c
// GPIO pins - adjust these according to your hardware setup
#define PIN_NUM_MOSI      7   // SDA on LCD
#define PIN_NUM_CLK       6   // SCL on LCD
#define PIN_NUM_CS        10
#define PIN_NUM_DC        8   // DC/RS on LCD
#define PIN_NUM_RST       4   // RES on LCD
#define PIN_NUM_BK_LIGHT  5   // Backlight control (optional)
```

For buttons, you can modify the default configuration in `components/button_handler/button_handler.c` or add custom buttons in your main application.

## Usage

### Navigation

- **Button 1 (GPIO 0)**: Navigate menu items / Return to main menu
- **Button 2 (GPIO 2)**: Select menu item / Confirm

### Main Menu Options

1. **Color Test**: Displays a 3x3 grid of different colors
2. **Circle Demo**: Shows concentric circles and filled circles
3. **Text Demo**: Demonstrates text rendering in different sizes
4. **System Info**: Shows ESP32-C3 system information

### Adding Custom Screens

To add your own screen/demo:

1. Add a new screen state in `main.c`:
```c
typedef enum {
    SCREEN_MAIN_MENU,
    SCREEN_COLOR_TEST,
    SCREEN_CIRCLE_DEMO,
    SCREEN_TEXT_DEMO,
    SCREEN_INFO,
    SCREEN_YOUR_CUSTOM  // Add your screen here
} screen_state_t;
```

2. Create a display function:
```c
static void display_your_custom(void)
{
    lcd_clear(COLOR_BLACK);
    // Your drawing code here
    lcd_draw_string(50, 100, "Custom Screen", COLOR_WHITE, COLOR_BLACK, 2);
}
```

3. Add menu item and handle selection in the appropriate switch cases.

## API Reference

### LCD Driver Functions

```c
// Initialize LCD
esp_err_t lcd_init(void);

// Clear screen with color
void lcd_clear(uint16_t color);

// Draw primitives
void lcd_draw_pixel(int16_t x, int16_t y, uint16_t color);
void lcd_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void lcd_draw_rectangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void lcd_draw_filled_rectangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void lcd_draw_circle(int16_t x, int16_t y, int16_t r, uint16_t color);
void lcd_draw_filled_circle(int16_t x, int16_t y, int16_t r, uint16_t color);

// Text rendering
void lcd_draw_char(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg_color, uint8_t size);
void lcd_draw_string(int16_t x, int16_t y, const char *str, uint16_t color, uint16_t bg_color, uint8_t size);

// Display control
void lcd_set_rotation(uint8_t rotation);
void lcd_display_on(bool on);
void lcd_set_brightness(uint8_t brightness);
```

### Button Handler Functions

```c
// Initialize button handler
esp_err_t button_init(QueueHandle_t event_queue);

// Add custom button
esp_err_t button_add(uint8_t button_num, const button_config_t *config);

// Check button state
bool button_is_pressed(uint8_t button_num);
```

## Color Format

Colors are in RGB565 format (16-bit). Common colors are defined in `main.c`:

```c
#define COLOR_BLACK       0x0000
#define COLOR_WHITE       0xFFFF
#define COLOR_RED         0xF800
#define COLOR_GREEN       0x07E0
#define COLOR_BLUE        0x001F
#define COLOR_YELLOW      0xFFE0
#define COLOR_CYAN        0x07FF
#define COLOR_MAGENTA     0xF81F
```

## Troubleshooting

### Display Not Working

1. Check all connections, especially power (3.3V) and ground
2. Verify SPI pins are correctly connected
3. Ensure the display is receiving proper reset signal
4. Check serial monitor for error messages

### Buttons Not Responding

1. Verify button connections (should connect to GND when pressed)
2. Check GPIO pin assignments
3. Ensure internal pull-ups are enabled
4. Monitor serial output for button events

### Build Errors

1. Ensure ESP-IDF is properly installed and environment is set
2. Verify target is set to esp32c3: `idf.py set-target esp32c3`
3. Clean and rebuild: `idf.py fullclean && idf.py build`

## Performance Tips

1. Use `lcd_draw_filled_rectangle()` for clearing areas instead of pixel-by-pixel
2. Minimize full screen clears - clear only changed areas
3. Use DMA-capable memory for large buffers
4. Adjust SPI clock speed in `LCD_PIXEL_CLOCK_HZ` if needed

## License

This project is provided as-is for educational and development purposes.

## Contributing

Feel free to submit issues, fork the repository, and create pull requests for any improvements.

## Acknowledgments

- ESP-IDF LCD component examples
- GC9A01 datasheet and reference implementations
- ESP32-C3 community