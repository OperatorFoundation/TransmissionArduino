cmake_minimum_required(VERSION 3.31)
project(transmission-rp2040)

set(CMAKE_CXX_STANDARD 17)
set(TRANSMISSION_LIB_DIR ${CMAKE_SOURCE_DIR}/libraries/transmission-cpp)
set(TRANSMISSION_LIB_SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(TRANSMISSION_RP2040_DIR ${CMAKE_SOURCE_DIR}/libraries/transmission-rp2040)
set(TRANSMISSION_RP2040_SRC_DIR ${TRANSMISSION_RP2040_DIR}/src)
set(TRANSMISSION_RP2040_EXAMPLE_DIR ${TRANSMISSION_RP2040_DIR}/examples/echo-server)
include_directories(${TRANSMISSION_LIB_SRC_DIR} ${TRANSMISSION_RP2040_SRC_DIR})

# Define your sketch file
set(SKETCH_FILE ${TRANSMISSION_RP2040_EXAMPLE_DIR}/echo-server.ino)

set(BOARD "rp2040:rp2040:adafruit_feather_rp2350_hstx")
set(PORT "/dev/cu.usbmodem1101")

set(OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rp2040/rp2350/echo-server)
file(GLOB_RECURSE TRANSMISSION_FILES ${TRANSMISSION_LIB_SRC_DIR}/*.cpp ${TRANSMISSION_RP2040_SRC_DIR}/*.cpp ${TRANSMISSION_RP2040_EXAMPLE_DIR}/*.cpp ${TRANSMISSION_RP2040_EXAMPLE_DIR}/*.ino)

add_executable(echo-server ${TRANSMISSION_FILES}
  ../transmission-arduino/src/transmission-arduino.h
  src/transmission-rp2040.h
  ../transmission-teensy/src/transmission-teensy.h
  ../transmission-arduino/src/ReliableConnectionSerial1.cpp
  ../transmission-arduino/src/ReliableConnectionSerial1.h)
add_dependencies(echo-server build_transmission_rp2040)

add_custom_target(build_transmission_rp2040
        COMMAND arduino-cli compile
            --fqbn ${BOARD}
            --output-dir ${OUTPUT_DIR}
            --build-property "build.extra_flags=-I${CMAKE_SOURCE_DIR}"
            --build-property "compiler.cpp.extra_flags=-std=c++17"
            --libraries ${CMAKE_SOURCE_DIR}/libraries-rp2040
            ${SKETCH_FILE}
        WORKING_DIRECTORY ${TRANSMISSION_RP2040_SRC_DIR}
        COMMENT "Building echo-server with arduino-cli"
)

add_custom_target(upload_transmission_rp2040
        COMMAND arduino-cli upload
        -p ${PORT}
        --fqbn ${BOARD}
        --output-dir ${OUTPUT_DIR}
        --build-property "build.extra_flags=-I${CMAKE_SOURCE_DIR}"
        --build-property "compiler.cpp.extra_flags=-std=c++17"
        --libraries ${CMAKE_SOURCE_DIR}/libraries
        ${SKETCH_FILE}
        WORKING_DIRECTORY ${TRANSMISSION_RP2040_SRC_DIR}
        COMMENT "Uploading echo-server with arduino-cli"
)