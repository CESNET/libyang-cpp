/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#include <libyang-cpp/Enum.hpp>
#include <libyang-cpp/export.h>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

struct ly_ctx;
struct lysc_ident;
struct lysc_type;
struct lysp_type;

namespace libyang {
class DataNodeTerm;
class Leaf;
class LeafList;
class Module;
/**
 * @brief Contains representations of `leaf` schema data types.
 *
 * TODO: add examples/tutorials on how to work datatypes in libyang-cpp
 */
namespace types {
class Bits;
class Enumeration;
class IdentityRef;
class LeafRef;
class String;
class Union;
}
/**
 * @brief Contains information about a leaf's type.
 *
 * Wraps `lysc_type`.
 */
class LIBYANG_CPP_EXPORT Type {
public:
    LeafBaseType base() const;

    types::Enumeration asEnum() const;
    types::IdentityRef asIdentityRef() const;
    types::LeafRef asLeafRef() const;
    types::Bits asBits() const;
    types::Union asUnion() const;
    types::String asString() const;

    std::string_view name() const;
    std::optional<std::string_view> description() const;

    friend Leaf;
    friend LeafList;
    friend types::LeafRef;
    friend types::Union;

protected:
    void throwIfParsedUnavailable() const;

    const lysc_type* m_type;
    const lysp_type* m_typeParsed = nullptr;
    std::shared_ptr<ly_ctx> m_ctx;

private:
    Type(const lysc_type* type, const lysp_type* typeParsed, std::shared_ptr<ly_ctx> ctx);
};

/**
 * @brief Contains information about an identity.
 *
 * Wraps `lysc_ident`.
 */
class LIBYANG_CPP_EXPORT Identity {
public:
    friend DataNodeTerm;
    friend Module;
    friend types::IdentityRef;
    std::vector<Identity> derived() const;
    std::vector<Identity> derivedRecursive() const;
    Module module() const;
    std::string_view name() const;

    bool operator==(const Identity& other) const;

private:
    Identity(const lysc_ident* ident, std::shared_ptr<ly_ctx> ctx);

    const lysc_ident* m_ident;
    std::shared_ptr<ly_ctx> m_ctx;
};

namespace types {
/**
 * @brief Contains information about the `enumeration` leaf type.
 *
 * Wraps `lysc_type_enum`.
 */
class LIBYANG_CPP_EXPORT Enumeration : public Type {
public:
    friend Type;

    /**
     * @brief Contains information about an enum from an `enumeration` leaf type.
     *
     * Wraps `lysc_type_bitenum_item`.
     */
    struct Enum {
        auto operator<=>(const Enum& other) const = default;
        std::string name;
        int32_t value;
    };

    std::vector<Enum> items() const;

private:
    using Type::Type;
};

/**
 * @brief Contains information about the `identityref` leaf type.
 *
 * Wraps `lysc_type_identityref`.
 */
class LIBYANG_CPP_EXPORT IdentityRef : public Type {
public:
    friend Type;

    std::vector<Identity> bases() const;

private:
    using Type::Type;
};

/**
 * @brief Contains information about the `leafref` leaf type.
 */
class LIBYANG_CPP_EXPORT LeafRef : public Type {
public:
    friend Type;

    std::string_view path() const;
    Type resolvedType() const;

private:
    using Type::Type;
};

/**
 * @brief Contains information about the `bits` leaf type.
 *
 * Wraps `lysc_type_bits`.
 */
class LIBYANG_CPP_EXPORT Bits : public Type {
public:
    /**
     * @brief Contains information about a specific bit from a `bits` leaf type.
     *
     * Wraps `lysc_type_bitenum_item`.
     */
    struct Bit {
        auto operator<=>(const Bit& other) const = default;
        std::string name;
        uint32_t position;
    };

    std::vector<Bit> items() const;

    friend Type;

private:
    using Type::Type;
};

/**
 * @brief Contains information about the `union` leaf type.
 *
 * Wraps `lysc_type_union`.
 */
class LIBYANG_CPP_EXPORT Union : public Type {
public:
    std::vector<Type> types() const;
    friend Type;

private:
    using Type::Type;
};

/**
 * @brief Contains information about the `string` leaf type.
 *
 * Wraps `lysc_type_str`.
 */
class LIBYANG_CPP_EXPORT String : public Type {
public:

    /**
     * @brief Information about the `pattern` statement
     *
     * Wraps `struct lysc_pattern`.
     */
    struct LIBYANG_CPP_EXPORT Pattern {
        /** @brief The original pattern */
        std::string pattern;
        /** @brief The `invert-match` YANG flag */
        bool isInverted;
        std::optional<std::string> description;
        std::optional<std::string> errorAppTag;
        std::optional<std::string> errorMessage;
    };

    std::vector<Pattern> patterns() const;

private:
    using Type::Type;
};
}
}
