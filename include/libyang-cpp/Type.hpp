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
#include <variant>
#include <vector>

struct ly_ctx;
struct lysc_ident;
struct lysc_type;

namespace libyang {
class Leaf;
namespace types {
    class Enumeration;
    class IdentityRef;
    class LeafRef;
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
    friend Leaf;

protected:
    const lysc_type* m_type;
    std::shared_ptr<ly_ctx> m_ctx;

private:
    Type(const lysc_type* type, std::shared_ptr<ly_ctx> ctx);

};

class Identity {
public:
    friend types::IdentityRef;
    std::vector<Identity> derived() const;
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
        friend Enumeration;
        std::string_view name;

        int32_t value;
    private:
        std::shared_ptr<ly_ctx> m_ctx;
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

private:
    using Type::Type;
};
}
}
