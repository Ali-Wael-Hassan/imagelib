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
    src/core/memory/Memory.cpp
    src/core/memory/DefaultAllocator.cpp
    src/core/memory/Buffer.cpp
    # core
    src/core/image/Image.cpp
    src/core/image/Pixel.cpp
    # math
    src/math/Matrix.cpp
    src/math/Geometry.cpp
    src/math/Scalar.cpp
    src/math/Noise.cpp
    # codecs
    src/codecs/registry/Registry.cpp
    src/codecs/api/Api.cpp
    src/codecs/stb/Stb.cpp
    # simd
    src/simd/Simd.cpp
    # threading
    src/threading/ThreadPool.cpp
    src/threading/Job.cpp
    src/threading/Thread.cpp
    src/threading/ParallelFor.cpp
    # benchmark utility (non-template halves of Benchmark.h)
    src/benchmark/measure/Measure.cpp
    src/benchmark/record/Entry.cpp
    src/benchmark/record/PairEntry.cpp
    src/benchmark/report/BenchReport.cpp
    # processing primitives (kernel container + color math only; filters are
    # composed by the user)
    src/processing/Color.cpp
    src/processing/Convolution.cpp
    # procedural is header-only (Noise render layer over math::noise)
)

target_include_directories(imagelib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
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
# Filter demos: the 13 classic image operations, each a standalone binary that
# load/save an image and print a scalar-vs-SIMD-vs-parallel benchmark report
# (see examples/example_util.h). Used for performance benchmarking.
# Each accepts:  <target> [input] [output] ["serial"] [filter args...]
# ---------------------------------------------------------------------------
set(IMAGELIB_FILTER_DEMOS
    "grayscale_demo|examples/filter_grayscale.cpp"
    "black_white_demo|examples/filter_black_white.cpp"
    "invert_demo|examples/filter_invert.cpp"
    "merge_demo|examples/filter_merge.cpp"
    "flip_demo|examples/filter_flip.cpp"
    "rotate_demo|examples/filter_rotate.cpp"
    "darken_lighten_demo|examples/filter_darken_lighten.cpp"
    "crop_demo|examples/filter_crop.cpp"
    "frame_demo|examples/filter_frame.cpp"
    "edge_demo|examples/filter_edge.cpp"
    "resize_demo|examples/filter_resize.cpp"
    "blur_demo|examples/filter_blur.cpp"
    "oil_demo|examples/filter_oil.cpp"
)
foreach(pair IN LISTS IMAGELIB_FILTER_DEMOS)
    string(REPLACE "|" ";" pair_list "${pair}")
    list(GET pair_list 0 demo_name)
    list(GET pair_list 1 demo_src)
    add_executable("${demo_name}" "${demo_src}")
    target_link_libraries("${demo_name}" PRIVATE imagelib)
endforeach()

# ---------------------------------------------------------------------------
# I/O benchmark demo: times disk load and save (PNG / BMP) throughput.
# ---------------------------------------------------------------------------
add_executable(io_speed_demo examples/io_speed_demo.cpp)
target_link_libraries(io_speed_demo PRIVATE imagelib)

# ---------------------------------------------------------------------------
# Tutorial demos (see tutorial/README.md). Each lesson is a .md + .cpp pair.
# ---------------------------------------------------------------------------
set(IMAGELIB_TUTORIALS
    tutorial/01_image.cpp
    tutorial/02_math.cpp
    tutorial/03_parallel.cpp
    tutorial/04_simd.cpp
)
foreach(tutorial_src IN LISTS IMAGELIB_TUTORIALS)
    get_filename_component(tutorial_base "${tutorial_src}" NAME_WE)
    add_executable("tutorial_${tutorial_base}" "${tutorial_src}")
    target_link_libraries("tutorial_${tutorial_base}" PRIVATE imagelib)
endforeach()