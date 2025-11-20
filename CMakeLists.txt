cmake_minimum_required(VERSION 3.31)
project(transmission)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#include(arduino-cli.cmake)
#include(rp2040.cmake)
#include(esp32.cmake)

set(ONDA ${CMAKE_SOURCE_DIR}/libraries-esp32/onda)
set(CMAKE_HELPERS ${CMAKE_SOURCE_DIR}/libraries-esp32/cmake-helpers)

add_subdirectory("libraries/onda")
add_subdirectory("libraries/transmission-cpp")
add_subdirectory("libraries/transmission-arduino")
add_subdirectory("libraries/transmission-rp2040")
add_subdirectory("libraries/transmission-esp32")
add_subdirectory("libraries/transmission-teensy")
add_subdirectory("libraries/transmission-macos")
add_subdirectory("transmission-stdio")
add_subdirectory("tests/")
