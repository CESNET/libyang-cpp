/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#include <memory>
#include <libyang-cpp/Enum.hpp>
#include <libyang-cpp/String.hpp>

struct lysc_node;
struct ly_ctx;
namespace libyang {
class Container;
class Context;
class DataNode;

/**
 * @brief Class representing a schema definition of a node.
 */
class SchemaNode {
public:
    String path() const;
    NodeType nodeType() const;
    Container asContainer() const;

    friend Context;
    friend DataNode;
protected:
    const lysc_node* m_node;
private:
    SchemaNode(const lysc_node* node, std::shared_ptr<ly_ctx> ctx);

    std::shared_ptr<ly_ctx> m_ctx;
};

class Container : SchemaNode {
public:
    using SchemaNode::path;
    using SchemaNode::nodeType;

    bool isPresence() const;

    friend SchemaNode;
private:
    using SchemaNode::SchemaNode;
};
}
