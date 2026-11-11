#pragma once

#include <concepts>
#include <meta>
#include <stdexcept>
#include <type_traits>
#include <variant>

#include <Mimo/Serializer/Detail/TypeTraits.hpp>
#include <Mimo/Serializer/OutputArchive.hpp>
#include <Mimo/Serializer/Types.hpp>

namespace mimo::serializer
{

template <typename Archive, typename T>
void serializeNode(
    OutputArchive<Archive>& archive,
    const T& value,
    NodeMetadata metadata)
{
    using Value = std::remove_cvref_t<T>;
    metadata = metadata.withType<Value>();

    if constexpr (detail::isOptional<Value>)
    {
        if (value.has_value())
            serializeNode(archive, *value, metadata);
    }
    else if constexpr (detail::isVector<Value>)
    {
        auto scope = archive.sequence(metadata);
        for (const auto& item : value)
            serializeNode(archive, item, NodeMetadata{.name = "item"});
    }
    else if constexpr (detail::isVariant<Value>)
    {
        if (value.valueless_by_exception())
            throw std::runtime_error("Cannot serialize a valueless std::variant.");

        std::visit([&](const auto& alternative)
        {
            using Alternative = std::remove_cvref_t<decltype(alternative)>;
            metadata.typeDiscriminator = TypeDiscriminator<Alternative>::value;
            metadata = metadata.withType<Alternative>();
            if constexpr (std::same_as<Alternative, std::monostate>)
                archive.empty(metadata);
            else
                serializeNode(archive, alternative, metadata);
        }, value);
    }
    else if constexpr (is_primitive_type(^^T))
    {
        archive.value(metadata, value);
    }
    else if constexpr (is_structure_type(^^T))
    {
        auto scope = archive.object(metadata);

        constexpr auto context =
            std::meta::access_context::unchecked();

        template for (constexpr auto member :
            std::define_static_array(
                std::meta::nonstatic_data_members_of(^^T, context)))
        {
            serializeNode(
                archive,
                value.[:member:],
                NodeMetadata::forMember<member>());
        }
    }
    else
    {
        static_assert(detail::alwaysFalse<T>,
            "Mimo cannot serialize this type yet. Only primitive and reflected "
            "structure types are currently supported.");
    }
}

} // namespace mimo::serializer
