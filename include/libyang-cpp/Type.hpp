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
struct lysc_type;

namespace libyang {
class TypeEnum;
class Leaf;
/**
 * @brief Contains information about leaf's type.
 */
class Type {
public:
    LeafBaseType base() const;

    TypeEnum asEnum() const;
    friend Leaf;

protected:
    const lysc_type* m_type;
    std::shared_ptr<ly_ctx> m_ctx;

private:
    Type(const lysc_type* type, std::shared_ptr<ly_ctx> ctx);

};

class TypeEnum : public Type {
public:
    friend Type;

    struct EnumItem {
        friend TypeEnum;
        std::string_view name;

        int32_t value;
    private:
        std::shared_ptr<ly_ctx> m_ctx;
    };

    std::vector<EnumItem> items() const;

private:
    using Type::Type;
};
}
