/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include <libyang-cpp/DataNode.hpp>
#include <libyang-cpp/SchemaNode.hpp>
#include <libyang-cpp/Utils.hpp>
#include <memory>
#include <set>
#include <vector>

struct ly_set;
struct ly_ctx;
struct lyd_node;

namespace libyang {
class Context;
template <typename NodeType>
class Set;

template <typename Operation>
void handleLyTreeOperation(std::vector<DataNode*> nodes, Operation operation, std::shared_ptr<internal_refcount> newRefs);

struct internal_refcount;

template <typename NodeType>
class SetIterator {
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = NodeType;
    using reference = void;
    using difference_type = SetIterator;

    ~SetIterator();
    friend Set<NodeType>;
    NodeType operator*() const;
    SetIterator& operator++();
    SetIterator operator++(int);
    SetIterator& operator--();
    SetIterator operator--(int);
    SetIterator operator-(int) const;
    SetIterator operator+(int) const;
    bool operator==(const SetIterator&) const;

    struct NodeProxy {
        NodeType node;
        NodeType* operator->()
        {
            return &node;
        }
    };
    NodeProxy operator->() const;

private:
    void throwIfInvalid() const;

    SetIterator(underlying_node_t<NodeType>*const * start, underlying_node_t<NodeType>*const * end, const Set<NodeType>* set);
    underlying_node_t<NodeType>* const* m_start;
    underlying_node_t<NodeType>* const* m_current;
    underlying_node_t<NodeType>* const* const m_end;
    const Set<NodeType>* m_set;
};

/**
 * @brief An array-like collection of nodes.
 */
template <typename NodeType>
class Set {
public:
    ~Set();
    SetIterator<NodeType> begin() const;
    SetIterator<NodeType> end() const;
    NodeType front() const;
    NodeType back() const;
    uint32_t size() const;
    bool empty() const;

private:
    Set(ly_set* set, impl::refs_type_t<NodeType> refs);
    friend NodeType;
    friend SetIterator<NodeType>;
    friend Context;

    template <typename Operation>
    friend void handleLyTreeOperation(std::vector<NodeType*> nodes, Operation operation, std::shared_ptr<internal_refcount> newRefs);
    void invalidate();
    void throwIfInvalid() const;

    mutable std::set<SetIterator<NodeType>*> m_iterators;
    std::shared_ptr<ly_set> m_set;
    impl::refs_type_t<NodeType> m_refs;
    bool m_valid = true;
};
}
