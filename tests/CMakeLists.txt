if(${${PROJECT_NAME}_BUILD_TESTS})
    set(TESTPROJ ${PROJECT_NAME}_tests)

    add_executable(${TESTPROJ})
    target_compile_features(${TESTPROJ} PRIVATE cxx_std_20)
    target_compile_options( ${TESTPROJ}
                            PRIVATE
                            $<$<CXX_COMPILER_ID:MSVC>:/std:c++20> )
    target_compile_options( ${TESTPROJ}
                            PRIVATE
                            $<$<CXX_COMPILER_ID:Clang>:-fexperimental-library> )

    if(CMAKE_CXX_COMPILER_ID MATCHES [Cc][Ll][Aa][Nn][Gg])
        message(STATUS "Using sanitizers for leak and address on ${CMAKE_CXX_COMPILER_ID}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,leak")
        target_link_libraries(${TESTPROJ} PRIVATE -fsanitize=address,leak)
        message(STATUS "Enable instrumentation for code coverage")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-instr-generate")
        target_compile_options(${TESTPROJ} PRIVATE -fprofile-instr-generate -fcoverage-mapping)
        target_link_options(${TESTPROJ} PRIVATE -fprofile-instr-generate) 
    endif()

    if( CMAKE_BUILD_TYPE MATCHES [Dd][Ee][Bb][Uu][Gg])
        add_compile_definitions(${TESTPROJ} PRIVATE DEBUG=1)
    endif()

    target_sources( ${TESTPROJ}
                    PRIVATE
                    ${PROJECT_SOURCE_DIR}/tests/periodic_worker.cpp
                    ${PROJECT_SOURCE_DIR}/tests/roundrobin_pool.cpp
                    ${PROJECT_SOURCE_DIR}/tests/simple_pool.cpp
                    ${PROJECT_SOURCE_DIR}/tests/simple_worker.cpp)

    # Dependencies
    cpmaddpackage("gh:google/googletest#v1.15.2")
    target_link_libraries(${TESTPROJ} PRIVATE GTest::gtest_main)
    cpmaddpackage("gh:nlohmann/json#v3.11.3")
    target_link_libraries(${TESTPROJ} PRIVATE nlohmann_json::nlohmann_json)
    cpmaddpackage("gh:siddiqsoft/RunOnEnd#v1.3.2")
    target_link_libraries(${TESTPROJ} PRIVATE RunOnEnd::RunOnEnd)
    
    include(GoogleTest)
    gtest_discover_tests(${TESTPROJ} XML_OUTPUT_DIR "${PROJECT_SOURCE_DIR}/tests/results")

    include(CTest)
    message(STATUS "  Finished configuring for ${PROJECT_NAME} -- ${PROJECT_NAME}_BUILD_TESTS = ${${PROJECT_NAME}_BUILD_TESTS}")
endif()
