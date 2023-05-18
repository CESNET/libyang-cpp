/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <algorithm>
#include <libyang-cpp/SchemaNode.hpp>
#include <libyang-cpp/Type.hpp>
#include <libyang-cpp/Utils.hpp>
#include <libyang/libyang.h>
#include <span>
#include "utils/enum.hpp"

namespace libyang::types {
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
Enumeration Type::asEnum() const
{
    if (base() != LeafBaseType::Enum) {
        throw Error("Type is not an enum");
    }

    return Enumeration{m_type, m_typeParsed, m_ctx};
}

/**
 * @brief Try to cast this Type to a binary definition.
 * @throws Error If not bits.
 */
Binary Type::asBinary() const
{
    if (base() != LeafBaseType::Binary) {
        throw Error("Type is not a binary");
    }

    return Binary{m_type, m_typeParsed, m_ctx};
}

/**
 * @brief Try to cast this Type to a bits definition.
 * @throws Error If not bits.
 */
Bits Type::asBits() const
{
    if (base() != LeafBaseType::Bits) {
        throw Error("Type is not a bit field");
    }

    return Bits{m_type, m_typeParsed, m_ctx};
}

/**
 * @brief Try to cast this Type to an identityref definition.
 * @throws Error If not an identityref.
 */
IdentityRef Type::asIdentityRef() const
{
    if (base() != LeafBaseType::IdentityRef) {
        throw Error("Type is not an identityref");
    }

    return IdentityRef{m_type, m_typeParsed, m_ctx};
}

/**
 * @brief Try to cast this Type to a leafref definition.
 * @throws Error If not a leafref.
 */
LeafRef Type::asLeafRef() const
{
    if (base() != LeafBaseType::Leafref) {
        throw Error("Type is not a leafref");
    }

    return LeafRef{m_type, m_typeParsed, m_ctx};
}

/**
 * @brief Try to cast this Type to a union definition.
 * @throws Error If not a union.
 */
Union Type::asUnion() const
{
    if (base() != LeafBaseType::Union) {
        throw Error("Type is not a union");
    }

    return Union{m_type, m_typeParsed, m_ctx};
}

/**
 * @brief Try to cast this Type to a string definition.
 * @throws Error If not a string.
 */
String Type::asString() const
{
    if (base() != LeafBaseType::String) {
        throw Error("Type is not a string");
    }

    return String{m_type, m_typeParsed, m_ctx};
}

/**
 * @brief Try to cast this Type to a numerical type definition.
 * @throws Error If not a numeric type.
 */
Numeric Type::asNumeric() const
{
    switch (base()) {
    case LeafBaseType::Int8:
    case LeafBaseType::Int16:
    case LeafBaseType::Int32:
    case LeafBaseType::Int64:
    case LeafBaseType::Uint8:
    case LeafBaseType::Uint16:
    case LeafBaseType::Uint32:
    case LeafBaseType::Uint64:
    case LeafBaseType::Dec64:
        break;
    default:
        throw Error("Type is not a numeric type");
    }

    return Numeric{m_type, m_typeParsed, m_ctx};
}

/**
 * @brief Returns a collection containing the enum definitions.
 *
 * Wraps `lysc_type_enum::enums`.
 */
std::vector<Enumeration::Enum> Enumeration::items() const
{
    auto enm = reinterpret_cast<const lysc_type_enum*>(m_type);
    std::vector<Enumeration::Enum> res;
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
std::vector<Bits::Bit> Bits::items() const
{
    auto enm = reinterpret_cast<const lysc_type_bits*>(m_type);
    std::vector<Bits::Bit> res;
    for (const auto& it : std::span(enm->bits, LY_ARRAY_COUNT(enm->bits))) {
        res.emplace_back(Bits::Bit{.name = it.name, .position = it.position});
    }

    return res;
}

/**
 * @brief Returns the base definitions of the identityref.
 *
 * Wraps `lysc_type_identityref::bases`.
 */
std::vector<Identity> IdentityRef::bases() const
{
    auto ident = reinterpret_cast<const lysc_type_identityref*>(m_type);
    std::vector<Identity> res;
    for (const auto& it : std::span(ident->bases, LY_ARRAY_COUNT(ident->bases))) {
        res.emplace_back(Identity{it, m_ctx});
    }

    return res;
}

/**
 * @brief Returns the contents of the `path` statement of the leafref.
 *
 * Careful: this is the raw string that's contained inside the YANG module. It is probably not very usable for other
 * libyang functions (without some sort of a transformation).
 *
 * Wraps `lyxp_get_expr`.
 */
std::string_view LeafRef::path() const
{
    auto lref = reinterpret_cast<const lysc_type_leafref*>(m_type);
    return lyxp_get_expr(lref->path);
}

/**
 * @brief Returns the first non-leafref type in the chain of leafrefs.
 *
 * Wraps `lysc_type_leafref::realtype`
 */
Type LeafRef::resolvedType() const
{
    auto lref = reinterpret_cast<const lysc_type_leafref*>(m_type);
    return Type{lref->realtype, m_typeParsed, m_ctx};
}

/**
 * @brief Returns the types contained within this union.
 *
 * Wraps `lysc_type_union::types`
 */
std::vector<Type> Union::types() const
{
    auto types = reinterpret_cast<const lysc_type_union*>(m_type)->types;
    std::vector<Type> res;
    if (m_typeParsed && LY_ARRAY_COUNT(types) != LY_ARRAY_COUNT(m_typeParsed->types)) {
        throw std::logic_error("libyang-cpp internal error: mismatch between the number of types in parsed type.");
    }

    for (size_t i = 0; i < LY_ARRAY_COUNT(types); i++) {
        auto typeParsed = m_typeParsed ? &m_typeParsed->types[i] : nullptr;
        res.emplace_back(Type{types[i], typeParsed, m_ctx});
    }

    return res;
}

namespace {
template <typename T> std::optional<std::string> extractDescription(T& struct_ptr)
{
    return struct_ptr->dsc ? std::optional<std::string>{struct_ptr->dsc} : std::nullopt;
}

template <typename T> std::optional<std::string> extractErrAppTag(T& struct_ptr)
{
    return struct_ptr->eapptag ? std::optional<std::string>{struct_ptr->eapptag} : std::nullopt;
}

template <typename T> std::optional<std::string> extractErrMessage(T& struct_ptr)
{
    return struct_ptr->emsg ? std::optional<std::string>{struct_ptr->emsg} : std::nullopt;
}
}

/**
 * @brief Returns the contents of the `length` statement of the a binary-based leaf.
 */
Length Binary::length() const
{
    throwIfParsedUnavailable();

    auto bin = reinterpret_cast<const lysc_type_bin*>(m_type);
    if (!bin->length) {
        return Length{};
    }

    std::vector<Length::Part> parts;
    for (const auto& it : std::span(bin->length->parts, LY_ARRAY_COUNT(bin->length->parts))) {
        parts.emplace_back(Length::Part{
            .min = it.min_u64,
            .max = it.max_u64,
        });
    }

    return Length{
        .parts = parts,
        .description = extractDescription(bin->length),
        .errorAppTag = extractErrAppTag(bin->length),
        .errorMessage = extractErrMessage(bin->length),
    };
}

/**
 * @brief Returns the contents of the `pattern` statement of the a string-based leaf.
 */
std::vector<String::Pattern> String::patterns() const
{
    throwIfParsedUnavailable();

    auto str = reinterpret_cast<const lysc_type_str*>(m_type);
    std::vector<Pattern> res;
    for (const auto& it : std::span(str->patterns, LY_ARRAY_COUNT(str->patterns))) {
        res.emplace_back(Pattern{
            .pattern = it->expr,
            .isInverted = !!it->inverted,
            .description = extractDescription(it),
            .errorAppTag = extractErrAppTag(it),
            .errorMessage = extractErrMessage(it),
        });
    }
    return res;
}

/**
 * @brief Returns the contents of the `length` statement of the a string-based leaf.
 */
Length String::length() const
{
    throwIfParsedUnavailable();

    auto str = reinterpret_cast<const lysc_type_str*>(m_type);
    if (!str->length) {
        return Length{};
    }

    std::vector<Length::Part> parts;
    for (const auto& it : std::span(str->length->parts, LY_ARRAY_COUNT(str->length->parts))) {
        parts.emplace_back(Length::Part{
            .min = it.min_u64,
            .max = it.max_u64,
        });
    }

    return Length{
        .parts = parts,
        .description = extractDescription(str->length),
        .errorAppTag = extractErrAppTag(str->length),
        .errorMessage = extractErrMessage(str->length),
    };
}

Numeric::Range Numeric::range() const
{
    throwIfParsedUnavailable();
    std::vector<Range::Part> parts;
    if (base() == LeafBaseType::Dec64) {
        auto dec = reinterpret_cast<const lysc_type_dec*>(m_type);
        if (!dec->range) {
            return Range{};
        }
        for (const auto& it : std::span(dec->range->parts, LY_ARRAY_COUNT(dec->range->parts))) {
            parts.emplace_back(Decimal64{it.min_64, dec->fraction_digits}, Decimal64{it.max_64, dec->fraction_digits});
        }
        return Range{
            .parts = parts,
            .description = extractDescription(dec->range),
            .errorAppTag = extractErrAppTag(dec->range),
            .errorMessage = extractErrMessage(dec->range),
        };
    } else {
        auto num = reinterpret_cast<const lysc_type_num*>(m_type);
        if (!num->range) {
            return Range{};
        }
        for (const auto& it : std::span(num->range->parts, LY_ARRAY_COUNT(num->range->parts))) {
            Value min, max;
            switch(base()) {
            case LeafBaseType::Int8:
                min = static_cast<int8_t>(it.min_64);
                max = static_cast<int8_t>(it.max_64);
                break;
            case LeafBaseType::Int16:
                min = static_cast<int16_t>(it.min_64);
                max = static_cast<int16_t>(it.max_64);
                break;
            case LeafBaseType::Int32:
                min = static_cast<int32_t>(it.min_64);
                max = static_cast<int32_t>(it.max_64);
                break;
            case LeafBaseType::Int64:
                min = static_cast<int64_t>(it.min_64);
                max = static_cast<int64_t>(it.max_64);
                break;
            case LeafBaseType::Uint8:
                min = static_cast<uint8_t>(it.min_u64);
                max = static_cast<uint8_t>(it.max_u64);
                break;
            case LeafBaseType::Uint16:
                min = static_cast<uint16_t>(it.min_u64);
                max = static_cast<uint16_t>(it.max_u64);
                break;
            case LeafBaseType::Uint32:
                min = static_cast<uint32_t>(it.min_u64);
                max = static_cast<uint32_t>(it.max_u64);
                break;
            case LeafBaseType::Uint64:
                min = static_cast<uint64_t>(it.min_u64);
                max = static_cast<uint64_t>(it.max_u64);
                break;
            default:
                throw std::logic_error{"libyang-cpp internal error: unexpected numeric type"};
            }
            parts.emplace_back(min, max);
        }
        return Range{
            .parts = parts,
            .description = extractDescription(num->range),
            .errorAppTag = extractErrAppTag(num->range),
            .errorMessage = extractErrMessage(num->range),
        };
    }
}

/** @brief For decimal64 types, return the `fraction-digits` statement. For integers, return 0. */
uint8_t Numeric::fractionDigits() const
{
    if (base() == LeafBaseType::Dec64) {
        return reinterpret_cast<const lysc_type_dec*>(m_type)->fraction_digits;
    } else {
        return 0;
    }
}
}
