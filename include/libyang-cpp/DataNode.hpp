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
#include <memory>
#include <optional>
#include <libyang-cpp/DfsCollection.hpp>
#include <libyang-cpp/Enum.hpp>
#include <libyang-cpp/SchemaNode.hpp>
#include <libyang-cpp/String.hpp>
#include <libyang-cpp/Value.hpp>
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

    void unlink();

    void validateAll(const std::optional<ValidationOptions>& opts = std::nullopt);

    DfsCollection<DataNode> childrenDfs() const;

    ParsedOp parseOp(const char* input, const DataFormat format, const OperationType opType) const;

    friend Context;
    friend DataNodeAny;
    friend DataNodeSet;
    friend DataNodeTerm;
    friend DfsIterator<DataNode>;
    friend DataNodeSetIterator;
    friend DataNode wrapRawNode(lyd_node* node);
    friend const DataNode wrapUnmanagedRawNode(const lyd_node* node);

    bool operator==(const DataNode& node) const;

    friend std::optional<DataNode> impl::newPath(lyd_node* node, ly_ctx* parent, std::shared_ptr<internal_refcount> viewCount, const char* path, const char* value, const std::optional<CreationOptions> options);

protected:
    lyd_node* m_node;
private:
    DataNode(lyd_node* node, std::shared_ptr<ly_ctx> ctx);
    DataNode(lyd_node* node, std::shared_ptr<internal_refcount> viewCount);
    DataNode(lyd_node* node, const unmanaged_tag);

    void registerRef();
    void unregisterRef();
    void freeIfNoRefs();

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
