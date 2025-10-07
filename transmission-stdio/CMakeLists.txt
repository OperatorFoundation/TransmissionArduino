cmake_minimum_required(VERSION 3.31)
project(transmission-stdio)

set(CMAKE_CXX_STANDARD 17)

set(TRANSMISSION_STDIO_DIR ${CMAKE_SOURCE_DIR}/transmission-stdio)
set(TRANSMISSION_LIB_DIR ${CMAKE_SOURCE_DIR}/libraries/transmission-cpp)
set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin)

add_executable(transmission-stdio
  TransmissionMain.cpp
  main.cpp
)
target_link_libraries(transmission-stdio transmission-cpp-lib)

# Generate ctags for vim
set(TAGS_FILE ${TRANSMISSION_STDIO_DIR}/tags)
add_custom_target(
        GenerateStdtransmissiongs
        COMMAND ctags -R -f ${TAGS_FILE} ${TRANSMISSION_STDIO_DIR} ${TRANSMISSION_LIB_DIR}
        WORKING_DIRECTORY ${TRANSMISSION_DIR}
        COMMENT "Generating ctags"
)
add_dependencies(${PROJECT_NAME} GenerateStdtransmissiongs)
