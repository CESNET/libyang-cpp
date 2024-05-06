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
        oss << "code: " << err.code << "\n        ";
        oss << "message: " << err.message << "\n        ";
        oss << "dataPath: " << (err.dataPath ? *err.dataPath : "std::nullopt") << "\n        ";
        oss << "schemaPath: " << (err.schemaPath ? *err.schemaPath : "std::nullopt") << "\n        ";
        oss << "line: " << err.line << "\n        ";
        oss << "level: " << err.level << "\n        ";
        oss << "validationCode: " << err.validationCode << "\n    }";
        return oss.str();
    });

    oss << "\n}";
    return oss.str().c_str();
}

doctest::String toString(const std::optional<std::string_view>& optString)
{
    if (!optString) {
        return "std::nullopt";
    }

    return std::string{*optString}.c_str();
}

doctest::String toString(const std::optional<libyang::DataNode>& optTree)
{
    if (!optTree) {
        return "std::nullopt";
    }

    auto printOut = optTree->path();

    return toString(printOut);
}
}

namespace libyang {
doctest::String toString(const Value& value)
{
    auto str = std::visit(ValuePrinter{}, value);
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
