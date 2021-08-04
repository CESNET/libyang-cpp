#include <cassert>
#include <libyang/libyang.h>
#include <libyang-cpp/DataNode.hpp>
#include <libyang-cpp/DfsCollection.hpp>
#include "utils/ref_count.hpp"

namespace libyang {
/**
 * Creates a new iterator starting at `start`.
 */
DfsIterator::DfsIterator(lyd_node* start, const DataNodeCollectionDfs* coll)
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
DfsIterator::DfsIterator(const end)
    : m_current(nullptr)
    , m_collection(nullptr)
{
}

DfsIterator::~DfsIterator()
{
    unregisterThis();
}

DfsIterator::DfsIterator(const DfsIterator& other)
    : m_current(other.m_current)
    , m_start(other.m_start)
    , m_next(other.m_next)
    , m_collection(other.m_collection)
{
    registerThis();
}

void DfsIterator::registerThis()
{
    if (m_collection) {
        assert(m_collection->m_valid); // registerThis)) is only run on construction -> the collection must be valid
        m_collection->m_iterators.emplace(this);
    }
}

void DfsIterator::unregisterThis()
{
    if (m_collection) {
        m_collection->m_iterators.erase(this);
    }
}

/**
 * Advances the iterator.
 */
DfsIterator& DfsIterator::operator++()
{
    throwIfInvalid();
    if (!m_current) {
        return *this;
    }

    // select element for the next run - children first
    m_next = lyd_child(m_current);

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
        m_current = reinterpret_cast<lyd_node*>(m_current->parent);
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
DfsIterator DfsIterator::operator++(int)
{
    throwIfInvalid();
    auto copy = *this;
    operator++();
    return copy;
}

/**
 * Dereferences the iterator and returns a DataNode instance.
 */
DataNode DfsIterator::operator*() const
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
DfsIterator::NodeProxy<DataNode> DfsIterator::operator->() const
{
    throwIfInvalid();
    return NodeProxy<DataNode>{**this};
}

/**
 * Checks if the iterator point to the same tree element.
 */
bool DfsIterator::operator==(const DfsIterator& it) const
{
    throwIfInvalid();
    return m_current == it.m_current;
}

void DfsIterator::throwIfInvalid() const
{
    if (!m_collection || !m_collection->m_valid) {
        throw std::out_of_range("Iterator is invalid");
    }
};

DataNodeCollectionDfs::DataNodeCollectionDfs(lyd_node* start, std::shared_ptr<internal_refcount> refs)
    : m_start(start)
    , m_refs(refs)
{
    m_refs->collections.emplace(this);
}

DataNodeCollectionDfs::DataNodeCollectionDfs(const DataNodeCollectionDfs& other)
    : m_start(other.m_start)
    , m_refs(other.m_refs)
    , m_valid(other.m_valid)
{
    m_refs->collections.emplace(this);
}

DataNodeCollectionDfs& DataNodeCollectionDfs::operator=(const DataNodeCollectionDfs& other)
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

DataNodeCollectionDfs::~DataNodeCollectionDfs()
{
    invalidateIterators();

    m_refs->collections.erase(this);
}

void DataNodeCollectionDfs::invalidateIterators()
{
    for (const auto& iterator : m_iterators) {
        iterator->m_collection = nullptr;
    }
}

/**
 * Returns an iterator pointing to the starting element.
 */
DfsIterator DataNodeCollectionDfs::begin() const
{
    throwIfInvalid();
    return DfsIterator{m_start, this};
};

/**
 * Returns an iterator used as the `end` iterator.
 */
DfsIterator DataNodeCollectionDfs::end() const
{
    throwIfInvalid();
    return DfsIterator{DfsIterator::end{}};
}

void DataNodeCollectionDfs::throwIfInvalid() const
{
    if (!m_valid) {
        throw std::out_of_range("Collection is invalid");
    }
}
}
