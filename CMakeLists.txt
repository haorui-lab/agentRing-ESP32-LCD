# Top-level CMakeLists.txt for AgentRing ESP32 LCD
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

add_compile_options(-Wno-format)
add_compile_options(-Wno-error=attributes)

project(agentring_esp_lcd)
