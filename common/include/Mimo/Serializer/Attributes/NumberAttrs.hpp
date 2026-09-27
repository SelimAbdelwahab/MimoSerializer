#pragma once

#include <concepts>
#include <type_traits>

namespace mimo::serializer::attributes::detail
{

template <typename T>
concept Number = (std::integral<T> || std::floating_point<T>) && !std::same_as<std::remove_cv_t<T>, bool>;

} // namespace mimo::serializer::attributes::detail

namespace mimo::serializer::attributes
{
template <detail::Number T>
struct Minimum
{
    T value;

    constexpr auto operator==(const Minimum&) const -> bool = default;
};

template <detail::Number T>
Minimum(T) -> Minimum<T>;

template <detail::Number T>
struct Maximum
{
    T value;

    constexpr auto operator==(const Maximum&) const -> bool = default;
};

template <detail::Number T>
Maximum(T) -> Maximum<T>;

} // namespace mimo::serializer::attributes
