# Serializer attributes

Serializer metadata uses C++26 reflection annotations: structural compile-time
values written with the `[[=expression]]` syntax standardized by
[P3394R4](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3394r4.html).

```cpp
#include <Mimo/Serializer/Attributes.hpp>

namespace Attr = mimo::serializer::attributes;

struct User {
    [[=Attr::Rename{"user_id"}, =Attr::Minimum{1}]]
    int id;

    [[=Attr::MinimumLength{1}, =Attr::MaximumLength{80}]]
    const char* name;

    [[=Attr::Skip{}]]
    const char* cached_display_name;
};
```

Include `Attributes.hpp` for all annotations, or a category header:

- `Attributes/CommonAttrs.hpp`: `Skip` and `Rename`
- `Attributes/NumberAttrs.hpp`: `Minimum` and `Maximum`
- `Attributes/StringAttrs.hpp`: `MinimumLength`, `MaximumLength`, and `Pattern`

String annotations own a fixed-size character array because annotation values
must have structural types; `std::string` does not qualify.

Retrieve metadata with `std::meta::annotations_of` or
`std::meta::annotations_of_with_type`. Consume the returned vectors during
constant evaluation. To keep the results for later expansion, use the
`define_static_array` pattern in
[P2996R13](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2996r13.html).

Ordinary C++ attribute reflection is a separate C++29 proposal
([P3385R8](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p3385r8.html))
and is not the mechanism used by this library.
