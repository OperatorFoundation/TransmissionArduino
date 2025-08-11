cmake_minimum_required(VERSION 3.31)
project(transmission)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(arduino-cli.cmake)
include(rp2040.cmake)
include(esp32.cmake)

add_subdirectory("libraries/transmission-cpp")
add_subdirectory("libraries/transmission-rp2040")
add_subdirectory("libraries/transmission-esp32")
add_subdirectory("libraries/transmission-teensy")
add_subdirectory("libraries/transmission-macos")
add_subdirectory("transmission-stdio")
add_subdirectory("tests/")
