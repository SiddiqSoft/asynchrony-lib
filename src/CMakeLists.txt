cmake_minimum_required(VERSION 3.25)

file(GLOB ${CMAKE_PROJECT_NAME}
     "*/*.hpp"
     "*.hpp"
)

add_library(${CMAKE_PROJECT_NAME} INTERFACE)

message(STATUS "CMake install directory: " ${CMAKE_INSTALL_INCLUDEDIR})

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${NLOHMANNJSON_INCLUDE_DIRS})

target_include_directories(${CMAKE_PROJECT_NAME}
                           INTERFACE
                           $<BUILD_INTERFACE:${PROJECT_INCLUDE_DIR}>
                           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)