/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include <libyang-cpp/DataNode.hpp>
#include <libyang-cpp/Enum.hpp>
#include <memory>
#include <stdexcept>

struct ly_ctx;
struct lysc_node;
struct lyd_meta;
struct lyd_node;

namespace libyang {
LogOptions setLogOptions(const libyang::LogOptions options);
LogLevel setLogLevel(const LogLevel level);

/**
 * A generic libyang error. All other libyang errors inherit from this exception type.
 */
class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/**
 * A libyang error containing a message and an error code.
 */
class ErrorWithCode : public Error {
public:
    explicit ErrorWithCode(const std::string& what, uint32_t errCode);

    ErrorCode code();

private:
    ErrorCode m_errCode;
};

class DataNode;
class Meta;
class SchemaNode;

template <typename NodeType>
struct underlying_node;
template <>
struct underlying_node<SchemaNode> {
    using type = const lysc_node;
};
template <>
struct underlying_node<DataNode> {
    using type = lyd_node;
};
template <>
struct underlying_node<Meta> {
    using type = lyd_meta;
};

template <typename NodeType>
using underlying_node_t = typename underlying_node<NodeType>::type;
struct internal_refcount;

namespace impl {
template <typename RefType>
struct refs_type;

template <typename RefType>
using refs_type_t = typename refs_type<RefType>::type;

template <>
struct refs_type<DataNode> {
    using type = std::shared_ptr<internal_refcount>;
};

template <>
struct refs_type<SchemaNode> {
    using type = std::shared_ptr<ly_ctx>;
};

template <>
struct refs_type<Meta> {
    using type = libyang::DataNode;
};
}

struct PointerCompare {
    bool operator()(const DataNode& a, const DataNode& b) const;
};

/**
 * @brief A string conversion visitor for libyang::Value.
 */
struct ValuePrinter {
    std::string operator()(const libyang::Empty) const;
    std::string operator()(const std::vector<libyang::types::Bits::Bit>& val) const;
    std::string operator()(const libyang::Decimal64& val) const;
    std::string operator()(const libyang::Binary& val) const;
    std::string operator()(const libyang::types::Enumeration::Enum& val) const;
    std::string operator()(const libyang::IdentityRef& val) const;
    std::string operator()(const std::optional<libyang::DataNode>& val) const;
    template <typename ValueType>
    std::string operator()(const ValueType& val) const;
};
}
