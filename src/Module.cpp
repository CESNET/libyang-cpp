/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <libyang/libyang.h>
#include <libyang-cpp/utils/exception.hpp>
#include <libyang-cpp/Module.hpp>
#include <span>

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
 * Returns whether feature is enabled. Throws if the feature doesn't exist.
 */
bool Module::featureEnabled(const char* featureName) const
{
    using namespace std::string_literals;
    auto ret = lys_feature_value(m_module, featureName);
    switch (ret) {
    case LY_SUCCESS:
        return true;
    case LY_ENOT:
        return false;
    case LY_ENOTFOUND:
        throw ErrorWithCode("Feature '"s + featureName + "' doesn't exist within module '" + std::string(name()) + "'", ret);
    default:
        throw ErrorWithCode("Error while enabling feature (" + std::to_string(ret) + ")", ret);
    }
}

/**
 * Sets the implemented status of the module and sets enabled features.
 *
 * @param features std::vector of features to enable. empty vector means no features enabled. vector with one string "*"
 * means enable all features. std::nullopt means no change to enabled features.
 */
void Module::setImplemented(const std::optional<std::vector<std::string>>& features)
{
    auto featuresArray = features ? std::make_unique<const char*[]>(features->size() + 1) : nullptr;
    if (featuresArray) {
        std::transform(features->begin(), features->end(), featuresArray.get(), [] (const auto& feature) {
            return feature.c_str();
        });
    }

    auto err = lys_set_implemented(m_module, featuresArray.get());
    if (err != LY_SUCCESS) {
        throw ErrorWithCode("Couldn't set module '" + std::string{name()} + "' (" + std::to_string(err) + ")", err);
    }
}

std::vector<Feature> Module::features() const
{
    std::vector<Feature> res;
    for (const auto& feature : std::span(m_module->parsed->features, LY_ARRAY_COUNT(m_module->parsed->features))) {
        res.emplace_back(Feature{&feature, m_ctx});
    }
    return res;
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
