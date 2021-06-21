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
#include <libyang-cpp/Enum.hpp>
#include <libyang-cpp/SchemaNode.hpp>
#include <libyang-cpp/String.hpp>
#include <libyang-cpp/Value.hpp>

struct lyd_node;
struct ly_ctx;
namespace libyang {
class Context;
class DataNode;
class DfsIterator;
class DataNodeCollection;

struct internal_refcount;

class DataNodeTerm;

namespace impl {
std::optional<DataNode> newPath(lyd_node* node, ly_ctx* parent, std::shared_ptr<internal_refcount> refs, const char* path, const char* value, const std::optional<CreationOptions> options);
}
/**
 * @brief Class representing a node in a libyang tree.
 */
class DataNode {
public:
    ~DataNode();
    DataNode(const DataNode& node);
    DataNode& operator=(const DataNode& node);

    String printStr(const DataFormat format, const PrintFlags flags) const;
    std::optional<DataNode> findPath(const char* path) const;
    String path() const;
    DataNodeTerm asTerm() const;
    SchemaNode schema() const;
    std::optional<DataNode> newPath(const char* path, const char* value = nullptr, const std::optional<CreationOptions> options = std::nullopt);

    void unlink();

    DataNodeCollection iterateDepthFirst() const;

    friend Context;
    friend DataNodeTerm;
    friend DfsIterator;

    bool operator==(const DataNode& node) const;

    friend std::optional<DataNode> impl::newPath(lyd_node* node, ly_ctx* parent, std::shared_ptr<internal_refcount> viewCount, const char* path, const char* value, const std::optional<CreationOptions> options);

protected:
    lyd_node* m_node;
private:
    DataNode(lyd_node* node, std::shared_ptr<ly_ctx> ctx);
    DataNode(lyd_node* node, std::shared_ptr<internal_refcount> viewCount);

    void registerRef();
    void unregisterRef();

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

class DataNodeCollection;

class DfsIterator {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = DataNode;
    using reference = void;
    using difference_type = void;

    ~DfsIterator();

    struct end {
    };

    DfsIterator& operator++();
    DfsIterator& operator++(int);
    DataNode operator*() const;

    struct DataNodeProxy {
        DataNode node;
        DataNode* operator->();
    };

    DataNodeProxy operator->() const;
    bool operator==(const DfsIterator& it) const;

    friend DataNodeCollection;
private:
    DfsIterator(lyd_node* start, std::shared_ptr<internal_refcount> refs);
    DfsIterator(const end);
    lyd_node* m_current;

    lyd_node* m_start;
    lyd_node* m_next;

    std::shared_ptr<internal_refcount> m_refs;

    bool valid = true;

    void throwIfInvalid() const;
};

class DataNodeCollection {
public:
    friend DataNode;
    ~DataNodeCollection();

    DfsIterator begin() const;
    DfsIterator end() const;
private:
    DataNodeCollection(lyd_node* start, std::shared_ptr<internal_refcount> refs);
    lyd_node* m_start;
    std::shared_ptr<internal_refcount> m_refs;
    bool valid = true;

    void invalidate();
    void validOrThrow() const;
};
}
