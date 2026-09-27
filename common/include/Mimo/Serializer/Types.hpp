#pragma once

#include <meta>
#include <string>

namespace mimo::serializer
{

consteval std::meta::info get_type(std::meta::info info)
{
    if (std::meta::is_type(info))
        return info;

    return std::meta::dealias(std::meta::remove_cvref(std::meta::type_of(info)));
}

consteval bool is_primitive_type(std::meta::info member)
{
    const auto type = get_type(member);
    const auto stringType = std::meta::dealias(^^std::string);

    return type == ^^int
        || type == ^^bool
        || type == ^^float
        || type == stringType;
}

consteval bool is_structure_type(std::meta::info member)
{
    const auto type = get_type(member);
    return !is_primitive_type(member) && std::meta::is_class_type(type);
}

} // namespace mimo::serializer
