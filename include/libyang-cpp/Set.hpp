/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
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

struct internal_refcount;

class DataNodeSetIterator {
public:
    ~DataNodeSetIterator();
    friend DataNodeSet;
    DataNode operator*() const;
    DataNodeSetIterator& operator++();
    DataNodeSetIterator operator++(int);
    bool operator==(const DataNodeSetIterator&) const;

    template<typename NodeType>
    struct DataNodeProxy {
        NodeType node;
        NodeType* operator->()
        {
            return &node;
        }
    };
    DataNodeProxy<DataNode> operator->() const;
private:
    void throwIfInvalid() const;

    DataNodeSetIterator(lyd_node** start, lyd_node** const end, const DataNodeSet* set);
    lyd_node** m_current;
    lyd_node** const m_end;
    const DataNodeSet* m_set;
};

class DataNodeSet {
public:
    ~DataNodeSet();
    DataNodeSetIterator begin() const;
    DataNodeSetIterator end() const;

private:
    DataNodeSet(ly_set* set, std::shared_ptr<internal_refcount> refs);
    friend DataNode;
    friend DataNodeSetIterator;
    void invalidate();
    void throwIfInvalid() const;

    mutable std::set<DataNodeSetIterator*> m_iterators;
    ly_set* m_set;
    std::shared_ptr<internal_refcount> m_refs;
    bool m_valid = true;
};
}
