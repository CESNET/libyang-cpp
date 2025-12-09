/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#include <algorithm>
#include <cstring>
#include <functional>
#include <libyang-cpp/Collection.hpp>
#include <libyang-cpp/DataNode.hpp>
#include <libyang-cpp/Set.hpp>
#include <libyang-cpp/Utils.hpp>
#include <libyang/libyang.h>
#include <libyang/tree_data.h>
#include <stdexcept>
#include <string>
#include "libyang-cpp/Module.hpp"
#include "utils/deleters.hpp"
#include "utils/enum.hpp"
#include "utils/newPath.hpp"
#include "utils/ref_count.hpp"

#ifdef _MSC_VER
#  define __builtin_unreachable() __assume(0)
#endif

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

void DataNode::registerRef()
{
    if (m_refs) {
        m_refs->nodes.emplace(this);
    }
}

void DataNode::unregisterRef()
{
    if (m_refs) {
        m_refs->nodes.erase(this);
    }
}

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
 * @brief Returns the first sibling of this node list.
 *
 * Wraps `lyd_first_sibling`.
 */
DataNode DataNode::firstSibling() const
{
    return DataNode{lyd_first_sibling(m_node), m_refs};
}

/**
 * @brief Returns the previous sibling node. If there's no previous sibling node, returns this node.
 *
 * Wraps `lyd_node::prev`.
 */
DataNode DataNode::previousSibling() const
{
    return DataNode{m_node->prev, m_refs};
}

/**
 * @brief Returns the next sibling node. If there's no next sibling node, returns std::nullopt.
 *
 * Wraps `lyd_node::next`.
 */
std::optional<DataNode> DataNode::nextSibling() const
{
    if (!m_node->next) {
        return std::nullopt;
    }
    return DataNode{m_node->next, m_refs};
}

/**
 * @brief Returns the first child of this node. Works for opaque nodes as well.
 * @return The first child. std::nullopt if this node has no children.
 *
 * Wraps `lyd_child`.
 */
std::optional<DataNode> DataNode::child() const
{
    auto node = lyd_child(m_node);
    if (!node) {
        return std::nullopt;
    }

    return DataNode{node, m_refs};
}

/**
 * @brief Returns the parent of this node.
 * @return The parent. std::nullopt if this node is a top-level node.
 *
 * Wraps `lyd_node::parent`.
 */
std::optional<DataNode> DataNode::parent() const
{
    if (!m_node->parent) {
        return std::nullopt;
    }

    return DataNode{reinterpret_cast<lyd_node*>(m_node->parent), m_refs};
}

/**
 * @brief Prints the tree into a string.
 * @param format Format of the output string.
 *
 * Wraps `lyd_print_mem`.
 */
std::optional<std::string> DataNode::printStr(const DataFormat format, const PrintFlags flags) const
{
    char* str;
    std::optional<std::string> res;
    auto err = lyd_print_mem(&str, m_node, utils::toLydFormat(format), utils::toPrintFlags(flags));
    throwIfError(err, "DataNode::printStr");
    if (!str) {
        return std::nullopt;
    }

    auto strDeleter = std::unique_ptr<char, deleter_free_t>(str);
    return str;
}

/**
 * @brief Returns a node specified by `path`.
 * If the node is not found, returns std::nullopt.
 * Throws on errors.
 *
 * @param path Node to search for.
 * @param inputOutputNodes Consider input or output nodes
 * @return DataView is the node is found, other std::nullopt.
 *
 * Wraps `lyd_find_path`.
 */
std::optional<DataNode> DataNode::findPath(const std::string& path, const InputOutputNodes inputOutputNodes) const
{
    lyd_node* node;
    auto err = lyd_find_path(m_node, path.c_str(), inputOutputNodes == InputOutputNodes::Output ? true : false, &node);

    switch (err) {
    case LY_SUCCESS:
        return DataNode{node, m_refs};
    case LY_ENOTFOUND:
    case LY_EINCOMPLETE: // TODO: is this really important?
        return std::nullopt;
    default:
        throwError(err, "Error in DataNode::findPath");
    }
}

/**
 * @brief Returns the path of the pointed-to node.
 *
 * Wraps `lyd_path`.
 */
std::string DataNode::path() const
{
    // TODO: handle all path types, not just LYD_PATH_STD
    auto strDeleter = std::unique_ptr<char, deleter_free_t>(lyd_path(m_node, LYD_PATH_STD, nullptr, 0));
    if (!strDeleter) {
        throw std::bad_alloc();
    }

    return strDeleter.get();
}

/**
 * @brief Creates a new node with the supplied path, changing this tree.
 *
 * This method also creates all parents of the specified node if needed. When using `CreationOptions::Update`, and the specified node exists, it is not recreated and only the value is updated.
 *
 * @param path Path of the new node.
 * @param value String representation of the value. Use std::nullopt for non-leaf nodes and the `empty` type.
 * @param options Options that change the behavior of this method.
 * @return Returns the first created node. If no nodes were created, returns std::nullopt.
 *
 * Wraps `lyd_new_path`.
 */
std::optional<DataNode> DataNode::newPath(const std::string& path, const std::optional<std::string>& value, const std::optional<CreationOptions> options) const
{
    return impl::newPath(m_node, nullptr, m_refs, path, value, options);
}

/**
 * @brief Creates a new node with the supplied path, changing this tree.
 *
 * This method also creates all parents of the specified node if needed. When using `CreationOptions::Update`, and the specified node exists, it is not recreated and only the value is updated.
 *
 * @param path Path of the new node.
 * @param value String representation of the value. Use std::nullopt for non-leaf nodes and the `empty` type.
 * @param options Options that change the behavior of this method.
 * @return Returns the first created parent and also the node specified by `path`. These might be the same node.
 *
 * Wraps `lyd_new_path2`.
 */
CreatedNodes DataNode::newPath2(const std::string& path, const std::optional<std::string>& value, const std::optional<CreationOptions> options) const
{
    return impl::newPath2(m_node, nullptr, m_refs, path, value ? value->c_str() : nullptr, AnydataValueType::String, options);
}

/**
 * @brief Creates a new AnyData node with the supplied path, with a JSON value, changing this tree.
 *
 * This method also creates all parents of the specified node if needed. When using `CreationOptions::Update`, and the specified node exists, it is not recreated and only the value is updated.
 *
 * @param path Path of the new node.
 * @param json JSON value.
 * @param options Options that change the behavior of this method.
 * @return Returns the first created parent and also the node specified by `path`. These might be the same node.
 *
 * Wraps `lyd_new_path2`.
 */
CreatedNodes DataNode::newPath2(const std::string& path, libyang::JSON json, const std::optional<CreationOptions> options) const
{
    return impl::newPath2(m_node, nullptr, m_refs, path, json.content.data(), AnydataValueType::JSON, options);
}

/**
 * @brief Creates a new anyxml node with the supplied path, changing this tree.
 *
 * @param path Path of the new node.
 * @param xml XML value.
 * @param options Options that change the behavior of this method.
 * @return Returns the first created parent and also the node specified by `path`. These might be the same node.
 *
 * Wraps `lyd_new_path2`.
 */
CreatedNodes DataNode::newPath2(const std::string& path, libyang::XML xml, const std::optional<CreationOptions> options) const
{
    return impl::newPath2(m_node, nullptr, m_refs, path, xml.content.data(), AnydataValueType::XML, options);
}

/**
 * @brief Creates a new extension node with the supplied path, changing this tree.
 *
 * @param ext Extension instance where the node being created is defined.
 * @param path Path of the new node.
 * @param value String representation of the value. Use std::nullopt for non-leaf nodes and the `empty` type.
 * @param options Options that change the behavior of this method.
 * @return Returns the first created parent.
 */
std::optional<DataNode> DataNode::newExtPath(const ExtensionInstance& ext, const std::string& path, const std::optional<std::string>& value, const std::optional<CreationOptions> options) const
{
    auto out = impl::newExtPath(m_node, ext.m_instance, nullptr, path, value, options);

    if (!out) {
        throw std::logic_error("Expected a new node to be created");
    }

    return *out;
}

/**
 * @brief Check whether this is a term node (a leaf or a leaf-list).
 *
 * Wraps `LYD_NODE_TERM`.
 */
bool DataNode::isTerm() const
{
    return m_node->schema && (m_node->schema->nodetype & LYD_NODE_TERM);
}

/**
 * @brief Try to cast this DataNode to a DataNodeTerm node (i.e. a leaf or a leaf-list).
 * @throws Error If not a leaf or a leaflist
 */
DataNodeTerm DataNode::asTerm() const
{
    if (!isTerm()) {
        throw Error("Node is not a leaf or a leaflist");
    }

    return DataNodeTerm{m_node, m_refs};
}

/**
 * @brief Try to cast this DataNode to a DataNodeAny node (i.e. anydata or anyxml node).
 * @throws Error If not an anydata or anyxml node
 */
DataNodeAny DataNode::asAny() const
{
    if (!m_node->schema || !(m_node->schema->nodetype & LYS_ANYDATA)) {
        throw Error("Node is not anydata/anyxml");
    }

    return DataNodeAny{m_node, m_refs};
}

/**
 * @brief Parses YANG data into an operation data tree.
 *
 * Use this method to parse an operation whose format requires some out-of-band information, and the schema
 * from the Context. This method can therefore parse:
 *
 *   - a YANG RPC,
 *   - a RESTCONF RPC,
 *   - a NETCONF RPC reply,
 *   - a YANG RPC reply,
 *   - a RESTCONF RPC reply.
 *
 * In case of a RESTCONF RPC, the RPC name is encoded in the URL. In case of a response to an RPC, the RPC that
 * this response "belongs to" is known out-of-band based on the communication history. In any of these cases, create
 * an empty RPC/action node via newPath() to provide the original context, and then use this method to parse the
 * data. Or parse the original NETCONF RPC via Context::parseOp(), and then invoke the result's .op->parseOp() to
 * parse the NETCONF RPC reply.
 *
 * The returned `tree` contains opaque nodes holding the information extracted from the "envelopes" (i.e., an
 * almost-useless `input` or `output` node for RPC and its reply, respectively). The returned `op` is always empty.
 * Actual parsed payload is appended as extra child nodes to `this` object.
 *
 * Wraps `lyd_parse_op`.
 */
ParsedOp DataNode::parseOp(const std::string& input, const DataFormat format, const OperationType opType, const std::optional<ParseOptions> parseOpts) const
{
    auto in = wrap_ly_in_new_memory(input);

    switch (opType) {
    case OperationType::RpcYang:
    case OperationType::ReplyYang:
    case OperationType::ReplyNetconf:
    case OperationType::RpcRestconf:
    case OperationType::ReplyRestconf: {
        lyd_node* op = nullptr;
        lyd_node* tree = nullptr;
        auto err = lyd_parse_op(m_node->schema->module->ctx,
                                m_node,
                                in.get(),
                                utils::toLydFormat(format),
                                utils::toOpType(opType),
                                parseOpts ? utils::toParseOptions(*parseOpts) : 0,
                                &tree,
                                nullptr);
        ParsedOp res{
            .tree = tree ? std::optional{libyang::wrapRawNode(tree)} : std::nullopt,
            .op = op ? std::optional{libyang::wrapRawNode(op)} : std::nullopt
        };
        throwIfError(err, "Can't parse into operation data tree");
        return res;
    }
    case OperationType::RpcNetconf:
    case OperationType::NotificationNetconf:
    case OperationType::NotificationRestconf:
        throw Error("To parse a notification, or a NETCONF RPC, use Context::parseOp");
    default:
        throw Error("Context::parseOp: unsupported op");
    }
}

/**
 * @brief Releases the contained value from the tree.
 *
 * In case of DataNode, this returned value takes ownership of the node, and the value will no longer be available.
 * In case of JSON or XML, no ownership is transferred and one can call this function repeatedly.
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
    case LYD_ANYDATA_XML:
        if (!any->value.xml) {
            return std::nullopt;
        }

        return XML{any->value.xml};
    default:
        throw std::logic_error{"Unsupported anydata value type: " + std::to_string(any->value_type)};
    }

    __builtin_unreachable();
}

/**
 * @brief Check if both operands point to the same node in the same tree.
 *
 * This comparison is a pointer comparison, meaning you could have two identical nodes in the same tree and they won't
 * be equal.
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
 * @brief Creates a copy of this DataNode.
 * @return The first duplicated node.
 *
 * Wraps `lyd_dup_single`.
 */
DataNode DataNode::duplicate(const std::optional<DuplicationOptions> opts) const
{
    lyd_node* dup;
    auto ret = lyd_dup_single(m_node, nullptr, opts ? utils::toDuplicationOptions(*opts) : 0, &dup);

    throwIfError(ret, "DataNode::duplicate:");

    return DataNode{dup, m_refs->context};
}

/**
 * @brief Creates a copy of this DataNode with siblings to the right of this nodes (following siblings).
 * @return The first duplicated node.
 *
 * Wraps `lyd_dup_siblings`.
 */
DataNode DataNode::duplicateWithSiblings(const std::optional<DuplicationOptions> opts) const
{
    lyd_node* dup;
    auto ret = lyd_dup_siblings(m_node, nullptr, opts ? utils::toDuplicationOptions(*opts) : 0, &dup);

    throwIfError(ret, "DataNode::duplicateWithSiblings:");

    return DataNode{dup, m_refs->context};
}

enum class OperationScope {
    JustThisNode,
    AffectsFollowingSiblings,
};

/**
 * @brief INTERNAL: handle memory management when working with low-level tree functions
 *
 * Some C-level libyang operations might split or combine trees into independent forests, which means that the
 * C++-wrapper's ideas about "what lyd_node belongs to which internal_refcount have to be updated. That's what
 * this function is doing.
 *
 * First, we gather nodes that are going to be affected (i.e., whose internal_refount should be switched to the new one).
 * Then, the requested operation is invoked.
 * Finally, if there are any orphaned, leftover C nodes, these are released.
 */
template <typename Operation, typename OperationScope>
void handleLyTreeOperation(DataNode* affectedNode, Operation operation, OperationScope scope, std::shared_ptr<internal_refcount> newRefs)
{
    std::vector<DataNode*> wrappedSiblings{affectedNode};
    if (scope == OperationScope::AffectsFollowingSiblings) {
        auto fs = affectedNode->gatherReachableFollowingSiblings();
        wrappedSiblings.reserve(fs.size() + 1 /* the original node */);
        std::copy(fs.begin(), fs.end(), std::back_inserter(wrappedSiblings));
    }

    auto oldRefs = affectedNode->m_refs;

    if (!oldRefs) {
        // The node is an unmanaged node, we will do nothing.
        operation();
        return;
    }

    // Find a node that is not going to be affected by the operation. That node will be used as a handle to the old
    // part of the original tree (the one to be released if it becomes unreachable from the C++ wrappers).
    //
    // Let's try the parent of the affected node as the first candidate.
    auto oldTree = reinterpret_cast<lyd_node*>(affectedNode->m_node->parent);
    if (!oldTree) {
        // If there's no parent, consider all siblings, starting from the first (ever) one, and ignoring those
        // siblings that cannot be used.
        auto candidate = lyd_first_sibling(affectedNode->m_node);
        while (candidate) {
            if (candidate != affectedNode->m_node) {
                oldTree = candidate;
                break;
            }

            if (scope == OperationScope::AffectsFollowingSiblings) {
                // all the remaining ones are going to be affected -> no luck
                break;
            }

            // none of the preceding siblings have matched so far; try the next ones
            candidate = candidate->next;
        }
        // If we didn't find any such sibling (oldTree == nullptr), we're updating the refcounter of all siblings,
        // and there's nothing that's being orphaned.
    }

    if (oldRefs != newRefs) { // If the nodes already have the new refcounter, then there's nothing to do.
        for (auto& node : wrappedSiblings) {
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

            // Sibling collections also have to be unvalidated, in case we free something we hold an iterator to.
            for (const auto& it : oldRefs->dataCollectionsSibling) {
                it->invalidate();
            }
        }
    }

    operation();

    // If oldTree exists and we don't hold any references to it, we must also free it.
    if (oldTree && oldRefs->nodes.size() == 0) {
        lyd_free_all(reinterpret_cast<lyd_node*>(oldTree));
    }
}

/**
 * @brief Unlinks this node, creating a new tree.
 *
 * Wraps `lyd_unlink_tree`.
 */
void DataNode::unlink()
{
    handleLyTreeOperation(this, [this] () {
        lyd_unlink_tree(m_node);
    }, OperationScope::JustThisNode, std::make_shared<internal_refcount>(m_refs ? m_refs->context : nullptr));
}

/**
 * @brief Gather all directly reachable instances of the C++ wrapper of all following siblings
 *
 * This is very different from "all following siblings"; only those nodes which have a libyang::DataNode already
 * instantiated (and reachable through the same reference_wrapper) are returned.
 * */
std::vector<DataNode*> DataNode::gatherReachableFollowingSiblings()
{
    std::vector<DataNode*> res;

    if (!m_refs) {
        return res;
    }

    auto sibling = m_node->next;
    while (sibling) {
        std::copy_if(m_refs->nodes.begin(), m_refs->nodes.end(), std::back_inserter(res), [&sibling](auto ref) { return ref->m_node == sibling; });
        sibling = sibling->next;
    }

    return res;
}

/**
 * @brief Unlinks this node, together with all following siblings, creating a new tree.
 *
 * Wraps `lyd_unlink_siblings`.
 */
void DataNode::unlinkWithSiblings()
{
    handleLyTreeOperation(this, [this] {
            lyd_unlink_siblings(m_node);
    }, OperationScope::AffectsFollowingSiblings, std::make_shared<internal_refcount>(m_refs ? m_refs->context : nullptr));
}

/**
 * @brief Inserts `toInsert` below `this`.
 *
 * 1) If `toInsert` has a parent, `toInsert` is automatically unlinked from its old tree.
 * 2) If `toInsert` does not have a parent, this method also inserts all its following siblings.
 *
 * Wraps `lyd_insert_child`.
 */
void DataNode::insertChild(DataNode toInsert)
{
    handleLyTreeOperation(&toInsert, [this, &toInsert] {
        lyd_insert_child(this->m_node, toInsert.m_node);
    }, toInsert.parent() ? OperationScope::JustThisNode : OperationScope::AffectsFollowingSiblings, m_refs);
}

/**
 * @brief Inserts `toInsert` as a sibling to `this`.
 *
 * 1) If `toInsert` has a parent, `toInsert` is automatically unlinked from its old tree.
 * 2) If `toInsert` does not have a parent, this method also inserts all its following siblings.
 *
 * @return The first sibling after insertion.
 *
 * Wraps `lyd_insert_sibling`.
 */
DataNode DataNode::insertSibling(DataNode toInsert)
{
    lyd_node* firstSibling;
    handleLyTreeOperation(&toInsert, [this, &toInsert, &firstSibling] {
        lyd_insert_sibling(this->m_node, toInsert.m_node, &firstSibling);
    }, toInsert.parent() ? OperationScope::JustThisNode : OperationScope::AffectsFollowingSiblings, m_refs);

    return DataNode{firstSibling, m_refs};
}

/**
 * @brief Inserts `toInsert` as a following sibling to `this`.
 *
 * Wraps `lyd_insert_after`.
 */
void DataNode::insertAfter(DataNode toInsert)
{
    handleLyTreeOperation(&toInsert, [this, &toInsert] {
        lyd_insert_after(this->m_node, toInsert.m_node);
    }, OperationScope::JustThisNode, m_refs);
}

/**
 * @brief Inserts `toInsert` as a preceeding sibling to `this`.
 *
 * Wraps `lyd_insert_before`.
 */
void DataNode::insertBefore(DataNode toInsert)
{
    handleLyTreeOperation(&toInsert, [this, &toInsert] {
        lyd_insert_before(this->m_node, toInsert.m_node);
    }, OperationScope::JustThisNode, m_refs);
}

/**
 * @brief Merges `toMerge` into `this`. After the operation, `this` will always point to the first sibling.
 *
 * Both `this` and `toMerge` must be a top-level node.
 *
 * Wraps `lyd_merge_tree`.
 */
void DataNode::merge(DataNode toMerge)
{
    // No memory management needed, the original tree is left untouched. The m_refs is not shared between `this` and
    // `toMerge` after this operation. Merge in this situation is more like a "copy stuff from `toMerge` to `this`".
    // lyd_merge_tree can also spend the source tree using LYD_MERGE_DESTRUCT, but this method does not implement that.
    // TODO: implement LYD_MERGE_DESTRUCT
    lyd_merge_tree(&this->m_node, toMerge.m_node, 0);
}

/**
 * @brief Gets the value of this term node as a string.
 */
std::string DataNodeTerm::valueStr() const
{
    return lyd_get_value(m_node);
}

/**
 * @brief Checks whether this DataNodeTerm contains the default value.
 *
 * Wraps `lyd_is_default`.
 */
bool DataNodeTerm::hasDefaultValue() const
{
    return lyd_is_default(m_node);
}

/**
 * @brief Checks if this DataNodeTerm contains a default value that was created implicitly (during the validation process)
 */
bool DataNodeTerm::isImplicitDefault() const
{
    return m_node->flags & LYD_DEFAULT;
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
 * @brief Retrieves a value in a machine-readable format.
 *
 * The lifetime of the value is not connected to the lifetime of the original DataNodeTerm.
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
            return valueStr();
        case LY_TYPE_UNION:
            return impl(value.subvalue->value);
        case LY_TYPE_DEC64: {
            return Decimal64{value.dec64, reinterpret_cast<const lysc_type_dec*>(value.realtype)->fraction_digits};
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
            return Enum{.name = value.enum_item->name, .value = value.enum_item->value};
        case LY_TYPE_IDENT:
            return IdentityRef{.module = value.ident->module->name, .name = value.ident->name, .schema = Identity(value.ident, this->m_refs ? this->m_refs->context : nullptr)};
        case LY_TYPE_INST: {
            lyd_node* out;
            auto err = lyd_find_target(value.target, m_node, &out);
            switch (err) {
            case LY_SUCCESS:
                return InstanceIdentifier{lyd_get_value(m_node), DataNode{out, m_refs}};
            case LY_ENOTFOUND:
                return InstanceIdentifier{lyd_get_value(m_node), std::nullopt};
            default:
                throwError(err, "Error when finding inst-id target");
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
 * @brief Returns the actual, resolved type which holds the current value
 *
 * This might be a different type compared to the type returned through the associated schema node; the schema node
 * represents type information about *possible* data types, while this function returns a representation of a type
 * which is used for storing the current value of this data node.
 *
 * Due to the libyang limitations, the returned type will not contain any parsed-type representation, and therefore
 * even calling types::Type::name() on it will fail (possibly with a misleading exception
 * "Context not created with libyang::ContextOptions::SetPrivParsed" even when the context *was* created with this
 * option).
 */
types::Type DataNodeTerm::valueType() const
{
    std::function<types::Type(lyd_value)> impl = [this, &impl](const lyd_value value) -> types::Type {
        switch (value.realtype->basetype) {
        case LY_TYPE_UNION:
            return impl(value.subvalue->value);
        default:
            return types::Type{value.realtype, nullptr, m_refs->context};
        }
    };
    return impl(reinterpret_cast<const lyd_node_term*>(m_node)->value);
}

/** @short Change the term's value
 *
 * Wraps `lyd_change_term`.
 * */
DataNodeTerm::ValueChange DataNodeTerm::changeValue(const std::string value)
{
    auto ret = lyd_change_term(m_node, value.c_str());

    switch (ret) {
    case LY_SUCCESS:
        return ValueChange::Changed;
    case LY_EEXIST:
        return ValueChange::ExplicitNonDefault;
    case LY_ENOT:
        return ValueChange::EqualValueNotChanged;
    default:
        throwIfError(ret, "DataNodeTerm::changeValue failed");
        __builtin_unreachable();
    }
}

/**
 * @brief Returns a collection for iterating depth-first over the subtree this instance points to.
 *
 * Any kind of low-level manipulation (e.g. unlinking) of the subtree invalidates the iterator.
 * If the `DataNodeCollectionDfs` object gets destroyed, all iterators associated with it get invalidated.
 */
Collection<DataNode, IterationType::Dfs> DataNode::childrenDfs() const
{
    return Collection<DataNode, IterationType::Dfs>{m_node, m_refs};
}

/**
 * @brief Returns a collection for iterating over the following siblings of this instance.
 *
 * Preceeding siblings are not part of the iteration. The iteration does not wrap, it ends when there are no more
 * following siblings.
 */
Collection<DataNode, IterationType::Sibling> DataNode::siblings() const
{
    return Collection<DataNode, IterationType::Sibling>{m_node, m_refs};
}

/**
 * @brief Returns a collection for iterating over the immediate children of this instance.
 *
 * This is a convenience function for iterating over this->child().siblings() which does not throw even if this is a leaf.
 */
Collection<DataNode, IterationType::Sibling> DataNode::immediateChildren() const
{
    auto c = child();
    return c ? c->siblings() : Collection<DataNode, IterationType::Sibling>{nullptr, nullptr};
}

/**
 * @brief Returns the associated SchemaNode to this DataNode.
 *
 * Does not work for opaque nodes.
 */
SchemaNode DataNode::schema() const
{
    if (isOpaque()) {
        throw Error{"DataNode::schema(): node is opaque"};
    }
    return SchemaNode{m_node->schema, m_refs ? m_refs->context : nullptr};
}

/**
 * @brief Creates metadata for the node.
 *
 * Wraps `lyd_new_meta`.
 */
void DataNode::newMeta(const Module& module, const std::string& name, const std::string& value)
{
    if (!m_node->schema) {
        throw Error{"DataNode::newMeta: can't add attributes to opaque nodes"};
    }

    // TODO: allow setting the clear_dflt argument
    // TODO: allow returning the lyd_meta struct
    auto ret = lyd_new_meta(m_refs->context.get(), m_node, module.m_module, name.c_str(), value.c_str(), false, nullptr);

    throwIfError(ret, "DataNode::newMeta: couldn't add metadata for " + path());
}

/**
 * @brief Returns a collection of metadata of this node.
 *
 * Wraps `lyd_node::meta`.
 */
MetaCollection DataNode::meta() const
{
    return MetaCollection{m_node->meta, *this};
}

Meta::Meta(lyd_meta* meta, std::shared_ptr<ly_ctx> ctx)
    : m_name(meta->name)
    , m_value(lyd_get_meta_value(meta))
    , m_mod(meta->annotation->module, ctx)
    , m_isInternal(lyd_meta_is_internal(meta))
{
}

std::string Meta::name() const
{
    return m_name;
}

std::string Meta::valueStr() const
{
    return m_value;
}

Module Meta::module() const
{
    return m_mod;
}

/** @brief Checks if the meta attribute is considered internal for libyang, see `lyd_meta_is_internal` */
bool Meta::isInternal() const
{
    return m_isInternal;
}

/**
 * Creates a JSON attribute for an opaque data node.
 * Wraps `lyd_new_attr`.
 *
 * @param moduleName Optional name of the module of the attribute being created.
 * @param attrName Attribute name, can include module name is the prefix.
 * @param attrValue Optional attribute value.
 */
void DataNode::newAttrOpaqueJSON(const std::optional<std::string>& moduleName, const std::string& attrName, const std::optional<std::string>& attrValue) const
{
    if (!isOpaque()) {
        throw Error{"DataNode::newAttrOpaqueJSON: node is not opaque"};
    }

    // TODO: allow returning the lyd_attr struct.
    lyd_new_attr(m_node, moduleName ? moduleName->c_str() : nullptr, attrName.c_str(), attrValue ? attrValue->c_str() : nullptr, nullptr);
}

/**
 * @brief Returns a set of nodes matching `xpath`.
 *
 * Wraps `lyd_find_xpath`.
 */
Set<DataNode> DataNode::findXPath(const std::string& xpath) const
{
    ly_set* set;
    auto ret = lyd_find_xpath(m_node, xpath.c_str(), &set);

    throwIfError(ret, "DataNode::findXPath:");

    return Set<DataNode>{set, m_refs};
}

/**
 * Finds a sibling that corresponds to a SchemaNode instance.
 * @param schema The SchemaNode node used for searching.
 * @param value Optional value of the searched node:
 *              for leaf-lists: the value of the leaf-list
 *              for lists: instance key values in the form [key1='val1'][key2='val2']
 *              If not specified, the moethod returns the first instance.
 * @return The found DataNode. std::nullopt if no node is found.
 *
 * Wraps `lyd_find_sibling_val`.
 */
std::optional<DataNode> DataNode::findSiblingVal(SchemaNode schema, const std::optional<std::string>& value) const
{
    lyd_node* node;
    auto ret = lyd_find_sibling_val(m_node, schema.m_node, value ? value->c_str() : nullptr, 0, &node);

    switch (ret) {
    case LY_SUCCESS:
        return DataNode{node, m_refs};
    case LY_ENOTFOUND:
        return std::nullopt;
    case LY_EINVAL:
        throwError(ret, "DataNode::findSiblingVal: `schema` is a key-less list");
    default:
        throwError(ret, "DataNode::findSiblingVal: couldn't find sibling");
    }
}

/**
 * Checks whether a node is opaque, i.e. it doesn't have a schema node associated with it.
 */
bool DataNode::isOpaque() const
{
    return !m_node->schema;
}

/**
 * @brief Try to cast this DataNode to a DataNodeOpaque node (i.e. a generic node without a schema definition).
 * @throws Error If not an opaque node.
 */
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
        .moduleOrNamespace = opaq->name.module_name,
        .prefix = opaq->name.prefix ? std::optional{opaq->name.prefix} : std::nullopt,
        .name = opaq->name.name};
}

std::string DataNodeOpaque::value() const
{
    return reinterpret_cast<lyd_node_opaq*>(m_node)->value;
}

std::string OpaqueName::pretty() const
{
    if (prefix) {
        if (*prefix == moduleOrNamespace) {
            return *prefix + ':' + name;
        } else {
            return "{" + moduleOrNamespace + "}, " + *prefix + ':' + name;
        }
    } else {
        return "{" + moduleOrNamespace + "}, " + name;
    }
}

/**
 * @short Fuzzy-match a real-world name against a combination of "something like a prefix" and "unqualified name"
 *
 * Because libyang doesn't propagate inherited prefixes, and because opaque nodes are magic, we seem to require
 * this "fuzzy matching". It won't properly report a match on opaque nodes with a prefix that's inherited when
 * using XML namespaces, though.
 * */
bool OpaqueName::matches(const std::string& prefixIsh, const std::string& name) const
{
    return name == this->name
        && (prefixIsh == moduleOrNamespace
            || (!!prefix && prefixIsh == *prefix));
}

/**
 * Wraps a raw non-null lyd_node pointer.
 * @param node The pointer to be wrapped. Must not be null.
 * @returns The wrapped pointer.
 */
DataNode wrapRawNode(lyd_node* node, std::shared_ptr<void> customCtx)
{
    if (!node) {
        throw Error{"wrapRawNode: arg must not be null"};
    }

    return DataNode{
        node,
        std::make_shared<internal_refcount>(
                std::shared_ptr<ly_ctx>(node->schema ? node->schema->module->ctx : nullptr, [](ly_ctx*) {}),
                customCtx)};
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
 * Validate `node`. DANGEROUS, this might lead to memory corruption.
 *
 * The validation process is fragile and so the argument `node` must be the only reference to this tree. The
 * validation can potentially change libyang::DataNode to a std::nullopt and vice versa (through the reference
 * argument). Beware of threading issues.
 */
void validateAll(std::optional<libyang::DataNode>& node, const std::optional<ValidationOptions>& opts)
{
    if (node && node->m_refs.use_count() != 1) {
        // FIXME: use_count() is fragile, other threads might happily copy. I guess there's no way around that.
        // The validation process is simply destructive in nature, and there's nothing we can do here.
        throw Error("validateAll: Node is not a unique reference");
    }

    // TODO: support the `diff` argument
    auto ret = lyd_validate_all(node ? &node->m_node : nullptr, nullptr, opts ? utils::toValidationOptions(*opts) : 0, nullptr);
    throwIfError(ret, "libyang:validateAll: lyd_validate_all failed");

    if (!node->m_node) {
        node = std::nullopt;
    }
}

/** @brief Validate op after parsing with lyd_parse_op.
 *
 * Wraps `lyd_validate_op`.
 *
 * @param input The tree with the op to validate.
 * @param opsTree The optional data tree to validate the input against.
 * @param opType The operation type. Contrary to `lyd_validate_op`, we accept not only YANG but also NETCONF and RESTCONF operation types (and internally convert them to YANG).
 */
void validateOp(libyang::DataNode& input, const std::optional<libyang::DataNode>& opsTree, OperationType opType)
{
    if (opType == OperationType::RpcYang || opType == OperationType::RpcRestconf || opType == OperationType::RpcNetconf) {
        opType = OperationType::RpcYang;
    } else if (opType == OperationType::ReplyYang || opType == OperationType::ReplyRestconf || opType == OperationType::ReplyNetconf) {
        opType = OperationType::ReplyYang;
    } else if (opType == OperationType::NotificationYang || opType == OperationType::NotificationRestconf || opType == OperationType::NotificationNetconf) {
        opType = OperationType::NotificationYang;
    } else {
        throw Error("validateOp: DataYang datatype is not supported");
    }

    auto ret = lyd_validate_op(input.m_node, opsTree ? opsTree->m_node : nullptr, utils::toOpType(opType), nullptr);
    throwIfError(ret, "libyang:validateOp: lyd_validate_op failed");
}

/** @short Find instances matching the provided XPath
 *
 * @param contextNode The node which serves as the "context node" for XPath evaluation. Use nullopt to start at root.
 * @param forest Any instance of DataNode which lives in the forest (set of trees) to be searched.
 * @param path XPath to search for
 *
 * Wraps `lyd_find_xpath3()`.
 */
Set<DataNode> findXPathAt(
        const std::optional<libyang::DataNode>& contextNode,
        const libyang::DataNode& forest,
        const std::string& xpath)
{
    ly_set* set;
    auto ret = lyd_find_xpath3(contextNode ? contextNode->m_node : nullptr, forest.m_node, xpath.c_str(),
            LY_VALUE_JSON, nullptr, nullptr, &set);

    throwIfError(ret, "libyang::findXPathAt:");

    return Set<DataNode>{set, forest.m_refs};
}

/** @short Parses data from a string into a subtree of the current node
 *
 * Due to the C API design and operation, it's strongly recommended to use ParseOptions::ParseOnly to skip validation
 * and to invoke this function on an empty parent node:
 *
 * \code{.cpp}
 * auto x = ctx.newPath("/example:a/b");
 * x.parseSubtree(R"({"example:c": 666})"s, libyang::DataFormat::JSON,
 *      libyang::ParseOptions::Strict | libyang::ParseOptions::NoState| libyang::ParseOptions::ParseOnly
 *      );
 * \endcode
 *
 * Note that the ParseOptions::Subtree flag is *not* required for parsing of subtrees (i.e., data which do not
 * start at the root).
 * Refer to the C documentation for details.
 *
 * Wraps `lyd_parse_data()`.
 */
void DataNode::parseSubtree(
        const std::string& data,
        const DataFormat format,
        const std::optional<ParseOptions> parseOpts,
        const std::optional<ValidationOptions> validationOpts)
{
    auto in = wrap_ly_in_new_memory(data);
    auto ret = lyd_parse_data(m_refs->context.get(),
            m_node,
            in.get(),
            utils::toLydFormat(format),
            parseOpts ? utils::toParseOptions(*parseOpts) : 0,
            validationOpts ? utils::toValidationOptions(*validationOpts) : 0,
            nullptr);
    throwIfError(ret, "DataNode::parseSubtree: lyd_parse_data failed");
}

/**
 * @short Compare a single node, possibly recusrively, possibly across contexts.
 *
 * Wraps `lyd_compare_single()`.
 */
bool DataNode::isEqual(const libyang::DataNode& other, const DataCompare flags) const
{
    auto res = lyd_compare_single(m_node, other.m_node, utils::toDataCompareOptions(flags));
    switch (res) {
    case LY_SUCCESS:
        return true;
    case LY_ENOT:
        return false;
    default:
        throwError(res, "lyd_compare_single");
    }
}

/**
 * @short Compare a node along its siblings, possibly recursively, possibly across contexts.
 *
 * Wraps `lyd_compare_siblings()`.
 */
bool DataNode::siblingsEqual(const libyang::DataNode& other, const DataCompare flags) const
{
    auto res = lyd_compare_siblings(m_node, other.m_node, utils::toDataCompareOptions(flags));
    switch (res) {
    case LY_SUCCESS:
        return true;
    case LY_ENOT:
        return false;
    default:
        throwError(res, "lyd_compare_single");
    }
}

/**
 * @short Find the first opaque node among the siblings
 *
 * This function was inspired by `lyd_find_sibling_opaq_next()`.
 * */
std::optional<DataNodeOpaque> DataNode::firstOpaqueSibling() const
{
    struct lyd_node *candidate = m_node;

    // Skip all non-opaque nodes; libyang guarantees to have them first, followed by a (possibly empty) set
    // of opaque nodes. This is not documented anywhere, but it was explicitly confirmed by the maintainer:
    //
    // JK: can I rely on all non-opaque nodes being listed first among the siblings, and then all opaque nodes
    // in one continuous sequence (but with an unspecified order among the opaque nodes themselves)?
    //
    // MV: yep
    while (candidate && candidate->schema) {
        candidate = candidate->next;
    }

    // walk back through the opaque nodes; however, libyang lists are not your regular linked lists
    while (candidate
           && !candidate->prev->schema // don't go from the first opaque node through the non-opaque ones
           && candidate->prev->next // don't wrap from the first node to the last one in case all of them are opaque
    ) {
        candidate = candidate->prev;
    }

    if (candidate) {
        return DataNode{candidate, m_refs}.asOpaque();
    }

    return std::nullopt;
}

}
