/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include <libyang-cpp/DataNode.hpp>
#include <memory>
#include <set>
#include <vector>

struct ly_set;
struct ly_ctx;
struct lyd_node;

namespace libyang {
class Context;
class DataNode;
template <typename NodeType>
class Set;

template <typename Operation>
void handleLyTreeOperation(std::vector<DataNode*> nodes, Operation operation, std::shared_ptr<internal_refcount> newRefs);

struct internal_refcount;

template <typename NodeType>
class SetIterator {
public:
    ~SetIterator();
    friend Set<NodeType>;
    DataNode operator*() const;
    SetIterator& operator++();
    SetIterator operator++(int);
    SetIterator& operator--();
    SetIterator operator--(int);
    SetIterator operator-(int) const;
    SetIterator operator+(int) const;
    bool operator==(const SetIterator&) const;

    struct DataNodeProxy {
        DataNode node;
        DataNode* operator->()
        {
            return &node;
        }
    };
    DataNodeProxy operator->() const;

private:
    void throwIfInvalid() const;

    SetIterator(lyd_node** start, lyd_node** const end, const Set<NodeType>* set);
    lyd_node** m_start;
    lyd_node** m_current;
    lyd_node** const m_end;
    const Set<NodeType>* m_set;
};

template <typename NodeType>
class Set {
public:
    ~Set();
    SetIterator<NodeType> begin() const;
    SetIterator<NodeType> end() const;
    NodeType front() const;
    NodeType back() const;

private:
    Set(ly_set* set, std::shared_ptr<internal_refcount> refs);
    friend NodeType;
    friend SetIterator<NodeType>;

    template <typename Operation>
    friend void handleLyTreeOperation(std::vector<DataNode*> nodes, Operation operation, std::shared_ptr<internal_refcount> newRefs);
    void invalidate();
    void throwIfInvalid() const;

    mutable std::set<SetIterator<NodeType>*> m_iterators;
    std::shared_ptr<ly_set> m_set;
    std::shared_ptr<internal_refcount> m_refs;
    bool m_valid = true;
};
}
