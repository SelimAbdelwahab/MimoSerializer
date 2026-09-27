# Mimo Serializer

An experimental header-only XML serializer built with C++26 reflection.

```cpp
#include <Mimo/Serializer/XmlSerializer.hpp>

const auto xml = mimo::serializer::xml::serialize(value);
```

Structures serialize as named XML elements. Vectors use repeated `item`
elements, disengaged optionals are omitted, and variants include a `type`
discriminator for their active alternative.

```xml
<Example>
    <numbers>
        <item>1</item>
        <item>2</item>
    </numbers>
    <value type="string">hello</value>
</Example>
```

`std::any` supports `bool`, `int`, `float`, and `std::string` automatically.
Register every other payload type with a stable discriminator before serializing
it:

```cpp
mimo::serializer::xml::XmlArchive::registerAnyType<MyType>("my-type");
```

Link the CMake target:

```cmake
target_link_libraries(your_target PRIVATE Mimo::Serializer::Xml)
```

## Project structure

```text
common/
  CMakeLists.txt
  include/Mimo/Serializer/  # Shared attributes, types, and reflection helpers
  src/
  tests/
xml/
  CMakeLists.txt
  include/Mimo/Serializer/  # XML serializer
  include/Mimo/Xml/         # XML writer and escaping
  src/
  tests/
json/
  CMakeLists.txt
  include/                 # Reserved for the JSON implementation
  src/
```

The header-only CMake targets are `Mimo::Serializer::Common`,
`Mimo::Serializer::Xml`, and `Mimo::Serializer::Json`. XML and JSON depend only
on Common for shared headers and compiler settings. JSON is a placeholder
with no serialization API yet. The `src/` directories are unused.

Include `<Mimo/Serializer/Reflection.hpp>` to use reflection without XML.

## Building the tests

The project requires a C++26 reflection-capable compiler. GCC builds also need
`-freflection`, which the library target supplies transitively.

```sh
cmake -S . -B build -DCMAKE_CXX_COMPILER=g++-16
cmake --build build
ctest --test-dir build --output-on-failure
```

Set `MIMO_SERIALIZER_BUILD_TESTS=OFF` when embedding the library without its
GoogleTest suite. Tests fetch the pinned GoogleTest 1.17.0 source by default so
it is built with the same C++ toolchain. Set `MIMO_SERIALIZER_USE_SYSTEM_GTEST=ON`
only when the installed GoogleTest uses a compatible standard-library ABI.
