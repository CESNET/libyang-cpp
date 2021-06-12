/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#pragma once
#include <libyang/libyang.h>
#include <libyang-cpp/Enum.hpp>
namespace libyang::utils {
constexpr LYS_INFORMAT toLysInformat(const SchemaFormat format)
{
    return static_cast<LYS_INFORMAT>(format);
}
// These tests ensure that I used the right numbers when defining my enum.
static_assert(LYS_INFORMAT::LYS_IN_UNKNOWN == toLysInformat(SchemaFormat::Detect));
static_assert(LYS_INFORMAT::LYS_IN_YANG == toLysInformat(SchemaFormat::Yang));
static_assert(LYS_INFORMAT::LYS_IN_YIN == toLysInformat(SchemaFormat::Yin));

constexpr LYD_FORMAT toLydFormat(const DataFormat format)
{
    return static_cast<LYD_FORMAT>(format);
}
// These tests ensure that I used the right numbers when defining my enum.
static_assert(LYD_FORMAT::LYD_UNKNOWN == toLydFormat(DataFormat::Detect));
static_assert(LYD_FORMAT::LYD_XML == toLydFormat(DataFormat::XML));
static_assert(LYD_FORMAT::LYD_JSON == toLydFormat(DataFormat::JSON));

constexpr uint32_t toPrintFlags(const PrintFlags flags)
{
    return static_cast<uint32_t>(flags);
}
// These tests ensure that I used the right numbers when defining my enum.
// TODO: add asserts for operator|(PrintFlags, PrintFlags)
static_assert(LYD_PRINT_KEEPEMPTYCONT == toPrintFlags(PrintFlags::KeepEmptyCont));
static_assert(LYD_PRINT_SHRINK == toPrintFlags(PrintFlags::Shrink));
static_assert(LYD_PRINT_WD_ALL == toPrintFlags(PrintFlags::WithDefaultsAll));
static_assert(LYD_PRINT_WD_ALL_TAG == toPrintFlags(PrintFlags::WithDefaultsAllTag));
static_assert(LYD_PRINT_WD_EXPLICIT == toPrintFlags(PrintFlags::WithDefaultsExplicit));
static_assert(LYD_PRINT_WD_IMPL_TAG == toPrintFlags(PrintFlags::WithDefaultsImplicitTag));
static_assert(LYD_PRINT_WD_MASK == toPrintFlags(PrintFlags::WithDefaultsMask));
static_assert(LYD_PRINT_WD_TRIM == toPrintFlags(PrintFlags::WithDefaultsTrim));
static_assert(LYD_PRINT_WITHSIBLINGS == toPrintFlags(PrintFlags::WithSiblings));

static_assert(std::is_same_v<std::underlying_type_t<LY_ERR>, std::underlying_type_t<ErrorCode>>);
static_assert(static_cast<std::underlying_type_t<ErrorCode>>(ErrorCode::Success) == LY_SUCCESS);
static_assert(static_cast<std::underlying_type_t<ErrorCode>>(ErrorCode::MemoryFailure) == LY_EMEM);
static_assert(static_cast<std::underlying_type_t<ErrorCode>>(ErrorCode::SyscallFail) == LY_ESYS);
static_assert(static_cast<std::underlying_type_t<ErrorCode>>(ErrorCode::InvalidValue) == LY_EINVAL);
static_assert(static_cast<std::underlying_type_t<ErrorCode>>(ErrorCode::ItemAlreadyExists) == LY_EEXIST);
static_assert(static_cast<std::underlying_type_t<ErrorCode>>(ErrorCode::NotFound) == LY_ENOTFOUND);
static_assert(static_cast<std::underlying_type_t<ErrorCode>>(ErrorCode::InternalError) == LY_EINT);
static_assert(static_cast<std::underlying_type_t<ErrorCode>>(ErrorCode::ValidationFailure) == LY_EVALID);
static_assert(static_cast<std::underlying_type_t<ErrorCode>>(ErrorCode::OperationDenied) == LY_EDENIED);
static_assert(static_cast<std::underlying_type_t<ErrorCode>>(ErrorCode::OperationIncomplete) == LY_EINCOMPLETE);
static_assert(static_cast<std::underlying_type_t<ErrorCode>>(ErrorCode::RecompileRequired) == LY_ERECOMPILE);
static_assert(static_cast<std::underlying_type_t<ErrorCode>>(ErrorCode::Negative) == LY_ENOT);
static_assert(static_cast<std::underlying_type_t<ErrorCode>>(ErrorCode::Unknown) == LY_EOTHER);
static_assert(static_cast<std::underlying_type_t<ErrorCode>>(ErrorCode::PluginError) == LY_EPLUGIN);
}
