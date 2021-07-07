/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#include <memory>
#include <optional>
#include <libyang-cpp/Enum.hpp>
#include <libyang-cpp/String.hpp>
#include <libyang-cpp/Type.hpp>
#include <vector>

struct lysc_node;
struct ly_ctx;
namespace libyang {
class Container;
class Leaf;
class List;
class Context;
class DataNode;

/**
 * @brief Class representing a schema definition of a node.
 */
class SchemaNode {
public:
    String path() const;
    std::string_view name() const;
    std::optional<std::string_view> description() const;
    Status status() const;
    Config config() const;
    NodeType nodeType() const;
    // TODO: turn these into a templated `as<>` method.
    Container asContainer() const;
    Leaf asLeaf() const;
    List asList() const;

    friend Context;
    friend DataNode;
    friend List;
protected:
    const lysc_node* m_node;
    std::shared_ptr<ly_ctx> m_ctx;
private:
    SchemaNode(const lysc_node* node, std::shared_ptr<ly_ctx> ctx);
};

class Container : public SchemaNode {
public:
    bool isPresence() const;

    friend SchemaNode;
private:
    using SchemaNode::SchemaNode;
};

class Leaf : public SchemaNode {
public:
    bool isKey() const;
    Type valueType() const;
    std::optional<std::string_view> defaultValueStr() const;
    friend SchemaNode;
private:
    using SchemaNode::SchemaNode;
};

class List : public SchemaNode {
public:
    std::vector<Leaf> keys() const;
    friend SchemaNode;
private:
    using SchemaNode::SchemaNode;
};
}
