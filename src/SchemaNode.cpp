/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <libyang/libyang.h>
#include <libyang-cpp/SchemaNode.hpp>
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

NodeType SchemaNode::nodeType() const
{
    return utils::toNodeType(m_node->nodetype);
}
}
