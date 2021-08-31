/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <libyang/libyang.h>
#include <libyang-cpp/DataNode.hpp>
#include <libyang-cpp/Set.hpp>
#include <span>

namespace libyang {
DataNodeSetIterator::DataNodeSetIterator(lyd_node** start, lyd_node** const end, std::shared_ptr<internal_refcount> refs)
    : m_current(start)
    , m_end(end)
    , m_refs(refs)
{
}

DataNodeSet::DataNodeSet(ly_set* set, std::shared_ptr<internal_refcount> refs)
    : m_set(set)
    , m_refs(refs)
{
}

DataNodeSet::~DataNodeSet()
{
    ly_set_free(m_set, nullptr);
}

DataNodeSetIterator DataNodeSet::begin() const
{
    return DataNodeSetIterator{m_set->dnodes, m_set->dnodes + m_set->count, m_refs};
}
DataNodeSetIterator DataNodeSet::end() const
{
    return DataNodeSetIterator{m_set->dnodes + m_set->count, m_set->dnodes + m_set->count, m_refs};
}

DataNode DataNodeSetIterator::operator*() const
{
    if (m_current == m_end) {
        throw std::out_of_range("Dereferenced an .end() iterator");
    }
    return DataNode{*m_current, m_refs};
}

DataNodeSetIterator& DataNodeSetIterator::operator++()
{
    m_current++;
    return *this;
}

DataNodeSetIterator DataNodeSetIterator::operator++(int)
{
    auto copy = *this;
    operator++();
    return copy;
}

bool DataNodeSetIterator::operator==(const DataNodeSetIterator& other) const
{
    return this->m_current == other.m_current;
}

DataNodeSetIterator::DataNodeProxy<DataNode> DataNodeSetIterator::operator->() const
{
    return {DataNode{*m_current, m_refs}};
}

template <>
DataNode* DataNodeSetIterator::DataNodeProxy<DataNode>::operator->()
{
    return &node;
}
}
