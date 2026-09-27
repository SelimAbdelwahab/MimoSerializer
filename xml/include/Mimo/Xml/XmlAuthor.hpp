#pragma once
#include <string_view>
#include <variant>
#include <vector>
#include <string>

namespace mimo::serializer::xml
{
struct attribute
{
    std::string name;
    std::string value;
};

struct Element
{
    enum Type
    {
        empty,
        field,
        complex
    };

    Element(const std::string_view name)
        : name(name)
        , value(std::vector<Element>{}) {}

    std::string name;
    std::vector<attribute> attributes;
    std::variant<std::string, std::vector<Element>> value;

    [[nodiscard]]
    Type elementType() const
    {
        if (std::holds_alternative<std::vector<Element>>(value))
        {
            if (std::get<std::vector<Element>>(value).empty())
                return Type::empty;
            return Type::complex;
        }

        if (std::get<std::string>(value).empty())
            return Type::empty;

        return Type::field;
    }
};
}
