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
template <typename NodeType, IterationType ITER_TYPE>
Iterator<NodeType, ITER_TYPE>::Iterator(underlying_node_t<NodeType>* start, const Collection<NodeType, ITER_TYPE>* coll)
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
template <typename NodeType, IterationType ITER_TYPE>
Iterator<NodeType, ITER_TYPE>::Iterator(const end)
    : m_current(nullptr)
    , m_collection(nullptr)
{
}

template <typename NodeType, IterationType ITER_TYPE>
Iterator<NodeType, ITER_TYPE>::~Iterator()
{
    unregisterThis();
}

template <typename NodeType, IterationType ITER_TYPE>
Iterator<NodeType, ITER_TYPE>::Iterator(const Iterator<NodeType, ITER_TYPE>& other)
    : m_current(other.m_current)
    , m_start(other.m_start)
    , m_next(other.m_next)
    , m_collection(other.m_collection)
{
    registerThis();
}

template <typename NodeType, IterationType ITER_TYPE>
void Iterator<NodeType, ITER_TYPE>::registerThis()
{
    if (m_collection) {
        assert(m_collection->m_valid); // registerThis)) is only run on construction -> the collection must be valid
        m_collection->m_iterators.emplace(this);
    }
}

template <typename NodeType, IterationType ITER_TYPE>
void Iterator<NodeType, ITER_TYPE>::unregisterThis()
{
    if (m_collection) {
        m_collection->m_iterators.erase(this);
    }
}

/**
 * Advances the iterator.
 */
template <typename NodeType, IterationType ITER_TYPE>
Iterator<NodeType, ITER_TYPE>& Iterator<NodeType, ITER_TYPE>::operator++()
{
    throwIfInvalid();
    if (!m_current) {
        return *this;
    }

    if constexpr (ITER_TYPE == IterationType::Dfs) {
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
    } else {
        m_current = m_current->next;
    }
    return *this;
}

/**
 * Advances the iterator and returns the previous one.
 */
template <typename NodeType, IterationType ITER_TYPE>
Iterator<NodeType, ITER_TYPE> Iterator<NodeType, ITER_TYPE>::operator++(int)
{
    throwIfInvalid();
    auto copy = *this;
    operator++();
    return copy;
}

/**
 * Dereferences the iterator and returns a DataNode instance.
 */
template <typename NodeType, IterationType ITER_TYPE>
NodeType Iterator<NodeType, ITER_TYPE>::operator*() const
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
template <typename NodeType, IterationType ITER_TYPE>
typename Iterator<NodeType, ITER_TYPE>::NodeProxy Iterator<NodeType, ITER_TYPE>::operator->() const
{
    throwIfInvalid();
    return NodeProxy{**this};
}

/**
 * Checks if the iterator point to the same tree element.
 */
template <typename NodeType, IterationType ITER_TYPE>
bool Iterator<NodeType, ITER_TYPE>::operator==(const Iterator<NodeType, ITER_TYPE>& it) const
{
    throwIfInvalid();
    return m_current == it.m_current;
}

template <typename NodeType, IterationType ITER_TYPE>
void Iterator<NodeType, ITER_TYPE>::throwIfInvalid() const
{
    if (!m_collection || !m_collection->m_valid) {
        throw std::out_of_range("Iterator is invalid");
    }
};

template <typename NodeType, IterationType ITER_TYPE>
Collection<NodeType, ITER_TYPE>::Collection(underlying_node_t<NodeType>* start, std::shared_ptr<impl::refs_type_t<NodeType>> refs)
    : m_start(start)
    , m_refs(refs)
{
    if constexpr (std::is_same_v<NodeType, DataNode>) {
        if constexpr (ITER_TYPE == IterationType::Dfs) {
            m_refs->dataCollectionsDfs.emplace(this);
        } else {
            m_refs->dataCollectionsSibling.emplace(this);
        }
    }
}

template <typename NodeType, IterationType ITER_TYPE>
Collection<NodeType, ITER_TYPE>::Collection(const Collection<NodeType, ITER_TYPE>& other)
    : m_start(other.m_start)
    , m_refs(other.m_refs)
    , m_valid(other.m_valid)
{
    if constexpr (std::is_same_v<NodeType, DataNode>) {
        if constexpr (ITER_TYPE == IterationType::Dfs) {
            m_refs->dataCollectionsDfs.emplace(this);
        } else {
            m_refs->dataCollectionsSibling.emplace(this);
        }
    }
}

template <typename NodeType, IterationType ITER_TYPE>
void Collection<NodeType, ITER_TYPE>::invalidateIterators()
{
    if constexpr (std::is_same_v<NodeType, DataNode>) {
        for (const auto& iterator : m_iterators) {
            iterator->m_collection = nullptr;
        }
    }
}

template <typename NodeType, IterationType ITER_TYPE>
Collection<NodeType, ITER_TYPE>& Collection<NodeType, ITER_TYPE>::operator=(const Collection<NodeType, ITER_TYPE>& other)
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

template <typename NodeType, IterationType ITER_TYPE>
Collection<NodeType, ITER_TYPE>::~Collection()
{
    if constexpr (std::is_same_v<NodeType, DataNode>) {
        invalidateIterators();
        if constexpr (ITER_TYPE == IterationType::Dfs) {
            m_refs->dataCollectionsDfs.erase(this);
        } else {
            m_refs->dataCollectionsSibling.erase(this);
        }
    }
}

/**
 * Returns an iterator pointing to the starting element.
 */
template <typename NodeType, IterationType ITER_TYPE>
Iterator<NodeType, ITER_TYPE> Collection<NodeType, ITER_TYPE>::begin() const
{
    throwIfInvalid();
    return Iterator{m_start, this};
};

/**
 * Returns an iterator used as the `end` iterator.
 */
template <typename NodeType, IterationType ITER_TYPE>
Iterator<NodeType, ITER_TYPE> Collection<NodeType, ITER_TYPE>::end() const
{
    throwIfInvalid();
    return Iterator<NodeType, ITER_TYPE>{typename Iterator<NodeType, ITER_TYPE>::end{}};
}

template <typename NodeType, IterationType ITER_TYPE>
void Collection<NodeType, ITER_TYPE>::throwIfInvalid() const
{
    if (!m_valid) {
        throw std::out_of_range("Collection is invalid");
    }
}

template class Collection<DataNode, IterationType::Dfs>;
template class Iterator<DataNode, IterationType::Dfs>;

template class Collection<SchemaNode, IterationType::Dfs>;
template class Iterator<SchemaNode, IterationType::Dfs>;

template class Collection<DataNode, IterationType::Sibling>;
template class Iterator<DataNode, IterationType::Sibling>;
}
