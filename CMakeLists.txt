cmake_minimum_required(VERSION 3.28)
project(allocator2d LANGUAGES CXX)

include(FetchContent)

option(ENABLE_TEST "Build allocator2d executable in visual test mode" OFF)
option(ENABLE_BENCHMARK "Build allocator2d executable in benchmark mode" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ALLOCATOR2D_THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

add_executable(allocator2d examples/run_sample.cpp)
target_include_directories(allocator2d PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(ENABLE_TEST)
    target_compile_definitions(allocator2d PRIVATE ENABLE_TEST)
endif()

if(ENABLE_BENCHMARK)
    target_compile_definitions(allocator2d PRIVATE ENABLE_BENCHMARK)
endif()

if(EXISTS "${ALLOCATOR2D_THIRD_PARTY_DIR}/googletest-1.16.0/CMakeLists.txt")
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    add_subdirectory("${ALLOCATOR2D_THIRD_PARTY_DIR}/googletest-1.16.0" EXCLUDE_FROM_ALL)
else()
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.16.0.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

enable_testing()

add_executable(allocator2d_tests
    tests/allocator2d_test.cpp
    tests/test_allocator.cpp
)
target_link_libraries(allocator2d_tests PRIVATE GTest::gtest_main)
target_include_directories(allocator2d_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_discover_tests(allocator2d_tests)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
if(EXISTS "${ALLOCATOR2D_THIRD_PARTY_DIR}/benchmark-1.9.0/CMakeLists.txt")
    add_subdirectory("${ALLOCATOR2D_THIRD_PARTY_DIR}/benchmark-1.9.0" EXCLUDE_FROM_ALL)
else()
    FetchContent_Declare(
        benchmark
        URL https://github.com/google/benchmark/archive/refs/tags/v1.9.0.zip
    )
    FetchContent_MakeAvailable(benchmark)
endif()

add_executable(allocator2d_benchmark benchmarks/allocator2d_benchmark.cpp)
target_link_libraries(allocator2d_benchmark PRIVATE benchmark::benchmark)
target_include_directories(allocator2d_benchmark PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
