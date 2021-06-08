/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#pragma once
#include <libyang/libyang.h>
#include "Enum.hpp"
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
static_assert(LYD_PRINT_WD_ALL == toPrintFlags(PrintFlags::WdAll));
static_assert(LYD_PRINT_WD_ALL_TAG == toPrintFlags(PrintFlags::WdAllTag));
static_assert(LYD_PRINT_WD_EXPLICIT == toPrintFlags(PrintFlags::WdExplicit));
static_assert(LYD_PRINT_WD_IMPL_TAG == toPrintFlags(PrintFlags::WdImplicitTag));
static_assert(LYD_PRINT_WD_MASK == toPrintFlags(PrintFlags::WdMask));
static_assert(LYD_PRINT_WD_TRIM == toPrintFlags(PrintFlags::WdTrim));
static_assert(LYD_PRINT_WITHSIBLINGS == toPrintFlags(PrintFlags::WithSiblings));
}
