cmake_minimum_required(VERSION 3.31)
project(transmission-stdio)

set(CMAKE_CXX_STANDARD 17)

set(TRANSMISSION_STDIO_DIR ${CMAKE_SOURCE_DIR}/transmission-stdio)
set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin)

add_executable(transmission-stdio
  TransmissionMain.cpp
  main.cpp
)

target_link_libraries(transmission-stdio
  transmission-cpp-lib
  transmission-macos  # Add this for TCP connection
)

# Find pthreads (needed for std::thread in main loop)
find_package(Threads REQUIRED)
target_link_libraries(transmission-stdio Threads::Threads)