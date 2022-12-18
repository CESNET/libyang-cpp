/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <algorithm>
#include <cassert>
#include <libyang-cpp/Module.hpp>
#include <libyang-cpp/SchemaNode.hpp>
#include <libyang-cpp/Type.hpp>
#include <libyang-cpp/Utils.hpp>
#include <libyang/libyang.h>
#include <span>
#include <stack>
#include "utils/enum.hpp"

namespace libyang {
Type::Type(const lysc_type* type, const lysp_type* typeParsed, std::shared_ptr<ly_ctx> ctx)
    : m_type(type)
    , m_typeParsed(typeParsed)
    , m_ctx(ctx)
{
}

void Type::throwIfParsedUnavailable() const
{
    if (!m_typeParsed) {
        throw libyang::ParsedInfoUnavailable();
    }
}

/**
 * @brief Returns the base type of this Type. This is one of the YANG built-in types.
 *
 * Wraps `lysc_type::basetype`.
 */
LeafBaseType Type::base() const
{
    return utils::toLeafBaseType(m_type->basetype);
}

/**
 * @brief Try to cast this Type to an enumeration definition.
 * @throws Error If not an enumeration.
 */
types::Enumeration Type::asEnum() const
{
    if (base() != LeafBaseType::Enum) {
        throw Error("Type is not an enum");
    }

    return types::Enumeration{m_type, m_typeParsed, m_ctx};
}

/**
 * @brief Try to cast this Type to a bits definition.
 * @throws Error If not bits.
 */
types::Bits Type::asBits() const
{
    if (base() != LeafBaseType::Bits) {
        throw Error("Type is not a bit field");
    }

    return types::Bits{m_type, m_typeParsed, m_ctx};
}

/**
 * @brief Try to cast this Type to an identityref definition.
 * @throws Error If not an identityref.
 */
types::IdentityRef Type::asIdentityRef() const
{
    if (base() != LeafBaseType::IdentityRef) {
        throw Error("Type is not an identityref");
    }

    return types::IdentityRef{m_type, m_typeParsed, m_ctx};
}

/**
 * @brief Try to cast this Type to a leafref definition.
 * @throws Error If not a leafref.
 */
types::LeafRef Type::asLeafRef() const
{
    if (base() != LeafBaseType::Leafref) {
        throw Error("Type is not a leafref");
    }

    return types::LeafRef{m_type, m_typeParsed, m_ctx};
}

/**
 * @brief Try to cast this Type to a union definition.
 * @throws Error If not a union.
 */
types::Union Type::asUnion() const
{
    if (base() != LeafBaseType::Union) {
        throw Error("Type is not a union");
    }

    return types::Union{m_type, m_typeParsed, m_ctx};
}

/**
 * @brief Try to cast this Type to a string definition.
 * @throws Error If not a union.
 */
types::String Type::asString() const
{
    if (base() != LeafBaseType::String) {
        throw Error("Type is not a string");
    }

    return types::String{m_type, m_typeParsed, m_ctx};
}

/**
 * @brief Returns a collection containing the enum definitions.
 *
 * Wraps `lysc_type_enum::enums`.
 */
std::vector<types::Enumeration::Enum> types::Enumeration::items() const
{
    auto enm = reinterpret_cast<const lysc_type_enum*>(m_type);
    std::vector<types::Enumeration::Enum> res;
    for (const auto& it : std::span(enm->enums, LY_ARRAY_COUNT(enm->enums))) {
        res.emplace_back(Enum{.name = it.name, .value = it.value});
    }
    return res;
}

/**
 * @brief Returns the name of the type.
 * This method only works if the associated context was created with the libyang::ContextOptions::SetPrivParsed flag.
 *
 * Wraps `lysp_type::name`.
 */
std::string_view Type::name() const
{
    throwIfParsedUnavailable();

    return m_typeParsed->name;
}

/**
 * @brief Returns the description of the type.
 *
 * This method only works if the associated context was created with the libyang::ContextOptions::SetPrivParsed flag.
 */
std::optional<std::string_view> Type::description() const
{
    throwIfParsedUnavailable();

    auto typedefs = std::span(m_typeParsed->pmod->typedefs, LY_ARRAY_COUNT(m_typeParsed->pmod->typedefs));
    auto it = std::find_if(typedefs.begin(), typedefs.end(), [nameToFind = name()] (const lysp_tpdf& tpdf) { return nameToFind == tpdf.name; } );

    if (it == typedefs.end()) {
        return std::nullopt;
    }

    if (!it->dsc) {
        return std::nullopt;
    }

    return it->dsc;
}

/**
 * @brief Returns a collection containing the individual bit definitions.
 *
 * Wraps `lysc_type_bits::bits`.
 */
std::vector<types::Bits::Bit> types::Bits::items() const
{
    auto enm = reinterpret_cast<const lysc_type_bits*>(m_type);
    std::vector<Bits::Bit> res;
    for (const auto& it : std::span(enm->bits, LY_ARRAY_COUNT(enm->bits))) {
        res.emplace_back(types::Bits::Bit{.name = it.name, .position = it.position});
    }

    return res;
}

/**
 * @brief Returns the base definitions of the identityref.
 *
 * Wraps `lysc_type_identityref::bases`.
 */
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

/**
 * @brief Returns the derived identities of this identity non-recursively.
 *
 * Wraps `lysc_ident::dereived`
 */
std::vector<Identity> Identity::derived() const
{
    std::vector<Identity> res;
    for (const auto& it : std::span(m_ident->derived, LY_ARRAY_COUNT(m_ident->derived))) {
        res.emplace_back(Identity{it, m_ctx});
    }

    return res;
}

/**
 * @brief Returns the derived identities of this identity recursively.
 */
std::vector<Identity> Identity::derivedRecursive() const
{
    std::stack<libyang::Identity> stack({*this});
    std::set<libyang::Identity, SomeOrder> visited({*this});

    while (!stack.empty()) {
        auto currentIdentity = std::move(stack.top());
        stack.pop();

        for (const auto& derived : currentIdentity.derived()) {
            if (auto ins = visited.insert(derived); ins.second) {
                stack.push(derived);
            }
        }
    }

    return {visited.begin(), visited.end()};
}

/**
 * @brief Returns the module of the identity.
 *
 * Wraps `lysc_ident::module`
 */
Module Identity::module() const
{
    return Module{m_ident->module, m_ctx};
}

/**
 * @brief Returns the name of the identity.
 *
 * Wraps `lysc_ident::name`
 */
std::string_view Identity::name() const
{
    return m_ident->name;
}

bool Identity::operator==(const Identity& other) const
{
    return module().name() == other.module().name() && name() == other.name();
}

/**
 * @brief Returns the contents of the `path` statement of the leafref.
 *
 * Careful: this is the raw string that's contained inside the YANG module. It is probably not very usable for other
 * libyang functions (without some sort of a transformation).
 *
 * Wraps `lyxp_get_expr`.
 */
std::string_view types::LeafRef::path() const
{
    auto lref = reinterpret_cast<const lysc_type_leafref*>(m_type);
    return lyxp_get_expr(lref->path);
}

/**
 * @brief Returns the first non-leafref type in the chain of leafrefs.
 *
 * Wraps `lysc_type_leafref::realtype`
 */
Type types::LeafRef::resolvedType() const
{
    auto lref = reinterpret_cast<const lysc_type_leafref*>(m_type);
    return Type{lref->realtype, m_typeParsed, m_ctx};
}

/**
 * @brief Returns the types contained within this union.
 *
 * Wraps `lysc_type_union::types`
 */
std::vector<Type> types::Union::types() const
{
    auto types = reinterpret_cast<const lysc_type_union*>(m_type)->types;
    std::vector<Type> res;
    assert(!m_typeParsed || LY_ARRAY_COUNT(types) == LY_ARRAY_COUNT(m_typeParsed->types));
    for (size_t i = 0; i < LY_ARRAY_COUNT(types); i++) {
        auto typeParsed = m_typeParsed ? &m_typeParsed->types[i] : nullptr;
        res.emplace_back(Type{types[i], typeParsed, m_ctx});
    }

    return res;
}

/**
 * @brief Returns the contents of the `pattern` statement of the a string-based leaf.
 */
std::vector<types::String::Pattern> types::String::patterns() const
{
    throwIfParsedUnavailable();

    auto str = reinterpret_cast<const lysc_type_str*>(m_type);
    std::vector<Pattern> res;
    for (const auto& it : std::span(str->patterns, LY_ARRAY_COUNT(str->patterns))) {
        res.emplace_back(Pattern{
            .pattern = it->expr,
            .isInverted = !!it->inverted,
            .description = it->dsc ? std::optional<std::string>{it->dsc} : std::nullopt,
            .errorAppTag = it->eapptag ? std::optional<std::string>{it->eapptag} : std::nullopt,
            .errorMessage = it->emsg ? std::optional<std::string>{it->emsg} : std::nullopt,
        });
    }
    return res;
}

/**
 * @brief Returns the contents of the `length` statement of the a string-based leaf.
 */
types::Length types::String::length() const
{
    throwIfParsedUnavailable();

    auto str = reinterpret_cast<const lysc_type_str*>(m_type);

    std::vector<Length::Part> parts;
    for (const auto& it : std::span(str->length->parts, LY_ARRAY_COUNT(str->length->parts))) {
        parts.emplace_back(Length::Part{
            .min = it.min_u64,
            .max = it.max_u64,
        });
    }

    return Length{
        .parts = parts,
        .description = str->length->dsc ? std::optional<std::string>{str->length->dsc} : std::nullopt,
        .errorAppTag = str->length->eapptag ? std::optional<std::string>{str->length->eapptag} : std::nullopt,
        .errorMessage = str->length->emsg ? std::optional<std::string>{str->length->emsg} : std::nullopt,
    };
}
}
