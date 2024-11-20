/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#include <libyang-cpp/Enum.hpp>
#include <libyang-cpp/Type.hpp>
#include <libyang-cpp/export.h>
#include <memory>
#include <optional>
#include <vector>

struct lysc_node;
struct lysc_when;
struct ly_ctx;
namespace libyang {
class AnyDataAnyXML;
class ActionRpc;
class ActionRpcInput;
class ActionRpcOutput;
class Case;
class Choice;
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
class When;
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
class LIBYANG_CPP_EXPORT SchemaNode {
public:
    Module module() const;
    std::string path(PathType pathType = PathType::Data) const;
    std::string name() const;
    std::optional<std::string> description() const;
    Status status() const;
    Config config() const;
    bool isInput() const;
    NodeType nodeType() const;
    // It is possible to cast SchemaNode to another type via the following methods. The types are children classes of
    // SchemaNode. No problems with slicing can occur, because these types are value-based and aren't constructible
    // drectly by the user.
    // TODO: turn these into a templated `as<>` method.
    AnyDataAnyXML asAnyDataAnyXML() const;
    Case asCase() const;
    Choice asChoice() const;
    Container asContainer() const;
    Leaf asLeaf() const;
    LeafList asLeafList() const;
    List asList() const;
    ActionRpc asActionRpc() const;

    std::optional<SchemaNode> child() const;
    std::optional<SchemaNode> parent() const;
    ChildInstanstiables childInstantiables() const;
    Collection<SchemaNode, IterationType::Dfs> childrenDfs() const;
    Collection<SchemaNode, IterationType::Sibling> siblings() const;
    Collection<SchemaNode, IterationType::Sibling> immediateChildren() const;
    std::vector<ExtensionInstance> extensionInstances() const;

    std::vector<When> when() const;

    std::string printStr(const SchemaOutputFormat format, const std::optional<SchemaPrintFlags> flags = std::nullopt, std::optional<size_t> lineLength = std::nullopt) const;

    friend Context;
    friend DataNode;
    friend List;
    friend Module;
    friend ChildInstanstiablesIterator;
    friend Iterator<SchemaNode, IterationType::Dfs>;
    friend Iterator<SchemaNode, IterationType::Sibling>;
    friend Set<SchemaNode>;
    friend SetIterator<SchemaNode>;

    bool operator==(const SchemaNode& other) const;
    bool operator!=(const SchemaNode& other) const;

protected:
    const lysc_node* m_node;
    std::shared_ptr<ly_ctx> m_ctx;
    SchemaNode(const lysc_node* node, std::shared_ptr<ly_ctx> ctx);
    SchemaNode(const lysc_node* node, std::nullptr_t);
};

/**
 * @brief Contains information about a when statement.
 *
 * Wraps `lysc_when`.
 */
class LIBYANG_CPP_EXPORT When {
public:
    std::string condition() const;
    std::optional<std::string> description() const;

private:
    const lysc_when* m_when;
    std::shared_ptr<ly_ctx> m_ctx;
    When(const lysc_when* m_when, std::shared_ptr<ly_ctx> ctx);
    friend class SchemaNode;
};

/**
 * @brief Class representing a schema definition of a `anydata` or `anyxml` node.
 */
class LIBYANG_CPP_EXPORT AnyDataAnyXML : public SchemaNode {
public:
    bool isMandatory() const;
    friend SchemaNode;

private:
    using SchemaNode::SchemaNode;
};

/**
 * @brief Class representing a schema definition of a `case` node.
 *
 * Wraps `lysc_node_choice`.
 */
class LIBYANG_CPP_EXPORT Case : public SchemaNode {
public:
    friend SchemaNode;
    friend Choice;

private:
    using SchemaNode::SchemaNode;
};

/**
 * @brief Class representing a schema definition of a `choice` node.
 *
 * Wraps `lysc_node_choice`.
 */
class LIBYANG_CPP_EXPORT Choice : public SchemaNode {
public:
    bool isMandatory() const;
    std::vector<Case> cases() const;
    std::optional<Case> defaultCase() const;
    friend SchemaNode;

private:
    using SchemaNode::SchemaNode;
};

/**
 * @brief Class representing a schema definition of a `container` node.
 */
class LIBYANG_CPP_EXPORT Container : public SchemaNode {
public:
    bool isMandatory() const;
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
class LIBYANG_CPP_EXPORT Leaf : public SchemaNode {
public:
    bool isKey() const;
    bool isMandatory() const;
    types::Type valueType() const;
    std::optional<std::string> defaultValueStr() const;
    std::optional<std::string> units() const;
    friend SchemaNode;

private:
    using SchemaNode::SchemaNode;
};

/**
 * @brief Class representing a schema definition of a `leaflist` node.
 *
 * Wraps `lysc_node_leaflist`.
 */
class LIBYANG_CPP_EXPORT LeafList : public SchemaNode {
public:
    bool isMandatory() const;
    types::Type valueType() const;
    libyang::types::constraints::ListSize maxElements() const;
    libyang::types::constraints::ListSize minElements() const;
    std::optional<std::string> units() const;
    bool isUserOrdered() const;
    friend SchemaNode;

private:
    using SchemaNode::SchemaNode;
};

/**
 * @brief Class representing a schema definition of a `list` node.
 *
 * Wraps `lysc_node_list`.
 */
class LIBYANG_CPP_EXPORT List : public SchemaNode {
public:
    bool isMandatory() const;
    std::vector<Leaf> keys() const;
    libyang::types::constraints::ListSize maxElements() const;
    libyang::types::constraints::ListSize minElements() const;
    bool isUserOrdered() const;
    friend SchemaNode;

private:
    using SchemaNode::SchemaNode;
};

/**
 * @brief Class representing a schema definition of an `input` node of an RPC/action node.
 *
 * Wraps `lysc_node_action_inout`.
 */
class LIBYANG_CPP_EXPORT ActionRpcInput : public SchemaNode {
public:
    friend ActionRpc;

private:
    using SchemaNode::SchemaNode;
};

/**
 * @brief Class representing a schema definition of an `output` node of an RPC/action node.
 *
 * Wraps `lysc_node_action_inout`.
 */
class LIBYANG_CPP_EXPORT ActionRpcOutput : public SchemaNode {
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
class LIBYANG_CPP_EXPORT ActionRpc : public SchemaNode {
public:
    ActionRpcInput input() const;
    ActionRpcOutput output() const;
    friend SchemaNode;

private:
    using SchemaNode::SchemaNode;
};
}
