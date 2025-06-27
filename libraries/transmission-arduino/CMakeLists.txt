cmake_minimum_required(VERSION 3.31)
project(transmission-arduino)

set(CMAKE_CXX_STANDARD 17)
set(TRANSMISSION_LIB_DIR ${CMAKE_SOURCE_DIR}/libraries/transmission-cpp)
set(TRANSMISSION_LIB_SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(TRANSMISSION_ARDUINO_DIR ${CMAKE_SOURCE_DIR}/libraries/transmission-arduino)
set(TRANSMISSION_ARDUINO_SRC_DIR ${TRANSMISSION_ARDUINO_DIR}/src)
set(TRANSMISSION_ARDUINO_EXAMPLE_DIR ${TRANSMISSION_ARDUINO_DIR}/examples/echo-server)
include_directories(${TRANSMISSION_LIB_SRC_DIR} ${TRANSMISSION_ARDUINO_SRC_DIR})

# Define your sketch file
set(SKETCH_FILE ${TRANSMISSION_ARDUINO_EXAMPLE_DIR}/echo-server.ino)

set(BOARD "rp2040:rp2040:adafruit_feather_rp2350_hstx")
set(PORT "/dev/cu.usbmodem1101")

set(OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/arduino/rp2350/echo-server)
file(GLOB_RECURSE TRANSMISSION_FILES ${TRANSMISSION_LIB_SRC_DIR}/*.cpp ${TRANSMISSION_ARDUINO_SRC_DIR}/*.cpp ${TRANSMISSION_ARDUINO_EXAMPLE_DIR}/*.cpp ${TRANSMISSION_ARDUINO_EXAMPLE_DIR}/*.ino)

add_executable(echo-server ${TRANSMISSION_FILES})
add_dependencies(echo-server build_transmission_arduino)

add_custom_target(build_transmission_arduino ALL
        COMMAND arduino-cli compile
            --fqbn ${BOARD}
            --output-dir ${OUTPUT_DIR}
            --build-property "build.extra_flags=-I${CMAKE_SOURCE_DIR}"
            --build-property "compiler.cpp.extra_flags=-std=c++17"
            --libraries ${CMAKE_SOURCE_DIR}/libraries
            ${SKETCH_FILE}
        WORKING_DIRECTORY ${TRANSMISSION_ARDUINO_SRC_DIR}
        COMMENT "Building echo-server with arduino-cli"
)

add_custom_target(upload_transmission_arduino ALL
        COMMAND arduino-cli upload
        -p ${PORT}
        --fqbn ${BOARD}
        --output-dir ${OUTPUT_DIR}
        --build-property "build.extra_flags=-I${CMAKE_SOURCE_DIR}"
        --build-property "compiler.cpp.extra_flags=-std=c++17"
        --libraries ${CMAKE_SOURCE_DIR}/libraries
        ${SKETCH_FILE}
        WORKING_DIRECTORY ${TRANSMISSION_ARDUINO_SRC_DIR}
        COMMENT "Uploading echo-server with arduino-cli"
)

# Generate ctags for vim
set(TAGS_FILE ${TRANSMISSION_ARDUINO_EXAMPLE_DIR}/tags)
add_custom_target(
        GenerateArduinoEchoServerTags
        COMMAND ctags -R -f ${TAGS_FILE} ${TRANSMISSION_LIB_DIR} ${TRANSMISSION_ARDUINO_DIR} ${TRANSMISSION_ARDUINO_1BIT_DIR}
        WORKING_DIRECTORY ${TRANSMISSION_ARDUINO_1BIT_DIR}
        COMMENT "Generating ctags"
)
add_dependencies(build_transmission_arduino GenerateArduinoEchoServerTags)