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
#include <libyang-cpp/utils/exception.hpp>
#include "utils/enum.hpp"
#include "utils/findPath.hpp"
#include "utils/ref_count.hpp"
namespace libyang {
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
        throw ErrorCode("Error in DataNode::findPath (" + std::to_string(err) + ")", err);
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

/**
 * @brief Creates a new node with the supplied path, changing this tree.
 *
 * @param path Path of the new node.
 * @param value String representation of the value. Use nullptr for non-leaf nodes and the `empty` type.
 * @param options Options that change the behavior of this method.
 * @return If a new node got created, returns it. Otherwise returns std::nullopt.
 */
std::optional<DataNode> DataNode::newPath(const char* path, const char* value, const std::optional<CreationOptions> options)
{
    return impl::newPath(m_node, nullptr, m_viewCount, path, value, options);
}

DataNodeTerm DataNode::asTerm() const
{
    if (!(m_node->schema->nodetype & LYD_NODE_TERM)) {
        throw Error("Node is not a leaf or a leaflist");
    }

    return DataNodeTerm{m_node, m_viewCount};
}

std::string_view DataNodeTerm::valueStr() const
{
    return lyd_get_value(m_node);
}
}
