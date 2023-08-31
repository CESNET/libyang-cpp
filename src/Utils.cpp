/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <algorithm>
#include <experimental/iterator>
#include <inttypes.h>
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

std::string ValuePrinter::operator()(const libyang::InstanceIdentifier& val) const
{
    if (val.node()) {
        return "InstanceIdentifier{" + val.path + "}";
    } else {
        return "InstanceIdentifier{no-instance, " + val.path + "}";
    }
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
template std::string LIBYANG_CPP_EXPORT ValuePrinter::operator()(const uint8_t& val) const;
template std::string LIBYANG_CPP_EXPORT ValuePrinter::operator()(const int8_t& val) const;
template std::string LIBYANG_CPP_EXPORT ValuePrinter::operator()(const uint16_t& val) const;
template std::string LIBYANG_CPP_EXPORT ValuePrinter::operator()(const int16_t& val) const;
template std::string LIBYANG_CPP_EXPORT ValuePrinter::operator()(const uint32_t& val) const;
template std::string LIBYANG_CPP_EXPORT ValuePrinter::operator()(const int32_t& val) const;
template std::string LIBYANG_CPP_EXPORT ValuePrinter::operator()(const uint64_t& val) const;
template std::string LIBYANG_CPP_EXPORT ValuePrinter::operator()(const int64_t& val) const;
template std::string LIBYANG_CPP_EXPORT ValuePrinter::operator()(const bool& val) const;
template std::string LIBYANG_CPP_EXPORT ValuePrinter::operator()(const std::string& val) const;

std::string qualifiedName(const Identity& identity)
{
    return std::string{identity.module().name()} + ':' + std::string{identity.name()};
}

InstanceIdentifier::InstanceIdentifier(const std::string& path, const std::optional<DataNode>& node)
    : path(path)
    , m_node(node ? std::make_any<DataNode>(*node) : std::any{})
{
    if (node && node->path() != path) {
        throw Error{"instance-identifier: got path " + path + ", but the node points to " + node->path()};
    }
}

bool InstanceIdentifier::operator==(const InstanceIdentifier& other) const
{
    if (this->path != other.path)
        return false;
    if (this->m_node.has_value() != other.m_node.has_value())
        return false;
    if (this->m_node.has_value() && std::any_cast<DataNode>(this->m_node) != std::any_cast<DataNode>(other.m_node))
        return false;
    return true;
}

std::optional<DataNode> InstanceIdentifier::node() const
{
    if (m_node.has_value())
        return std::any_cast<DataNode>(m_node);
    return std::nullopt;
}

Decimal64::operator std::string() const
{
    char buf[
        18 // maximal fraction-digits
        + 1 // decimal point
        + 1 // non-fractional digits
        + 1 // minus sign
        + 1 // trailing NUL byte
    ];
    static_assert(sizeof buf == 22);
    if (snprintf(buf, sizeof(buf), "%" PRId64 ".%0*" PRId64,
                number / impl::pow10int(digits), digits, impl::abs(number % impl::pow10int(digits)))
            >= static_cast<ssize_t>(sizeof(buf))) {
        throw std::runtime_error{"libayng::Decimal64::operator std::string(): buffer overflow"};
    }
    return buf;
}
}
