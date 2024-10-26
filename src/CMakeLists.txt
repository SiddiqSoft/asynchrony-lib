cmake_minimum_required(VERSION 3.25)

file(GLOB asynchronylib
     "*/*.hpp"
     "*.hpp"
)

add_library(asynchronylib INTERFACE)

message(STATUS "CMake install directory: " ${CMAKE_INSTALL_INCLUDEDIR})

target_include_directories(asynchronylib PUBLIC ${NLOHMANNJSON_INCLUDE_DIRS})

target_include_directories(asynchronylib
                           INTERFACE
                           $<BUILD_INTERFACE:${PROJECT_INCLUDE_DIR}>
                           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)