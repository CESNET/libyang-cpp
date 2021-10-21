/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#include <iterator>
#include <libyang-cpp/Enum.hpp>
#include <memory>
#include <set>
#include <vector>

struct lyd_node;
struct lysc_node;
struct ly_ctx;

namespace libyang {

template <typename NodeType, IterationType ITER_TYPE>
class Collection;
class DataNode;
class SchemaNode;
template <typename NodeType>
struct underlying_node;
template <>
struct underlying_node<SchemaNode> {
    using type = const lysc_node;
};
template <>
struct underlying_node<DataNode> {
    using type = lyd_node;
};

template <typename NodeType>
using underlying_node_t = typename underlying_node<NodeType>::type;
struct internal_refcount;

class DataNode;

template <typename Operation>
void handleLyTreeOperation(std::vector<DataNode*> nodes, Operation operation, std::shared_ptr<internal_refcount> newRefs);

template <typename NodeType, IterationType ITER_TYPE>
class Iterator {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = NodeType;
    using reference = void;
    using difference_type = void;

    struct end {
    };

    ~Iterator();
    Iterator(const Iterator&);

    Iterator<NodeType, ITER_TYPE>& operator++();
    Iterator<NodeType, ITER_TYPE> operator++(int);
    NodeType operator*() const;

    struct NodeProxy {
        NodeType node;
        NodeType* operator->()
        {
            return &node;
        }
    };

    NodeProxy operator->() const;
    bool operator==(const Iterator& it) const;

    friend Collection<NodeType, ITER_TYPE>;

private:
    Iterator(underlying_node_t<NodeType>* start, const Collection<NodeType, ITER_TYPE>* coll);
    Iterator(const end);
    underlying_node_t<NodeType>* m_current;

    underlying_node_t<NodeType>* m_start;
    underlying_node_t<NodeType>* m_next;

    const Collection<NodeType, ITER_TYPE>* m_collection;

    void throwIfInvalid() const;

    void registerThis();
    void unregisterThis();
};

namespace impl {
template <typename RefType>
struct refs_type;

template <typename RefType>
using refs_type_t = typename refs_type<RefType>::type;

template <>
struct refs_type<DataNode> {
    using type = internal_refcount;
};

template <>
struct refs_type<SchemaNode> {
    using type = ly_ctx;
};
}

template <typename NodeType, IterationType ITER_TYPE>
class Collection {
public:
    friend DataNode;
    friend Iterator<NodeType, ITER_TYPE>;
    friend SchemaNode;
    ~Collection();
    Collection(const Collection<NodeType, ITER_TYPE>&);
    Collection& operator=(const Collection<NodeType, ITER_TYPE>&);

    Iterator<NodeType, ITER_TYPE> begin() const;
    Iterator<NodeType, ITER_TYPE> end() const;

private:
    Collection(underlying_node_t<NodeType>* start, std::shared_ptr<impl::refs_type_t<NodeType>> refs);
    underlying_node_t<NodeType>* m_start;

    std::shared_ptr<impl::refs_type_t<NodeType>> m_refs;
    bool m_valid = true;

    // mutable is needed:
    // `begin` and `end` need to be const
    // because of that DfsIterator can only get a `const DataNodeCollectionDfs*`,
    // however, DfsIterator needs to register itself into m_iterators.
    mutable std::set<Iterator<NodeType, ITER_TYPE>*> m_iterators;
    void invalidate();

    template <typename Operation>
    friend void handleLyTreeOperation(std::vector<DataNode*> nodes, Operation operation, std::shared_ptr<internal_refcount> newRefs);


    void throwIfInvalid() const;
};
}
