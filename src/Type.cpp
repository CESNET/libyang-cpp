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
 * Returns base type.
 */
LeafBaseType Type::base() const
{
    return utils::toLeafBaseType(m_type->basetype);
}

TypeEnum Type::asEnum() const
{
    if (base() != LeafBaseType::Enum) {
        throw Error("Type is not an enum");
    }

    return TypeEnum{m_type, m_ctx};
}

TypeIdentityRef Type::asIdentityRef() const
{
    if (base() != LeafBaseType::IdentityRef) {
        throw Error("Type is not an identityref");
    }

    return TypeIdentityRef{m_type, m_ctx};
}

TypeLeafRef Type::asLeafRef() const
{
    if (base() != LeafBaseType::Leafref) {
        throw Error("Type is not a leafref");
    }

    return TypeLeafRef{m_type, m_ctx};
}

std::vector<TypeEnum::EnumItem> TypeEnum::items() const
{
    auto enm = reinterpret_cast<const lysc_type_enum*>(m_type);
    std::vector<TypeEnum::EnumItem> res;
    for (const auto& it : std::span(enm->enums, LY_ARRAY_COUNT(enm->enums))) {
        auto& resIt = res.emplace_back();
        resIt.m_ctx = m_ctx;
        resIt.name = it.name;
        resIt.value = it.value;
    }
    return res;
}

std::vector<Identity> TypeIdentityRef::bases() const
{
    auto ident = reinterpret_cast<const lysc_type_identityref*>(m_type);
    std::vector<Identity> res;
    for (const auto& it : std::span(ident->bases, LY_ARRAY_COUNT(ident->bases))) {
        res.emplace_back(Identity{it, m_ctx});
    }

    return res;
}

Identity::Identity(const lysc_ident* ident, std::shared_ptr<ly_ctx> ctx)
    : m_ident(ident)
    , m_ctx(ctx)
{
}

std::vector<Identity> Identity::derived() const
{
    std::vector<Identity> res;
    for (const auto& it : std::span(m_ident->derived, LY_ARRAY_COUNT(m_ident->derived))) {
        res.emplace_back(Identity{it, m_ctx});
    }

    return res;
}

std::string_view Identity::name() const
{
    return m_ident->name;
}

std::string_view TypeLeafRef::path() const
{
    auto lref = reinterpret_cast<const lysc_type_leafref*>(m_type);
    return lyxp_get_expr(lref->path);
}
}
