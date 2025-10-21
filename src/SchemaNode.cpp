/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <algorithm>
#include <libyang-cpp/Collection.hpp>
#include <libyang-cpp/ChildInstantiables.hpp>
#include <libyang-cpp/Module.hpp>
#include <libyang-cpp/SchemaNode.hpp>
#include <libyang-cpp/Utils.hpp>
#include <libyang/libyang.h>
#include <libyang/tree.h>
#include <libyang/tree_schema.h>
#include <span>
#include "utils/deleters.hpp"
#include "utils/enum.hpp"

using namespace std::string_literals;

namespace libyang {
SchemaNode::SchemaNode(const lysc_node* node, std::shared_ptr<ly_ctx> ctx)
    : m_node(node)
    , m_ctx(ctx)
{
}

/**
 * @brief Wraps a lysc_node pointer with no managed context.
 */
SchemaNode::SchemaNode(const lysc_node* node, std::nullptr_t)
    : m_node(node)
    , m_ctx(nullptr)
{
}

/**
 * @brief Returns the module of this schema node.
 *
 * Wraps `lysc_node::module`.
 */
Module SchemaNode::module() const
{
    return Module{m_node->module, m_ctx};
}

/**
 * @brief Returns the schema path of this node.
 *
 * Wraps `lysc_path`.
 */
std::string SchemaNode::path() const
{
    // TODO: support all path formats
    auto strDeleter = std::unique_ptr<char, deleter_free_t>(lysc_path(m_node, LYSC_PATH_DATA, nullptr, 0));
    if (!strDeleter) {
        throw std::bad_alloc();
    }

    return strDeleter.get();
}

/**
 * @brief Returns the name of this node.
 *
 * The name does NOT include any kind of prefix (YANG prefix, module name, etc.).
 *
 * Wraps `lysc_node::name`.
 */
std::string SchemaNode::name() const
{
    return m_node->name;
}

/**
 * @brief Returns a collection of data-instantiable children. The order of schema order.
 *
 * Wraps `lys_getnext`.
 */
ChildInstanstiables SchemaNode::childInstantiables() const
{
    return ChildInstanstiables{m_node, nullptr, m_ctx};
}

/**
 * @brief Returns a collection for iterating depth-first over the subtree this SchemaNode points to.
 *
 * If the `DataNodeCollectionDfs` object gets destroyed, all iterators associated with it get invalidated.
 */
Collection<SchemaNode, IterationType::Dfs> SchemaNode::childrenDfs() const
{
    return Collection<SchemaNode, IterationType::Dfs>{m_node, m_ctx};
}

/**
 * @brief Returns a collection for iterating over the following siblings of where this SchemaNode points to.
 *
 * Preceeding siblings are not part of the iteration. The iteration does not wrap, it ends when there are no more
 * following siblings.
 */
Collection<SchemaNode, IterationType::Sibling> SchemaNode::siblings() const
{
    return Collection<SchemaNode, IterationType::Sibling>{m_node, m_ctx};
}

/**
 * @brief Returns a collection for iterating over the immediate children of where this SchemaNode points to.
 *
 * This is a convenience function for iterating over this->child().siblings() which does not throw even if this is a leaf.
 */
Collection<SchemaNode, IterationType::Sibling> SchemaNode::immediateChildren() const
{
    auto c = child();
    return c ? c->siblings() : Collection<SchemaNode, IterationType::Sibling>{nullptr, nullptr};
}

/**
 * @brief Returns a collection of action nodes (not RPC nodes) as SchemaNode
 */
std::vector<SchemaNode> SchemaNode::actionRpcs() const
{
    std::vector<SchemaNode> res;
    for (auto action = reinterpret_cast<const struct lysc_node*>(lysc_node_actions(m_node)); action; action = action->next) {
        res.emplace_back(SchemaNode{action, m_ctx});
    }
    return res;
}

/**
 * Returns the YANG description of the node.
 *
 * @return view of the description if it exists, std::nullopt if not.
 *
 * Wraps `lysc_node::dsc`.
 */
std::optional<std::string> SchemaNode::description() const
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

    throw Error("Couldn't retrieve the status of '" + path());
}

/**
 * @brief Checks whether this node is YANG `config false` or `config true`.
 *
 * Wraps `LYS_CONFIG_W` and `LYS_CONFIG_R`.
 */
Config SchemaNode::config() const
{
    if (m_node->flags & LYS_CONFIG_W) {
        return Config::True;
    }

    if (m_node->flags & LYS_CONFIG_R) {
        return Config::False;
    }

    throw Error("Couldn't retrieve config value of '" + path());
}

/**
 * @brief Checks whether this node is inside a subtree of an input statement.
 *
 * Wraps `LYS_INPUT`.
 */
bool SchemaNode::isInput() const
{
    return m_node->flags & LYS_INPUT;
}

/**
 * Returns the node type of this node (e.g. leaf, container...).
 *
 * Wraps `lysc_node::nodetype`.
 */
NodeType SchemaNode::nodeType() const
{
    return utils::toNodeType(m_node->nodetype);
}

/**
 * @brief Try to cast this SchemaNode to a Case node.
 * @throws Error If this node is not a case.
 */
Case SchemaNode::asCase() const
{
    if (nodeType() != NodeType::Case) {
        throw Error("Schema node is not a case: " + path());
    }

    return Case{m_node, m_ctx};
}

/**
 * @brief Try to cast this SchemaNode to a Choice node.
 * @throws Error If this node is not a choice.
 */
Choice SchemaNode::asChoice() const
{
    if (nodeType() != NodeType::Choice) {
        throw Error("Schema node is not a choice: " + path());
    }

    return Choice{m_node, m_ctx};
}

/**
 * @brief Try to cast this SchemaNode to a Container node.
 * @throws Error If this node is not a container.
 */
Container SchemaNode::asContainer() const
{
    if (nodeType() != NodeType::Container) {
        throw Error("Schema node is not a container: " + path());
    }

    return Container{m_node, m_ctx};
}

/**
 * @brief Try to cast this SchemaNode to a Leaf node.
 * @throws Error If this node is not a leaf.
 */
Leaf SchemaNode::asLeaf() const
{
    if (nodeType() != NodeType::Leaf) {
        throw Error("Schema node is not a leaf: " + path());
    }

    return Leaf{m_node, m_ctx};
}

/**
 * @brief Try to cast this SchemaNode to a Leaflist node.
 * @throws Error If this node is not a leaflist.
 */
LeafList SchemaNode::asLeafList() const
{
    if (nodeType() != NodeType::Leaflist) {
        throw Error("Schema node is not a leaf-list: " + path());
    }

    return LeafList{m_node, m_ctx};
}

/**
 * @brief Try to cast this SchemaNode to a List node.
 * @throws Error If this node is not a list.
 */
List SchemaNode::asList() const
{
    if (nodeType() != NodeType::List) {
        throw Error("Schema node is not a list: " + path());
    }

    return List{m_node, m_ctx};
}

/**
 * @brief Returns the first child node of this SchemaNode.
 * @return The child, or std::nullopt if there are no children.
 */
std::optional<SchemaNode> SchemaNode::child() const
{
    auto child = lysc_node_child(m_node);

    if (!child) {
        return std::nullopt;
    }

    return SchemaNode{child, m_ctx};
}

/**
 * @brief Returns the parent node of this SchemaNode.
 * @return The parent, or std::nullopt for top-level nodes.
 */
std::optional<SchemaNode> SchemaNode::parent() const
{
    if (!m_node->parent) {
        return std::nullopt;
    }

    return SchemaNode{m_node->parent, m_ctx};
}

/**
 * @brief Try to cast this SchemaNode to an ActionRpc node.
 * @throws Error If this node is not an action or an RPC.
 */
ActionRpc SchemaNode::asActionRpc() const
{
    if (auto type = nodeType(); type != NodeType::RPC && type != NodeType::Action) {
        throw Error("Schema node is not an action or an RPC: " + path());
    }

    return ActionRpc{m_node, m_ctx};
}

/**
 * @brief Try to cast this SchemaNode to an AnyDataAnyXml node.
 * @throws Error If this node is not an anydata or an anyxml.
 */
AnyDataAnyXML SchemaNode::asAnyDataAnyXML() const
{
    if (auto type = nodeType(); type != NodeType::AnyData && type != NodeType::AnyXML) {
        throw Error("Schema node is not an anydata or an anyxml: " + path());
    }

    return AnyDataAnyXML{m_node, m_ctx};
}

/**
 * @brief Are both schema node instances representing the same node in the schema?
 */
bool SchemaNode::operator==(const SchemaNode& other) const
{
    return m_node == other.m_node;
}

bool SchemaNode::operator!=(const SchemaNode& other) const
{
    return m_node != other.m_node;
}

/**
 * @brief Wraps a lysc_when pointer with managed context.
 */
When::When(const lysc_when* when, std::shared_ptr<ly_ctx> ctx)
    : m_when(when)
    , m_ctx(ctx)
{
}

/**
 * Returns the YANG condition of the when statement.
 *
 * @return view of the condition
 *
 * Wraps `lysc_when::cond`.
 */
std::string When::condition() const
{
    return lyxp_get_expr(m_when->cond);
}

/**
 * Returns the YANG description of the when statement.
 *
 * @return view of the description if it exists, std::nullopt if not.
 *
 * Wraps `lysc_when::dsc`.
 */
std::optional<std::string> When::description() const
{
    if (!m_when->dsc) {
        return std::nullopt;
    }

    return m_when->dsc;
}

/**
 * @brief Retrieves the list of `when` statements.
 *
 * Wraps `lysc_when`.
 */
std::vector<When> SchemaNode::when() const
{
    auto whenList = lysc_node_when(m_node);
    std::vector<When> res;
    for (const auto& it : std::span(whenList, LY_ARRAY_COUNT(whenList))) {
        res.emplace_back(When{it, m_ctx});
    }
    return res;
}

/**
 * @brief Retrieves the list of extension instances.
 */
std::vector<ExtensionInstance> SchemaNode::extensionInstances() const
{
    std::vector<ExtensionInstance> res;
    auto span = std::span<lysc_ext_instance>(m_node->exts, LY_ARRAY_COUNT(m_node->exts));
    std::transform(span.begin(), span.end(), std::back_inserter(res), [this](const lysc_ext_instance& ext) {
        return ExtensionInstance(&ext, m_ctx);
    });
    return res;
}

/**
 * @brief Print the (sub)schema of this schema node
 *
 * Wraps `lys_print_node`.
 */
std::string SchemaNode::printStr(const SchemaOutputFormat format, const std::optional<SchemaPrintFlags> flags, std::optional<size_t> lineLength) const
{
    std::string str;
    auto buf = wrap_ly_out_new_buf(str);
    auto res = lys_print_node(buf.get(), m_node, utils::toLysOutFormat(format), lineLength.value_or(0), flags ? utils::toSchemaPrintFlags(*flags) : 0);
    throwIfError(res, "lys_print_node failed");
    return str;
}

/**
 * @brief Checks whether this anydata or anyxml is mandatory.
 *
 * AnyDataAnyXML is mandatory if it is not presence container and has at least one mandatory node as a child.
 *
 * Wraps flag `LYS_MAND_TRUE`.
 */
bool AnyDataAnyXML::isMandatory() const
{
    return m_node->flags & LYS_MAND_TRUE;
}

/**
 * @brief Checks whether this choice is mandatory.
 *
 * Wraps flag `LYS_MAND_TRUE`.
 */
bool Choice::isMandatory() const
{
    return m_node->flags & LYS_MAND_TRUE;
}

/**
 * @brief Retrieves the list of cases for this choice.
 *
 * Wraps `lysc_node_choice::cases`.
 */
std::vector<Case> Choice::cases() const
{
    auto choice = reinterpret_cast<const lysc_node_choice*>(m_node);
    auto cases = reinterpret_cast<lysc_node*>(choice->cases);
    std::vector<Case> res;
    lysc_node* elem;
    LY_LIST_FOR(cases, elem)
    {
        res.emplace_back(Case(elem, m_ctx));
    }
    return res;
}

/**
 * @brief Retrieves the default case for this choice.
 *
 * Wraps `lysc_node_choice::dflt`.
 */
std::optional<Case> Choice::defaultCase() const
{
    auto choice = reinterpret_cast<const lysc_node_choice*>(m_node);
    if (!choice->dflt) {
        return std::nullopt;
    }
    return Case{reinterpret_cast<lysc_node*>(choice->dflt), m_ctx};
}

/**
 * @brief Checks whether this container is mandatory.
 *
 * Container is mandatory if it is not presence container and has at least one mandatory node as a child.
 *
 * Wraps flag `LYS_MAND_TRUE`.
 */
bool Container::isMandatory() const
{
    return m_node->flags & LYS_MAND_TRUE;
}

/**
 * @brief Checks whether this container is a presence container.
 *
 * Wraps `lysc_is_np_cont`.
 */
bool Container::isPresence() const
{
    return !lysc_is_np_cont(m_node);
}

/**
 * @brief Checks whether this leaf is a key leaf.
 *
 * Wraps `lysc_is_key`.
 */
bool Leaf::isKey() const
{
    return lysc_is_key(m_node);
}

/**
 * @brief Checks whether this leaf is mandatory.
 *
 * Wraps flag `LYS_MAND_TRUE`.
 */
bool Leaf::isMandatory() const
{
    return m_node->flags & LYS_MAND_TRUE;
}

/**
 * @brief Retrieves type info about the leaf.
 *
 * Wraps `lysc_node_leaf::type`.
 */
types::Type Leaf::valueType() const
{
    auto typeParsed =
        ly_ctx_get_options(m_ctx.get()) & LY_CTX_SET_PRIV_PARSED ? &reinterpret_cast<const lysp_node_leaf*>(m_node->priv)->type :
        nullptr;

    return types::Type{reinterpret_cast<const lysc_node_leaf*>(m_node)->type, typeParsed, m_ctx};
}

/**
 * @brief Retrieves type info about the leaf-list.
 *
 * Wraps `lysc_node_leaflist::type`.
 */
types::Type LeafList::valueType() const
{
    // FIXME: add test for this
    auto typeParsed =
        ly_ctx_get_options(m_ctx.get()) & LY_CTX_SET_PRIV_PARSED ? &reinterpret_cast<const lysp_node_leaf*>(m_node->priv)->type :
        nullptr;

    return types::Type{reinterpret_cast<const lysc_node_leaflist*>(m_node)->type, typeParsed, m_ctx};
}

/**
 * @brief Retrieves the default string values for this leaf-list.
 *
 * @return The default values, or an empty vector if the leaf-list does not have default values.
 *
 * Wraps `lysc_node_leaflist::dflts`.
 */
std::vector<std::string> LeafList::defaultValuesStr() const
{
    auto dflts = reinterpret_cast<const lysc_node_leaflist*>(m_node)->dflts;
    std::vector<std::string> res;
    for (const auto& it : std::span(dflts, LY_ARRAY_COUNT(dflts))) {
        res.emplace_back(it.str);
    }
    return res;
}

/**
 * @brief Retrieves the units for this leaf.
 * @return The units, or std::nullopt if no units are available.
 *
 * Wraps `lysc_node_leaf::units`.
 */
std::optional<std::string> Leaf::units() const
{
    auto units = reinterpret_cast<const lysc_node_leaf*>(m_node)->units;
    if (!units) {
        return std::nullopt;
    }

    return units;
}

static_assert(std::is_same_v<libyang::types::constraints::ListSize, decltype(lysc_node_leaflist::min)> &&
        std::is_same_v<libyang::types::constraints::ListSize, decltype(lysc_node_leaflist::max)> &&
        std::is_same_v<libyang::types::constraints::ListSize, decltype(lysc_node_list::min)> &&
        std::is_same_v<libyang::types::constraints::ListSize, decltype(lysc_node_list::max)>,
        "unexpected change of libyang's internal data type for (leaf)list size constraints");

/**
 * @brief Checks whether this leaf list is mandatory.
 *
 * Leaf list is mandatory if minElements is greater than 0.
 *
 * Wraps flag `LYS_MAND_TRUE`.
 */
bool LeafList::isMandatory() const
{
    return m_node->flags & LYS_MAND_TRUE;
}

/**
 * @brief Retrieves the number of max elements for this leaflist.
 * @return The maximal number of elements, or std::numeric_limits<libyang::types::constraints::ListSize>::max() if unlimited.
 *
 * Wraps `lysc_node_leaflist::max`.
 */
libyang::types::constraints::ListSize LeafList::maxElements() const
{
    return reinterpret_cast<const lysc_node_leaflist*>(m_node)->max;
}

/**
 * @brief Retrieves the number of min elements for this leaflist.
 * @return The minimal number of elements, or 0 if unlimited.
 *
 * Wraps `lysc_node_leaflist::min`.
 */
libyang::types::constraints::ListSize LeafList::minElements() const
{
    return reinterpret_cast<const lysc_node_leaflist*>(m_node)->min;
}

/**
 * @brief Retrieves the units for this leaflist.
 * @return The units, or std::nullopt if no units are available.
 *
 * Wraps `lysc_node_leaflist::units`.
 */
std::optional<std::string> LeafList::units() const
{
    auto units = reinterpret_cast<const lysc_node_leaflist*>(m_node)->units;
    if (!units) {
        return std::nullopt;
    }

    return units;
}

/**
 * @brief Returns if the leaflist is ordered by user.
 *
 * Wraps `lysc_is_userordered`.
 */
bool LeafList::isUserOrdered() const
{
    return lysc_is_userordered(reinterpret_cast<const lysc_node_leaflist*>(m_node));
}

/**
 * @brief Retrieves the default string value for this node.
 * @return The default value, or std::nullopt if the leaf does not have default value.
 *
 * Wraps `lysc_node_leaf::dflt`.
 */
std::optional<std::string> Leaf::defaultValueStr() const
{
    auto dflt = reinterpret_cast<const lysc_node_leaf*>(m_node)->dflt;
    if (dflt.str) {
        return std::string{dflt.str};
    } else {
        return std::nullopt;
    }
}

/**
 * @brief Checks whether this list is mandatory.
 *
 * List is mandatory if minElements is greater than 0.
 *
 * Wraps flag `LYS_MAND_TRUE`.
 */
bool List::isMandatory() const
{
    return m_node->flags & LYS_MAND_TRUE;
}

/**
 * @brief Returns key nodes of the list.
 */
std::vector<Leaf> List::keys() const
{
    auto list = reinterpret_cast<const lysc_node_list*>(m_node);
    std::vector<Leaf> res;
    lysc_node* elem;
    LY_LIST_FOR(list->child, elem)
    {
        if (lysc_is_key(elem)) {
            res.emplace_back(Leaf(elem, m_ctx));
        }
    }

    return res;
}

/**
 * @brief Retrieves the number of max elements for this list.
 * @return The maximal number of elements, or std::numeric_limits<libyang::types::constraints::ListSize>::max() if unlimited.
 *
 * Wraps `lysc_node_list::max`.
 */
libyang::types::constraints::ListSize List::maxElements() const
{
    return reinterpret_cast<const lysc_node_list*>(m_node)->max;
}

/**
 * @brief Retrieves the number of min elements for this list.
 * @return The minimal number of elements, or 0 if unlimited.
 *
 * Wraps `lysc_node_list::min`.
 */
libyang::types::constraints::ListSize List::minElements() const
{
    return reinterpret_cast<const lysc_node_list*>(m_node)->min;
}

/**
 * @brief Returns if the list is ordered by user.
 *
 * Wraps `lysc_is_userordered`.
 */
bool List::isUserOrdered() const
{
    return lysc_is_userordered(reinterpret_cast<const lysc_node_list*>(m_node));
}

/**
 * @brief Retrieve the input node of this RPC/action.
 *
 * Wraps `lysc_node_action::input`.
 */
ActionRpcInput ActionRpc::input() const
{
    return ActionRpcInput{reinterpret_cast<const lysc_node*>(&reinterpret_cast<const lysc_node_action*>(m_node)->input), m_ctx};
}

/**
 * @brief Retrieve the output node of this RPC/action.
 *
 * Wraps `lysc_node_action::output`.
 */
ActionRpcOutput ActionRpc::output() const
{
    return ActionRpcOutput{reinterpret_cast<const lysc_node*>(&reinterpret_cast<const lysc_node_action*>(m_node)->output), m_ctx};
}
}
