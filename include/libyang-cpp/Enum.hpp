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

enum class CreationOptions : uint32_t {
    Update = 0x01,
    Output = 0x02,
    // Opaq = 0x04, TODO
    // BinaryLyb = 0x08, TODO
    CanonicalValue = 0x10
};

template <typename Enum>
constexpr Enum implEnumBitOr(const Enum a, const Enum b)
{
    using Type = std::underlying_type_t<Enum>;
    return static_cast<Enum>(static_cast<Type>(a) | static_cast<Type>(b));
}

constexpr PrintFlags operator|(const PrintFlags a, const PrintFlags b)
{
    return implEnumBitOr(a, b);
}

constexpr CreationOptions operator|(const CreationOptions a, const CreationOptions b)
{
    return implEnumBitOr(a, b);
}
}
