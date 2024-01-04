/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <algorithm>
#include <libyang-cpp/ChildInstantiables.hpp>
#include <libyang-cpp/Module.hpp>
#include <libyang-cpp/Utils.hpp>
#include <libyang/libyang.h>
#include <span>
#include <stack>
#include "utils/deleters.hpp"
#include "utils/enum.hpp"

using namespace std::string_literals;

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
std::string_view Module::name() const
{
    return m_module->name;
}

/**
 * @brief Returns the (optional) revision of the module.
 *
 * Wraps `lys_module::revision`.
 */
std::optional<std::string_view> Module::revision() const
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
std::string_view Module::ns() const
{
    return m_module->ns;
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
        throwError(ret, "Feature '"s + featureName + "' doesn't exist within module '" + std::string(name()) + "'");
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
    throwIfError(err, "Couldn't set module '" + std::string{name()} + "' to implemented");
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
    throwIfError(err, "Couldn't set module '" + std::string{name()} + "' to implemented");
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

std::vector<Typedef> Module::typedefs() const
{
    std::vector<Typedef> res;
    auto span = std::span<lysp_tpdf>(m_module->parsed->typedefs, LY_ARRAY_COUNT(m_module->parsed->typedefs));
    std::transform(span.begin(), span.end(), std::back_inserter(res), [this](const auto& tpdf) {
        return Typedef{&tpdf, m_ctx};
    });
    return res;
}

Typedef::Typedef(const lysp_tpdf* tpdf, std::shared_ptr<ly_ctx> ctx)
    : m_tpdf(tpdf)
    , m_ctx(ctx)
{
}

std::string Typedef::name() const
{
    return m_tpdf->name;
}

std::optional<std::string> Typedef::description() const
{
    if (!m_tpdf->dsc) {
        return std::nullopt;
    }
    return m_tpdf->dsc;
}

std::optional<std::string> Typedef::reference() const
{
    if (!m_tpdf->ref) {
        return std::nullopt;
    }
    return m_tpdf->ref;
}

std::optional<std::string> Typedef::units() const
{
    if (!m_tpdf->units) {
        return std::nullopt;
    }
    return m_tpdf->units;
}

types::Type Typedef::type() const
{
    return types::Type{m_tpdf->type.compiled, &m_tpdf->type, m_ctx};
}

std::vector<ExtensionInstance> Module::extensionInstances() const
{
    if (!m_module->compiled) {
        throw Error{"Module \"" + std::string{this->name()} + "\" not implemented"};
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
        throw Error{"Module \"" + std::string{this->name()} + "\" not implemented"};
    }

    auto span = std::span<lysc_ext_instance>(m_module->compiled->exts, LY_ARRAY_COUNT(m_module->compiled->exts));
    auto it = std::find_if(span.begin(), span.end(), [name](const auto& ext) {
        return ext.argument == name;
    });
    if (it == span.end()) {
        throw Error{"Extension \""s + name + "\" not defined in module \"" + std::string{this->name()} + "\""};
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
    std::string str;
    auto buf = wrap_ly_out_new_buf(str);
    auto res = lys_print_module(buf.get(), m_module, utils::toLysOutFormat(format), lineLength.value_or(0), flags ? utils::toSchemaPrintFlags(*flags) : 0);
    throwIfError(res, "lys_print_module failed");
    return str;
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
std::string_view Feature::name() const
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
std::string_view Identity::name() const
{
    return m_ident->name;
}

bool Identity::operator==(const Identity& other) const
{
    return module().name() == other.module().name() && name() == other.name();
}

ExtensionInstance::ExtensionInstance(const lysc_ext_instance* ext, std::shared_ptr<ly_ctx> ctx)
    : m_ext(ext)
    , m_ctx(ctx)
{
}

/**
 * @brief Returns the argument
 *
 * Wraps `lysc_ext_instance::argument`.
 */
std::string_view ExtensionInstance::argument() const
{
    return m_ext->argument;
}

/**
 * @brief Returns the extension definition
 *
 * Wraps `lysc_ext_instance::name`.
 */
Extension ExtensionInstance::definition() const
{
    return Extension{m_ext->def, m_ctx};
}

Extension::Extension(const lysc_ext* ext, std::shared_ptr<ly_ctx> ctx)
    : m_ext(ext)
    , m_ctx(ctx)
{
}

/**
 * @brief Returns the name of the module.
 *
 * Wraps `lysc_ext::name`.
 */
std::string_view Extension::name() const
{
    return m_ext->name;
}
}
