# ESP-IDF project for the PlotToSerial example (M5Unit-KEYBOARD).
# Build: idf.py set-target <esp32|esp32s3|esp32p4|...> && idf.py build
cmake_minimum_required(VERSION 3.16.0)
set(SDKCONFIG_DEFAULTS "${CMAKE_CURRENT_LIST_DIR}/../common/sdkconfig.defaults")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# M5 log level (E=1 W=2 I=3 D=4 V=5).
idf_build_set_property(COMPILE_OPTIONS "-DM5_LOG_LEVEL=3" "-DCORE_DEBUG_LEVEL=3" APPEND)
project(unitkeyboard-plottoserial)
