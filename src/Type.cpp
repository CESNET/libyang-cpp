/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <libyang/libyang.h>
#include <libyang-cpp/Type.hpp>
#include "utils/enum.hpp"

namespace libyang {
Type::Type(const lysc_type* type, std::shared_ptr<ly_ctx> ctx)
    : m_type(type)
    , m_ctx(ctx)
{
}

/**
 * Returns the base type of this Type. This is one of the YANG built-in types.
 */
LeafBaseType Type::base() const
{
    return utils::toLeafBaseType(m_type->basetype);
}
}
