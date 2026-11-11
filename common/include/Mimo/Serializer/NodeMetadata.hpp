#pragma once

#include <meta>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include <Mimo/Serializer/AnnotationView.hpp>

namespace mimo::serializer
{

struct NodeMetadata
{
    std::string_view name;
    std::optional<std::string_view> typeDiscriminator = std::nullopt;

    // Member (or caller-supplied) annotations override the current value type's
    // annotations. They stay attached when unwrapping optionals and variants.
    AnnotationView annotations{};
    AnnotationView typeAnnotations{};

    template <typename A>
    [[nodiscard]] const A* getAnnotation() const noexcept
    {
        if (const auto* annotation = annotations.getAnnotation<A>())
            return annotation;
        return typeAnnotations.getAnnotation<A>();
    }

    template <typename A>
    [[nodiscard]] bool hasAnnotation() const noexcept
    {
        return getAnnotation<A>() != nullptr;
    }

    // Set the fallback to the actual value type without changing the node's
    // name, discriminator, or member annotations. No runtime tree is built.
    template <typename T>
    [[nodiscard]] constexpr NodeMetadata withType() const
    {
        using Type = std::remove_cvref_t<T>;
        auto result = *this;
        constexpr auto typeAnnotations = AnnotationView::from<std::meta::dealias(^^Type)>();
        result.typeAnnotations = typeAnnotations;
        return result;
    }

    template <std::meta::info Member>
    [[nodiscard]] static consteval NodeMetadata forMember()
    {
        return {
            .name = std::meta::has_identifier(Member)
                ? std::meta::identifier_of(Member) : std::string_view{"<unnamed>"},
            .annotations = AnnotationView::from<Member>(),
            .typeAnnotations = AnnotationView::from<std::meta::dealias(
                std::meta::remove_cvref(std::meta::type_of(Member)))>()
        };
    }
};

template <typename T>
struct TypeDiscriminator
{
    using Type = std::remove_cvref_t<T>;

    static constexpr std::string_view value =
        std::define_static_string(
            std::meta::display_string_of(std::meta::dealias(^^Type)));
};

template <>
struct TypeDiscriminator<bool>
{
    static constexpr std::string_view value = "bool";
};

template <>
struct TypeDiscriminator<int>
{
    static constexpr std::string_view value = "int";
};

template <>
struct TypeDiscriminator<float>
{
    static constexpr std::string_view value = "float";
};

template <>
struct TypeDiscriminator<std::string>
{
    static constexpr std::string_view value = "string";
};

template <>
struct TypeDiscriminator<std::monostate>
{
    static constexpr std::string_view value = "monostate";
};

template <typename T>
struct RootElementName
{
private:
    static consteval std::string_view get()
    {
        constexpr auto type = std::meta::dealias(^^T);
        if constexpr (std::meta::has_identifier(type))
            return std::meta::identifier_of(type);
        else
            return TypeDiscriminator<T>::value;
    }

public:
    static constexpr std::string_view value = get();
};

template <typename T, typename Allocator>
struct RootElementName<std::vector<T, Allocator>>
{
    static constexpr std::string_view value = "vector";
};

template <typename T>
struct RootElementName<std::optional<T>>
{
    static constexpr std::string_view value = "optional";
};

template <typename... T>
struct RootElementName<std::variant<T...>>
{
    static constexpr std::string_view value = "variant";
};

} // namespace mimo::serializer
