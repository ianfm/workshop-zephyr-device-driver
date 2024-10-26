# Minimum CMake version
cmake_minimum_required(VERSION 3.22.0)

# Assign devicetree overlay (must be called before find_pacakge())
set(DTC_OVERLAY_FILE "boards/esp32s3_devkitc.overlay")

# Add additional modules
set(ZEPHYR_EXTRA_MODULES "${CMAKE_SOURCE_DIR}/../../modules/mcp9808")

# Locate the Zephyr RTOS source
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# Name the project
project(temp_test)

# Locate the source code for the application
target_sources(app PRIVATE src/main.c)