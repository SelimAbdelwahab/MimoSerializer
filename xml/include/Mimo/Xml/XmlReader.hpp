#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>

#include "XmlAuthor.hpp"

namespace mimo::serializer::xml
{

// Invalid XML and unsupported XML features are reported through the same error.
// The offset is a zero-based byte position in the original input; input.size()
// denotes an unexpected end of input.
class XmlReadError : public std::runtime_error
{
public:
    XmlReadError(std::string message, std::size_t offset);

    [[nodiscard]] std::size_t offset() const noexcept;

private:
    std::size_t m_offset;
};

// Parses the XML data model used by XmlWriter into the existing Element tree.
// This is a serializer-oriented XML subset, not a general-purpose XML processor.
//
// Input:
// - One complete UTF-8 XML 1.0 document with exactly one root element.
// - An optional UTF-8 BOM and XML declaration; any declared encoding must be
//   UTF-8. Whitespace and comments may surround the root.
// - Nested elements, self-closing elements, and attributes with either quote
//   style. Names and closing tags are case-sensitive; attributes must be unique
//   within an element. Repeated sibling names are allowed.
// - The five predefined entities and decimal/hexadecimal character references
//   in text and attributes. CDATA sections contribute literal text.
//
// Result:
// - Element names, attributes, and children retain their input order and case.
// - Comments are discarded. Adjacent text/CDATA sections are combined.
// - Text-only content preserves whitespace after XML line-ending normalization;
//   attribute values follow XML normalization for undeclared (CDATA) attributes.
//   Character references are decoded after literal whitespace normalization.
// - For elements with children, whitespace-only text between/beside children is
//   discarded. Non-whitespace text alongside children is rejected: Element's
//   value can hold either text or children, not mixed content.
// - <x/> and <x></x> both produce an empty children vector. Whitespace-only
//   text without children remains a string, not an empty element.
//
// Rejected:
// - Malformed XML, invalid UTF-8/XML characters, mismatched tags, duplicate
//   attributes, unknown entities, multiple roots, or trailing non-comment data.
// - DTDs, custom/external entities, namespace declarations or qualified names,
//   and processing instructions other than the initial XML declaration.
//   No filesystem or network resources are accessed.
//
// C++ value conversion, reflection, field matching, type discriminators, schema
// validation, and stream/incremental parsing belong outside this first version.
class XmlReader
{
public:
    // Interface only: parsing and error definitions will be implemented next.
    // The input is borrowed only during this call; the returned tree owns all
    // strings. Calls are independent and retain no parsing state.
    // Throws XmlReadError on invalid/unsupported input; no partial tree escapes.
    [[nodiscard]] Element read(std::string_view input) const;
};

} // namespace mimo::serializer::xml
