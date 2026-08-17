cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_USE_RESPONSE_FILE_ON_STARTUP 1)
set(CMAKE_CXX_USE_RESPONSE_FILE_ON_STARTUP 1)
set(CMAKE_NINJA_FORCE_RESPONSE_FILE 1 ON CACHE INTERNAL "Force Ninja to use response files")

set(IDF_TARGET esp32c3)

include(/home/sergio/esp-idf-v6.0.1/tools/cmake/project.cmake)
project(temperature-transmitter)

