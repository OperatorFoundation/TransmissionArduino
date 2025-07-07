cmake_minimum_required(VERSION 3.31)
project(transmission-teensy)

set(CMAKE_CXX_STANDARD 17)
set(TRANSMISSION_LIB_DIR ${CMAKE_SOURCE_DIR}/libraries/transmission-cpp)
set(TRANSMISSION_LIB_SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(TRANSMISSION_TEENSY_DIR ${CMAKE_SOURCE_DIR}/libraries/transmission-teensy)
set(TRANSMISSION_TEENSY_SRC_DIR ${TRANSMISSION_TEENSY_DIR}/src)
set(TRANSMISSION_TEENSY_EXAMPLE_DIR ${TRANSMISSION_TEENSY_DIR}/examples/echo-server)
include_directories(${TRANSMISSION_LIB_SRC_DIR} ${TRANSMISSION_TEENSY_SRC_DIR})

# Define your sketch file
set(SKETCH_FILE ${TRANSMISSION_TEENSY_EXAMPLE_DIR}/echo-server.ino)

set(BOARD "teensy:avr:teensy41")
set(PORT "/dev/cu.usbmodem1101")

set(OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/teensy/teensy41/echo-server)
file(GLOB_RECURSE TRANSMISSION_FILES ${TRANSMISSION_LIB_SRC_DIR}/*.cpp ${TRANSMISSION_TEENSY_SRC_DIR}/*.cpp ${TRANSMISSION_TEENSY_EXAMPLE_DIR}/*.cpp ${TRANSMISSION_TEENSY_EXAMPLE_DIR}/*.ino)

add_custom_target(build_transmission_teensy ALL
        COMMAND arduino-cli compile
            --fqbn ${BOARD}
            --output-dir ${OUTPUT_DIR}
            --build-property "build.extra_flags=-I${CMAKE_SOURCE_DIR}"
            --build-property "compiler.cpp.extra_flags=-std=c++17"
            --libraries ${CMAKE_SOURCE_DIR}/libraries-teensy
            ${SKETCH_FILE}
        WORKING_DIRECTORY ${TRANSMISSION_TEENSY_SRC_DIR}
        COMMENT "Building echo-server with arduino-cli"
)

add_custom_target(upload_transmission_teensy ALL
        COMMAND arduino-cli upload
        -p ${PORT}
        --fqbn ${BOARD}
        --output-dir ${OUTPUT_DIR}
        --build-property "build.extra_flags=-I${CMAKE_SOURCE_DIR}"
        --build-property "compiler.cpp.extra_flags=-std=c++17"
        --libraries ${CMAKE_SOURCE_DIR}/libraries
        ${SKETCH_FILE}
        WORKING_DIRECTORY ${TRANSMISSION_TEENSY_SRC_DIR}
        COMMENT "Uploading echo-server with arduino-cli"
)