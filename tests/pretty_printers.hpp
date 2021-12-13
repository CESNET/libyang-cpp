/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#include <doctest/doctest.h>
#include <experimental/iterator>
#include <libyang-cpp/Context.hpp>
#include <sstream>

using namespace std::string_literals;

namespace std {
doctest::String toString(const std::vector<std::string>& vec)
{
    std::ostringstream oss;
    oss << "std::vector<std::string>{\n    ";
    std::copy(vec.begin(), vec.end(), std::experimental::make_ostream_joiner(oss, ",\n    "));

    oss << "\n}";

    return oss.str().c_str();
}

doctest::String toString(const std::vector<std::pair<std::string, std::string>>& vec)
{
    std::ostringstream oss;
    oss << "std::vector<std::pair<std::string, std::string>>{\n    ";
    std::transform(vec.begin(), vec.end(), std::experimental::make_ostream_joiner(oss, ",\n    "), [] (auto pair) { return "{" + pair.first + ", " + pair.second + "}"; });

    oss << "\n}";

    return oss.str().c_str();
}

doctest::String toString(const std::vector<int32_t>& vec)
{
    std::ostringstream oss;
    oss << "std::vector<std::string>{\n    ";
    std::copy(vec.begin(), vec.end(), std::experimental::make_ostream_joiner(oss, ",\n    "));

    oss << "\n}";

    return oss.str().c_str();
}

doctest::String toString(const std::vector<libyang::ErrorInfo>& errors)
{
    std::ostringstream oss;
    oss << "std::vector{\n    ";

    std::transform(errors.begin(), errors.end(), std::experimental::make_ostream_joiner(oss, ",\n   "), [] (const libyang::ErrorInfo err) {
        std::ostringstream oss;
        oss << "libyang::ErrorInfo{\n        ";
        oss << "appTag: " << (err.appTag ? *err.appTag : "std::nullopt")  << "\n        ";
        oss << "code: " << static_cast<std::underlying_type_t<decltype(err.code)>>(err.code) << "\n        ";
        oss << "message: " << err.message << "\n        ";
        oss << "path: " << (err.path ? *err.path : "std::nullopt") << "\n        ";
        oss << "level: " << static_cast<std::underlying_type_t<decltype(err.level)>>(err.level) << "\n        ";
        oss << "validationCode: " << static_cast<std::underlying_type_t<decltype(err.level)>>(err.validationCode) << "\n    }";
        return oss.str();
    });

    oss << "\n}";
    return oss.str().c_str();
}
}

namespace libyang {
doctest::String toString(const String& value)
{
    return std::string{value}.c_str();
}

namespace {
struct impl_toStruct {
    std::string operator()(const Binary& value)
    {
        std::ostringstream oss;
        oss << std::hex;
        std::transform(value.data.begin(), value.data.end(), std::experimental::make_ostream_joiner(oss, ", "), [](uint8_t num) {
            return std::to_string(static_cast<unsigned int>(num));
        });
        return oss.str();
    }

    std::string operator()(const Empty)
    {
        return "<empty>";
    }

    std::string operator()(const std::string& value)
    {
        return value;
    }

    std::string operator()(const std::optional<DataNode>& value)
    {
        if (!value) {
            return "std::nullopt (DataNode)";
        }

        return std::string{value->path()};
    }

    std::string operator()(const Decimal64& value)
    {
        return "Decimal64{"s + std::to_string(value.number) + ", " + std::to_string(value.digits) + "}";
    }

    std::string operator()(const std::vector<Bit>& value)
    {
        std::ostringstream oss;
        oss << "Bits{";
        std::transform(value.begin(), value.end(), std::experimental::make_ostream_joiner(oss, ", "), [](const Bit& bit) {
            return bit.name + "(" + std::to_string(bit.position) + ")";
        });
        oss << "}";
        return oss.str();
    }

    std::string operator()(const Enum& value)
    {
        return "Enum{"s + value.name + "}";
    }

    std::string operator()(const IdentityRef& value)
    {
        return "IdentityRef{"s + value.module + ":" + value.name + "}";
    }

    template <typename Type>
    std::string operator()(const Type& value)
    {
        return std::to_string(value);
    }
};
}
doctest::String toString(const Value& value)
{
    auto str = std::visit(impl_toStruct{}, value);
    return str.c_str();
}
doctest::String toString(const std::vector<libyang::DataNode>& nodes)
{
    std::ostringstream oss;
    std::transform(nodes.begin(), nodes.end(), std::experimental::make_ostream_joiner(oss, ", "), [](const DataNode& node) {
        return "DataNode -> " + std::string{node.path()};
    });

    return oss.str().c_str();
}
}
