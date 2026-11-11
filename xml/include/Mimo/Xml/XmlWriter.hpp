#pragma once

#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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
    XmlWriter() = default;
    XmlWriter(const XmlWriter&) = delete;
    XmlWriter& operator=(const XmlWriter&) = delete;
    XmlWriter(XmlWriter&&) = delete;
    XmlWriter& operator=(XmlWriter&&) = delete;

    // Attributes must precede text or child elements.
    void attribute(std::string_view name, std::string_view value)
    {
        auto& current = currentElement();
        if (current.content != Content::pending)
            throw std::runtime_error("Attributes must precede element content.");

        m_output += ' ';
        m_output.append(name);
        m_output += "=\"";
        detail::appendEscaped(m_output, value, true);
        m_output += '"';
    }

    void value(std::string_view value)
    {
        auto& current = currentElement();
        if (current.content == Content::children)
            throw ClashingWrite::hasChildren(current.name);
        if (current.content == Content::text)
            throw std::runtime_error("Element already has a value.");

        current.content = Content::text;
        current.hasText = !value.empty();
        if (current.hasText)
        {
            m_output += '>';
            detail::appendEscaped(m_output, value);
        }
    }

    class Scope
    {
        XmlWriter& m_writer;
        bool valid = true;
    public:
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
        Scope& operator=(Scope&&) = delete;

        Scope(XmlWriter& writer, std::string_view name)
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

    [[nodiscard]] auto scope(std::string_view name)
    {
        return Scope(*this, name);
    }

    // Return a complete snapshot, including any currently open scopes.
    [[nodiscard]] std::string toString() const
    {
        auto output = m_output;
        for (std::size_t depth = m_stack.size(); depth > 0; --depth)
            closeElement(output, m_stack[depth - 1], depth - 1);
        return output;
    }

private:
    enum class Content { pending, text, children };

    struct Frame
    {
        std::string name;
        Content content = Content::pending;
        bool hasText = false;
    };

    Frame& currentElement()
    {
        if (m_stack.empty())
            throw std::runtime_error("Attempting to write before starting element.");
        return m_stack.back();
    }

    static void closeElement(std::string& output, const Frame& frame,
                             std::size_t depth)
    {
        if (frame.content != Content::children && !frame.hasText)
        {
            output += " />\n";
            return;
        }

        if (frame.content == Content::children)
            output.append(depth * 4, ' ');
        output += "</";
        output += frame.name;
        output += ">\n";
    }

    void startElement(std::string_view name)
    {
        // Own the name before stack growth can invalidate references.
        Frame next{.name = std::string(name)};
        if (!m_stack.empty())
        {
            auto& parent = m_stack.back();
            if (parent.hasText)
                throw ClashingWrite::hasValue(parent.name);
            if (parent.content != Content::children)
                m_output += ">\n";
            parent.content = Content::children;
        }

        m_output.append(m_stack.size() * 4, ' ');
        m_output += '<';
        m_output += next.name;
        m_stack.push_back(std::move(next));
    }

    void endElement()
    {
        closeElement(m_output, m_stack.back(), m_stack.size() - 1);
        m_stack.pop_back();
    }

    std::string m_output;
    std::vector<Frame> m_stack;
};
}
