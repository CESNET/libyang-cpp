/*
 * Copyright (C) 2021-2023 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Jan Kundrát <jan.kundrat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#include <libyang-cpp/Type.hpp>
#include <libyang-cpp/Value.hpp>

namespace libyang::types {
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
