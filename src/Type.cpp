/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <libyang/libyang.h>
#include <libyang-cpp/Type.hpp>
#include <span>
#include "libyang-cpp/utils/exception.hpp"
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

types::Enumeration Type::asEnum() const
{
    if (base() != LeafBaseType::Enum) {
        throw Error("Type is not an enum");
    }

    return types::Enumeration{m_type, m_ctx};
}

std::vector<types::Enumeration::Enum> types::Enumeration::items() const
{
    auto enm = reinterpret_cast<const lysc_type_enum*>(m_type);
    std::vector<types::Enumeration::Enum> res;
    for (const auto& it : std::span(enm->enums, LY_ARRAY_COUNT(enm->enums))) {
        auto& resIt = res.emplace_back();
        resIt.m_ctx = m_ctx;
        resIt.name = it.name;
        resIt.value = it.value;
    }
    return res;
}
}
