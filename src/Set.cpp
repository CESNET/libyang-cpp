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
template <typename NodeType>
SetIterator<NodeType>::SetIterator(lyd_node** start, lyd_node** const end, const Set<NodeType>* set)
    : m_start(start)
    , m_current(start)
    , m_end(end)
    , m_set(set)
{
    m_set->m_iterators.emplace(this);
}

template <typename NodeType>
SetIterator<NodeType>::~SetIterator()
{
    if (m_set) {
        m_set->m_iterators.erase(this);
    }
}

template <typename NodeType>
Set<NodeType>::Set(ly_set* set, std::shared_ptr<internal_refcount> refs)
    : m_set(set, [] (auto* set) { ly_set_free(set, nullptr); })
    , m_refs(refs)
{
    m_refs->dataSets.emplace(this);
}

template <typename NodeType>
Set<NodeType>::~Set()
{
    invalidate();
    m_refs->dataSets.erase(this);
}

template <typename NodeType>
void Set<NodeType>::throwIfInvalid() const
{
    if (!m_valid) {
        throw std::out_of_range("Set is invalid");
    }
}

template <typename NodeType>
SetIterator<NodeType> Set<NodeType>::begin() const
{
    throwIfInvalid();
    return SetIterator{m_set->dnodes, m_set->dnodes + m_set->count, this};
}

template <typename NodeType>
SetIterator<NodeType> Set<NodeType>::end() const
{
    throwIfInvalid();
    return SetIterator{m_set->dnodes, m_set->dnodes + m_set->count, this} + int(m_set->count);
}

template <typename NodeType>
NodeType Set<NodeType>::front() const
{
    if (m_set->count == 0) {
        throw std::out_of_range("The set is empty");
    }
    return *begin();
}

template <typename NodeType>
NodeType Set<NodeType>::back() const
{
    if (m_set->count == 0) {
        throw std::out_of_range("The set is empty");
    }
    return *(end() - 1);
}

template <typename NodeType>
void Set<NodeType>::invalidate()
{
    m_valid = false;
    for (const auto& iterator : m_iterators) {
        iterator->m_set = nullptr;
    }
    m_iterators.clear();
}

template <typename NodeType>
void SetIterator<NodeType>::throwIfInvalid() const
{
    if (!m_set || !m_set->m_valid) {
        throw std::out_of_range("Iterator is invalid");
    }
}

template <typename NodeType>
NodeType SetIterator<NodeType>::operator*() const
{
    throwIfInvalid();
    if (m_current >= m_end) {
        throw std::out_of_range("Dereferenced an .end() iterator");
    }
    return NodeType{*m_current, m_set->m_refs};
}

template <typename NodeType>
SetIterator<NodeType>& SetIterator<NodeType>::operator++()
{
    throwIfInvalid();
    m_current++;
    return *this;
}

template <typename NodeType>
SetIterator<NodeType> SetIterator<NodeType>::operator++(int)
{
    throwIfInvalid();
    auto copy = *this;
    operator++();
    return copy;
}

template <typename NodeType>
SetIterator<NodeType>& SetIterator<NodeType>::operator--()
{
    throwIfInvalid();
    if (m_current == m_start) {
        throw std::out_of_range("Cannot go past the beginning");
    }

    m_current--;
    return *this;
}

template <typename NodeType>
SetIterator<NodeType> SetIterator<NodeType>::operator--(int)
{
    throwIfInvalid();
    auto copy = *this;
    operator--();
    return copy;
}

template <typename NodeType>
SetIterator<NodeType> SetIterator<NodeType>::operator-(int n) const
{
    if (m_current - n < m_start) {
        throw std::out_of_range("Cannot go past the beginning");
    }

    auto copy = *this;
    copy.m_current = copy.m_current - n;
    return copy;
}

template <typename NodeType>
SetIterator<NodeType> SetIterator<NodeType>::operator+(int n) const
{
    if (m_current + n > m_end) {
        throw std::out_of_range("Cannot go past the end");
    }

    auto copy = *this;
    copy.m_current = copy.m_current + n;
    return copy;
}

template <typename NodeType>
bool SetIterator<NodeType>::operator==(const SetIterator& other) const
{
    throwIfInvalid();
    return this->m_current == other.m_current;
}

template <typename NodeType>
typename SetIterator<NodeType>::NodeProxy SetIterator<NodeType>::operator->() const
{
    throwIfInvalid();
    return {NodeType{*m_current, m_set->m_refs}};
}

template
class SetIterator<DataNode>;
template
class Set<DataNode>;
}
