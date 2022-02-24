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
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

struct ly_ctx;
struct lysc_ident;
struct lysc_type;
struct lysp_type;

namespace libyang {
class Leaf;
class LeafList;
class Module;
namespace types {
class Bits;
class Enumeration;
class IdentityRef;
class LeafRef;
class Union;
}
/**
 * @brief Contains information about leaf's type.
 */
class Type {
public:
    LeafBaseType base() const;

    types::Enumeration asEnum() const;
    types::IdentityRef asIdentityRef() const;
    types::LeafRef asLeafRef() const;
    types::Bits asBits() const;
    types::Union asUnion() const;

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

class Identity {
public:
    friend types::IdentityRef;
    std::vector<Identity> derived() const;
    Module module() const;
    std::string_view name() const;

private:
    Identity(const lysc_ident* ident, std::shared_ptr<ly_ctx> ctx);

    const lysc_ident* m_ident;
    std::shared_ptr<ly_ctx> m_ctx;
};

namespace types {
class Enumeration : public Type {
public:
    friend Type;

    struct Enum {
        auto operator<=>(const Enum& other) const = default;
        std::string name;
        int32_t value;
    };

    std::vector<Enum> items() const;

private:
    using Type::Type;
};

class IdentityRef : public Type {
public:
    friend Type;

    std::vector<Identity> bases() const;

private:
    using Type::Type;
};

class LeafRef : public Type {
public:
    friend Type;

    std::string_view path() const;
    Type resolvedType() const;

private:
    using Type::Type;
};

class Bits : public Type {
public:
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

class Union : public Type {
public:
    std::vector<Type> types() const;
    friend Type;

private:
    using Type::Type;
};
}
}
