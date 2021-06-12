/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#include <cstdint>
#include <type_traits>
namespace libyang {
enum class SchemaFormat {
    Detect = 0,
    Yang = 1,
    Yin = 3
};

enum class DataFormat {
    Detect = 0,
    XML,
    JSON
};

enum class PrintFlags : uint32_t {
    WdExplicit = 0x00,
    WithSiblings = 0x01,
    Shrink = 0x02,
    KeepEmptyCont = 0x04,
    WdTrim = 0x10,
    WdAll = 0x20,
    WdAllTag = 0x40,
    WdImplicitTag = 0x80,
    WdMask = 0xF0,
};

constexpr PrintFlags operator|(const PrintFlags a, const PrintFlags b)
{
    using Type = std::underlying_type_t<PrintFlags>;
    return static_cast<PrintFlags>(static_cast<Type>(a) | static_cast<Type>(b));
}
}
