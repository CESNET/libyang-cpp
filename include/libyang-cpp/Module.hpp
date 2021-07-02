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

struct ly_ctx;
struct lys_module;

namespace libyang {
class Context;

/**
 * @brief libyang module class.
 */
class Module {
public:
    std::string_view name() const;
    bool featureEnabled(const char* featureName) const;

    friend Context;
private:
    Module(lys_module* module, std::shared_ptr<ly_ctx> ctx);

    std::shared_ptr<ly_ctx> m_ctx;
    lys_module* m_module;
};
}
