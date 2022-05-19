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
#include "utils/exception.hpp"

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
}
