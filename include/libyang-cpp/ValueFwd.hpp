/*
 * Copyright (C) 2021-2023 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 * Written by Jan Kundrát <jan.kundrat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include <cstdint>
#include <libyang-cpp/export.h>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace libyang {
struct Empty;
struct Binary;
struct Bit;
struct Enum;
struct IdentityRef;
struct Decimal64;
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
    Decimal64,
    std::vector<Bit>,
    Enum,
    IdentityRef
>;

}
