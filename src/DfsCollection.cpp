#include <cassert>
#include <libyang/libyang.h>
#include <libyang-cpp/DataNode.hpp>
#include <libyang-cpp/DfsCollection.hpp>
#include "utils/ref_count.hpp"

namespace libyang {
/**
 * Creates a new iterator starting at `start`.
 */
template <typename NodeType>
DfsIterator<NodeType>::DfsIterator(underlying_node_t<NodeType>* start, const DfsCollection<NodeType>* coll)
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
DfsIterator<NodeType>::DfsIterator(const end)
    : m_current(nullptr)
    , m_collection(nullptr)
{
}

template <typename NodeType>
DfsIterator<NodeType>::~DfsIterator()
{
    unregisterThis();
}

template <typename NodeType>
DfsIterator<NodeType>::DfsIterator(const DfsIterator<NodeType>& other)
    : m_current(other.m_current)
    , m_start(other.m_start)
    , m_next(other.m_next)
    , m_collection(other.m_collection)
{
    registerThis();
}

template <typename NodeType>
void DfsIterator<NodeType>::registerThis()
{
    if (m_collection) {
        assert(m_collection->m_valid); // registerThis)) is only run on construction -> the collection must be valid
        m_collection->m_iterators.emplace(this);
    }
}

template <typename NodeType>
void DfsIterator<NodeType>::unregisterThis()
{
    if (m_collection) {
        m_collection->m_iterators.erase(this);
    }
}

/**
 * Advances the iterator.
 */
template <typename NodeType>
DfsIterator<NodeType>& DfsIterator<NodeType>::operator++()
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
DfsIterator<NodeType> DfsIterator<NodeType>::operator++(int)
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
NodeType DfsIterator<NodeType>::operator*() const
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
typename DfsIterator<NodeType>::NodeProxy DfsIterator<NodeType>::operator->() const
{
    throwIfInvalid();
    return NodeProxy{**this};
}

/**
 * Checks if the iterator point to the same tree element.
 */
template <typename NodeType>
bool DfsIterator<NodeType>::operator==(const DfsIterator<NodeType>& it) const
{
    throwIfInvalid();
    return m_current == it.m_current;
}

template <typename NodeType>
void DfsIterator<NodeType>::throwIfInvalid() const
{
    if (!m_collection || !m_collection->m_valid) {
        throw std::out_of_range("Iterator is invalid");
    }
};

template <typename NodeType>
DfsCollection<NodeType>::DfsCollection(underlying_node_t<NodeType>* start, std::shared_ptr<impl::refs_type_t<NodeType>> refs)
    : m_start(start)
    , m_refs(refs)
{
    if constexpr (std::is_same_v<NodeType, DataNode>) {
        m_refs->dataCollections.emplace(this);
    }
}

template <typename NodeType>
DfsCollection<NodeType>::DfsCollection(const DfsCollection<NodeType>& other)
    : m_start(other.m_start)
    , m_refs(other.m_refs)
    , m_valid(other.m_valid)
{
    if constexpr (std::is_same_v<NodeType, DataNode>) {
        m_refs->dataCollections.emplace(this);
    }
}

template <>
void DfsCollection<DataNode>::invalidateIterators()
{
    for (const auto& iterator : m_iterators) {
        iterator->m_collection = nullptr;
    }
}

template <>
void DfsCollection<SchemaNode>::invalidateIterators()
{
}

template <typename NodeType>
DfsCollection<NodeType>& DfsCollection<NodeType>::operator=(const DfsCollection<NodeType>& other)
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
DfsCollection<SchemaNode>::~DfsCollection() = default;

template <>
DfsCollection<DataNode>::~DfsCollection()
{
    invalidateIterators();
    m_refs->dataCollections.erase(this);
}

/**
 * Returns an iterator pointing to the starting element.
 */
template <typename NodeType>
DfsIterator<NodeType> DfsCollection<NodeType>::begin() const
{
    throwIfInvalid();
    return DfsIterator{m_start, this};
};

/**
 * Returns an iterator used as the `end` iterator.
 */
template <typename NodeType>
DfsIterator<NodeType> DfsCollection<NodeType>::end() const
{
    throwIfInvalid();
    return DfsIterator<NodeType>{typename DfsIterator<NodeType>::end{}};
}

template <typename NodeType>
void DfsCollection<NodeType>::throwIfInvalid() const
{
    if (!m_valid) {
        throw std::out_of_range("Collection is invalid");
    }
}

template class DfsCollection<DataNode>;
template class DfsIterator<DataNode>;

template class DfsCollection<SchemaNode>;
template class DfsIterator<SchemaNode>;
}
