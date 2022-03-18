/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

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
 * Returns the name of the module.
 */
std::string_view Module::name() const
{
    return m_module->name;
}

/**
 * Returns the (optional) revision of the module.
 */
std::optional<std::string_view> Module::revision() const
{
    if (!m_module->revision) {
        return std::nullopt;
    }

    return m_module->revision;
}

/**
 * Checks whether the module is implemented (or just imported).
 */
bool Module::implemented() const
{
    return m_module->implemented;
}

/**
 * Returns whether feature is enabled. Throws if the feature doesn't exist.
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
 * Sets the implemented status of the module and enables no features. Using this on an already implemented module is not
 * an error. In that case it does nothing (doesn't change enabled features).
 */
void Module::setImplemented()
{
    auto err = lys_set_implemented(m_module, nullptr);
    throwIfError(err, "Couldn't set module '" + std::string{name()} + "' to implemented");
}

/**
 * Sets the implemented status of the module and sets enabled features. Using this on an already implemented module is
 * not an error. In that case it still sets enabled features.
 *
 * @param features std::vector of features to enable. empty vector means no features enabled.
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
 * Sets the implemented status of the module and enables all of its features. Using this on an already implemented
 * module is not an error. In that case it still enables all features.
 */
void Module::setImplemented(const AllFeatures)
{
    setImplemented({"*"});
}

std::vector<Feature> Module::features() const
{
    std::vector<Feature> res;
    for (const auto& feature : std::span(m_module->parsed->features, LY_ARRAY_COUNT(m_module->parsed->features))) {
        res.emplace_back(Feature{&feature, m_ctx});
    }
    return res;
}

/**
 * Returns a collection of data instantiable top-level nodes of this module.
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

std::string_view Feature::name() const
{
    return m_feature->name;
}
}
