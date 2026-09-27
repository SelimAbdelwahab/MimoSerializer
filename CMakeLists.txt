cmake_minimum_required(VERSION 3.25)
project(MimoSerializer VERSION 0.1.0 LANGUAGES CXX)

option(MIMO_SERIALIZER_BUILD_TESTS "Build Mimo XmlSerializer tests" ${PROJECT_IS_TOP_LEVEL})
option(MIMO_SERIALIZER_USE_SYSTEM_GTEST "Use an already-installed GoogleTest" OFF)

if(MIMO_SERIALIZER_BUILD_TESTS)
    include(CTest)

    if(MIMO_SERIALIZER_USE_SYSTEM_GTEST)
        find_package(GTest 1.17 CONFIG REQUIRED)
    else()
        include(FetchContent)
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
        FetchContent_MakeAvailable(googletest)
    endif()

    include(GoogleTest)
endif()

add_subdirectory(common)
add_subdirectory(xml)
add_subdirectory(json)
