cmake_minimum_required(VERSION 3.31)
project(transmission-esp32)

set(CMAKE_CXX_STANDARD 17)
set(TRANSMISSION_LIB_DIR ${CMAKE_SOURCE_DIR}/libraries/transmission-cpp)
set(TRANSMISSION_LIB_SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(TRANSMISSION_ESP32_DIR ${CMAKE_SOURCE_DIR}/libraries/transmission-esp32)
set(TRANSMISSION_ESP32_SRC_DIR ${TRANSMISSION_ESP32_DIR}/src)
set(TRANSMISSION_ESP32_EXAMPLE_DIR ${TRANSMISSION_ESP32_DIR}/examples/echo-server)
include_directories(${TRANSMISSION_LIB_SRC_DIR} ${TRANSMISSION_ESP32_SRC_DIR})

# Define your sketch file
set(SKETCH_FILE ${TRANSMISSION_ESP32_EXAMPLE_DIR}/echo-server.ino)

set(BOARD "esp32:esp32:adafruit_feather_esp32s3_nopsram")
set(PORT "/dev/cu.usbmodem1101")

set(OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/esp32/s3/echo-server)
file(GLOB_RECURSE TRANSMISSION_FILES ${TRANSMISSION_LIB_SRC_DIR}/*.cpp ${TRANSMISSION_ESP32_SRC_DIR}/*.cpp ${TRANSMISSION_ESP32_EXAMPLE_DIR}/*.cpp ${TRANSMISSION_ESP32_EXAMPLE_DIR}/*.ino)

add_executable(echo-server-esp32 ${TRANSMISSION_FILES}
  src/transmission-esp32.h)
add_dependencies(echo-server-esp32 build_transmission_esp32)

add_custom_target(build_transmission_esp32 ALL
        COMMAND arduino-cli compile
            --fqbn ${BOARD}
            --output-dir ${OUTPUT_DIR}
            --build-property "build.extra_flags=-I${CMAKE_SOURCE_DIR}"
            --build-property "compiler.cpp.extra_flags=-std=c++17"
            --libraries ${CMAKE_SOURCE_DIR}/libraries-esp32
            ${SKETCH_FILE}
        WORKING_DIRECTORY ${TRANSMISSION_ESP32_SRC_DIR}
        COMMENT "Building echo-server with arduino-cli"
)

add_custom_target(upload_transmission_esp32 ALL
        COMMAND arduino-cli upload
        -p ${PORT}
        --fqbn ${BOARD}
        --output-dir ${OUTPUT_DIR}
        --build-property "build.extra_flags=-I${CMAKE_SOURCE_DIR}"
        --build-property "compiler.cpp.extra_flags=-std=c++17"
        --libraries ${CMAKE_SOURCE_DIR}/libraries
        ${SKETCH_FILE}
        WORKING_DIRECTORY ${TRANSMISSION_ESP32_SRC_DIR}
        COMMENT "Uploading echo-server with arduino-cli"
)