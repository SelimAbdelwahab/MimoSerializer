#pragma once

#include <format>
#include <string>
#include <type_traits>

#include <Mimo/Serializer/SerializeNode.hpp>

#include "Mimo/Xml/XmlWriter.hpp"

namespace mimo::serializer::xml
{

class XmlArchive : public OutputArchive<XmlArchive>
{
public:
    [[nodiscard]]
    std::string finalize() const
    {
        return m_writer.toString();
    }
private:
    friend class mimo::serializer::OutputArchive<XmlArchive>;

    template <typename T>
    void valueImpl(const NodeMetadata& metadata, const T& value)
    {
        auto scope = beginNode(metadata);
        m_writer.value(std::format("{}", value));
    }

    XmlWriter::Scope objectImpl(const NodeMetadata& metadata)
    {
        return beginNode(metadata);
    }

    XmlWriter::Scope sequenceImpl(const NodeMetadata& metadata)
    {
        return beginNode(metadata);
    }

    void emptyImpl(const NodeMetadata& metadata)
    {
        auto scope = beginNode(metadata);
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
