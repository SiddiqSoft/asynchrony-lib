cmake_minimum_required(VERSION 3.25)

# The project version is updated below with the tag from git
#   We need to support "C" language in order for the googletest to build.
project(asynchrony
        VERSION 0.0.0
        LANGUAGES CXX C
        HOMEPAGE_URL "%%myurl%%"
        DESCRIPTION "${CMAKE_PROJECT_NAME} header-only library for some asynchronous worker pools"
)

if(WIN32)
  message(STATUS "Selecting Visual Studio as generator")
  set(CMAKE_GENERATOR "Visual Studio 17 2022")
  set(CMAKE_GENERATOR_PLATFORM "x64,version=10.0.22621.0")
  set(CMAKE_GENERATOR_TOOLSET "v143")
  set(CMAKE_VS_PLATFORM_TOOLSET_VERSION "14.41.34120")
endif(WIN32)

message(STATUS "Require C++20")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

message(STATUS "-- Importing git, gnuinstalldirs..")
find_package(Git REQUIRED)
include(FetchContent)
include(GNUInstallDirs)

message(STATUS "-- Importing the nlohmann package directly..")
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
    GIT_NO_TAGS TRUE
)
# Get the googletest
FetchContent_Declare(googletest
					GIT_REPOSITORY https://github.com/google/googletest.git
					GIT_TAG	v1.15.2
				    GIT_SHALLOW TRUE
					GIT_NO_TAGS TRUE
)

FetchContent_MakeAvailable(googletest nlohmann_json)

message(STATUS "-- Going for the src..")
add_subdirectory("src")

message(STATUS "-- The project source directory............: ${PROJECT_SOURCE_DIR}")
message(STATUS "-- The PROJECT_BINARY_DIR directory........: ${PROJECT_BINARY_DIR}")
message(STATUS "-- The CMAKE_CURRENT_BINARY_DIR directory..: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "-- The CMAKE_BINARY_DIR directory..........: ${CMAKE_BINARY_DIR}")
message(STATUS "-- The CMAKE_RUNTIME_OUTPUT_DIRECTORY......: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

message(STATUS "The test..")
include(CTest)
enable_testing()
add_subdirectory("tests")
