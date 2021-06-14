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
/**
 * Wraps LYS_INFORMAT.
 */
enum class SchemaFormat {
    Detect = 0,
    Yang = 1,
    Yin = 3
};

/**
 * Wraps LYD_FORMAT.
 */
enum class DataFormat {
    Detect = 0,
    XML,
    JSON
};

/**
 * Wraps LYD_PRINT_* flags.
 */
enum class PrintFlags : uint32_t {
    WithDefaultsExplicit = 0x00,
    WithSiblings = 0x01,
    Shrink = 0x02,
    KeepEmptyCont = 0x04,
    WithDefaultsTrim = 0x10,
    WithDefaultsAll = 0x20,
    WithDefaultsAllTag = 0x40,
    WithDefaultsImplicitTag = 0x80,
    WithDefaultsMask = 0xF0,
};

/**
 * Wraps LY_ERR.
 */
enum class ErrorCode : uint32_t {
    Success,
    MemoryFailure,
    SyscallFail,
    InvalidValue,
    ItemAlreadyExists,
    NotFound,
    InternalError,
    ValidationFailure,
    OperationDenied,
    OperationIncomplete,
    RecompileRequired,
    Negative,
    Unknown,
    PluginError = 128
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
