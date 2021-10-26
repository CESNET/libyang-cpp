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
class DataNodeSet;

template <typename Operation>
void handleLyTreeOperation(std::vector<DataNode*> nodes, Operation operation, std::shared_ptr<internal_refcount> newRefs);

struct internal_refcount;

class DataNodeSetIterator {
public:
    ~DataNodeSetIterator();
    friend DataNodeSet;
    DataNode operator*() const;
    DataNodeSetIterator& operator++();
    DataNodeSetIterator operator++(int);
    DataNodeSetIterator& operator--();
    DataNodeSetIterator operator--(int);
    DataNodeSetIterator operator-(int) const;
    DataNodeSetIterator operator+(int) const;
    bool operator==(const DataNodeSetIterator&) const;

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

    DataNodeSetIterator(lyd_node** start, lyd_node** const end, const DataNodeSet* set);
    lyd_node** m_start;
    lyd_node** m_current;
    lyd_node** const m_end;
    const DataNodeSet* m_set;
};

class DataNodeSet {
public:
    ~DataNodeSet();
    DataNodeSetIterator begin() const;
    DataNodeSetIterator end() const;
    DataNode front() const;
    DataNode back() const;

private:
    DataNodeSet(ly_set* set, std::shared_ptr<internal_refcount> refs);
    friend DataNode;
    friend DataNodeSetIterator;

    template <typename Operation>
    friend void handleLyTreeOperation(std::vector<DataNode*> nodes, Operation operation, std::shared_ptr<internal_refcount> newRefs);
    void invalidate();
    void throwIfInvalid() const;

    mutable std::set<DataNodeSetIterator*> m_iterators;
    std::shared_ptr<ly_set> m_set;
    std::shared_ptr<internal_refcount> m_refs;
    bool m_valid = true;
};
}
