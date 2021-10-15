/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include <cstdlib>
#include <iterator>
#include <libyang-cpp/Collection.hpp>
#include <libyang-cpp/Enum.hpp>
#include <libyang-cpp/SchemaNode.hpp>
#include <libyang-cpp/String.hpp>
#include <libyang-cpp/Value.hpp>
#include <memory>
#include <optional>
#include <set>

struct lyd_node;
struct ly_ctx;
namespace libyang {
class Context;
class DataNode;
class DataNodeSet;
class DataNodeSetIterator;

struct internal_refcount;

class DataNodeAny;
class DataNodeOpaque;
class DataNodeTerm;
struct ParsedOp;

namespace impl {
std::optional<DataNode> newPath(lyd_node* node, ly_ctx* parent, std::shared_ptr<internal_refcount> refs, const char* path, const char* value, const std::optional<CreationOptions> options);
}

DataNode wrapRawNode(lyd_node* node);
const DataNode wrapUnmanagedRawNode(const lyd_node* node);
lyd_node* releaseRawNode(DataNode node);
lyd_node* getRawNode(DataNode node);

template <typename Operation>
void handleLyTreeOperation(std::vector<DataNode*> nodes, Operation operation, std::shared_ptr<internal_refcount> newRefs);

void validateAll(std::optional<libyang::DataNode>& node, const std::optional<ValidationOptions>& opts = std::nullopt);

struct unmanaged_tag {
};

/**
 * @brief Class representing a node in a libyang tree.
 */
class DataNode {
public:
    ~DataNode();
    DataNode(const DataNode& node);
    DataNode& operator=(const DataNode& node);

    DataNode previousSibling() const;
    std::optional<DataNode> nextSibling() const;
    String printStr(const DataFormat format, const PrintFlags flags) const;
    std::optional<DataNode> findPath(const char* path, const OutputNodes output = OutputNodes::No) const;
    DataNodeSet findXPath(const char* xpath) const;
    String path() const;
    DataNodeTerm asTerm() const;
    DataNodeAny asAny() const;
    SchemaNode schema() const;
    std::optional<DataNode> newPath(const char* path, const char* value = nullptr, const std::optional<CreationOptions> options = std::nullopt) const;
    void newMeta(const Module& module, const char* name, const char* value);

    bool isOpaque() const;
    DataNodeOpaque asOpaque() const;

    // TODO: allow setting the `parent` argument
    DataNode duplicateWithSiblings(const std::optional<DuplicationOptions> opts = std::nullopt) const;
    void unlink();

    Collection<DataNode, IterationType::Dfs> childrenDfs() const;

    Collection<DataNode, IterationType::Sibling> siblings() const;

    ParsedOp parseOp(const char* input, const DataFormat format, const OperationType opType) const;

    friend Context;
    friend DataNodeAny;
    friend DataNodeSet;
    friend DataNodeTerm;
    friend Iterator<DataNode, IterationType::Dfs>;
    friend Iterator<DataNode, IterationType::Sibling>;
    friend DataNodeSetIterator;
    friend DataNode wrapRawNode(lyd_node* node);
    friend const DataNode wrapUnmanagedRawNode(const lyd_node* node);
    friend lyd_node* releaseRawNode(DataNode node);
    friend lyd_node* getRawNode(DataNode node);

    friend void validateAll(std::optional<libyang::DataNode>& node, const std::optional<ValidationOptions>& opts);

    bool operator==(const DataNode& node) const;

    friend std::optional<DataNode> impl::newPath(lyd_node* node, ly_ctx* parent, std::shared_ptr<internal_refcount> viewCount, const char* path, const char* value, const std::optional<CreationOptions> options);

protected:
    lyd_node* m_node;

private:
    DataNode(lyd_node* node, std::shared_ptr<ly_ctx> ctx);
    DataNode(lyd_node* node, std::shared_ptr<internal_refcount> viewCount);
    DataNode(lyd_node* node, const unmanaged_tag);

    [[nodiscard]] std::vector<DataNode*> getFollowingSiblingRefs();
    void registerRef();
    void unregisterRef();
    void freeIfNoRefs();

    template <typename Operation>
    friend void handleLyTreeOperation(std::vector<DataNode*> nodes, Operation operation, std::shared_ptr<internal_refcount> newRefs);

    std::shared_ptr<internal_refcount> m_refs;
};

/**
 * @brief Class representing a term node - leaf or leaf-list.
 */
class DataNodeTerm : public DataNode {
public:
    std::string_view valueStr() const;

    friend DataNode;
    Value value() const;

private:
    using DataNode::DataNode;
};

struct OpaqueName {
    std::optional<std::string_view> prefix;
    std::string_view name;
};

class DataNodeOpaque : public DataNode {
public:
    OpaqueName name() const;
    std::string_view value() const;
    friend DataNode;

private:
    using DataNode::DataNode;
};

/**
 * @brief Class representing a node of type anydata.
 *
 * TODO: Add anyxml support for this class.
 */
class DataNodeAny : public DataNode {
public:
    friend DataNode;
    AnydataValue releaseValue();

private:
    using DataNode::DataNode;
};

struct ParsedOp {
    std::optional<libyang::DataNode> tree;
    std::optional<libyang::DataNode> op;
};
}
