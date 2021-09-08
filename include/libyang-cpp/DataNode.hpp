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
#include <libyang-cpp/Set.hpp>
#include <libyang-cpp/String.hpp>
#include <libyang-cpp/Value.hpp>
#include <set>

struct lyd_node;
struct ly_ctx;
namespace libyang {
class Context;
class DataNode;

struct internal_refcount;

class DataNodeTerm;

namespace impl {
std::optional<DataNode> newPath(lyd_node* node, ly_ctx* parent, std::shared_ptr<internal_refcount> refs, const char* path, const char* value, const std::optional<CreationOptions> options);
}

DataNode wrapRawNode(lyd_node* node);
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
    SchemaNode schema() const;
    std::optional<DataNode> newPath(const char* path, const char* value = nullptr, const std::optional<CreationOptions> options = std::nullopt) const;
    void newMeta(const Module& module, const char* name, const char* value);

    void unlink();

    void validateAll(const std::optional<ValidationOptions>& opts = std::nullopt);

    DfsCollection<DataNode> childrenDfs() const;

    friend Context;
    friend DataNodeSet;
    friend DataNodeTerm;
    friend DfsIterator<DataNode>;
    friend DataNodeSetIterator;
    friend DataNode wrapRawNode(lyd_node* node);

    bool operator==(const DataNode& node) const;

    friend std::optional<DataNode> impl::newPath(lyd_node* node, ly_ctx* parent, std::shared_ptr<internal_refcount> viewCount, const char* path, const char* value, const std::optional<CreationOptions> options);

protected:
    lyd_node* m_node;
private:
    DataNode(lyd_node* node, std::shared_ptr<ly_ctx> ctx);
    DataNode(lyd_node* node, std::shared_ptr<internal_refcount> viewCount);

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
}
