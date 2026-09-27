add_library(MimoSerializerCommon INTERFACE)
add_library(Mimo::Serializer::Common ALIAS MimoSerializerCommon)

target_compile_features(MimoSerializerCommon INTERFACE cxx_std_26)
target_include_directories(MimoSerializerCommon INTERFACE
    include)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(MimoSerializerCommon INTERFACE -freflection)
    message(STATUS "Configured target for GNU/GCC C++26 Reflection")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    message(FATAL_ERROR
        "Apple Clang does not support C++26 reflection. "
        "Use GCC 16+ or a reflection-capable Clang build.")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Configured target for reflection-capable Clang")
else()
    message(WARNING
        "Unknown compiler: C++26 reflection flags may not be configured correctly.")
endif()

if(MIMO_SERIALIZER_BUILD_TESTS)
    add_executable(MimoSerializerCommonTests tests/TypesTests.cpp
            tests/ReflectionTests.cpp)
    set_target_properties(MimoSerializerCommonTests PROPERTIES CXX_EXTENSIONS OFF)
    target_link_libraries(MimoSerializerCommonTests PRIVATE
        Mimo::Serializer::Common GTest::gtest_main)
    target_compile_options(MimoSerializerCommonTests PRIVATE -Wall -Wextra -Wpedantic)
    gtest_discover_tests(MimoSerializerCommonTests)
endif()
