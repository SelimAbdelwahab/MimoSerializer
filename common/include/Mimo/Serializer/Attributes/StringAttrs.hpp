#pragma once

#include <cstddef>

#include <Mimo/Serializer/Attributes/Detail/AttributeString.hpp>

namespace mimo::serializer::attributes
{

struct MinimumLength
{
    std::size_t value;

    constexpr auto operator==(const MinimumLength&) const -> bool = default;
};

struct MaximumLength
{
    std::size_t value;

    constexpr auto operator==(const MaximumLength&) const -> bool = default;
};

template <std::size_t N>
struct Pattern
{
    detail::AttributeString<N> value;

    constexpr explicit Pattern(const char (&pattern)[N]) : value{pattern} {}

    constexpr auto operator==(const Pattern&) const -> bool = default;
};

template <std::size_t N>
Pattern(const char (&)[N]) -> Pattern<N>;

} // namespace mimo::serializer::attributes
