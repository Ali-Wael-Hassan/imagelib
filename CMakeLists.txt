cmake_minimum_required(VERSION 3.16)

project(imagelib VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Optional native architecture tuning (disabled by default for portability).
option(IML_ENABLE_NATIVE "Enable -march=native / -mcpu=native tuning" OFF)
if(IML_ENABLE_NATIVE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i[3-6]86|AMD64")
            add_compile_options(-march=native)
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm")
            add_compile_options(-mcpu=native)
        endif()
    endif()
endif()

# ---------------------------------------------------------------------------
# External codec backends (isolated from the core library).
# ---------------------------------------------------------------------------
add_subdirectory(external)

# ---------------------------------------------------------------------------
# Static library: core ImageLib (image + math + processing + codecs).
#
# Files for the subsystems not yet implemented (Phases F-J) are commented out
# and re-enabled as they land, so the project stays buildable after each
# incremental step.
# ---------------------------------------------------------------------------
add_library(imagelib STATIC
    # memory
    src/memory/Memory.cpp
    src/memory/Allocator.cpp
    src/memory/Buffer.cpp
    # core
    src/core/Image.cpp
    # math
    src/math/Matrix.cpp
    src/math/Geometry.cpp
    src/math/Scalar.cpp
    src/math/Noise.cpp
    # codecs
    src/codecs/Codec.cpp
    src/codecs/StbCodec.cpp
    # compression
    src/compression/Compression.cpp
    # simd
    src/simd/Simd.cpp
    # threading
    src/threading/ThreadPool.cpp
    src/threading/Job.cpp
    src/threading/Thread.cpp
    src/threading/ParallelFor.cpp
    # benchmark utility (non-template halves of Benchmark.h)
    src/benchmark/Benchmark.cpp
    # processing
    src/processing/Pixel.cpp
    src/processing/Color.cpp
    src/processing/Convolution.cpp
    src/processing/Filters.cpp
    src/processing/Resize.cpp
    src/processing/Transform.cpp
    src/processing/Morphology.cpp
    src/processing/Analysis.cpp
    # procedural is header-only (Noise/Masks/Heightmaps/Textures)
)

target_include_directories(imagelib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# STB is private to the STB codec backend only.
target_link_libraries(imagelib PRIVATE stb_image)
target_include_directories(imagelib PRIVATE
    ${CMAKE_SOURCE_DIR}/external/stb_image/include
)

set_target_properties(imagelib PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# ---------------------------------------------------------------------------
# Demo application (grayscale conversion demo).
# ---------------------------------------------------------------------------
add_executable(imagelib_demo
    src/main.cpp
)
target_link_libraries(imagelib_demo PRIVATE imagelib)

# ---------------------------------------------------------------------------
# Tests.
# ---------------------------------------------------------------------------
enable_testing()
add_executable(imagelib_tests
    tests/test_runner.cpp
    tests/test_types.cpp
    tests/test_memory.cpp
    tests/test_math.cpp
    tests/test_image.cpp
    tests/test_compression.cpp
    tests/test_simd.cpp
    tests/test_threading.cpp
    tests/test_processing.cpp
    tests/test_procedural.cpp
    tests/test_codecs.cpp
)
target_link_libraries(imagelib_tests PRIVATE imagelib)
add_test(NAME imagelib_tests COMMAND imagelib_tests)

# ---------------------------------------------------------------------------
# Benchmarks.
# ---------------------------------------------------------------------------
add_executable(imagelib_bench
    benchmarks/bench_main.cpp
)
target_link_libraries(imagelib_bench PRIVATE imagelib)

# ---------------------------------------------------------------------------
# Examples.
# ---------------------------------------------------------------------------
add_executable(basic_demo examples/basic_demo.cpp)
target_link_libraries(basic_demo PRIVATE imagelib)

add_executable(processing_demo examples/processing_demo.cpp)
target_link_libraries(processing_demo PRIVATE imagelib)

# The 13 image-operation example binaries.
set(IMAGELIB_EXAMPLES
    grayscale_demo
    black_white_demo
    invert_demo
    merge_demo
    flip_demo
    rotate_demo
    darken_lighten_demo
    crop_demo
    frame_demo
    edge_demo
    resize_demo
    blur_demo
    oil_demo
)
foreach(exe IN LISTS IMAGELIB_EXAMPLES)
    add_executable(${exe} examples/${exe}.cpp)
    target_link_libraries(${exe} PRIVATE imagelib)
endforeach()

# The 13 teaching demos that implement the same filters from scratch.
# Each shows students exactly how to write and optimize the kernel.
set(IMAGELIB_CUSTOM_EXAMPLES
    custom_grayscale_demo
    custom_black_white_demo
    custom_invert_demo
    custom_merge_demo
    custom_flip_demo
    custom_rotate_demo
    custom_darken_lighten_demo
    custom_crop_demo
    custom_frame_demo
    custom_edge_demo
    custom_resize_demo
    custom_blur_demo
    custom_oil_demo
)
foreach(exe IN LISTS IMAGELIB_CUSTOM_EXAMPLES)
    add_executable(${exe} examples/custom/${exe}.cpp)
    target_link_libraries(${exe} PRIVATE imagelib)
endforeach()