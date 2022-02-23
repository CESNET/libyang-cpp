/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <libyang-cpp/Collection.hpp>
#include <libyang-cpp/ChildInstantiables.hpp>
#include <libyang-cpp/Module.hpp>
#include <libyang-cpp/SchemaNode.hpp>
#include <libyang-cpp/Utils.hpp>
#include <libyang/libyang.h>
#include <libyang/tree.h>
#include <libyang/tree_schema.h>
#include "utils/enum.hpp"

using namespace std::string_literals;

namespace libyang {
SchemaNode::SchemaNode(const lysc_node* node, std::shared_ptr<ly_ctx> ctx)
    : m_node(node)
    , m_ctx(ctx)
{
}

/**
 * Creates a SchemaNode resulting with no managed context.
 */
SchemaNode::SchemaNode(const lysc_node* node, std::nullptr_t)
    : m_node(node)
    , m_ctx(nullptr)
{
}

/**
 * Returns the module of the schema node.
 */
Module SchemaNode::module() const
{
    return Module{m_node->module, m_ctx};
}

/**
 * Returns the schema noda of this node.
 */
String SchemaNode::path() const
{
    // TODO: support all path formats
    auto str = lysc_path(m_node, LYSC_PATH_DATA, nullptr, 0);

    if (!str) {
        throw std::bad_alloc();
    }

    return String{str};
}

/**
 * Returns the name of the node.
 */
std::string_view SchemaNode::name() const
{
    return m_node->name;
}

/**
 * Returns a collection of data-instantiable children. The order of iteration is unspecified. Wraps `lys_getnext`.
 */
ChildInstanstiables SchemaNode::childInstantiables() const
{
    return ChildInstanstiables{m_node, nullptr, m_ctx};
}

/**
 * Returns a collection for iterating depth-first over the subtree this SchemaNode points to.
 * If the `DataNodeCollectionDfs` object gets destroyed, all iterators associated with it get invalidated.
 */
Collection<SchemaNode, IterationType::Dfs> SchemaNode::childrenDfs() const
{
    return Collection<SchemaNode, IterationType::Dfs>{m_node, m_ctx};
}

/**
 * Returns the YANG description of the node.
 *
 * @return view of the description if it exists, std::nullopt if not.
 */
std::optional<std::string_view> SchemaNode::description() const
{
    if (!m_node->dsc) {
        return std::nullopt;
    }

    return m_node->dsc;
}

/**
 * Returns the YANG status of the node.
 */
Status SchemaNode::status() const
{
    if (m_node->flags & LYS_STATUS_CURR) {
        return Status::Current;
    }

    if (m_node->flags & LYS_STATUS_DEPRC) {
        return Status::Deprecated;
    }

    if (m_node->flags & LYS_STATUS_OBSLT) {
        return Status::Obsolete;
    }

    throw Error(std::string{"Couldn't retrieve the status of '"} + path().get().get());
}

Config SchemaNode::config() const
{
    if (m_node->flags & LYS_CONFIG_W) {
        return Config::True;
    }

    if (m_node->flags & LYS_CONFIG_R) {
        return Config::False;
    }

    throw Error(std::string{"Couldn't retrieve config value of '"} + path().get().get());
}

NodeType SchemaNode::nodeType() const
{
    return utils::toNodeType(m_node->nodetype);
}

Container SchemaNode::asContainer() const
{
    if (nodeType() != NodeType::Container) {
        throw Error("Schema node is not a container: " + std::string{path()});
    }

    return Container{m_node, m_ctx};
}

Leaf SchemaNode::asLeaf() const
{
    if (nodeType() != NodeType::Leaf) {
        throw Error("Schema node is not a leaf: " + std::string{path()});
    }

    return Leaf{m_node, m_ctx};
}

LeafList SchemaNode::asLeafList() const
{
    if (nodeType() != NodeType::Leaflist) {
        throw Error("Schema node is not a leaf-list: " + std::string{path()});
    }

    return LeafList{m_node, m_ctx};
}

List SchemaNode::asList() const
{
    if (nodeType() != NodeType::List) {
        throw Error("Schema node is not a list: " + std::string{path()});
    }

    return List{m_node, m_ctx};
}

std::optional<SchemaNode> SchemaNode::child() const
{
    auto child = lysc_node_child(m_node);

    if (!child) {
        return std::nullopt;
    }

    return SchemaNode{child, m_ctx};
}

std::optional<SchemaNode> SchemaNode::parent() const
{
    if (!m_node->parent) {
        return std::nullopt;
    }

    return SchemaNode{m_node->parent, m_ctx};
}

ActionRpc SchemaNode::asActionRpc() const
{
    if (auto type = nodeType(); type != NodeType::RPC && type != NodeType::Action) {
        throw Error("Schema node is not an action or an RPC: " + std::string{path()});
    }

    return ActionRpc{m_node, m_ctx};
}

bool Container::isPresence() const
{
    return !lysc_is_np_cont(m_node);
}

bool Leaf::isKey() const
{
    return lysc_is_key(m_node);
}

/**
 * Retrieves type info about the leaf.
 */
Type Leaf::valueType() const
{
    auto typeParsed =
        ly_ctx_get_options(m_ctx.get()) & LY_CTX_SET_PRIV_PARSED ? &reinterpret_cast<const lysp_node_leaf*>(m_node->priv)->type :
        nullptr;

    return Type{reinterpret_cast<const lysc_node_leaf*>(m_node)->type, typeParsed, m_ctx};
}

/**
 * Retrieves type info about the leaf-list.
 */
Type LeafList::valueType() const
{
    auto typeParsed =
        ly_ctx_get_options(m_ctx.get()) & LY_CTX_SET_PRIV_PARSED ? &reinterpret_cast<const lysp_node_leaf*>(m_node->priv)->type :
        nullptr;

    return Type{reinterpret_cast<const lysc_node_leaf*>(m_node)->type, typeParsed, m_ctx};
}

/**
 * Retrieves the units for this leaf.
 */
std::optional<std::string_view> Leaf::units() const
{
    auto units = reinterpret_cast<const lysc_node_leaf*>(m_node)->units;
    if (!units) {
        return std::nullopt;
    }

    return units;
}

/**
 * Retrieves the units for this leaf-list.
 */
std::optional<std::string_view> LeafList::units() const
{
    auto units = reinterpret_cast<const lysc_node_leaflist*>(m_node)->units;
    if (!units) {
        return std::nullopt;
    }

    return units;
}

/**
 * Retrieves the default string value for this node.
 */
std::optional<std::string_view> Leaf::defaultValueStr() const
{
    auto dflt = reinterpret_cast<const lysc_node_leaf*>(m_node)->dflt;
    if (dflt) {
        return std::string_view{lyd_value_get_canonical(m_ctx.get(), dflt)};
    } else {
        return std::nullopt;
    }
}

/**
 * Returns key nodes of the list.
 */
std::vector<Leaf> List::keys() const
{
    auto list = reinterpret_cast<const lysc_node_list*>(m_node);
    std::vector<Leaf> res;
    lysc_node* elem;
    LY_LIST_FOR(list->child, elem)
    {
        if (lysc_is_key(elem)) {
            Leaf leaf(elem, m_ctx);
            res.emplace_back(std::move(leaf));
        }
    }

    return res;
}

/**
 * Retrieve the input node of the RPC.
 */
ActionRpcInput ActionRpc::input() const
{
    // I need a lysc_node* for ActionRpcInput, but m_node->input is a lysp_node_action_inout. lysp_node_action_inout is
    // still just a lysc_node, so I'll just convert to lysc_node.
    // This is not very pretty, but I don't want to introduce another member for ActionRpcInput and ActionRpcOutput.
    return ActionRpcInput{reinterpret_cast<const lysc_node*>(&reinterpret_cast<const lysc_node_action*>(m_node)->input), m_ctx};
}

/**
 * Retrieve the output node of the RPC.
 */
ActionRpcOutput ActionRpc::output() const
{
    return ActionRpcOutput{reinterpret_cast<const lysc_node*>(&reinterpret_cast<const lysc_node_action*>(m_node)->output), m_ctx};
}
}
