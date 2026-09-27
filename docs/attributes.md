# Serializer attributes

Mimo serializer metadata uses C++26 reflection **annotations**, not ordinary C++
attributes. An annotation is a structural compile-time value written with the
`[[=expression]]` syntax standardized by
[P3394R4](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3394r4.html).

```cpp
#include <Mimo/Serializer/Attributes.hpp>

namespace Attr = Mimo::Serializer::Attributes;

struct User {
    [[=Attr::Rename{"user_id"}, =Attr::Minimum{1}]]
    int id;

    [[=Attr::MinimumLength{1}, =Attr::MaximumLength{80}]]
    const char* name;

    [[=Attr::Skip{}]]
    const char* cached_display_name;
};
```

`Attributes.hpp` is the convenient umbrella header. Code that wants fewer
dependencies can include one category directly:

- `Attributes/CommonAttrs.hpp`: `Skip` and `Rename`
- `Attributes/NumberAttrs.hpp`: `Minimum` and `Maximum`
- `Attributes/StringAttrs.hpp`: `MinimumLength`, `MaximumLength`, and `Pattern`

String-bearing annotations own a fixed-size character array. This is necessary
because annotation expressions must have structural types; `std::string` is not
a valid annotation value type.

The reflection side should retrieve metadata with `std::meta::annotations_of`
or `std::meta::annotations_of_with_type`. These functions return vectors that
must be consumed during constant evaluation. If the results need to persist for
later expansion, use the `define_static_array` pattern described in
[P2996R13](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2996r13.html).

Ordinary C++ attribute reflection is a separate C++29 proposal
([P3385R8](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p3385r8.html))
and is not the mechanism used by this library.
