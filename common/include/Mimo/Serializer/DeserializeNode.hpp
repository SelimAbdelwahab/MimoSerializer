#pragma once

#include <concepts>
#include <cstddef>
#include <meta>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

#include <Mimo/Serializer/Detail/TypeTraits.hpp>
#include <Mimo/Serializer/InputArchive.hpp>
#include <Mimo/Serializer/Types.hpp>

namespace mimo::serializer
{

template <typename Archive, typename T>
void deserializeNode(InputArchive<Archive>& archive, T& value, NodeMetadata metadata);

namespace detail
{
    template <typename Variant, std::size_t... I>
    std::size_t variantIndex(std::string_view discriminator, std::index_sequence<I...>)
    {
        std::size_t index = std::variant_npos;
        std::size_t matches = 0;
        ((TypeDiscriminator<std::variant_alternative_t<I, Variant>>::value == discriminator
            ? (index = I, ++matches, void()) : void()), ...);
        if (matches != 1)
            throw std::runtime_error("Variant discriminator must identify exactly one alternative.");
        return index;
    }

    template <typename Archive, typename Variant, std::size_t... I>
    void deserializeVariant(InputArchive<Archive>& archive, Variant& value, NodeMetadata metadata,
                            std::size_t index, std::index_sequence<I...>)
    {
        auto read = [&]<std::size_t Index>()
        {
            using Alternative = std::variant_alternative_t<Index, Variant>;
            metadata.typeDiscriminator = TypeDiscriminator<Alternative>::value;
            metadata = metadata.withType<Alternative>();
            Alternative alternative{};
            if constexpr (std::same_as<Alternative, std::monostate>)
                archive.empty(metadata);
            else
                deserializeNode(archive, alternative, metadata);
            value.template emplace<Index>(std::move(alternative));
        };
        ((index == I ? (read.template operator()<I>(), void()) : void()), ...);
    }
}

// InputArchive.hpp defines the required archive operations and scope behavior.
// Missing optionals reset; present optionals and vector/variant entries must be
// default-constructible. Vectors replace previous contents and retain allocators.
// Objects update in place, so an error can leave earlier members updated.
template <typename Archive, typename T>
void deserializeNode(InputArchive<Archive>& archive, T& value, NodeMetadata metadata)
{
    using Value = std::remove_cvref_t<T>;
    metadata = metadata.withType<Value>();
    static_assert(!std::is_const_v<T>, "Deserialization requires a mutable destination.");

    if constexpr (detail::isOptional<Value>)
    {
        if (!archive.contains(metadata))
        {
            value.reset();
            return;
        }
        typename Value::value_type replacement{};
        deserializeNode(archive, replacement, metadata);
        value = std::move(replacement);
    }
    else if constexpr (detail::isVector<Value>)
    {
        auto scope = archive.sequence(metadata);
        Value replacement(value.get_allocator());
        while (scope.hasNext())
        {
            typename Value::value_type item{};
            deserializeNode(archive, item, NodeMetadata{.name = "item"});
            replacement.push_back(std::move(item));
        }
        value = std::move(replacement);
    }
    else if constexpr (detail::isVariant<Value>)
    {
        const auto discriminator = archive.typeDiscriminator(metadata);
        if (!discriminator)
            throw std::runtime_error("Cannot deserialize a variant without a type discriminator.");
        constexpr auto indices = std::make_index_sequence<std::variant_size_v<Value>>{};
        const auto index = detail::variantIndex<Value>(*discriminator, indices);
        detail::deserializeVariant(archive, value, metadata, index, indices);
    }
    else if constexpr (is_primitive_type(^^T))
    {
        archive.value(metadata, value);
    }
    else if constexpr (is_structure_type(^^T))
    {
        auto scope = archive.object(metadata);
        constexpr auto context = std::meta::access_context::unchecked();
        template for (constexpr auto member : std::define_static_array(
            std::meta::nonstatic_data_members_of(^^T, context)))
        {
            deserializeNode(archive, value.[:member:],
                NodeMetadata::forMember<member>());
        }
    }
    else
    {
        static_assert(detail::alwaysFalse<T>, "Mimo cannot deserialize this type yet.");
    }
}

} // namespace mimo::serializer
