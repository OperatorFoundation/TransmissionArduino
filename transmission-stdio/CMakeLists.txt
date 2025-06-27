cmake_minimum_required(VERSION 3.31)
project(novela-stdio)

set(CMAKE_CXX_STANDARD 17)

set(NOVELA_STDIO_DIR ${CMAKE_SOURCE_DIR}/novela-stdio)
set(NOVELA_LIB_DIR ${CMAKE_SOURCE_DIR}/libraries/novela-cpp)
set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin)

add_executable(novela-stdio
  NovelaMain.cpp
  main.cpp
)
target_link_libraries(novela-stdio novela-cpp-lib)

# Generate ctags for vim
set(TAGS_FILE ${NOVELA_STDIO_DIR}/tags)
add_custom_target(
        GenerateStdnovelags
        COMMAND ctags -R -f ${TAGS_FILE} ${NOVELA_STDIO_DIR} ${NOVELA_LIB_DIR}
        WORKING_DIRECTORY ${NOVELA_DIR}
        COMMENT "Generating ctags"
)
add_dependencies(${PROJECT_NAME} GenerateStdnovelags)
