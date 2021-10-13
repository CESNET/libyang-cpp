/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <cassert>
#include <libyang/libyang.h>
#include <libyang-cpp/DataNode.hpp>
#include <libyang-cpp/Collection.hpp>
#include "utils/ref_count.hpp"

namespace libyang {
/**
 * Creates a new iterator starting at `start`.
 */
template <typename NodeType>
Iterator<NodeType>::Iterator(underlying_node_t<NodeType>* start, const Collection<NodeType>* coll)
    : m_current(start)
    , m_start(start)
    , m_next(start)
    , m_collection(coll)
{
    registerThis();
}

/**
 * Creates an iterator that acts as the `end()` for iteration.
 */
template <typename NodeType>
Iterator<NodeType>::Iterator(const end)
    : m_current(nullptr)
    , m_collection(nullptr)
{
}

template <typename NodeType>
Iterator<NodeType>::~Iterator()
{
    unregisterThis();
}

template <typename NodeType>
Iterator<NodeType>::Iterator(const Iterator<NodeType>& other)
    : m_current(other.m_current)
    , m_start(other.m_start)
    , m_next(other.m_next)
    , m_collection(other.m_collection)
{
    registerThis();
}

template <typename NodeType>
void Iterator<NodeType>::registerThis()
{
    if (m_collection) {
        assert(m_collection->m_valid); // registerThis)) is only run on construction -> the collection must be valid
        m_collection->m_iterators.emplace(this);
    }
}

template <typename NodeType>
void Iterator<NodeType>::unregisterThis()
{
    if (m_collection) {
        m_collection->m_iterators.erase(this);
    }
}

/**
 * Advances the iterator.
 */
template <typename NodeType>
Iterator<NodeType>& Iterator<NodeType>::operator++()
{
    throwIfInvalid();
    if (!m_current) {
        return *this;
    }

    // select element for the next run - children first
    if constexpr (std::is_same_v<decltype(m_next), lyd_node*>) {
        m_next = lyd_child(m_current);
    } else {
        m_next = lysc_node_child(m_current);
    }

    if (!m_next) {
        // no children
        if (m_current == m_start) {
            // we are done, m_start has no children
            // this iterator is now the `end` iterator, so wee need to set current to nullptr
            m_current = nullptr;
            return *this;
        }
        // try siblings
        m_next = m_current->next;
    }

    while (!m_next) {
        // parent is already processed, go to its sibling
        m_current = reinterpret_cast<underlying_node_t<NodeType>*>(m_current->parent);
        // no siblings, go back through parents
        if (m_current->parent == m_start->parent) {
            // we are done, no next element to process
            break;
        }
        m_next = m_current->next;
    }

    m_current = m_next;

    return *this;
}

/**
 * Advances the iterator and returns the previous one.
 */
template <typename NodeType>
Iterator<NodeType> Iterator<NodeType>::operator++(int)
{
    throwIfInvalid();
    auto copy = *this;
    operator++();
    return copy;
}

/**
 * Dereferences the iterator and returns a DataNode instance.
 */
template <typename NodeType>
NodeType Iterator<NodeType>::operator*() const
{
    throwIfInvalid();
    if (!m_current) {
        throw std::out_of_range("Dereferenced .end() iterator");
    }

    return NodeType{m_current, m_collection->m_refs};
}

/**
 * Dereferences the iterator and returns a DataNode instance.
 */
template <typename NodeType>
typename Iterator<NodeType>::NodeProxy Iterator<NodeType>::operator->() const
{
    throwIfInvalid();
    return NodeProxy{**this};
}

/**
 * Checks if the iterator point to the same tree element.
 */
template <typename NodeType>
bool Iterator<NodeType>::operator==(const Iterator<NodeType>& it) const
{
    throwIfInvalid();
    return m_current == it.m_current;
}

template <typename NodeType>
void Iterator<NodeType>::throwIfInvalid() const
{
    if (!m_collection || !m_collection->m_valid) {
        throw std::out_of_range("Iterator is invalid");
    }
};

template <typename NodeType>
Collection<NodeType>::Collection(underlying_node_t<NodeType>* start, std::shared_ptr<impl::refs_type_t<NodeType>> refs)
    : m_start(start)
    , m_refs(refs)
{
    if constexpr (std::is_same_v<NodeType, DataNode>) {
        m_refs->dataCollections.emplace(this);
    }
}

template <typename NodeType>
Collection<NodeType>::Collection(const Collection<NodeType>& other)
    : m_start(other.m_start)
    , m_refs(other.m_refs)
    , m_valid(other.m_valid)
{
    if constexpr (std::is_same_v<NodeType, DataNode>) {
        m_refs->dataCollections.emplace(this);
    }
}

template <>
void Collection<DataNode>::invalidateIterators()
{
    for (const auto& iterator : m_iterators) {
        iterator->m_collection = nullptr;
    }
}

template <>
void Collection<SchemaNode>::invalidateIterators()
{
}

template <typename NodeType>
Collection<NodeType>& Collection<NodeType>::operator=(const Collection<NodeType>& other)
{
    if (this == &other) {
        return *this;
    }

    // Our iterators must be invalidated, since we're assigning a different collection.
    invalidateIterators();
    m_iterators.clear();
    this->m_start = other.m_start;
    this->m_refs = other.m_refs;
    this->m_valid = other.m_valid;

    return *this;
}

template <>
Collection<SchemaNode>::~Collection() = default;

template <>
Collection<DataNode>::~Collection()
{
    invalidateIterators();
    m_refs->dataCollections.erase(this);
}

/**
 * Returns an iterator pointing to the starting element.
 */
template <typename NodeType>
Iterator<NodeType> Collection<NodeType>::begin() const
{
    throwIfInvalid();
    return Iterator{m_start, this};
};

/**
 * Returns an iterator used as the `end` iterator.
 */
template <typename NodeType>
Iterator<NodeType> Collection<NodeType>::end() const
{
    throwIfInvalid();
    return Iterator<NodeType>{typename Iterator<NodeType>::end{}};
}

template <typename NodeType>
void Collection<NodeType>::throwIfInvalid() const
{
    if (!m_valid) {
        throw std::out_of_range("Collection is invalid");
    }
}

template class Collection<DataNode>;
template class Iterator<DataNode>;

template class Collection<SchemaNode>;
template class Iterator<SchemaNode>;
}
