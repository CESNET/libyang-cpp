/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#include <libyang-cpp/Enum.hpp>
#include <libyang-cpp/String.hpp>
#include <libyang-cpp/Type.hpp>
#include <memory>
#include <optional>
#include <vector>

struct lysc_node;
struct ly_ctx;
namespace libyang {
class ActionRpc;
class ActionRpcInput;
class ActionRpcOutput;
class Container;
class Leaf;
class LeafList;
class List;
class Context;
class DataNode;
class SchemaNode;
class ChildInstanstiables;
class ChildInstanstiablesIterator;
class Module;
template <typename NodeType>
class Set;
template <typename NodeType>
class SetIterator;
template <typename NodeType, IterationType ITER_TYPE>
class Collection;
template <typename NodeType, IterationType ITER_TYPE>
class Iterator;

/**
 * @brief Class representing a schema definition of a node.
 *
 * Wraps `lysc_node`.
 */
class SchemaNode {
public:
    Module module() const;
    String path() const;
    std::string_view name() const;
    std::optional<std::string_view> description() const;
    Status status() const;
    Config config() const;
    NodeType nodeType() const;
    // TODO: turn these into a templated `as<>` method.
    Container asContainer() const;
    Leaf asLeaf() const;
    LeafList asLeafList() const;
    List asList() const;
    ActionRpc asActionRpc() const;

    std::optional<SchemaNode> child() const;
    std::optional<SchemaNode> parent() const;
    ChildInstanstiables childInstantiables() const;
    Collection<SchemaNode, IterationType::Dfs> childrenDfs() const;

    friend Context;
    friend DataNode;
    friend List;
    friend ChildInstanstiablesIterator;
    friend Iterator<SchemaNode, IterationType::Dfs>;
    friend Set<SchemaNode>;
    friend SetIterator<SchemaNode>;

protected:
    const lysc_node* m_node;
    std::shared_ptr<ly_ctx> m_ctx;
    SchemaNode(const lysc_node* node, std::shared_ptr<ly_ctx> ctx);
    SchemaNode(const lysc_node* node, std::nullptr_t);
};

/**
 * @brief Class representing a schema definition of a `container` node.
 */
class Container : public SchemaNode {
public:
    bool isPresence() const;
    friend SchemaNode;

private:
    using SchemaNode::SchemaNode;
};

/**
 * @brief Class representing a schema definition of a `leaf` node.
 *
 * Wraps `lysc_node_leaf`.
 */
class Leaf : public SchemaNode {
public:
    bool isKey() const;
    Type valueType() const;
    std::optional<std::string_view> defaultValueStr() const;
    std::optional<std::string_view> units() const;
    friend SchemaNode;

private:
    using SchemaNode::SchemaNode;
};

/**
 * @brief Class representing a schema definition of a `leaflist` node.
 *
 * Wraps `lysc_node_leaflist`.
 */
class LeafList : public SchemaNode {
public:
    Type valueType() const;
    std::optional<std::string_view> units() const;
    friend SchemaNode;

private:
    using SchemaNode::SchemaNode;
};

/**
 * @brief Class representing a schema definition of a `list` node.
 *
 * Wraps `lysc_node_list`.
 */
class List : public SchemaNode {
public:
    std::vector<Leaf> keys() const;
    friend SchemaNode;

private:
    using SchemaNode::SchemaNode;
};

/**
 * @brief Class representing a schema definition of an `input` node.
 *
 * Wraps `lysc_node_action_inout`.
 */
class ActionRpcInput : public SchemaNode {
public:
    friend ActionRpc;

private:
    using SchemaNode::SchemaNode;
};

/**
 * @brief Class representing a schema definition of an `output` node.
 *
 * Wraps `lysc_node_action_inout`.
 */
class ActionRpcOutput : public SchemaNode {
public:
    friend ActionRpc;

private:
    using SchemaNode::SchemaNode;
};

/**
 * @brief Class representing a schema definition of a `action` or `rpc` node.
 *
 * Wraps `lysc_node_action`.
 */
class ActionRpc : public SchemaNode {
public:
    ActionRpcInput input() const;
    ActionRpcOutput output() const;
    friend SchemaNode;

private:
    using SchemaNode::SchemaNode;
};
}
