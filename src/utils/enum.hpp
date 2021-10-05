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
static_assert(LYS_INFORMAT::LYS_IN_YANG == toLysInformat(SchemaFormat::YANG));
static_assert(LYS_INFORMAT::LYS_IN_YIN == toLysInformat(SchemaFormat::YIN));

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

constexpr uint32_t toCreationOptions(const CreationOptions flags)
{
    return static_cast<uint32_t>(flags);
}
static_assert(LYD_NEW_PATH_UPDATE == toCreationOptions(CreationOptions::Update));
static_assert(LYD_NEW_PATH_OUTPUT == toCreationOptions(CreationOptions::Output));
// static_assert(LYD_NEW_PATH_OPAQ == toCreationOptions(CreationOptions::Opaq));
// static_assert(LYD_NEW_PATH_BIN_VALUE == toCreationOptions(CreationOptions::BinaryLyb));
static_assert(LYD_NEW_PATH_CANON_VALUE == toCreationOptions(CreationOptions::CanonicalValue));

constexpr uint32_t toDuplicationOptions(const DuplicationOptions options)
{
    return static_cast<uint32_t>(options);
}
static_assert(LYD_DUP_NO_META == toDuplicationOptions(DuplicationOptions::NoMeta));
static_assert(LYD_DUP_RECURSIVE == toDuplicationOptions(DuplicationOptions::Recursive));
static_assert(LYD_DUP_WITH_FLAGS == toDuplicationOptions(DuplicationOptions::WithFlags));
static_assert(LYD_DUP_WITH_PARENTS == toDuplicationOptions(DuplicationOptions::WithParents));

constexpr NodeType toNodeType(const uint16_t type)
{
    return static_cast<NodeType>(type);
}

static_assert(toNodeType(LYS_UNKNOWN) == NodeType::Unknown);
static_assert(toNodeType(LYS_CONTAINER) == NodeType::Container);
static_assert(toNodeType(LYS_CHOICE) == NodeType::Choice);
static_assert(toNodeType(LYS_LEAF) == NodeType::Leaf);
static_assert(toNodeType(LYS_LEAFLIST) == NodeType::Leaflist);
static_assert(toNodeType(LYS_LIST) == NodeType::List);
static_assert(toNodeType(LYS_ANYXML) == NodeType::AnyXML);
static_assert(toNodeType(LYS_ANYDATA) == NodeType::AnyData);
static_assert(toNodeType(LYS_CASE) == NodeType::Case);
static_assert(toNodeType(LYS_RPC) == NodeType::RPC);
static_assert(toNodeType(LYS_ACTION) == NodeType::Action);
static_assert(toNodeType(LYS_NOTIF) == NodeType::Notification);
static_assert(toNodeType(LYS_USES) == NodeType::Uses);
static_assert(toNodeType(LYS_INPUT) == NodeType::Input);
static_assert(toNodeType(LYS_OUTPUT) == NodeType::Output);
static_assert(toNodeType(LYS_GROUPING) == NodeType::Grouping);
static_assert(toNodeType(LYS_AUGMENT) == NodeType::Augment);

constexpr uint32_t toContextOptions(const ContextOptions flags)
{
    return static_cast<uint32_t>(flags);
}

static_assert(toContextOptions(ContextOptions::AllImplemented) == LY_CTX_ALL_IMPLEMENTED);
static_assert(toContextOptions(ContextOptions::RefImplemented) == LY_CTX_REF_IMPLEMENTED);
static_assert(toContextOptions(ContextOptions::NoYangLibrary) == LY_CTX_NO_YANGLIBRARY);
static_assert(toContextOptions(ContextOptions::DisableSearchDirs) == LY_CTX_DISABLE_SEARCHDIRS);
static_assert(toContextOptions(ContextOptions::DisableSearchCwd) == LY_CTX_DISABLE_SEARCHDIR_CWD);
static_assert(toContextOptions(ContextOptions::PreferSearchDirs) == LY_CTX_PREFER_SEARCHDIRS);
static_assert(toContextOptions(ContextOptions::SetPrivParsed) == LY_CTX_SET_PRIV_PARSED);
static_assert(toContextOptions(ContextOptions::ExplicitCompile) == LY_CTX_EXPLICIT_COMPILE);

constexpr uint16_t toLogOptions(const LogOptions options)
{
    return static_cast<uint16_t>(options);
}

static_assert(toLogOptions(LogOptions::Log) == LY_LOLOG);
static_assert(toLogOptions(LogOptions::Store) == LY_LOSTORE);
static_assert(toLogOptions(LogOptions::StoreLast) == LY_LOSTORE_LAST);

constexpr LeafBaseType toLeafBaseType(const LY_DATA_TYPE type)
{
    return static_cast<LeafBaseType>(type);
}

static_assert(std::is_same_v<std::underlying_type_t<LY_DATA_TYPE>, std::underlying_type_t<LeafBaseType>>);

static_assert(toLeafBaseType(LY_DATA_TYPE::LY_TYPE_UNKNOWN) == LeafBaseType::Unknown);
static_assert(toLeafBaseType(LY_DATA_TYPE::LY_TYPE_BINARY) == LeafBaseType::Binary);
static_assert(toLeafBaseType(LY_DATA_TYPE::LY_TYPE_UINT8) == LeafBaseType::Uint8);
static_assert(toLeafBaseType(LY_DATA_TYPE::LY_TYPE_UINT16) == LeafBaseType::Uint16);
static_assert(toLeafBaseType(LY_DATA_TYPE::LY_TYPE_UINT32) == LeafBaseType::Uint32);
static_assert(toLeafBaseType(LY_DATA_TYPE::LY_TYPE_UINT64) == LeafBaseType::Uint64);
static_assert(toLeafBaseType(LY_DATA_TYPE::LY_TYPE_STRING) == LeafBaseType::String);
static_assert(toLeafBaseType(LY_DATA_TYPE::LY_TYPE_BITS) == LeafBaseType::Bits);
static_assert(toLeafBaseType(LY_DATA_TYPE::LY_TYPE_BOOL) == LeafBaseType::Bool);
static_assert(toLeafBaseType(LY_DATA_TYPE::LY_TYPE_DEC64) == LeafBaseType::Dec64);
static_assert(toLeafBaseType(LY_DATA_TYPE::LY_TYPE_EMPTY) == LeafBaseType::Empty);
static_assert(toLeafBaseType(LY_DATA_TYPE::LY_TYPE_ENUM) == LeafBaseType::Enum);
static_assert(toLeafBaseType(LY_DATA_TYPE::LY_TYPE_IDENT) == LeafBaseType::IdentityRef);
static_assert(toLeafBaseType(LY_DATA_TYPE::LY_TYPE_INST) == LeafBaseType::InstanceIdentifier);
static_assert(toLeafBaseType(LY_DATA_TYPE::LY_TYPE_LEAFREF) == LeafBaseType::Leafref);
static_assert(toLeafBaseType(LY_DATA_TYPE::LY_TYPE_UNION) == LeafBaseType::Union);
static_assert(toLeafBaseType(LY_DATA_TYPE::LY_TYPE_INT8) == LeafBaseType::Int8);
static_assert(toLeafBaseType(LY_DATA_TYPE::LY_TYPE_INT16) == LeafBaseType::Int16);
static_assert(toLeafBaseType(LY_DATA_TYPE::LY_TYPE_INT32) == LeafBaseType::Int32);
static_assert(toLeafBaseType(LY_DATA_TYPE::LY_TYPE_INT64) == LeafBaseType::Int64);

constexpr uint32_t toValidationOptions(const ValidationOptions opts)
{
    return static_cast<uint32_t>(opts);
}

static_assert(toValidationOptions(ValidationOptions::NoState) == LYD_VALIDATE_NO_STATE);
static_assert(toValidationOptions(ValidationOptions::Present) == LYD_VALIDATE_PRESENT);
}
