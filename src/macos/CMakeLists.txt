add_library(transmission-macos
  src/ReliableConnectionMacOS.cpp
  src/ReliableConnectionTcpMacOS.cpp
)

target_link_libraries(transmission-macos PUBLIC
    transmission-cpp-lib
    onda
    onda-macos
)

target_include_directories(transmission-macos PUBLIC
    ${ONDA}/src
    ${ONDA_MACOS}/src
    ${TRANSMISSION}/src
    ${TRANSMISSION_MACOS}/src
)

if(APPLE)
    target_link_libraries(transmission-macos PUBLIC
        "-framework CoreFoundation"
        "-framework IOKit"
    )
endif()

target_compile_features(transmission-macos PUBLIC cxx_std_17)