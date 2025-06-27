cmake_minimum_required(VERSION 3.31)
project(transmission)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory("libraries/transmission-cpp")
add_subdirectory("libraries/transmission-arduino")
add_subdirectory("transmission-stdio")
add_subdirectory("tests/")
