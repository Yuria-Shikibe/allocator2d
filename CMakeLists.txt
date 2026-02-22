cmake_minimum_required(VERSION 3.28)
project(allocator2d)


option(ENABLE_TEST "Enable test mode checks" OFF)

if(ENABLE_TEST)
    set(CMAKE_CXX_STANDARD 23)
else ()
    set(CMAKE_CXX_STANDARD 20)
endif ()

add_executable(allocator2d main.cpp)

if(ENABLE_TEST)
    target_compile_definitions(allocator2d PRIVATE ENABLE_TEST)
endif()
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
