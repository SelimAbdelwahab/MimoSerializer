#pragma once

#include <any>
#include <meta>
#include <mutex>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <utility>

#include <Mimo/Serializer/Reflection.hpp>

#include "Mimo/Xml/XmlWriter.hpp"

namespace mimo::serializer::xml
{

class XmlArchive
{
public:
    template <typename T>
    void value(const NodeMetadata& metadata, const T& value)
    {
        auto scope = beginNode(metadata);
        m_writer.value(std::format("{}", value));
    }

    XmlWriter::Scope object(const NodeMetadata& metadata)
    {
        return beginNode(metadata);
    }

    XmlWriter::Scope sequence(const NodeMetadata& metadata)
    {
        return beginNode(metadata);
    }

    XmlWriter::Scope empty(const NodeMetadata& metadata)
    {
        return beginNode(metadata);
    }

    void any(NodeMetadata metadata, const std::any& value)
    {
        if (writeBuiltinAny<bool>(metadata, value)
            || writeBuiltinAny<int>(metadata, value)
            || writeBuiltinAny<float>(metadata, value)
            || writeBuiltinAny<std::string>(metadata, value))
        {
            return;
        }

        AnyType entry;
        {
            std::scoped_lock lock(anyTypesMutex());
            const auto iterator = anyTypes().find(std::type_index(value.type()));
            if (iterator == anyTypes().end())
            {
                throw std::runtime_error(std::format(
                    "No XML serializer is registered for std::any type '{}'.",
                    value.type().name()));
            }
            entry = iterator->second;
        }

        metadata.typeDiscriminator = entry.discriminator;
        entry.write(*this, metadata, value);
    }

    template <typename T>
    static void registerAnyType(std::string discriminator)
    {
        AnyType entry{
            .discriminator = std::move(discriminator),
            .write = [](XmlArchive& archive,
                        NodeMetadata metadata,
                        const std::any& value)
            {
                serializeNode(
                    archive,
                    std::any_cast<const T&>(value),
                    metadata);
            },
        };

        std::scoped_lock lock(anyTypesMutex());
        anyTypes().insert_or_assign(std::type_index(typeid(T)), std::move(entry));
    }

    [[nodiscard]]
    std::string finalize() const
    {
        return m_writer.toString();
    }
private:
    struct AnyType
    {
        std::string discriminator;
        void (*write)(XmlArchive&, NodeMetadata, const std::any&){};
    };

    template <typename T>
    bool writeBuiltinAny(NodeMetadata metadata, const std::any& value)
    {
        if (value.type() != typeid(T))
            return false;

        metadata.typeDiscriminator = TypeDiscriminator<T>::value;
        serializeNode(*this, std::any_cast<const T&>(value), metadata);
        return true;
    }

    static std::unordered_map<std::type_index, AnyType>& anyTypes()
    {
        static std::unordered_map<std::type_index, AnyType> types;
        return types;
    }

    static std::mutex& anyTypesMutex()
    {
        static std::mutex mutex;
        return mutex;
    }

    XmlWriter::Scope beginNode(const NodeMetadata& metadata)
    {
        auto scope = m_writer.scope(metadata.name);
        if (metadata.typeDiscriminator.has_value())
            m_writer.attribute("type", *metadata.typeDiscriminator);

        return scope;
    }

    XmlWriter m_writer;
};

template <typename T>
[[nodiscard]] auto serialize(
    const T& object,
    NodeMetadata root) -> std::string
{
    XmlArchive archive;
    serializeNode(archive, object, root);
    return archive.finalize();
}

template <typename T>
[[nodiscard]] auto serialize(const T& object) -> std::string
{
    using Type = std::remove_cvref_t<T>;
    return serialize(object, NodeMetadata{.name = RootElementName<Type>::value});
}

} // namespace mimo::serializer::xml
