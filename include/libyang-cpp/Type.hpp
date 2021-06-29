/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#include <libyang-cpp/Enum.hpp>
#include <memory>

struct ly_ctx;
struct lysc_type;

namespace libyang {
class Leaf;
/**
 * @brief Contains information about leaf's type.
 */
class Type {
public:
    LeafBaseType base() const;
    friend Leaf;
private:
    Type(const lysc_type* type, std::shared_ptr<ly_ctx> ctx);

    const lysc_type* m_type;
    std::shared_ptr<ly_ctx> m_ctx;
};
}
