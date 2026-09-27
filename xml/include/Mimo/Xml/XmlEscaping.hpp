#pragma once

#include <ostream>
#include <string_view>

namespace mimo::serializer::xml::detail
{
inline void writeEscaped(std::ostream& stream, std::string_view value,
                         bool attribute = false)
{
    for (char ch : value)
    {
        switch (ch)
        {
            case '&': stream << "&amp;"; break;
            case '<': stream << "&lt;"; break;
            case '>': stream << "&gt;"; break;
            case '"':
                if (attribute) stream << "&quot;";
                else stream << ch;
                break;
            // References preserve whitespace through XML parser normalization.
            case '\r': stream << "&#13;"; break;
            case '\n':
                if (attribute) stream << "&#10;";
                else stream << ch;
                break;
            case '\t':
                if (attribute) stream << "&#9;";
                else stream << ch;
                break;
            default: stream << ch; break;
        }
    }
}
}
