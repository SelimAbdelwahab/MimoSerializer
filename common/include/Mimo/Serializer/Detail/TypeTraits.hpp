#pragma once

#include <optional>
#include <variant>
#include <vector>

namespace mimo::serializer
{

namespace detail
{
    template <typename>
    inline constexpr bool alwaysFalse = false;

    template <typename>
    inline constexpr bool isVector = false;

    template <typename T, typename Allocator>
    inline constexpr bool isVector<std::vector<T, Allocator>> = true;

    template <typename>
    inline constexpr bool isOptional = false;

    template <typename T>
    inline constexpr bool isOptional<std::optional<T>> = true;

    template <typename>
    inline constexpr bool isVariant = false;

    template <typename... T>
    inline constexpr bool isVariant<std::variant<T...>> = true;
}

} // namespace mimo::serializer
