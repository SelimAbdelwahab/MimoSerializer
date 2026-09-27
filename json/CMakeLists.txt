# The JSON implementation will live here; its only library dependency is common.
add_library(MimoSerializerJson INTERFACE)
add_library(Mimo::Serializer::Json ALIAS MimoSerializerJson)

target_include_directories(MimoSerializerJson INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(MimoSerializerJson INTERFACE Mimo::Serializer::Common)
