cmake_minimum_required(VERSION 3.31)
project(transmission-cpp-lib)

set(CMAKE_CXX_STANDARD 17)
enable_testing()

# Compile transmission-cpp library for local build machine architecture
set(TRANSMISSION_LIB_DIR ${CMAKE_SOURCE_DIR}/libraries/transmission-cpp)
set(TRANSMISSION_LIB_SOURCE_DIR ${TRANSMISSION_LIB_DIR}/src)

include_directories(${TRANSMISSION_LIB_SOURCE_DIR})
file(GLOB_RECURSE TRANSMISSION_FILES ${TRANSMISSION_LIB_SOURCE_DIR}/*.cpp)
add_library(transmission-cpp-lib STATIC ${TRANSMISSION_FILES})

target_include_directories(transmission-cpp-lib PUBLIC ${TRANSMISSION_LIB_SOURCE_DIR})