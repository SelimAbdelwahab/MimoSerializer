add_library(MimoSerializerXml INTERFACE)
add_library(Mimo::Serializer::Xml ALIAS MimoSerializerXml)

target_include_directories(MimoSerializerXml INTERFACE
    include)
target_link_libraries(MimoSerializerXml INTERFACE Mimo::Serializer::Common)

if(MIMO_SERIALIZER_BUILD_TESTS)
    add_executable(MimoSerializerTests
            tests/SerializerTests.cpp
            tests/XmlWriterTests.cpp
            include/Mimo/Xml/XmlWriter.hpp
            include/Mimo/Xml/XmlAuthor.hpp
    )
    set_target_properties(MimoSerializerTests PROPERTIES CXX_EXTENSIONS OFF)
    target_link_libraries(MimoSerializerTests PRIVATE
        Mimo::Serializer::Xml GTest::gtest_main)
    target_compile_options(MimoSerializerTests PRIVATE -Wall -Wextra -Wpedantic)
    gtest_discover_tests(MimoSerializerTests)
endif()
