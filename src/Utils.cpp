/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <algorithm>
#include <experimental/iterator>
#include <iomanip>
#include <libyang-cpp/Utils.hpp>
#include <sstream>
#include "utils/enum.hpp"

namespace libyang {
LogOptions setLogOptions(const libyang::LogOptions options)
{
    return static_cast<LogOptions>(ly_log_options(utils::toLogOptions(options)));
}

/**
 * Sets a new log level for libyang. Returns the old log level.
 */
LogLevel setLogLevel(const LogLevel level)
{
    return utils::toLogLevel(ly_log_level(utils::toLogLevel(level)));
}

bool SomeOrder::operator()(const DataNode& a, const DataNode& b) const
{
    return getRawNode(a) < getRawNode(b);
}

bool SomeOrder::operator()(const Identity& a, const Identity& b) const
{
    return std::make_tuple(a.module().name(), a.name()) < std::make_tuple(b.module().name(), b.name());
}

std::string ValuePrinter::operator()(const libyang::Empty) const
{
    return "empty";
}

std::string ValuePrinter::operator()(const std::vector<libyang::Bit>& val) const
{
    std::ostringstream oss;
    std::transform(val.begin(), val.end(), std::experimental::make_ostream_joiner(oss, " "), [] (const auto& bit) {
        return bit.name;
    });
    return oss.str();
}

std::string ValuePrinter::operator()(const libyang::Decimal64& val) const
{
    std::ostringstream oss;
    const auto mul = impl::pow10int(val.digits);
    oss << (val.number / mul) << '.' << std::setfill('0') << std::setw(val.digits) << impl::abs(val.number % mul);
    return oss.str();
}

std::string ValuePrinter::operator()(const libyang::Binary& val) const
{
    return val.base64;
}

std::string ValuePrinter::operator()(const libyang::Enum& val) const
{
    return val.name;
}

std::string ValuePrinter::operator()(const libyang::IdentityRef& val) const
{
    return val.module + ":" + val.name;
}

std::string ValuePrinter::operator()(const std::optional<libyang::DataNode>& val) const
{
    if (!val) {
        return "InstanceIdentifier{no-instance}";
    }

    return std::string{val->path()} + ": " + std::visit(*this, val->asTerm().value());
}

template <typename ValueType>
std::string ValuePrinter::operator()(const ValueType& val) const
{
    std::ostringstream oss;
    if constexpr (std::is_same_v<ValueType, uint8_t> || std::is_same_v<ValueType, int8_t>) {
        oss << static_cast<int>(val);
    } else if constexpr (std::is_same_v<ValueType, bool>) {
        oss << std::boolalpha << val;
    } else {
        oss << val;
    }

    return oss.str();
}

std::string qualifiedName(const Identity& identity)
{
    return std::string{identity.module().name()} + ':' + std::string{identity.name()};
}
}
