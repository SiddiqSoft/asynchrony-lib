cmake_minimum_required(VERSION 3.25)

file(GLOB ${CMAKE_PROJECT_NAME}
     "*/*.hpp"
     "*.hpp"
)

add_library(${CMAKE_PROJECT_NAME} INTERFACE)

message(STATUS "CMake install directory: " ${CMAKE_INSTALL_INCLUDEDIR})
message(STATUS "  $<BUILD_INTERFACE:${PROJECT_INCLUDE_DIR}>" $<BUILD_INTERFACE:${PROJECT_INCLUDE_DIR}> )
message(STATUS "  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>" $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}> )

target_include_directories(${CMAKE_PROJECT_NAME}
                              PRIVATE
                                   ${NLOHMANNJSON_INCLUDE_DIRS}
                              PUBLIC
                              INTERFACE
                                   $<BUILD_INTERFACE:${PROJECT_INCLUDE_DIR}>
                                   $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)