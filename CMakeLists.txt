cmake_minimum_required(VERSION 3.12)

project(imagelib)

set(SOURCES
    src/main.cpp
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(imagelib
    src/main.cpp
)

target_include_directories(imagelib PRIVATE
    include
)