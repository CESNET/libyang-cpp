/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#pragma once
#include <libyang-cpp/Enum.hpp>
#include <libyang/libyang.h>
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

#ifndef _MSC_VER
// MSVC doesn't respect the underlying enum size
static_assert(std::is_same_v<std::underlying_type_t<LY_ERR>, std::underlying_type_t<ErrorCode>>);
#endif

constexpr ValidationErrorCode toValidationErrorCode(const LY_VECODE code)
{
    return static_cast<ValidationErrorCode>(code);
}

#ifndef _MSC_VER
static_assert(std::is_same_v<std::underlying_type_t<LY_VECODE>, std::underlying_type_t<ValidationErrorCode>>);
#endif
static_assert(toValidationErrorCode(LYVE_SUCCESS) == ValidationErrorCode::Success);
static_assert(toValidationErrorCode(LYVE_SYNTAX) == ValidationErrorCode::Syntax);
static_assert(toValidationErrorCode(LYVE_SYNTAX_YANG) == ValidationErrorCode::YangSyntax);
static_assert(toValidationErrorCode(LYVE_SYNTAX_YIN) == ValidationErrorCode::YinSyntax);
static_assert(toValidationErrorCode(LYVE_REFERENCE) == ValidationErrorCode::Reference);
static_assert(toValidationErrorCode(LYVE_XPATH) == ValidationErrorCode::Xpath);
static_assert(toValidationErrorCode(LYVE_SEMANTICS) == ValidationErrorCode::Semantics);
static_assert(toValidationErrorCode(LYVE_SYNTAX_XML) == ValidationErrorCode::XmlSyntax);
static_assert(toValidationErrorCode(LYVE_SYNTAX_JSON) == ValidationErrorCode::JsonSyntax);
static_assert(toValidationErrorCode(LYVE_DATA) == ValidationErrorCode::Data);
static_assert(toValidationErrorCode(LYVE_OTHER) == ValidationErrorCode::Other);

constexpr uint32_t toCreationOptions(const CreationOptions flags)
{
    return static_cast<uint32_t>(flags);
}
static_assert(LYD_NEW_PATH_UPDATE == toCreationOptions(CreationOptions::Update));
static_assert(LYD_NEW_PATH_OUTPUT == toCreationOptions(CreationOptions::Output));
static_assert(LYD_NEW_PATH_OPAQ == toCreationOptions(CreationOptions::Opaque));
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

constexpr LY_LOG_LEVEL toLogLevel(const LogLevel options)
{
    return static_cast<LY_LOG_LEVEL>(options);
}

constexpr LogLevel toLogLevel(const LY_LOG_LEVEL options)
{
    return static_cast<LogLevel>(options);
}

static_assert(toLogLevel(LogLevel::Error) == LY_LLERR);
static_assert(toLogLevel(LogLevel::Warning) == LY_LLWRN);
static_assert(toLogLevel(LogLevel::Verbose) == LY_LLVRB);
static_assert(toLogLevel(LogLevel::Debug) == LY_LLDBG);

static_assert(toLogLevel(LY_LLERR) == LogLevel::Error);
static_assert(toLogLevel(LY_LLWRN) == LogLevel::Warning);
static_assert(toLogLevel(LY_LLVRB) == LogLevel::Verbose);
static_assert(toLogLevel(LY_LLDBG) == LogLevel::Debug);

constexpr LeafBaseType toLeafBaseType(const LY_DATA_TYPE type)
{
    return static_cast<LeafBaseType>(type);
}

#ifndef _MSC_VER
static_assert(std::is_same_v<std::underlying_type_t<LY_DATA_TYPE>, std::underlying_type_t<LeafBaseType>>);
#endif

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

constexpr uint32_t toParseOptions(const ParseOptions opts)
{
    return static_cast<uint32_t>(opts);
}

static_assert(toParseOptions(ParseOptions::ParseOnly) == LYD_PARSE_ONLY);
static_assert(toParseOptions(ParseOptions::Strict) == LYD_PARSE_STRICT);
static_assert(toParseOptions(ParseOptions::Opaque) == LYD_PARSE_OPAQ);
static_assert(toParseOptions(ParseOptions::NoState) == LYD_PARSE_NO_STATE);
static_assert(toParseOptions(ParseOptions::LybModUpdate) == LYD_PARSE_LYB_MOD_UPDATE);
static_assert(toParseOptions(ParseOptions::Ordered) == LYD_PARSE_ORDERED);

constexpr lyd_type toOpType(const OperationType type)
{
    return static_cast<lyd_type>(type);
}

#ifndef _MSC_VER
static_assert(std::is_same_v<std::underlying_type_t<lyd_type>, std::underlying_type_t<OperationType>>);
#endif
static_assert(toOpType(OperationType::DataYang) == LYD_TYPE_DATA_YANG);
static_assert(toOpType(OperationType::RpcYang) == LYD_TYPE_RPC_YANG);
static_assert(toOpType(OperationType::NotificationYang) == LYD_TYPE_NOTIF_YANG);
static_assert(toOpType(OperationType::ReplyYang) == LYD_TYPE_REPLY_YANG);
static_assert(toOpType(OperationType::RpcNetconf) == LYD_TYPE_RPC_NETCONF);
static_assert(toOpType(OperationType::NotificationNetconf) == LYD_TYPE_NOTIF_NETCONF);
static_assert(toOpType(OperationType::ReplyNetconf) == LYD_TYPE_REPLY_NETCONF);

constexpr LYD_ANYDATA_VALUETYPE toAnydataValueType(const AnydataValueType type)
{
    return static_cast<LYD_ANYDATA_VALUETYPE >(type);
}

#ifndef _MSC_VER
static_assert(std::is_same_v<std::underlying_type_t<LYD_ANYDATA_VALUETYPE>, std::underlying_type_t<AnydataValueType>>);
#endif
static_assert(toAnydataValueType(AnydataValueType::DataTree) == LYD_ANYDATA_DATATREE);
static_assert(toAnydataValueType(AnydataValueType::String) == LYD_ANYDATA_STRING);
static_assert(toAnydataValueType(AnydataValueType::XML) == LYD_ANYDATA_XML);
static_assert(toAnydataValueType(AnydataValueType::JSON) == LYD_ANYDATA_JSON);
static_assert(toAnydataValueType(AnydataValueType::LYB) == LYD_ANYDATA_LYB);
}
