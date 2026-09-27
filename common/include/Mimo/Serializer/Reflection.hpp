#pragma once

#include <any>
#include <meta>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include <Mimo/Serializer/Types.hpp>

namespace mimo::serializer
{

template <typename Annotation>
consteval bool has_annotation(std::meta::info member)
{
    return !std::meta::annotations_of_with_type(member, ^^Annotation).empty();
}

namespace detail
{
    template <typename A>
    inline constexpr char s_typeTag = 0;

    struct AnnotationEntry
    {
        const void* key;
        const void* value;
    };

}

class ReflectionNode
{
public:
    template <typename T>
    requires (!std::same_as<std::remove_cvref_t<T>, ReflectionNode>)
    explicit ReflectionNode(const T& object, std::string_view name="root")
        : ReflectionNode(object, name, buildTable<^^T>()) { }

    template <typename A>
    const A* getAnnotation() const
    {
        const void* key = &detail::s_typeTag<A>;
        for (const detail::AnnotationEntry& entry : m_annotations)
        {
            if (entry.key == key)
            {
                return static_cast<const A*>(entry.value);
            }
        }
        return nullptr;
    }

    template <typename A> [[nodiscard]]
    bool hasAnnotation() const
    {
        return getAnnotation<A>() != nullptr;
    }

    [[nodiscard]]
    std::string_view getName() const
    {
        return m_name;
    }

    [[nodiscard]]
    std::string_view getTypeName() const
    {
        return m_typeName;
    }

    [[nodiscard]]
    const std::vector<ReflectionNode>& getChildren() const
    {
        return m_children;
    }

private:
    template <typename T>
    ReflectionNode(const T& object,
                   std::string_view name,
                   std::span<const detail::AnnotationEntry> annotations)
        : m_name(name)
        , m_typeName(std::define_static_string(std::meta::display_string_of(^^T)))
        , m_annotations(annotations)
    {
        if constexpr (is_structure_type(^^T))
        {
            constexpr auto context = std::meta::access_context::unchecked();
            template for (constexpr std::meta::info member :
                          std::define_static_array(std::meta::nonstatic_data_members_of(^^T, context)))
            {
                constexpr std::string_view memberName =
                    std::meta::has_identifier(member)
                        ? std::meta::identifier_of(member)
                        : std::string_view{"<unnamed>"};

                // Member annotations first so they shadow type-level ones.
                constexpr auto memberType = std::meta::remove_cvref(std::meta::type_of(member));
                constexpr auto memberAnnotations = buildTable<member, memberType>();
                const auto& memberValue = object.[:member:];
                m_children.emplace_back(ReflectionNode(memberValue, memberName, memberAnnotations));

                // Compile error on GCC 16.1
                // m_children.emplace_back(ReflectionTree(object.[:member:], memberName, buildTable<member, memberType>()));
            }
        }
    }

    template <std::meta::info Source>
    static consteval void appendAnnotations(std::vector<detail::AnnotationEntry>& entries)
    {
        template for (constexpr auto annotation :
                      std::define_static_array(std::meta::annotations_of(Source)))
        {
            using A = [:std::meta::type_of(annotation):];
            entries.push_back({
                &detail::s_typeTag<A>,
                std::define_static_object([:std::meta::constant_of(annotation):])
            });
        }
    }

    template <std::meta::info... Sources>
    static consteval std::span<const detail::AnnotationEntry> buildTable()
    {
        std::vector<detail::AnnotationEntry> entries;
        (appendAnnotations<Sources>(entries), ...);
        return std::define_static_array(entries);
    }

    std::string_view m_name;
    std::string_view m_typeName;
    std::span<const detail::AnnotationEntry> m_annotations;
    std::vector<ReflectionNode> m_children;
};

struct NodeMetadata
{
    std::string_view name;
    std::optional<std::string_view> typeDiscriminator = std::nullopt;
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

template <typename Archive, typename T>
void serializeNode(
    Archive& archive,
    const T& value,
    NodeMetadata metadata)
{
    using Value = std::remove_cvref_t<T>;

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
            if constexpr (std::same_as<Alternative, std::monostate>)
                archive.empty(metadata);
            else
                serializeNode(archive, alternative, metadata);
        }, value);
    }
    else if constexpr (std::same_as<Value, std::any>)
    {
        if (value.has_value())
            archive.any(metadata, value);
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
            constexpr auto memberName =
                std::meta::identifier_of(member);

            serializeNode(
                archive,
                value.[:member:],
                NodeMetadata{.name = memberName});
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
