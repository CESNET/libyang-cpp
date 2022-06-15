/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include <cstdlib>
#include <iterator>
#include <libyang-cpp/Enum.hpp>
#include <libyang-cpp/Module.hpp>
#include <libyang-cpp/SchemaNode.hpp>
#include <libyang-cpp/Value.hpp>
#include <libyang-cpp/export.h>
#include <memory>
#include <optional>
#include <set>

struct lyd_node;
struct lyd_meta;
struct ly_ctx;
namespace libyang {
class Context;
class DataNode;
class MetaCollection;
template <typename NodeType>
class Set;
template <typename NodeType>
class SetIterator;

struct internal_refcount;
/**
 * @brief Internal use only.
 */
struct LIBYANG_CPP_EXPORT unmanaged_tag {
};


class Meta;
class DataNodeAny;
class DataNodeOpaque;
class DataNodeTerm;
struct ParsedOp;
struct CreatedNodes;

namespace impl {
std::optional<DataNode> newPath(lyd_node* node, ly_ctx* parent, std::shared_ptr<internal_refcount> refs, const std::string& path, const std::optional<std::string>& value, const std::optional<CreationOptions> options);
CreatedNodes newPath2(lyd_node* node, ly_ctx* ctx, std::shared_ptr<internal_refcount> refs, const std::string& path, const void* value, const AnydataValueType valueType, const std::optional<CreationOptions> options);
}

LIBYANG_CPP_EXPORT DataNode wrapRawNode(lyd_node* node, std::shared_ptr<void> customContext = nullptr);
LIBYANG_CPP_EXPORT const DataNode wrapUnmanagedRawNode(const lyd_node* node);
LIBYANG_CPP_EXPORT lyd_node* releaseRawNode(DataNode node);
LIBYANG_CPP_EXPORT lyd_node* getRawNode(DataNode node);

template <typename Operation>
void handleLyTreeOperation(std::vector<DataNode*> nodes, Operation operation, std::shared_ptr<internal_refcount> newRefs);

LIBYANG_CPP_EXPORT void validateAll(std::optional<libyang::DataNode>& node, const std::optional<ValidationOptions>& opts = std::nullopt);

/**
 * @brief Class representing a node in a libyang tree.
 *
 * Wraps `lyd_node`.
 */
class LIBYANG_CPP_EXPORT DataNode {
public:
    ~DataNode();
    DataNode(const DataNode& node);
    DataNode& operator=(const DataNode& node);

    DataNode firstSibling() const;
    DataNode previousSibling() const;
    std::optional<DataNode> nextSibling() const;
    std::optional<DataNode> parent() const;
    std::optional<DataNode> child() const;
    std::optional<std::string> printStr(const DataFormat format, const PrintFlags flags) const;
    std::optional<DataNode> findPath(const std::string& path, const OutputNodes output = OutputNodes::No) const;
    Set<DataNode> findXPath(const std::string& path) const;
    std::optional<DataNode> findSiblingVal(SchemaNode schema, const std::optional<std::string>& value = std::nullopt) const;
    std::string path() const;
    bool isTerm() const;
    DataNodeTerm asTerm() const;
    DataNodeAny asAny() const;
    SchemaNode schema() const;
    std::optional<DataNode> newPath(const std::string& path, const std::optional<std::string>& value = std::nullopt, const std::optional<CreationOptions> options = std::nullopt) const;
    CreatedNodes newPath2(const std::string& path, const std::optional<std::string>& value = std::nullopt, const std::optional<CreationOptions> options = std::nullopt) const;
    CreatedNodes newPath2(const std::string& path, libyang::JSON json, const std::optional<CreationOptions> options = std::nullopt) const;
    CreatedNodes newPath2(const std::string& path, libyang::XML xml, const std::optional<CreationOptions> options = std::nullopt) const;

    void newMeta(const Module& module, const std::string& name, const std::string& value);
    MetaCollection meta() const;
    void newAttrOpaqueJSON(const std::optional<std::string>& moduleName, const std::string& attrName, const std::optional<std::string>& attrValue) const;

    bool isOpaque() const;
    DataNodeOpaque asOpaque() const;

    // TODO: allow setting the `parent` argument
    DataNode duplicate(const std::optional<DuplicationOptions> opts = std::nullopt) const;
    // TODO: allow setting the `parent` argument
    DataNode duplicateWithSiblings(const std::optional<DuplicationOptions> opts = std::nullopt) const;
    void unlink();
    void unlinkWithSiblings();
    void insertChild(DataNode toInsert);
    DataNode insertSibling(DataNode toInsert);
    void insertAfter(DataNode toInsert);
    void insertBefore(DataNode toInsert);
    // TODO: allow setting options
    void merge(DataNode toInsert);

    Collection<DataNode, IterationType::Dfs> childrenDfs() const;

    Collection<DataNode, IterationType::Sibling> siblings() const;

    ParsedOp parseOp(const std::string& input, const DataFormat format, const OperationType opType) const;

    friend Context;
    friend DataNodeAny;
    friend Set<DataNode>;
    friend DataNodeTerm;
    friend Iterator<DataNode, IterationType::Dfs>;
    friend Iterator<DataNode, IterationType::Sibling>;
    friend Iterator<Meta, IterationType::Meta>;
    friend SetIterator<DataNode>;
    friend DataNode wrapRawNode(lyd_node* node, std::shared_ptr<void> customContext);
    friend const DataNode wrapUnmanagedRawNode(const lyd_node* node);
    friend lyd_node* releaseRawNode(DataNode node);
    friend lyd_node* getRawNode(DataNode node);

    friend void validateAll(std::optional<libyang::DataNode>& node, const std::optional<ValidationOptions>& opts);

    bool operator==(const DataNode& node) const;

    friend std::optional<DataNode> impl::newPath(lyd_node* node, ly_ctx* parent, std::shared_ptr<internal_refcount> viewCount, const std::string& path, const std::optional<std::string>& value, const std::optional<CreationOptions> options);
    friend CreatedNodes impl::newPath2(lyd_node* node, ly_ctx* ctx, std::shared_ptr<internal_refcount> refs, const std::string& path, const void* value, const AnydataValueType valueType, const std::optional<CreationOptions> options);

protected:
    lyd_node* m_node;

private:
    DataNode(lyd_node* node, std::shared_ptr<ly_ctx> ctx);
    DataNode(lyd_node* node, std::shared_ptr<internal_refcount> viewCount);
    DataNode(lyd_node* node, const unmanaged_tag);

    [[nodiscard]] std::vector<DataNode*> getFollowingSiblingRefs();
    void registerRef();
    void unregisterRef();
    void freeIfNoRefs();

    template <typename Operation>
    friend void handleLyTreeOperation(std::vector<DataNode*> nodes, Operation operation, std::shared_ptr<internal_refcount> newRefs);

    std::shared_ptr<internal_refcount> m_refs;
};

/**
 * @brief Represents a piece of metadata associated with a node.
 *
 * Represents a `lyd_meta` struct (but does not wrap it).
 */
class LIBYANG_CPP_EXPORT Meta {
public:
    std::string name() const;
    std::string valueStr() const;
    Module module() const;

private:
    friend Iterator<Meta, IterationType::Meta>;
    Meta(lyd_meta* meta, std::shared_ptr<ly_ctx> ctx);

    std::string m_name;
    std::string m_value;
    Module m_mod;
};


/**
 * @brief Class representing a term node - leaf or leaf-list.
 *
 * Wraps `lyd_node_term`.
 */
class LIBYANG_CPP_EXPORT DataNodeTerm : public DataNode {
public:
    std::string_view valueStr() const;
    bool isDefaultValue() const;

    friend DataNode;
    Value value() const;

private:
    using DataNode::DataNode;
};

/**
 * @brief Contains a (possibly module-qualified) name of an opaque node.
 *
 * This is generic container of a prefix/module string and a name string.
 *
 * Wraps `ly_opaq_name`.
 */
struct LIBYANG_CPP_EXPORT OpaqueName {
    std::optional<std::string_view> prefix;
    std::string_view name;
};

/**
 * @brief Class representing an opaque node.
 *
 * Wraps `lyd_node_opaq`.
 */
class LIBYANG_CPP_EXPORT DataNodeOpaque : public DataNode {
public:
    OpaqueName name() const;
    std::string_view value() const;
    friend DataNode;

private:
    using DataNode::DataNode;
};

/**
 * @brief Class representing a node of type anydata.
 */
class LIBYANG_CPP_EXPORT DataNodeAny : public DataNode {
public:
    friend DataNode;
    AnydataValue releaseValue();

private:
    using DataNode::DataNode;
};

/**
 * @brief Represents a YANG operation data tree.
 *
 * Used as the return value of `DataNode::parseOp` and Context::parseOp.
 */
struct LIBYANG_CPP_EXPORT ParsedOp {
    std::optional<libyang::DataNode> tree;
    std::optional<libyang::DataNode> op;
};

/**
 * @brief This struct represents the return value for newPath2.
 */
struct LIBYANG_CPP_EXPORT CreatedNodes {
    /**
     * @brief Contains the first created parent.
     *
     * Will be the same as `createdNode` if only one node was created.
     */
    std::optional<DataNode> createdParent;
    /**
     * @brief Contains the node specified by `path` from the original newPath2 call.
     */
    std::optional<DataNode> createdNode;
};
}
