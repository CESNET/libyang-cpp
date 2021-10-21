/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#include <cassert>
#include <cstring>
#include <functional>
#include <libyang-cpp/DataNode.hpp>
#include <libyang-cpp/Set.hpp>
#include <libyang-cpp/utils/exception.hpp>
#include <libyang/libyang.h>
#include <libyang/tree_data.h>
#include <stdexcept>
#include <string>
#include "libyang-cpp/Module.hpp"
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
 * @brief Wraps an unmanaged (non-owning) tree.
 */
DataNode::DataNode(lyd_node* node, const unmanaged_tag)
    : m_node(node)
    , m_refs(nullptr)
{
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
    freeIfNoRefs();
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
    freeIfNoRefs();
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
    if (m_refs) {
        m_refs->nodes.emplace(this);
    }
}

/**
 * @brief Unregisters the current instance into the refcounter.
 */
void DataNode::unregisterRef()
{
    if (m_refs) {
        m_refs->nodes.erase(this);
    }
}

/**
 * @brief Frees the tree if there are no more refs to the tree.
 */
void DataNode::freeIfNoRefs()
{
    if (!m_refs) {
        return;
    }

    if (m_refs->nodes.size() == 0) {
        for (const auto& set : m_refs->dataSets) {
            set->invalidate();
        }

        for (const auto& collection : m_refs->dataCollectionsDfs) {
            collection->invalidate();
        }

        for (const auto& collection : m_refs->dataCollectionsSibling) {
            collection->invalidate();
        }

        lyd_free_all(m_node);
    }
}

/**
 * Returns the first sibling of this node list.
 */
DataNode DataNode::firstSibling() const
{
    return DataNode{lyd_first_sibling(m_node), m_refs};
}

/**
 * Returns the previous sibling node. If there's no previous sibling node, returns this node.
 */
DataNode DataNode::previousSibling() const
{
    return DataNode{m_node->prev, m_refs};
}

/**
 * Returns the next sibling node. If there's no next sibling node, returns std::nullopt.
 */
std::optional<DataNode> DataNode::nextSibling() const
{
    if (!m_node->next) {
        return std::nullopt;
    }
    return DataNode{m_node->next, m_refs};
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
std::optional<DataNode> DataNode::newPath(const char* path, const char* value, const std::optional<CreationOptions> options) const
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

DataNodeAny DataNode::asAny() const
{
    if (!(m_node->schema->nodetype & LYS_ANYDATA)) {
        throw Error("Node is not anydata");
    }

    return DataNodeAny{m_node, m_refs};
}

/*
 * Parses YANG data into an operation data tree. Currently only supports OperationType::ReplyNetconf. For more info,
 * check the documentation of Context::parseOp.
 */
ParsedOp DataNode::parseOp(const char* input, const DataFormat format, const OperationType opType) const
{
    ly_in* in;
    ly_in_new_memory(input, &in);
    auto deleteFunc = [](auto* in) {
        ly_in_free(in, false);
    };
    auto deleter = std::unique_ptr<ly_in, decltype(deleteFunc)>(in, deleteFunc);

    lyd_node* op = nullptr;
    lyd_node* tree = nullptr;

    switch (opType) {
    case OperationType::ReplyNetconf:
        lyd_parse_op(m_node->schema->module->ctx, m_node, in, utils::toLydFormat(format), utils::toOpType(opType), &tree, nullptr);
        break;
    default:
        throw Error("Context::parseOp: unsupported op");
    }

    return {
        .tree = tree ? std::optional{libyang::wrapRawNode(tree)} : std::nullopt,
        .op = op ? std::optional{libyang::wrapRawNode(op)} : std::nullopt
    };
}

/**
 * Releases the contained value from the tree.
 * In case of DataNode, this returned value takes ownership of the node, and the value will no longer be available.
 * In case of JSON, no ownership is transferred and one can call this function repeatedly.
 */
AnydataValue DataNodeAny::releaseValue()
{
    auto any = reinterpret_cast<lyd_node_any*>(m_node);
    switch (any->value_type) {
    case LYD_ANYDATA_DATATREE: {
        if (!any->value.tree) {
            return std::nullopt;
        }

        auto res = DataNode{any->value.tree, m_refs->context};
        any->value.tree = nullptr;
        return res;
    }
    case LYD_ANYDATA_JSON:
        if (!any->value.json) {
            return std::nullopt;
        }

        return JSON{any->value.json};
    default:
        throw std::logic_error{std::string{"Unsupported anydata value type: "} + std::to_string(any->value_type)};
    }

    __builtin_unreachable();
}

/**
 * Check if both operands point to the same node in the same tree.
 */
bool DataNode::operator==(const DataNode& node) const
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
 * Creates a copy of this DataNode with siblings.
 * @param opts Options that modify the behavior of this method.
 */
DataNode DataNode::duplicateWithSiblings(const std::optional<DuplicationOptions> opts) const
{
    lyd_node* dup;
    auto ret = lyd_dup_siblings(m_node, nullptr, opts ? utils::toDuplicationOptions(*opts) : 0, &dup);

    if (ret != LY_SUCCESS) {
        throw ErrorWithCode("DataNode::duplicateWithSiblings: " + std::to_string(ret), ret);
    }

    return DataNode{dup, m_refs->context};
}

/**
 * This method handles memory management when working with low-level tree functions.
 * There are three steps:
 * 1) Update the m_refs field in all of the instances of DataNode inside nodes. The nodes must all be siblings (and
 *    therefore be from the same tree and have the same original m_refs field).
 * 2) Perform the actual libyang operation.
 * 3) Check if there is an "old tree", that needs to be released.
 */
template <typename Operation>
void handleLyTreeOperation(std::vector<DataNode*> nodes, Operation operation, std::shared_ptr<internal_refcount> newRefs)
{
    // Nodes must have at least one node.
    assert(!nodes.empty());
    auto oldRefs = nodes.front()->m_refs;
    // All nodes must be from the same tree (because they are siblings).
    assert(std::all_of(nodes.begin(), nodes.end(), [&oldRefs](DataNode* node) { return oldRefs == node->m_refs; }));

    // We need to find a lyd_node* that points to somewhere else outside the nodes inside `nodes` vector. That will be
    // used as our handle to the old tree.
    // Because all of the nodes inside the vector are siblings, the parent is definitely not in the vector.
    // If m_node is an inner node and has a parent, we'll use that.
    auto oldTree = reinterpret_cast<lyd_node*>(nodes.front()->m_node->parent);
    if (!oldTree) {
        // If parent is nullptr, then we'll search the siblings until we find one that is not inside our `nodes` vector.
        oldTree = lyd_first_sibling(nodes.front()->m_node);
        while (oldTree && std::any_of(nodes.begin(), nodes.end(), [&oldTree](DataNode* node) { return node->m_node == oldTree; })) {
            oldTree = oldTree->next;
        }

        // In the end, if we don't find any such sibling (oldTree == nullptr), then that means that the `nodes` holds
        // all of the siblings. In other words, we're updating the refcounter in all siblings.
    }

    if (oldRefs != newRefs) { // If the nodes already have the new refcounter, then there's nothing to do.
        for (auto& node : nodes) {
            node->unregisterRef();
            node->m_refs = newRefs;
            node->registerRef();

            // All references to this node and its children will need to have this new refcounter.
            for (auto it = oldRefs->nodes.begin(); it != oldRefs->nodes.end(); /* nothing */) {
                if (isDescendantOrEqual((*it)->m_node, node->m_node)) {
                    // The child needs to be added to the new refcounter and removed from the old refcounter.
                    (*it)->m_refs = node->m_refs;
                    (*it)->registerRef();
                    // Can't use unregisterRef, because I need the value from the erase method to continue the
                    // iteration. FIXME: maybe unregisterRef can return that? doesn't make much sense tho.
                    it = oldRefs->nodes.erase(it);
                } else {
                    it++;
                }
            }

            // If we're updating a node that belongs to a collection, we need to invalidate it.
            // For example:
            //      A <- iterator starts here (that's (*it)->m_start)
            //    |  |
            //    B  C <- we're updating this one (that's m_node). The iterator's m_current might also be this node.
            //
            // We must invalidate the whole collection and all its iterators, because the iterators might point to the node
            // being updated.
            //
            // If a collection is a descendant of currently updated node, we also invalidate it, because the whole
            // subtree now has a different m_refs and it's difficult to keep track of that.
            for (const auto& it : oldRefs->dataCollectionsDfs) {
                if (isDescendantOrEqual(node->m_node, it->m_start) || isDescendantOrEqual(it->m_start, node->m_node)) {
                    it->invalidate();
                }
            }

            // We need to invalidate all DataSets unconditionally, we can't be sure what's in them, potentially anything.
            for (const auto& it : oldRefs->dataSets) {
                it->invalidate();
            }

            // No need to invalidate siblings collections, the only thing they do is call node->next. If the user
            // changes the tree, it doesn't affect anything.
        }
    }

    operation();

    // If oldTree exists and we don't hold any references to it, we must also free it.
    if (oldTree && oldRefs->nodes.size() == 0) {
        lyd_free_all(reinterpret_cast<lyd_node*>(oldTree));
    }
}

/**
 * Unlinks this node, creating a new tree.
 */
void DataNode::unlink()
{
    handleLyTreeOperation({this}, [this] () {
        lyd_unlink_tree(m_node);
    }, std::make_shared<internal_refcount>(m_refs->context));
}

std::vector<DataNode*> DataNode::getFollowingSiblingRefs()
{
    std::vector<DataNode*> res;
    auto sibling = m_node->next;

    while (sibling) {
        std::copy_if(m_refs->nodes.begin(), m_refs->nodes.end(), std::back_inserter(res), [&sibling](auto ref) { return ref->m_node == sibling; });

        sibling = sibling->next;
    }

    return res;
}

/**
 * Unlinks this node, together with all following siblings, creating a new tree.
 */
void DataNode::unlinkWithSiblings()
{
    auto followingSiblings = getFollowingSiblingRefs();
    followingSiblings.push_back(this);
    handleLyTreeOperation(followingSiblings, [this] {
            lyd_unlink_siblings(m_node);
    }, std::make_shared<internal_refcount>(m_refs->context));
}

/**
 * Inserts `toInsert` below `this`.
 * 1) If `toInsert` has a parent, `toInsert` is automatically unlinked from its old tree.
 * 2) If `toInsert` does not have a parent, this method also inserts all its following siblings.
 */
void DataNode::insertChild(DataNode toInsert)
{
    std::vector<libyang::DataNode*> siblings;
    if (toInsert.m_node->parent) {
        toInsert.unlink();
    } else {
        toInsert.unlinkWithSiblings();
    }

    // If we don't have a parent, libyang also inserts all following siblings.
    if (!toInsert.m_node->parent) {
        siblings = toInsert.getFollowingSiblingRefs();
    }

    siblings.push_back(&toInsert);

    handleLyTreeOperation(siblings, [this, &toInsert] {
        lyd_insert_child(this->m_node, toInsert.m_node);
    }, m_refs);
}

/**
 * Inserts `toInsert` as a sibling `this`.
 */
DataNode DataNode::insertSibling(DataNode toInsert)
{
    lyd_node* firstSibling;
    handleLyTreeOperation({&toInsert}, [this, &toInsert, &firstSibling] {
        lyd_insert_sibling(this->m_node, toInsert.m_node, &firstSibling);
    }, m_refs);

    return DataNode{m_node, m_refs};
}

void DataNode::insertAfter(DataNode toInsert)
{
    toInsert.unlink();

    handleLyTreeOperation({&toInsert}, [this, &toInsert] {
        lyd_insert_after(this->m_node, toInsert.m_node);
    }, m_refs);
}

void DataNode::insertBefore(DataNode toInsert)
{
    toInsert.unlink();

    handleLyTreeOperation({&toInsert}, [this, &toInsert] {
        lyd_insert_before(this->m_node, toInsert.m_node);
    }, m_refs);
}

/**
 * Merges `toMerge` into `this`. After the operation, `this` will always point to the first sibling.
 * Both `this` and `toMerge` must be a top-level node.
 */
void DataNode::merge(DataNode toMerge)
{
    // No memory management needed, the original tree is left untouched.
    lyd_merge_tree(&this->m_node, toMerge.m_node, 0);
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
    std::function<Value(lyd_value)> impl = [this, &impl](const lyd_value value) -> Value {
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
        case LY_TYPE_BINARY: {
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
        case LY_TYPE_DEC64: {
            auto schemaDef = reinterpret_cast<const lysc_node_leaf*>(m_node->schema);
            auto dec = reinterpret_cast<const lysc_type_dec*>(schemaDef->type);
            return Decimal64{value.dec64, dec->fraction_digits};
        }
        case LY_TYPE_BITS: {
            auto bits = valueGetSpecial<lyd_value_bits>(&value);
            std::vector<Bit> res;
            std::transform(bits->items, bits->items + LY_ARRAY_COUNT(bits->items), std::back_inserter(res), [](const lysc_type_bitenum_item* bit) {
                return Bit{.position = bit->position, .name = bit->name};
            });

            return res;
        }
        case LY_TYPE_ENUM:
            return Enum{value.enum_item->name};
        case LY_TYPE_IDENT:
            return IdentityRef{.module = value.ident->module->name, .name = value.ident->name};
        case LY_TYPE_INST: {
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
Collection<DataNode, IterationType::Dfs> DataNode::childrenDfs() const
{
    return Collection<DataNode, IterationType::Dfs>{m_node, m_refs};
}

Collection<DataNode, IterationType::Sibling> DataNode::siblings() const
{
    return Collection<DataNode, IterationType::Sibling>{m_node, m_refs};
}

SchemaNode DataNode::schema() const
{
    if (isOpaque()) {
        throw Error{"DataNode::schema(): node is opaque"};
    }
    return SchemaNode{m_node->schema, m_refs ? m_refs->context : nullptr};
}

/**
 * Creates metadata for the node. Wraps `lyd_new_meta`.
 */
void DataNode::newMeta(const Module& module, const char* name, const char* value)
{
    // TODO: allow setting the clear_dflt argument
    // TODO: allow returning the lyd_meta struct
    lyd_new_meta(m_node->schema->module->ctx, m_node, module.m_module, name, value, false, nullptr);
}

DataNodeSet DataNode::findXPath(const char* xpath) const
{
    ly_set* set;
    auto ret = lyd_find_xpath(m_node, xpath, &set);

    if (ret != LY_SUCCESS) {
        throw ErrorWithCode("DataNode::findXPath: " + std::to_string(ret), ret);
    }

    return DataNodeSet{set, m_refs};
}

/**
 * Finds a sibling that corresponds to a SchemaNode instance.
 * @param schema The SchemaNode node used for searching.
 * @param value Optional value of the searched node:
 *              for leaf-lists: the value of the leaf-list
 *              for lists: instance key values in the form [key1='val1'][key2='val2']
 *              If not specified, the moethod returns the first instance.
 * @return The found DataNode. std::nullopt if no node is found.
 */
std::optional<DataNode> DataNode::findSiblingVal(SchemaNode schema, const char* value) const
{
    lyd_node* node;
    auto ret = lyd_find_sibling_val(m_node, schema.m_node, value, 0, &node);

    switch (ret) {
    case LY_SUCCESS:
        return DataNode{node, m_refs};
    case LY_ENOTFOUND:
        return std::nullopt;
    case LY_EINVAL:
        throw ErrorWithCode{"DataNode::findSiblingVal: `schema` is a key-less list", ret};
    default:
        throw ErrorWithCode{"DataNode::findSiblingVal: couldn't find sibling", ret};
    }
}

/**
 * Checks whether a node is opaque, i.e. it doesn't have a schema node associated with it.
 */
bool DataNode::isOpaque() const
{
    return !m_node->schema;
}

DataNodeOpaque DataNode::asOpaque() const
{
    if (!isOpaque()) {
        throw Error{"Node is not opaque"};
    }
    return DataNodeOpaque{m_node, m_refs};
}

OpaqueName DataNodeOpaque::name() const
{
    auto opaq = reinterpret_cast<lyd_node_opaq*>(m_node);
    return OpaqueName{
        .prefix = opaq->name.prefix ? std::optional(opaq->name.prefix) : std::nullopt,
        .name = opaq->name.name
    };
}

std::string_view DataNodeOpaque::value() const
{
    auto opaq = reinterpret_cast<lyd_node_opaq*>(m_node);
    return opaq->value;
}

/**
 * Wraps a raw non-null lyd_node pointer.
 * @param node The pointer to be wrapped. Must not be null.
 * @returns The wrapped pointer.
 */
DataNode wrapRawNode(lyd_node* node)
{
    if (!node) {
        throw Error{"wrapRawNode: arg must not be null"};
    }

    return DataNode{node, std::make_shared<internal_refcount>(std::shared_ptr<ly_ctx>(node->schema ? node->schema->module->ctx : nullptr, [](ly_ctx*) {}))};
}


/**
 * Wraps a raw non-null const lyd_node pointer. The DataNode does NOT free the original lyd_node (it is unmanaged).
 * Serves as a non-owning class.
 * @param node The pointer to be wrapped. Must not be null.
 * @returns The wrapped class.
 */
const DataNode wrapUnmanagedRawNode(const lyd_node* node)
{
    if (!node) {
        throw Error{"wrapRawNode: arg must not be null"};
    }
    return DataNode{const_cast<lyd_node*>(node), unmanaged_tag{}};
}

/**
 * Releases raw C-pointer from a DataNode instance without freeing it.
 * @param node The DataNode instance to be released.
 * @returns The wrapped class.
 */
lyd_node* releaseRawNode(DataNode node)
{
    node.m_refs = nullptr;
    return node.m_node;
}

/**
 * Retrieves raw C-pointer from a DataNode instance. The DataNode still manages the lifetime of the underlying pointer.
 * @returns The underlying raw `lyd_node` pointer.
 */
lyd_node* getRawNode(DataNode node)
{
    return node.m_node;
}

/**
 * Validate `node`. The validation process is fragile and so the argument must be the only reference to this tree. The
 * validation can potentially change libyang::DataNode to a std::nullopt and vice versa (through the reference
 * argument).
 */
void validateAll(std::optional<libyang::DataNode>& node, const std::optional<ValidationOptions>& opts)
{
    if (node && !node->m_refs.unique()) {
        throw Error("validateAll: Node is not a unique reference");
    }

    // TODO: support the `diff` argument
    lyd_validate_all(node ? &node->m_node : nullptr, nullptr, opts ? utils::toValidationOptions(*opts) : 0, nullptr);
    if (!node->m_node) {
        node = std::nullopt;
    }
}
}
