/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#include <libyang/libyang.h>
#include <stdexcept>
#include <string>
#include <libyang-cpp/DataNode.hpp>
#include <libyang-cpp/utils/enum.hpp>
#include <libyang-cpp/utils/exception.hpp>
namespace libyang {
struct internal_empty {
};

/**
 * @brief Wraps a completely new tree. Used only internally.
 */
DataNode::DataNode(lyd_node* node)
    : m_node(node)
    , m_viewCount(std::make_shared<internal_empty>())
{
}

/**
 * @brief Wraps an existing tree. Used only internally.
 */
DataNode::DataNode(lyd_node* node, std::shared_ptr<internal_empty> viewCount)
    : m_node(node)
    , m_viewCount(viewCount)
{
}

DataNode::~DataNode()
{
    if (m_viewCount.use_count() == 1) {
        lyd_free_all(m_node);
    }
}

/**
 * @brief Prints the tree into a string.
 * @param format Format of the output string.
 * @param flags Flags that change the behavior of the printing.
 */
String DataNode::printStr(const DataFormat format, const PrintFlags flags) const
{
    char* str;
    lyd_print_mem(&str, m_node, utils::toLydFormat(format), utils::toPrintFlags(flags));

    return String{str};
}

/**
 * Returns a view of the node specified by `path`.
 * If the node is not found, returns std::nullopt.
 * Throws on errors.
 *
 * @param path Node to search for.
 * @return DataView is the node is found, other std::nullopt.
 */
std::optional<DataNode> DataNode::findPath(const char* path) const
{
    lyd_node* node;
    auto err = lyd_find_path(m_node, path, false, &node);

    switch (err) {
    case LY_SUCCESS:
        return DataNode{node, m_viewCount};
    case LY_ENOTFOUND:
    case LY_EINCOMPLETE: // TODO: is this really important?
        return std::nullopt;
    default:
        throw LibyangErrorCode("Error in DataNode::findPath (" + std::to_string(err) + ")", err);
    }
}

/**
 * @brief Returns the path of the pointed-to node.
 */
String DataNode::path() const
{
    // TODO: handle all path types, not just LYD_PATH_STD

    auto str = lyd_path(m_node, LYD_PATH_STD, nullptr, 0);
    if (!str) {
        throw std::bad_alloc();
    }

    return String{str};
}

DataNodeTerm DataNode::asTerm() const
{
    if (!(m_node->schema->nodetype & LYD_NODE_TERM)) {
        throw LibyangError("Node is not a leaf or a leaflist");
    }

    return DataNodeTerm{m_node, m_viewCount};
}

std::string_view DataNodeTerm::valueStr() const
{
    return lyd_get_value(m_node);
}

Value DataNodeTerm::value() const
{
    auto value = reinterpret_cast<const lyd_node_term*>(m_node)->value;
    auto baseType = value.realtype->basetype;
    switch (baseType) {
    case LY_TYPE_INT8:
        return value.int8;
    case LY_TYPE_INT16:
        return value.int16;
    case LY_TYPE_INT32:
        return value.int32;
    case LY_TYPE_INT64:
        return value.int64;
    case LY_TYPE_UINT8:
        return value.uint8;
    case LY_TYPE_UINT16:
        return value.uint16;
    case LY_TYPE_UINT32:
        return value.uint32;
    case LY_TYPE_UINT64:
        return value.uint64;
    case LY_TYPE_BOOL:
        return static_cast<bool>(value.boolean);
    case LY_TYPE_EMPTY:
        return Empty{};
    case LY_TYPE_UNKNOWN:
        throw LibyangError("Unknown type");
    }
}
}
