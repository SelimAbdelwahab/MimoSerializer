#pragma once
#include <stack>
#include <format>
#include <string>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "XmlAuthor.hpp"
#include "XmlEscaping.hpp"

namespace mimo::serializer::xml
{

class ClashingWrite : public std::exception
{
    std::string m_message;

    explicit ClashingWrite(const std::string_view message)
        : m_message(message) {}

public:
    static ClashingWrite hasValue(const std::string_view element)
    {
        return ClashingWrite(std::format("Attempting to add children to element ({}) that already has a value.", element));
    }

    static ClashingWrite hasChildren(const std::string_view element)
    {
        return ClashingWrite(std::format("Attempting to assign value to element ({}) that already has children.", element));
    }

    [[nodiscard]] const char* what() const noexcept override
    {
        return m_message.c_str();
    }
};

class XmlWriter
{
public:
    XmlWriter(const XmlWriter&) = delete;
    XmlWriter& operator=(const XmlWriter&) = delete;

    XmlWriter(XmlWriter&&) = delete;
    XmlWriter& operator=(XmlWriter&&) = delete;

    explicit XmlWriter() : m_dummy({})
    {
        m_stack.push(&m_dummy);
    }

    void attribute(const std::string_view name, const std::string_view value)
    {
        if (atDummyLevel())
            throw std::runtime_error("Attempting to add attribute before starting element.");

        Element* current = m_stack.top();
        current->attributes.emplace_back(std::string(name), std::string(value));
    }

    void value(const std::string_view value)
    {
        if (atDummyLevel())
            throw std::runtime_error("Attempting to set value before starting element.");

        Element* current = m_stack.top();

        if (auto pVec = std::get_if<std::vector<Element>>(&current->value); pVec != nullptr && !pVec->empty())
            throw ClashingWrite::hasChildren(current->name);

        current->value = std::string(value);
    }

    class Scope
    {
        XmlWriter& m_writer;
        bool valid = true;
    public:
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
        Scope& operator=(Scope&&) = delete;

        Scope(XmlWriter& writer, const std::string_view name)
            : m_writer(writer)
        {
            writer.startElement(name);
        }

        Scope(Scope&& other) noexcept
            : m_writer(other.m_writer)
            , valid(std::exchange(other.valid, false))
        {
        }

        ~Scope()
        {
            if (valid)
                m_writer.endElement();
        }
    };

    [[nodiscard]]
    auto scope(const std::string_view name)
    {
        return Scope(*this, name);
    }

    [[nodiscard]]
    std::string toString() const
    {
        if (std::holds_alternative<std::string>(m_dummy.value))
            return "";

        std::ostringstream stream;
        for (auto& element : std::get<std::vector<Element>>(m_dummy.value))
            writeElement(stream, element);

        return stream.str();
    }
private:

    static void writeElement(std::ostream& stream, const Element& element, std::size_t indent=0)
    {
        const auto indentation = std::format("{:>{}}", "", indent);
        stream << std::format("{}<{}", indentation, element.name);

        bool hasAttributes = !element.attributes.empty();
        if (hasAttributes)
            stream << " " << writeAttributes(element);

        switch (element.elementType())
        {
            case Element::empty:
                stream << " />\n";
                break;
            case Element::field:
                stream << ">";
                detail::writeEscaped(stream, std::get<std::string>(element.value));
                stream << "</" << element.name << ">\n";
                break;
            case Element::complex:
                stream << ">\n";
                for (auto& child : std::get<std::vector<Element>>(element.value))
                    writeElement(stream, child, indent + 4u);

                stream << indentation;
                stream << std::format("</{}>\n", element.name);
                break;
            default:
                throw std::runtime_error("Unknown element type.");
        }
    }

    static std::string writeAttributes(const Element& element)
    {
        std::ostringstream stream;
        std::size_t sz = element.attributes.size();

        for (std::size_t i = 0; i < sz; i++)
        {
            const auto& attribute = element.attributes[i];
            stream << attribute.name << "=\"";
            detail::writeEscaped(stream, attribute.value, true);
            stream << '"';

            if (i == sz - 1)
                break;

            stream << " ";
        }

        return stream.str();
    }

    void startElement(const std::string_view name)
    {
        Element* current = m_stack.top();

        if (!std::holds_alternative<std::vector<Element>>(current->value))
        {
            if (!std::get<std::string>(current->value).empty())
                throw ClashingWrite::hasValue(current->name);

            current->value = std::vector<Element>();
        }

        m_stack.push(&std::get<std::vector<Element>>(current->value).emplace_back(name));
    }

    void endElement()
    {
        if (atDummyLevel())
            throw std::runtime_error("Attempting to end element before starting element.");

        m_stack.pop();
    }

    [[nodiscard]]
    bool atDummyLevel() const
    {
        return m_stack.size() == 1;
    }

    Element m_dummy;
    std::stack<Element*> m_stack;
};
}
