/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <libyang-cpp/Module.hpp>
#include <libyang-cpp/SchemaNode.hpp>
#include <libyang-cpp/Type.hpp>
#include <libyang-cpp/Utils.hpp>
#include <libyang/libyang.h>
#include <span>
#include "utils/enum.hpp"

namespace libyang {
Type::Type(const lysc_type* type, std::shared_ptr<ly_ctx> ctx)
    : m_type(type)
    , m_ctx(ctx)
{
}

Type::Type(const lysp_type* type, std::shared_ptr<ly_ctx> ctx)
    : m_type(type->compiled)
    , m_typeParsed(type)
    , m_ctx(ctx)
{
}

void Type::throwIfParsedUnavailable() const
{
    if (!m_typeParsed) {
        throw libyang::Error("Context not created with libyang::ContextOptions::SetPrivParsed");
    }
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

types::Bits Type::asBits() const
{
    if (base() != LeafBaseType::Bits) {
        throw Error("Type is not a bit field");
    }

    return types::Bits{m_type, m_ctx};
}

types::IdentityRef Type::asIdentityRef() const
{
    if (base() != LeafBaseType::IdentityRef) {
        throw Error("Type is not an identityref");
    }

    return types::IdentityRef{m_type, m_ctx};
}

types::LeafRef Type::asLeafRef() const
{
    if (base() != LeafBaseType::Leafref) {
        throw Error("Type is not a leafref");
    }

    return types::LeafRef{m_type, m_ctx};
}

types::Union Type::asUnion() const
{
    if (base() != LeafBaseType::Union) {
        throw Error("Type is not a union");
    }

    return types::Union{m_type, m_ctx};
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

/**
 * Returns the name of the type.
 * This method only works if the associated context was created with the libyang::ContextOptions::SetPrivParsed flag.
 */
std::string_view Type::name() const
{
    throwIfParsedUnavailable();

    return m_typeParsed->name;
}

std::vector<types::Bits::Bit> types::Bits::items() const
{
    auto enm = reinterpret_cast<const lysc_type_bits*>(m_type);
    std::vector<Bits::Bit> res;
    for (const auto& it : std::span(enm->bits, LY_ARRAY_COUNT(enm->bits))) {
        res.emplace_back(types::Bits::Bit{.name = it.name, .position = it.position});
    }

    return res;
}

std::vector<Identity> types::IdentityRef::bases() const
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

Module Identity::module() const
{
    return Module{m_ident->module, m_ctx};
}

std::string_view Identity::name() const
{
    return m_ident->name;
}

std::string_view types::LeafRef::path() const
{
    auto lref = reinterpret_cast<const lysc_type_leafref*>(m_type);
    return lyxp_get_expr(lref->path);
}

/**
 * Returns the first non-leafref type in the chain of leafrefs.
 */
Type types::LeafRef::resolvedType() const
{
    auto lref = reinterpret_cast<const lysc_type_leafref*>(m_type);
    return Type{lref->realtype, m_ctx};
}

std::vector<Type> types::Union::types() const
{
    auto types = reinterpret_cast<const lysc_type_union*>(m_type)->types;
    std::vector<Type> res;
    for (const auto& it : std::span(types, LY_ARRAY_COUNT(types))) {
        res.emplace_back(Type{it, m_ctx});
    }

    return res;
}
}
