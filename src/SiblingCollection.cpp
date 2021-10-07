/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <cassert>
#include <libyang/libyang.h>
#include <libyang-cpp/SiblingCollection.hpp>
#include "utils/ref_count.hpp"

namespace libyang {
/**
 * Creates a new iterator starting at `start`.
 */
SiblingIterator::SiblingIterator(lyd_node* start, const SiblingCollection* coll)
    : m_current(start)
    , m_collection(coll)
{
    registerThis();
}

/**
 * Creates an iterator that acts as the `end()` for iteration.
 */
SiblingIterator::SiblingIterator(const end)
    : m_current(nullptr)
    , m_collection(nullptr)
{
}

SiblingIterator::~SiblingIterator()
{
    unregisterThis();
}

SiblingIterator::SiblingIterator(const SiblingIterator& other) noexcept
    : m_current(other.m_current)
    , m_collection(other.m_collection)
{
    registerThis();
}

void SiblingIterator::registerThis()
{
    if (m_collection) {
        assert(m_collection->m_valid); // registerThis)) is only run on construction -> the collection must be valid
        m_collection->m_iterators.emplace(this);
    }
}

void SiblingIterator::unregisterThis()
{
    if (m_collection) {
        m_collection->m_iterators.erase(this);
    }
}

/**
 * Advances the iterator.
 */
SiblingIterator& SiblingIterator::operator++()
{
    throwIfInvalid();
    if (!m_current) {
        return *this;
    }

    m_current = m_current->next;

    return *this;
}

/**
 * Advances the iterator and returns the previous one.
 */
SiblingIterator SiblingIterator::operator++(int)
{
    throwIfInvalid();
    auto copy = *this;
    operator++();
    return copy;
}

/**
 * Dereferences the iterator and returns a DataNode instance.
 */
DataNode SiblingIterator::operator*() const
{
    throwIfInvalid();
    if (!m_current) {
        throw std::out_of_range("Dereferenced .end() iterator");
    }

    return DataNode{m_current, m_collection->m_refs};
}

/**
 * Dereferences the iterator and returns a DataNode instance.
 */
SiblingIterator::NodeProxy SiblingIterator::operator->() const
{
    throwIfInvalid();
    return NodeProxy{**this};
}

/**
 * Checks if the iterator point to the same tree element.
 */
bool SiblingIterator::operator==(const SiblingIterator& it) const
{
    throwIfInvalid();
    return m_current == it.m_current;
}

void SiblingIterator::throwIfInvalid() const
{
    if (!m_collection || !m_collection->m_valid) {
        throw std::out_of_range("Iterator is invalid");
    }
};

SiblingCollection::SiblingCollection(lyd_node* start, std::shared_ptr<internal_refcount> refs)
    : m_start(start)
    , m_refs(refs)
{
}

void SiblingCollection::invalidateIterators()
{
    for (const auto& iterator : m_iterators) {
        iterator->m_collection = nullptr;
    }
}

SiblingCollection& SiblingCollection::operator=(const SiblingCollection& other)
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

/**
 * Returns an iterator pointing to the starting element.
 */
SiblingIterator SiblingCollection::begin() const
{
    throwIfInvalid();
    return SiblingIterator{m_start, this};
};

/**
 * Returns an iterator used as the `end` iterator.
 */
SiblingIterator SiblingCollection::end() const
{
    throwIfInvalid();
    return SiblingIterator{SiblingIterator::end{}};
}

void SiblingCollection::throwIfInvalid() const
{
    if (!m_valid) {
        throw std::out_of_range("Collection is invalid");
    }
}
}
