#pragma once

#include <ostream>
#include <string>
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

inline void appendEscaped(std::string& output, std::string_view value,
                          bool attribute = false)
{
    std::size_t start = 0;
    for (std::size_t i = 0; i < value.size(); ++i)
    {
        std::string_view replacement;
        switch (value[i])
        {
            case '&': replacement = "&amp;"; break;
            case '<': replacement = "&lt;"; break;
            case '>': replacement = "&gt;"; break;
            case '\"': if (attribute) replacement = "&quot;"; break;
            case '\r': replacement = "&#13;"; break;
            case '\n': if (attribute) replacement = "&#10;"; break;
            case '\t': if (attribute) replacement = "&#9;"; break;
            default: break;
        }
        if (!replacement.empty())
        {
            output.append(value.substr(start, i - start));
            output.append(replacement);
            start = i + 1;
        }
    }
    output.append(value.substr(start));
}

}
