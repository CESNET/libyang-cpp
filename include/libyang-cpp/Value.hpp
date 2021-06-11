/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace libyang {
/**
 * @brief Represents a YANG value of type `empty`.
 */
struct Empty {
    auto operator<=>(const Empty&) const = default;
};

/**
 * Represents a YANG value of type `binary` as raw bytes and as a base64 string.
 */
struct Binary {
    auto operator<=>(const Binary&) const = default;
    std::vector<uint8_t> data;
    std::string base64;
};

/**
 * Represents a YANG value of type `binary` as raw bytes and as a base64 string.
 */
struct Decimal64 {
    auto operator<=>(const Decimal64&) const = default;
    int64_t value;
    uint8_t fractionDigits;
};

class DataNode;

/**
 * Represents a value of DataNodeTerm.
 */
using Value = std::variant<
    int8_t,
    int16_t,
    int32_t,
    int64_t,
    uint8_t,
    uint16_t,
    uint32_t,
    uint64_t,
    bool,
    Empty,
    Binary,
    std::string,
    std::optional<DataNode>, // Instance identifier value.
    Decimal64
>;
}
