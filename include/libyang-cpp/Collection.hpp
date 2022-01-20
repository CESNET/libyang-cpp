/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#include <iterator>
#include <libyang-cpp/DataNode.hpp>
#include <libyang-cpp/Enum.hpp>
#include <libyang-cpp/Utils.hpp>
#include <memory>
#include <set>
#include <vector>

struct lyd_meta;
struct lyd_node;
struct lysc_node;
struct ly_ctx;

namespace libyang {

template <typename NodeType, IterationType ITER_TYPE>
class Collection;
class DataNode;
class Meta;
class MetaCollection;
class SchemaNode;

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
    Iterator& operator=(const Iterator& it);

    friend Collection<NodeType, ITER_TYPE>;
    friend MetaCollection;

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

/**
 * @brief A collection of SchemaNode or DataNode supporting multiple iteration types.
 *
 * For more info on iteration types, see these methods:
 * - DataNode::childrenDfs
 * - DataNode::siblings
 * - SchemaNode::siblings
 * - SchemaNode::childrenDfs
 */
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
    bool empty() const;

protected:
    Collection(underlying_node_t<NodeType>* start, impl::refs_type_t<NodeType> refs);
    underlying_node_t<NodeType>* m_start;

    impl::refs_type_t<NodeType> m_refs;
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

/**
 * @brief A collection for iterating over metadata of a DataNode.
 *
 * For more information, check DataNode::meta.
 */
class MetaCollection : public Collection<Meta, IterationType::Meta> {
public:
    Iterator<Meta, IterationType::Meta> erase(Iterator<Meta, IterationType::Meta> what);
private:
    friend DataNode;
    using Collection<Meta, IterationType::Meta>::Collection;
};
}
