cmake_minimum_required(VERSION 3.20)
project(P LANGUAGES CXX)

option(ENABLE_SANITIZERS "Enable sanitizers" OFF)
option(ENABLE_BENCHMARKS "Enable project benchmarks" OFF)

if (ENABLE_SANITIZERS)
  message(STATUS "Sanitizers enabled")
  add_compile_options(-fsanitize=address,undefined,leak -g)
  add_link_options(-fsanitize=address,undefined,leak)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)

if (ENABLE_BENCHMARKS)
  include(FetchContent)

  FetchContent_Declare(
    benchmark
    GIT_REPOSITORY  https://github.com/google/benchmark.git
    GIT_TAG         v1.9.1
    SYSTEM
  )

  set(BENCHMARK_ENABLE_TESTING OFF)
  set(BENCHMARK_ENABLE_GTEST_TESTS OFF)

  FetchContent_MakeAvailable(benchmark)
endif()

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

if (ENABLE_BENCHMARKS)
  add_executable(PBench bench/tokenizer_bm.cpp bench/parser_bm.cpp)
  target_link_libraries(PBench PRIVATE PCore benchmark::benchmark_main)
endif()

enable_testing()

add_subdirectory(libp)
add_subdirectory(tests)
# add_subdirectory(bench)
