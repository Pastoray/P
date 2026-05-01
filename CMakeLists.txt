cmake_minimum_required(VERSION 3.20)
project(P LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)

include(FetchContent)

FetchContent_Declare(
        benchmark
        GIT_REPOSITORY  https://github.com/google/benchmark.git
        GIT_TAG         v1.8.4
)

set(BENCHMARK_ENABLE_TESTING OFF)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF)

FetchContent_MakeAvailable(benchmark)

file(GLOB SOURCES
        src/codegen.cpp
        src/ir.cpp
        src/ir_optimizer.cpp
        src/logger.cpp
        src/parser.cpp
        src/reg_alloc.cpp
        src/tokenizer.cpp
        src/sema.cpp
)
# add_executable(P ${SOURCES})

add_library(PCore STATIC ${SOURCES})

add_executable(P src/main.cpp)
target_link_libraries(P PRIVATE PCore)

add_executable(PBench bench/tokenizer_bm.cpp bench/parser_bm.cpp)
target_link_libraries(PBench PRIVATE PCore benchmark::benchmark_main)

enable_testing()

add_subdirectory(libp)
add_subdirectory(tests)
# add_subdirectory(bench)
