/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#include <libyang-cpp/Enum.hpp>
#include <libyang-cpp/Value.hpp>
#include <libyang-cpp/export.h>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

struct ly_ctx;
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
class Binary;
class Bits;
class Enumeration;
class IdentityRef;
class Numeric;
class LeafRef;
class String;
class Union;
class InstanceIdentifier;

namespace constraints {
using ListSize = uint32_t;
}

/**
 * @brief Contains information about a leaf's type.
 *
 * Wraps `lysc_type`.
 */
class LIBYANG_CPP_EXPORT Type {
public:
    LeafBaseType base() const;

    Enumeration asEnum() const;
    IdentityRef asIdentityRef() const;
    LeafRef asLeafRef() const;
    Binary asBinary() const;
    Bits asBits() const;
    Union asUnion() const;
    String asString() const;
    Numeric asNumeric() const;
    InstanceIdentifier asInstanceIdentifier() const;

    std::string name() const;
    std::optional<std::string> typedefName() const;
    std::optional<std::string> description() const;

    std::string internalPluginId() const;

    friend Leaf;
    friend LeafList;
    friend LeafRef;
    friend Union;
    friend DataNodeTerm;

protected:
    void throwIfParsedUnavailable() const;

    const lysc_type* m_type;
    const lysp_type* m_typeParsed = nullptr;
    std::shared_ptr<ly_ctx> m_ctx;

private:
    Type(const lysc_type* type, const lysp_type* typeParsed, std::shared_ptr<ly_ctx> ctx);
};

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
 * @brief Contains information about the `length` statement, not to be used for `range` statement.
 *
 * Wraps `struct lysc_range`.
 */
struct LIBYANG_CPP_EXPORT Length {
    /**
     * @brief Contains information about pair of possible (min/max) length
     *
     * Wraps `struct lysc_range_part`.
     */
    struct LIBYANG_CPP_EXPORT Part {
        uint64_t min;
        uint64_t max;
    };

    std::vector<Part> parts;
    std::optional<std::string> description;
    std::optional<std::string> errorAppTag;
    std::optional<std::string> errorMessage;
};

/**
 * @brief Contains information about the `leafref` leaf type.
 *
 * Wraps `struct lysc_type_leafref`.
 */
class LIBYANG_CPP_EXPORT LeafRef : public Type {
public:
    friend Type;

    std::string path() const;
    Type resolvedType() const;
    bool requireInstance() const;

private:
    using Type::Type;
};

/**
 * @brief Contains information about the `instance-identifier` leaf type.
 *
 * Wraps `struct lysc_type_instanceid`.
 */
class LIBYANG_CPP_EXPORT InstanceIdentifier : public Type {
public:
    friend Type;

    bool requireInstance() const;

private:
    using Type::Type;
};

/**
 * @brief Contains information about the `binary` leaf type.
 *
 * Wraps `lysc_type_bin`.
 */
class LIBYANG_CPP_EXPORT Binary : public Type {
public:
    friend Type;
    Length length() const;

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
    Length length() const;

    friend Type;

private:
    using Type::Type;
};

/**
 * @brief Info about numeric data types -- {u,}int{8,16,32,64} and decimal64
 *
 * Wraps `lysc_type_number` or `lysc_type_dec`.
 */
class LIBYANG_CPP_EXPORT Numeric : public Type {
public:
    /**
     * @brief Contains information about the `range` statement
     *
     * Wraps `struct lysc_range` for numeric data types.
     * */
    struct LIBYANG_CPP_EXPORT Range {
        using Part = std::pair<Value, Value>;
        std::vector<Part> parts;
        std::optional<std::string> description;
        std::optional<std::string> errorAppTag;
        std::optional<std::string> errorMessage;
    };

    Range range() const;
    uint8_t fractionDigits() const;

    friend Type;
private:
    using Type::Type;
};
}
}
