/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <libyang/tree.h>
#include <libyang/tree_schema.h>
#include <libyang-cpp/SchemaNode.hpp>
#include <libyang-cpp/utils/exception.hpp>
#include <libyang/libyang.h>
#include "utils/enum.hpp"

namespace libyang {
SchemaNode::SchemaNode(const lysc_node* node, std::shared_ptr<ly_ctx> ctx)
    : m_node(node)
    , m_ctx(ctx)
{
}

/**
 * Returns the schema noda of this node.
 */
String SchemaNode::path() const
{
    // TODO: support all path formats
    auto str = lysc_path(m_node, LYSC_PATH_DATA, nullptr, 0);

    if (!str) {
        throw std::bad_alloc();
    }

    return String{str};
}

/**
 * Returns the name of the node.
 */
std::string_view SchemaNode::name() const
{
    return m_node->name;
}

NodeType SchemaNode::nodeType() const
{
    return utils::toNodeType(m_node->nodetype);
}

Container SchemaNode::asContainer() const
{
    if (nodeType() != NodeType::Container) {
        throw Error("Schema node is not a container: " + std::string{path()});
    }

    return Container{m_node, m_ctx};
}

Leaf SchemaNode::asLeaf() const
{
    if (nodeType() != NodeType::Leaf) {
        throw Error("Schema node is not a leaf: " + std::string{path()});
    }

    return Leaf{m_node, m_ctx};
}

List SchemaNode::asList() const
{
    if (nodeType() != NodeType::List) {
        throw Error("Schema node is not a list: " + std::string{path()});
    }

    return List{m_node, m_ctx};
}

bool Container::isPresence() const
{
    return !lysc_is_np_cont(m_node);
}

bool Leaf::isKey() const
{
    return lysc_is_key(m_node);
}

/**
 * Retrieves type info about the leaf.
 */
Type Leaf::leafType() const
{
    return Type{reinterpret_cast<const lysc_node_leaf*>(m_node)->type, m_ctx};
}

/**
 * Returns key nodes of the list.
 */
std::vector<Leaf> List::keys() const
{
    auto list = reinterpret_cast<const lysc_node_list*>(m_node);
    std::vector<Leaf> res;
    lysc_node* elem;
    LY_LIST_FOR(list->child, elem) {
        if (lysc_is_key(elem)) {
            Leaf leaf(elem, m_ctx);
            res.emplace_back(std::move(leaf));
        }
    }

    return res;
}
}
