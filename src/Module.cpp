/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <algorithm>
#include <libyang-cpp/ChildInstantiables.hpp>
#include <libyang-cpp/Collection.hpp>
#include <libyang-cpp/Module.hpp>
#include <libyang-cpp/Utils.hpp>
#include <libyang/libyang.h>
#include <span>
#include <stack>
#include "utils/deleters.hpp"
#include "utils/enum.hpp"

using namespace std::string_literals;

namespace {

template <class Printer, class ModuleType>
std::string printModule(Printer printFunc, const ModuleType* mod, const libyang::SchemaOutputFormat format, std::optional<libyang::SchemaPrintFlags> flags, std::optional<size_t> lineLength, const std::string& printFuncName)
{
    using namespace libyang;

    std::string str;
    auto buf = libyang::wrap_ly_out_new_buf(str);
    auto res = printFunc(buf.get(), mod, utils::toLysOutFormat(format), lineLength.value_or(0), flags ? utils::toSchemaPrintFlags(*flags) : 0);
    throwIfError(res, printFuncName + " failed");
    return str;
}
}

namespace libyang {
Module::Module(lys_module* module, std::shared_ptr<ly_ctx> ctx)
    : m_ctx(ctx)
    , m_module(module)
{
}

/**
 * @brief Returns the name of the module.
 *
 * Wraps `lys_module::name`.
 */
std::string Module::name() const
{
    return m_module->name;
}

/**
 * @brief Returns the (optional) revision of the module.
 *
 * Wraps `lys_module::revision`.
 */
std::optional<std::string> Module::revision() const
{
    if (!m_module->revision) {
        return std::nullopt;
    }

    return m_module->revision;
}

/**
 * @brief Returns the module namespace
 *
 * Wraps `lys_module::ns`.
 */
std::string Module::ns() const
{
    return m_module->ns;
}

/**
 * @brief Returns the (optional) organization of the module.
 *
 * Wraps `lys_module::org`.
 */
std::optional<std::string> Module::org() const
{
    if (!m_module->org) {
        return std::nullopt;
    }

    return m_module->org;
}

/**
 * @brief Checks whether the module is implemented (or just imported).
 *
 * Wraps `lys_module::implemented`.
 */
bool Module::implemented() const
{
    return m_module->implemented;
}

/**
 * @brief Returns whether feature is enabled. Throws if the feature doesn't exist.
 *
 * Wraps `lys_feature_value`.
 */
bool Module::featureEnabled(const std::string& featureName) const
{
    using namespace std::string_literals;
    auto ret = lys_feature_value(m_module, featureName.c_str());
    switch (ret) {
    case LY_SUCCESS:
        return true;
    case LY_ENOT:
        return false;
    case LY_ENOTFOUND:
        throwError(ret, "Feature '"s + featureName + "' doesn't exist within module '" + name() + "'");
    default:
        throwError(ret, "Error while enabling feature");
    }
}

/**
 * @brief Sets the implemented status of the module and enables no features. Using this on an already implemented module is not
 * an error. In that case it does nothing (doesn't change enabled features).
 *
 * Wraps `lys_set_implemented`.
 */
void Module::setImplemented()
{
    auto err = lys_set_implemented(m_module, nullptr);
    throwIfError(err, "Couldn't set module '" + name() + "' to implemented");
}

/**
 * @brief Sets the implemented status of the module and sets enabled features. Using this on an already implemented module is
 * not an error. In that case it still sets enabled features.
 *
 * @param features std::vector of features to enable. empty vector means no features enabled.
 *
 * Wraps `lys_set_implemented`.
 */
void Module::setImplemented(std::vector<std::string> features)
{
    auto featuresArray = std::make_unique<const char*[]>(features.size() + 1);
    std::transform(features.begin(), features.end(), featuresArray.get(), [](const auto& feature) {
        return feature.c_str();
    });

    auto err = lys_set_implemented(m_module, featuresArray.get());
    throwIfError(err, "Couldn't set module '" + name() + "' to implemented");
}

/**
 * @brief Sets the implemented status of the module and enables all of its features. Using this on an already implemented
 * module is not an error. In that case it still enables all features.
 *
 * Wraps `lys_set_implemented`.
 */
void Module::setImplemented(const AllFeatures)
{
    setImplemented({"*"});
}

/**
 * @brief Returns Feature definitions of this module.
 *
 * Wraps `lysp_module::features`.
 */
std::vector<Feature> Module::features() const
{
    if (!m_module->parsed) {
        throw ParsedInfoUnavailable{"Module::features: lys_module::parsed is not available"};
    }
    std::vector<Feature> res;
    for (const auto& feature : std::span(m_module->parsed->features, LY_ARRAY_COUNT(m_module->parsed->features))) {
        res.emplace_back(Feature{&feature, m_ctx});
    }
    return res;
}

/**
 * @brief Returns Identity definitions of this module.
 *
 * Wraps `lys_module::identities`.
 */
std::vector<Identity> Module::identities() const
{
    std::vector<Identity> res;
    auto span = std::span<lysc_ident>(m_module->identities, LY_ARRAY_COUNT(m_module->identities));
    // Careful, libyang::Identity needs a pointer. The lambda HAS to take by reference, otherwise you get a copy and you
    // can't take the address of that (because it'd be a temporary).
    std::transform(span.begin(), span.end(), std::back_inserter(res), [this] (const lysc_ident& ident) {
        return Identity(&ident, m_ctx);
    });
    return res;
}

/**
 * @brief Returns the first child node of this module.
 * @return The child, or std::nullopt if there are no children.
 */
std::optional<SchemaNode> Module::child() const
{
    if (!m_module->implemented) {
        throw Error{"Module::child: module is not implemented"};
    }

    if (!m_module->compiled->data) {
        return std::nullopt;
    }

    return SchemaNode{m_module->compiled->data, m_ctx};
}

/**
 * @brief Returns a collection of data instantiable top-level nodes of this module.
 *
 * Wraps `lys_getnext`.
 */
ChildInstanstiables Module::childInstantiables() const
{
    if (!m_module->implemented) {
        throw Error{"Module::childInstantiables: module is not implemented"};
    }
    return ChildInstanstiables{nullptr, m_module->compiled, m_ctx};
}

/**
 * @brief Returns a collection for iterating depth-first over the subtree this module points to.
 */
Collection<SchemaNode, IterationType::Dfs> Module::childrenDfs() const
{
    if (!m_module->implemented) {
        throw Error{"Module::childrenDfs: module is not implemented"};
    }
    return Collection<SchemaNode, IterationType::Dfs>{m_module->compiled->data, m_ctx};
}

/**
 * @brief Returns a collection for iterating over the immediate children of where this module points to.
 *
 * This is a convenience function for iterating over this->child().siblings() which does not throw even when module has no children.
 */
Collection<SchemaNode, IterationType::Sibling> Module::immediateChildren() const
{
    auto c = child();
    return c ? c->siblings() : Collection<SchemaNode, IterationType::Sibling>{nullptr, nullptr};
}

/**
 * @brief Returns a collection of RPC nodes (not action nodes) as SchemaNode
 *
 * Wraps `lys_module::compiled::rpc`.
 */
std::vector<SchemaNode> Module::actionRpcs() const
{
    if (!m_module->compiled) {
        throw Error{"Module \"" + this->name() + "\" not implemented"};
    }

    std::vector<SchemaNode> res;
    for (auto node = m_module->compiled->rpcs; node; node = node->next) {
        res.emplace_back(SchemaNode(&node->node, m_ctx));
    }
    return res;
}

std::vector<ExtensionInstance> Module::extensionInstances() const
{
    if (!m_module->compiled) {
        throw Error{"Module \"" + this->name() + "\" not implemented"};
    }

    std::vector<ExtensionInstance> res;
    auto span = std::span<lysc_ext_instance>(m_module->compiled->exts, LY_ARRAY_COUNT(m_module->compiled->exts));
    std::transform(span.begin(), span.end(), std::back_inserter(res), [this] (const lysc_ext_instance& ext) {
        return ExtensionInstance(&ext, m_ctx);
    });
    return res;
}

ExtensionInstance Module::extensionInstance(const std::string& name) const
{
    if (!m_module->compiled) {
        throw Error{"Module \"" + this->name() + "\" not implemented"};
    }

    auto span = std::span<lysc_ext_instance>(m_module->compiled->exts, LY_ARRAY_COUNT(m_module->compiled->exts));
    auto it = std::find_if(span.begin(), span.end(), [name](const auto& ext) {
        return ext.argument == name;
    });
    if (it == span.end()) {
        throw Error{"Extension \""s + name + "\" not defined in module \"" + this->name() + "\""};
    }
    return ExtensionInstance(&*it, m_ctx);
}

/**
 * @brief Print the schema of this module
 *
 * Wraps `lys_print_module`.
 */
std::string Module::printStr(const SchemaOutputFormat format, const std::optional<SchemaPrintFlags> flags, std::optional<size_t> lineLength) const
{
    return printModule(lys_print_module, m_module, format, flags, lineLength, "lys_print_module");
}

bool Module::operator==(const Module& other) const
{
    return m_module == other.m_module;
}

SubmoduleParsed::SubmoduleParsed(const lysp_submodule* submodule, std::shared_ptr<ly_ctx> ctx)
    : m_ctx(ctx)
    , m_submodule(submodule)
{
}

/**
 * @brief Returns the name of the submodule.
 *
 * Wraps `lysp_submodule::name`.
 */
std::string SubmoduleParsed::name() const
{
    return m_submodule->name;
}

/**
 * @brief Returns the parent module of this submodule
 *
 * Wraps `lysp_submodule:mod`
 */
Module SubmoduleParsed::module() const
{
    return Module(m_submodule->mod, m_ctx);
}

/**
 * @brief Print the schema of this submodule
 *
 * Wraps `lys_print_submodule`.
 */
std::string SubmoduleParsed::printStr(const SchemaOutputFormat format, const std::optional<SchemaPrintFlags> flags, std::optional<size_t> lineLength) const
{
    return printModule(lys_print_submodule, m_submodule, format, flags, lineLength, "lys_print_submodule");
}

Feature::Feature(const lysp_feature* feature, std::shared_ptr<ly_ctx> ctx)
    : m_feature(feature)
    , m_ctx(ctx)
{
}

/**
 * @brief Returns the name of the feature.
 *
 * Wraps `lysp_feature::name`.
 */
std::string Feature::name() const
{
    return m_feature->name;
}

/**
 * @brief Is this feature enabled
 *
 * Wraps `lysp_feature::flags` AND-ed with `LYS_FENABLED`.
 */
bool Feature::isEnabled() const
{
    return m_feature->flags & LYS_FENABLED;
}

Identity::Identity(const lysc_ident* ident, std::shared_ptr<ly_ctx> ctx)
    : m_ident(ident)
    , m_ctx(ctx)
{
}

/**
 * @brief Returns the derived identities of this identity non-recursively.
 *
 * Wraps `lysc_ident::dereived`
 */
std::vector<Identity> Identity::derived() const
{
    std::vector<Identity> res;
    for (const auto& it : std::span(m_ident->derived, LY_ARRAY_COUNT(m_ident->derived))) {
        res.emplace_back(Identity{it, m_ctx});
    }

    return res;
}

/**
 * @brief Returns the derived identities of this identity recursively.
 */
std::vector<Identity> Identity::derivedRecursive() const
{
    std::stack<libyang::Identity> stack({*this});
    std::set<libyang::Identity, SomeOrder> visited({*this});

    while (!stack.empty()) {
        auto currentIdentity = std::move(stack.top());
        stack.pop();

        for (const auto& derived : currentIdentity.derived()) {
            if (auto ins = visited.insert(derived); ins.second) {
                stack.push(derived);
            }
        }
    }

    return {visited.begin(), visited.end()};
}

/**
 * @brief Returns the module of the identity.
 *
 * Wraps `lysc_ident::module`
 */
Module Identity::module() const
{
    return Module{m_ident->module, m_ctx};
}

/**
 * @brief Returns the name of the identity.
 *
 * Wraps `lysc_ident::name`
 */
std::string Identity::name() const
{
    return m_ident->name;
}

bool Identity::operator==(const Identity& other) const
{
    return module().name() == other.module().name() && name() == other.name();
}

ExtensionInstance::ExtensionInstance(const lysc_ext_instance* instance, std::shared_ptr<ly_ctx> ctx)
    : m_instance(instance)
    , m_ctx(ctx)
{
}

/**
 * @brief Returns the module of this extension instance.
 *
 * Wraps `lysc_ext_instance::module`.
 */
Module ExtensionInstance::module() const
{
    return Module{m_instance->module, m_ctx};
}

/**
 * @brief Returns the argument name
 *
 * As an example, the RESTCONF RFC defines an extension named "yang-data".
 * This extension is then instantiated at two places by that RFC, under
 * names "yang-errors" and "yang-api".
 *
 * Wraps `lysc_ext_instance::argument`.
 */
std::optional<std::string> ExtensionInstance::argument() const
{
    if (!m_instance->argument) {
        return std::nullopt;
    }
    return m_instance->argument;
}

/**
 * @brief Returns the extension definition
 *
 * Wraps `lysc_ext_instance::name`.
 */
Extension ExtensionInstance::definition() const
{
    return Extension{m_instance->def, m_ctx};
}

/**
 * @brief Returns instances of extensions which are extending this particular extensions instance
 *
 * Wraps `lysc_ext_instance::exts`.
 */
std::vector<ExtensionInstance> ExtensionInstance::extensionInstances() const
{
    std::vector<ExtensionInstance> res;
    for (const auto& ext : std::span(m_instance->exts, LY_ARRAY_COUNT(m_instance->exts))) {
        res.emplace_back(ExtensionInstance{&ext, m_ctx});
    }
    return res;
}

Extension::Extension(const lysc_ext* ext, std::shared_ptr<ly_ctx> ctx)
    : m_ext(ext)
    , m_ctx(ctx)
{
}

/**
 * @brief Returns the module in which this extension was defined
 *
 * An extension that's defined in module A might be instantiated in many places
 * in many modules, and possibly also under many schema nodes.
 *
 * Wraps `lysc_ext::module`.
 */
Module Extension::module() const
{
    return Module{m_ext->module, m_ctx};
}

/**
 * @brief Returns the name of the extension definition
 *
 * Wraps `lysc_ext::name`.
 */
std::string Extension::name() const
{
    return m_ext->name;
}

/**
 * @brief Returns all extension instances which extend this extension definition
 *
 * Wraps `lysc_ext::exts`.
 */
std::vector<ExtensionInstance> Extension::extensionInstances() const
{
    std::vector<ExtensionInstance> res;
    for (const auto& ext : std::span(m_ext->exts, LY_ARRAY_COUNT(m_ext->exts))) {
        res.emplace_back(ExtensionInstance{&ext, m_ctx});
    }
    return res;
}
}
