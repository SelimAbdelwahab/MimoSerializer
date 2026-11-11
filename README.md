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
This umbrella header includes `NodeMetadata.hpp` for names, discriminators,
and annotations, and `SerializeNode.hpp` /
`DeserializeNode.hpp` for archive traversal. Each can also be included directly.

Archives can inspect C++ annotations through `metadata.getAnnotation<A>()` and
`metadata.hasAnnotation<A>()`. Member or caller-supplied annotations override
type annotations. Optional and variant payloads retain member annotations and
use the active value type for fallback; sequence items get their own metadata.
`AnnotationView.hpp` supplies the lookup used by `NodeMetadata`, backed by
static annotation tables rather than a runtime tree.
Exposing annotations does not itself apply renaming, skipping, or validation;
those behaviors must be implemented by the traversal or archive.

Custom output archives derive from `OutputArchive<MyArchive>` and implement
`valueImpl`, `objectImpl`, `sequenceImpl`, and `emptyImpl`. The base checks all
primitive overloads and scope requirements when construction is compiled.
`serializeNode` accepts `OutputArchive<Derived>&`, with the archive type deduced
automatically. To wrap an existing archive, use composition so the CRTP base
dispatches to the wrapper's hooks. `XmlArchive` implements this output contract.

`deserializeNode(archive, destination, metadata)` reads primitives, reflected
members, vectors, optionals, and variants through an input archive. Missing
optionals reset, vectors replace their contents, and variants require a unique
type discriminator. Objects update in place; a failed read can leave earlier
members updated. Custom input archives derive from `InputArchive<MyArchive>` and
implement the six `*Impl` hooks documented in `InputArchive.hpp`. The base checks
all hook signatures when an archive is constructed, including primitive overloads
and scope requirements. `deserializeNode` accepts an `InputArchive<Derived>&`;
the concrete archive type is deduced automatically. Both the CRTP contract and
concept checks are enforced at compile time.
XML parsing and the concrete XML input archive are not implemented yet.

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
