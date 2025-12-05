add_library(transmission-macos
    src/ReliableConnectionMacOS.cpp
  src/transmission-macos.h
)

target_link_libraries(transmission-macos
    PUBLIC
    transmission-cpp-lib
)

target_include_directories(transmission-macos
    PUBLIC 
    include

    PRIVATE
    src
)

if(APPLE)
    target_link_libraries(transmission-macos PUBLIC
        "-framework CoreFoundation"
        "-framework IOKit"
    )
endif()

target_compile_features(transmission-macos PUBLIC cxx_std_17)