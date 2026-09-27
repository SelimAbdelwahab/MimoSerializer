#pragma once

#include <cstddef>
#include <string_view>

namespace mimo::serializer::attributes::detail
{

// Owns the characters so annotation values remain structural compile-time values.
template <std::size_t N>
struct AttributeString {
    char value[N]{};

    constexpr AttributeString(const char (&text)[N]) {
        for (std::size_t index = 0; index < N; ++index) {
            value[index] = text[index];
        }
    }

    [[nodiscard]] constexpr auto view() const noexcept -> std::string_view {
        return {value, N - 1};
    }

    constexpr auto operator==(const AttributeString&) const -> bool = default;
};

template <std::size_t N>
AttributeString(const char (&)[N]) -> AttributeString<N>;

} // namespace Mimo::XmlSerializer::Attributes::Detail
