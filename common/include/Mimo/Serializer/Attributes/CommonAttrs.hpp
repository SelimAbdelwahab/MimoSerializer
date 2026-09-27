#pragma once

#include <cstddef>

#include <Mimo/Serializer/Attributes/Detail/AttributeString.hpp>

namespace mimo::serializer::attributes
{
// Excludes a member from serialization and deserialization.
struct Skip {
    constexpr auto operator==(const Skip&) const -> bool = default;
};

// Replaces the reflected C++ member name in the serialized representation.
template <std::size_t N>
struct Rename {
    Detail::AttributeString<N> name;

    constexpr explicit Rename(const char (&value)[N]) : name{value} {}

    constexpr auto operator==(const Rename&) const -> bool = default;
};

template <std::size_t N>
Rename(const char (&)[N]) -> Rename<N>;

} // namespace mimo::serializer::attributes
