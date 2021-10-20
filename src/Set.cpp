/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <libyang-cpp/DataNode.hpp>
#include <libyang-cpp/Set.hpp>
#include <libyang/libyang.h>
#include <span>
#include "utils/ref_count.hpp"

namespace libyang {
DataNodeSetIterator::DataNodeSetIterator(lyd_node** start, lyd_node** const end, const DataNodeSet* set)
    : m_current(start)
    , m_end(end)
    , m_set(set)
{
    m_set->m_iterators.emplace(this);
}

DataNodeSetIterator::~DataNodeSetIterator()
{
    if (m_set) {
        m_set->m_iterators.erase(this);
    }
}

DataNodeSet::DataNodeSet(ly_set* set, std::shared_ptr<internal_refcount> refs)
    : m_set(set)
    , m_refs(refs)
{
    m_refs->dataSets.emplace(this);
}

DataNodeSet::~DataNodeSet()
{
    invalidate();
    m_refs->dataSets.erase(this);
    ly_set_free(m_set, nullptr);
}

void DataNodeSet::throwIfInvalid() const
{
    if (!m_valid) {
        throw std::out_of_range("Set is invalid");
    }
}

DataNodeSetIterator DataNodeSet::begin() const
{
    throwIfInvalid();
    return DataNodeSetIterator{m_set->dnodes, m_set->dnodes + m_set->count, this};
}
DataNodeSetIterator DataNodeSet::end() const
{
    throwIfInvalid();
    return DataNodeSetIterator{m_set->dnodes + m_set->count, m_set->dnodes + m_set->count, this};
}

DataNode DataNodeSet::front() const
{
    return *DataNodeSetIterator{m_set->dnodes, m_set->dnodes + m_set->count, this};
}

DataNode DataNodeSet::back() const
{
    return *DataNodeSetIterator{m_set->dnodes + m_set->count - 1, m_set->dnodes + m_set->count, this};
}

void DataNodeSet::invalidate()
{
    m_valid = false;
    for (const auto& iterator : m_iterators) {
        iterator->m_set = nullptr;
    }
    m_iterators.clear();
}

void DataNodeSetIterator::throwIfInvalid() const
{
    if (!m_set || !m_set->m_valid) {
        throw std::out_of_range("Iterator is invalid");
    }
}

DataNode DataNodeSetIterator::operator*() const
{
    throwIfInvalid();
    if (m_current >= m_end) {
        throw std::out_of_range("Dereferenced an .end() iterator");
    }
    return DataNode{*m_current, m_set->m_refs};
}

DataNodeSetIterator& DataNodeSetIterator::operator++()
{
    throwIfInvalid();
    m_current++;
    return *this;
}

DataNodeSetIterator DataNodeSetIterator::operator++(int)
{
    throwIfInvalid();
    auto copy = *this;
    operator++();
    return copy;
}

bool DataNodeSetIterator::operator==(const DataNodeSetIterator& other) const
{
    throwIfInvalid();
    return this->m_current == other.m_current;
}

DataNodeSetIterator::DataNodeProxy DataNodeSetIterator::operator->() const
{
    throwIfInvalid();
    return {DataNode{*m_current, m_set->m_refs}};
}
}
