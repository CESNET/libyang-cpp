/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#include <memory>
#include <string_view>
#include <vector>

struct ly_ctx;
struct lys_module;
struct lysp_feature;

namespace libyang {
class Context;
class Module;

class Feature {
public:
    std::string_view name() const;

    friend Module;
private:
    Feature(const lysp_feature* feature, std::shared_ptr<ly_ctx> ctx);

    const lysp_feature* m_feature;
    std::shared_ptr<ly_ctx> m_ctx;
};

/**
 * @brief libyang module class.
 */
class Module {
public:
    std::string_view name() const;
    bool featureEnabled(const char* featureName) const;
    std::vector<Feature> features() const;

    friend Context;
private:
    Module(const lys_module* module, std::shared_ptr<ly_ctx> ctx);

    std::shared_ptr<ly_ctx> m_ctx;
    const lys_module* m_module;
};
}
