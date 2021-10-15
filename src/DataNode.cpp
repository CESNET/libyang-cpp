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
#include <libyang/libyang.h>
#include <libyang/tree_data.h>
#include <stdexcept>
#include <string>
#include <libyang-cpp/DataNode.hpp>
#include <libyang-cpp/utils/exception.hpp>
#include "libyang-cpp/Module.hpp"
#include <libyang-cpp/Set.hpp>
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
            collection->invalidateIterators();
        }

        for (const auto& collection : m_refs->dataCollectionsSibling) {
            collection->invalidateIterators();
        }

        lyd_free_all(m_node);
    }
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
    auto deleteFunc = [] (auto* in){
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
    case LYD_ANYDATA_DATATREE:
    {
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
 * This method handles memory management when working with low-level tree functions. The method updates m_refs in the
 * `nodes` vector with newRefs and then calls the wanted libyang operation specified by `operation`.
 */
template <typename Operation>
void handleLyTreeOperation(std::vector<DataNode*> nodes, Operation operation, std::shared_ptr<internal_refcount> newRefs)
{
    // Nodes must have at least one node.
    assert(!nodes.empty());
    auto oldRefs = nodes.front()->m_refs;
    lyd_node* oldTree;

    for (auto& node : nodes) {
        // If this node already has the new refcounter, then there's nothing to do.
        if (node->m_refs == newRefs) {
            return;
        }

        node->unregisterRef();

        // We'll need a new refcounter for the unlinked tree.
        node->m_refs = newRefs;
        node->registerRef();

        // All references to this node and its children will need to have this new refcounter.
        for (auto it = oldRefs->nodes.begin(); it != oldRefs->nodes.end(); /* nothing */) {
            if (isDescendantOrEqual((*it)->m_node, node->m_node)) {
                // The child needs to be added to the new refcounter and removed from the old refcounter.
                (*it)->m_refs = node->m_refs;
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
        //
        // If a collection is a descendant of the node being unlinked, we also invalidate it, because the whole now has a
        // different m_refs and it's difficult to keep track of that.
        // FIXME: shouldn't I just invalidate everything?
        for (const auto& it : oldRefs->dataCollectionsDfs) {
            if (isDescendantOrEqual(node->m_node, it->m_start) || isDescendantOrEqual(it->m_start, node->m_node)) {
                it->m_valid = false;
            }

        }

        // We need to find a lyd_node* that points to somewhere else outside the subtree that's being unlinked.
        // If m_node is an inner node and has a parent, we'll use that.
        oldTree = reinterpret_cast<lyd_node*>(node->m_node->parent);
        if (!oldTree) {
            // If parent is nullptr, then that means that our node is either:
            // 1) A top-level node. We'll search for the first sibling. If the first sibling is our node, we'll just get the
            //    next one.
            // 2) An already unlinked node. In that case we don't have any siblings, because there's no common parent which
            //    would connect them. No freeing needs to be done. The algorithm will still work because lyd_first_sibling will
            //    return our node and ->next will be nullptr.
            oldTree = lyd_first_sibling(node->m_node);
            if (oldTree == node->m_node) {
                oldTree = oldTree->next;
            }
        }

    }

    operation();

    // If we don't hold any references to the old tree, we must also free it.
    if (oldRefs->nodes.size() == 0) {
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
    return OpaqueName {
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

    return DataNode{node, std::make_shared<internal_refcount>(std::shared_ptr<ly_ctx>(node->schema? node->schema->module->ctx : nullptr, [] (ly_ctx*) {}))};
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
