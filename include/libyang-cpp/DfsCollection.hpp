/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#include <iterator>
#include <memory>
#include <set>

struct lyd_node;
struct lysc_node;

namespace libyang {
class DataNode;
class DataNodeCollectionDfs;

struct internal_refcount;

class DfsIterator {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = DataNode;
    using reference = void;
    using difference_type = void;

    struct end {
    };

    ~DfsIterator();
    DfsIterator(const DfsIterator&);

    DfsIterator& operator++();
    DfsIterator operator++(int);
    DataNode operator*() const;

    template <typename NodeType>
    struct NodeProxy {
        NodeType node;
        NodeType* operator->()
        {
            return &node;
        }
    };


    NodeProxy<DataNode> operator->() const;
    bool operator==(const DfsIterator& it) const;

    friend DataNodeCollectionDfs;
private:
    DfsIterator(lyd_node* start, const DataNodeCollectionDfs* coll);
    DfsIterator(const end);
    lyd_node* m_current;

    lyd_node* m_start;
    lyd_node* m_next;

    const DataNodeCollectionDfs* m_collection;

    void throwIfInvalid() const;

    void registerThis();
    void unregisterThis();
};

class DataNodeCollectionDfs {
public:
    friend DataNode;
    friend DfsIterator;
    ~DataNodeCollectionDfs();
    DataNodeCollectionDfs(const DataNodeCollectionDfs&);
    DataNodeCollectionDfs& operator=(const DataNodeCollectionDfs&);

    DfsIterator begin() const;
    DfsIterator end() const;
private:
    DataNodeCollectionDfs(lyd_node* start, std::shared_ptr<internal_refcount> refs);
    lyd_node* m_start;
    std::shared_ptr<internal_refcount> m_refs;
    bool m_valid = true;

    // mutable is needed:
    // `begin` and `end` need to be const
    // because of that DfsIterator can only get a `const DataNodeCollectionDfs*`,
    // however, DfsIterator needs to register itself into m_iterators.
    mutable std::set<DfsIterator*> m_iterators;
    void invalidateIterators();

    void throwIfInvalid() const;
};
}
