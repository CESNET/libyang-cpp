/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#include <cstring>
#include <functional>
#include <libyang/libyang.h>
#include <libyang/tree_data.h>
#include <stdexcept>
#include <string>
#include <libyang-cpp/DataNode.hpp>
#include <libyang-cpp/utils/exception.hpp>
#include "utils/enum.hpp"
#include "utils/newPath.hpp"
#include "utils/ref_count.hpp"
namespace libyang {
/**
 * @brief Wraps a completely new tree. Used only internally.
 */
DataNode::DataNode(lyd_node* node, std::shared_ptr<ly_ctx> ctx)
    : m_node(node)
    , m_refs(std::make_shared<internal_refcount>(ctx))
{
    registerRef();
}

/**
 * @brief Wraps an existing tree. Used only internally.
 */
DataNode::DataNode(lyd_node* node, std::shared_ptr<internal_refcount> viewCount)
    : m_node(node)
    , m_refs(viewCount)
{
    registerRef();
}

DataNode::~DataNode()
{
    unregisterRef();
    if (m_refs->nodes.size() == 0) {
        lyd_free_all(m_node);
    }
}

DataNode::DataNode(const DataNode& other)
    : m_node(other.m_node)
    , m_refs(other.m_refs)
{
    registerRef();
}

DataNode& DataNode::operator=(const DataNode& other)
{
    if (this == &other) {
        return *this;
    }

    unregisterRef();
    this->m_node = other.m_node;
    this->m_refs = other.m_refs;
    registerRef();
    return *this;
}

/**
 * @brief Registers the current instance into the refcounter.
 */
void DataNode::registerRef()
{
    m_refs->nodes.emplace(this);
}

/**
 * @brief Unregisters the current instance into the refcounter.
 */
void DataNode::unregisterRef()
{
    m_refs->nodes.erase(this);
}

/**
 * @brief Prints the tree into a string.
 * @param format Format of the output string.
 * @param flags Flags that change the behavior of the printing.
 */
String DataNode::printStr(const DataFormat format, const PrintFlags flags) const
{
    char* str;
    lyd_print_mem(&str, m_node, utils::toLydFormat(format), utils::toPrintFlags(flags));

    return String{str};
}

/**
 * Returns a view of the node specified by `path`.
 * If the node is not found, returns std::nullopt.
 * Throws on errors.
 *
 * @param path Node to search for.
 * @return DataView is the node is found, other std::nullopt.
 */
std::optional<DataNode> DataNode::findPath(const char* path, const OutputNodes output) const
{
    lyd_node* node;
    auto err = lyd_find_path(m_node, path, output == OutputNodes::Yes ? true : false, &node);

    switch (err) {
    case LY_SUCCESS:
        return DataNode{node, m_refs};
    case LY_ENOTFOUND:
    case LY_EINCOMPLETE: // TODO: is this really important?
        return std::nullopt;
    default:
        throw ErrorWithCode("Error in DataNode::findPath (" + std::to_string(err) + ")", err);
    }
}

/**
 * @brief Returns the path of the pointed-to node.
 */
String DataNode::path() const
{
    // TODO: handle all path types, not just LYD_PATH_STD

    auto str = lyd_path(m_node, LYD_PATH_STD, nullptr, 0);
    if (!str) {
        throw std::bad_alloc();
    }

    return String{str};
}

/**
 * @brief Creates a new node with the supplied path, changing this tree.
 *
 * @param path Path of the new node.
 * @param value String representation of the value. Use nullptr for non-leaf nodes and the `empty` type.
 * @param options Options that change the behavior of this method.
 * @return If a new node got created, returns it. Otherwise returns std::nullopt.
 */
std::optional<DataNode> DataNode::newPath(const char* path, const char* value, const std::optional<CreationOptions> options)
{
    return impl::newPath(m_node, nullptr, m_refs, path, value, options);
}

DataNodeTerm DataNode::asTerm() const
{
    if (!(m_node->schema->nodetype & LYD_NODE_TERM)) {
        throw Error("Node is not a leaf or a leaflist");
    }

    return DataNodeTerm{m_node, m_refs};
}

/**
 * Check if both operands point to the same node in the same tree.
 */
bool DataNode::operator==(const DataNode &node) const
{
    return this->m_node == node.m_node;
}

namespace {
bool isDescendantOrEqual(lyd_node* node, lyd_node* target)
{
    do {
        if (node == target) {
            return true;
        }

        node = reinterpret_cast<lyd_node*>(node->parent);
    } while (node);

    return false;
}
}

/**
 * Unlinks this node, creating a new tree.
 */
void DataNode::unlink()
{
    unregisterRef();

    // We'll need a new refcounter for the unlinked tree.
    auto oldRefs = m_refs;
    m_refs = std::make_shared<internal_refcount>(m_refs->context);
    registerRef();

    // All references to this node and its children will need to have this new refcounter.
    for (auto it = oldRefs->nodes.begin(); it != oldRefs->nodes.end(); /* nothing */) {
        if (isDescendantOrEqual((*it)->m_node, m_node)) {
            // The child needs to be added to the new refcounter and removed from the old refcounter.
            (*it)->m_refs = m_refs;
            (*it)->registerRef();
            it = oldRefs->nodes.erase(it);
        } else {
            it++;
        }
    }

    // If we're unlinking a node that belongs to a collection, we need to invalidate it.
    // For example:
    //      A <- iterator starts here (that's (*it)->m_start)
    //    |  |
    //    B  C <- we're unlinking this one (that's m_node). The iterator's m_current might also be this node.
    //
    // We must invalidate the whole collection and all its iterators, because the iterators might point to the node
    // being unlinked.
    for (auto it = oldRefs->collections.begin(); it != oldRefs->collections.end(); it++) {
        if (isDescendantOrEqual((*it)->m_start, m_node)) {
            (*it)->m_valid = false;
        }
    }

    // We need to find a lyd_node* that points to somewhere else outside the subtree that's being unlinked.
    // If m_node is an inner node and has a parent, we'll use that.
    auto oldTree = reinterpret_cast<lyd_node*>(m_node->parent);
    if (!oldTree) {
        // If parent is nullptr, then that means that our node is either:
        // 1) A top-level node. We'll search for the first sibling. If the first sibling is our node, we'll just get the
        //    next one.
        // 2) An already unlinked node. In that case we don't have any siblings, because there's no common parent which
        //    would connect them. No freeing needs to be done. The algorithm will still work because lyd_first_sibling will
        //    return our node and ->next will be nullptr.
        oldTree = lyd_first_sibling(m_node);
        if (oldTree == m_node) {
            oldTree = oldTree->next;
        }
    }
    lyd_unlink_tree(m_node);

    // If we don't hold any references to the old tree, we must also free it.
    if (oldRefs->nodes.size() == 0) {
        lyd_free_all(reinterpret_cast<lyd_node*>(oldTree));
    }
}

std::string_view DataNodeTerm::valueStr() const
{
    return lyd_get_value(m_node);
}

namespace {
/**
 * This function emulates LYD_VALUE_GET. I can't use that macro directly, because it implicitly converts void* to Type*
 * and that's not allowed in C++.
 */
template <typename Type>
const Type* valueGetSpecial(const lyd_value* value)
{
    if (sizeof(Type) > LYD_VALUE_FIXED_MEM_SIZE) {
        return static_cast<const Type*>(value->dyn_mem);
    } else {
        return reinterpret_cast<const Type*>(value->fixed_mem);
    }
}
}

/**
 * Retrieves a value in a machine-readable format. The lifetime of the value is not connected to the lifetime of the
 * original DataNodeTerm.
 */
Value DataNodeTerm::value() const
{
    std::function<Value(lyd_value)> impl = [this, &impl] (const lyd_value value) -> Value {
        auto baseType = value.realtype->basetype;
        switch (baseType) {
        case LY_TYPE_INT8:
            return value.int8;
        case LY_TYPE_INT16:
            return value.int16;
        case LY_TYPE_INT32:
            return value.int32;
        case LY_TYPE_INT64:
            return value.int64;
        case LY_TYPE_UINT8:
            return value.uint8;
        case LY_TYPE_UINT16:
            return value.uint16;
        case LY_TYPE_UINT32:
            return value.uint32;
        case LY_TYPE_UINT64:
            return value.uint64;
        case LY_TYPE_BOOL:
            return static_cast<bool>(value.boolean);
        case LY_TYPE_EMPTY:
            return Empty{};
        case LY_TYPE_BINARY:
        {
            auto binValue = valueGetSpecial<lyd_value_binary>(&value);
            Binary res;
            std::copy(static_cast<uint8_t*>(binValue->data), static_cast<uint8_t*>(binValue->data) + binValue->size, std::back_inserter(res.data));
            res.base64 = valueStr();
            return res;
        }
        case LY_TYPE_STRING:
            // valueStr gives a string_view, so here I have to copy the string.
            return std::string(valueStr());
        case LY_TYPE_UNION:
            return impl(value.subvalue->value);
        case LY_TYPE_DEC64:
        {
            auto schemaDef = reinterpret_cast<const lysc_node_leaf*>(m_node->schema);
            auto dec = reinterpret_cast<const lysc_type_dec*>(schemaDef->type);
            return Decimal64{value.dec64, dec->fraction_digits};
        }
        case LY_TYPE_BITS:
        {
            auto bits = valueGetSpecial<lyd_value_bits>(&value);
            std::vector<Bit> res;
            std::transform(bits->items, bits->items + LY_ARRAY_COUNT(bits->items), std::back_inserter(res), [] (const lysc_type_bitenum_item* bit) {
                return Bit{.position = bit->position, .name = bit->name};
            });

            return res;
        }
        case LY_TYPE_ENUM:
            return Enum{value.enum_item->name};
        case LY_TYPE_IDENT:
            return IdentityRef{.module = value.ident->module->name, .name = value.ident->name};
        case LY_TYPE_INST:
        {
            lyd_node* out;
            auto err = lyd_find_target(value.target, m_node, &out);
            switch (err) {
            case LY_SUCCESS:
                return DataNode{out, m_refs};
            case LY_ENOTFOUND:
                return std::nullopt;
            default:
                throw ErrorWithCode("Error when finding inst-id target (" + std::to_string(err) + ")", err);
            }
        }
        case LY_TYPE_LEAFREF:
            // Leafrefs are resolved to the underlying types, so this should never happen.
            throw std::logic_error("Unknown type");
        case LY_TYPE_UNKNOWN:
            throw Error("Unknown type");
        }
        __builtin_unreachable();
    };


    return impl(reinterpret_cast<const lyd_node_term*>(m_node)->value);
}

/**
 * Returns a collection for iterating depth-first over the subtree this instance points to.
 * Any kind of low-level manipulation (e.g. unlinking) of the subtree invalidates the iterator.
 * If the `DataNodeCollectionDfs` object gets destroyed, all iterators associated with it get invalidated.
 */
DataNodeCollectionDfs DataNode::childrenDfs() const
{
    return DataNodeCollectionDfs{m_node, m_refs};
}

SchemaNode DataNode::schema() const
{
    return SchemaNode{m_node->schema, m_refs->context};
}

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
DfsIterator::DataNodeProxy DfsIterator::operator->() const
{
    throwIfInvalid();
    return DataNodeProxy{**this};
}

DataNode* DfsIterator::DataNodeProxy::operator->()
{
    return &node;
}

/**
 * Checks if the iterator point to the same tree element.
 */
bool DfsIterator::operator==(const DfsIterator& it) const
{
    throwIfInvalid();
    return m_current == it.m_current;
}

void DfsIterator::throwIfInvalid() const {
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
