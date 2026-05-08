cmake_minimum_required(VERSION 3.28)
project(allocator2d)

include(FetchContent)

set(ALLOCATOR2D_THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

set(CMAKE_CXX_STANDARD 23)

add_executable(allocator2d main.cpp)

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

add_executable(allocator2d_tests tests/allocator2d_test.cpp)
target_link_libraries(allocator2d_tests PRIVATE GTest::gtest_main)
target_include_directories(allocator2d_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME allocator2d_tests COMMAND allocator2d_tests)

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
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(allocator_tests tests/test_allocator.cpp)
target_link_libraries(allocator_tests PRIVATE GTest::gtest_main)
target_include_directories(allocator_tests PRIVATE include)
target_compile_features(allocator_tests PRIVATE cxx_std_20)

include(GoogleTest)
gtest_discover_tests(allocator_tests)
