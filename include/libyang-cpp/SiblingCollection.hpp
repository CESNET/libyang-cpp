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
#include <libyang-cpp/DataNode.hpp>

struct lyd_node;
struct lysc_node;
struct ly_ctx;

namespace libyang {
class SiblingCollection;

class SiblingIterator {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = DataNode;
    using reference = void;
    using difference_type = void;

    struct end {
    };

    ~SiblingIterator();
    SiblingIterator(const SiblingIterator&) noexcept;

    SiblingIterator& operator++();
    SiblingIterator operator++(int);
    DataNode operator*() const;

    struct NodeProxy {
        DataNode node;
        DataNode* operator->()
        {
            return &node;
        }
    };

    NodeProxy operator->() const;
    bool operator==(const SiblingIterator& it) const;

    friend SiblingCollection;
private:
    SiblingIterator(lyd_node* start, const SiblingCollection* coll);
    SiblingIterator(const end);

    lyd_node* m_current;

    const SiblingCollection* m_collection;

    void throwIfInvalid() const;

    void registerThis();
    void unregisterThis();
};

class SiblingCollection {
public:
    friend DataNode;
    friend SiblingIterator;
    friend SchemaNode;
    ~SiblingCollection();
    SiblingCollection(const DfsCollection<NodeType>&);
    SiblingCollection& operator=(const SiblingCollection& other);

    SiblingIterator begin() const;
    SiblingIterator end() const;
private:

    SiblingCollection(lyd_node* start, std::shared_ptr<internal_refcount> refs);
    lyd_node* m_start;

    std::shared_ptr<internal_refcount> m_refs;
    bool m_valid = true;

    // mutable is needed:
    // `begin` and `end` need to be const
    // because of that SiblingIterator can only get a `const SiblingCollection*`,
    // however, SiblingIterator needs to register itself into m_iterators.
    mutable std::set<SiblingIterator*> m_iterators;
    void invalidateIterators();

    void throwIfInvalid() const;
};
}
