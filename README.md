# project-dryad

### Installing Zephyr

1. **Install Dependencies**: Zephyr requires several dependencies to be installed on your system. The Zephyr documentation provides detailed instructions for different operating systems. For example, on Ubuntu, you can install the dependencies with:
   ```bash
   sudo apt-get install --no-install-recommends git ninja-build gperf ccache dfu-util device-tree-compiler wget python3-dev python3-pip python3-setuptools python3-tk python3-wheel xz-utils file
   ```

2. **Install Zephyr SDK**: Download the Zephyr SDK for your operating system from the Zephyr website (https://docs.zephyrproject.org/latest/getting_started/index.html) and follow the installation instructions.

3. **Set Up Environment**: Set up the Zephyr environment variables. This usually involves adding the Zephyr installation directory to your `PATH` and setting the `ZEPHYR_BASE` variable to point to the Zephyr source directory.

### Compiling Zephyr Code

1. **Create a Zephyr Project**: Use the Zephyr SDK tools to create a new Zephyr project. For example, to create a new project named `my_project`, you can use:
   ```bash
   west init -m https://github.com/zephyrproject-rtos/zephyr --mr main
   west update
   west zephyr-export
   cd zephyr
   west build -b <board_name> ./
   ```

2. **Configure the Project**: Modify the `CMakeLists.txt` and other configuration files in your project to include your source files and configure the project settings as needed for your application.

3. **Compile the Code**: Use the `west build` command to compile your Zephyr project. For example:
   ```bash
   west build -b <board_name> <path_to_project>
   ```

### Flashing Zephyr Code to ESP32

1. **Connect ESP32**: Connect your ESP32 board to your computer using a USB cable.

2. **Flash the Code**: Use the `west flash` command to flash your compiled Zephyr code to the ESP32 board. For example:
   ```bash
   west flash
   ```

3. **Monitor Output (Optional)**: You can monitor the output of your Zephyr application using a serial terminal program. For example, you can use `minicom`, `screen`, or `PuTTY` to connect to the serial port of the ESP32 and view the output.
