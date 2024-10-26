cmake_minimum_required(VERSION 3.25)

# Require C++20
message(STATUS "Require C++20..for tests..")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Git REQUIRED)
include(FetchContent)

file(GLOB testSRC "./*.cpp")

add_executable(${CMAKE_PROJECT_NAME}_test ${testSRC})

# Common stuff..
target_link_libraries(${CMAKE_PROJECT_NAME}_test PRIVATE GTest::gtest_main)
target_link_libraries(${CMAKE_PROJECT_NAME}_test PRIVATE nlohmann_json::nlohmann_json)

include(GoogleTest)

# In order to properly locate the tests folder, we must provide the environment variable
# SAMPLES_DIR. This should
gtest_discover_tests(${CMAKE_PROJECT_NAME}_test
                     XML_OUTPUT_DIR "${PROJECT_SOURCE_DIR}/tests"
)
