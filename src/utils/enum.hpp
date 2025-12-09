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
static_assert(LYD_PRINT_EMPTY_CONT == toPrintFlags(PrintFlags::EmptyContainers));
static_assert(LYD_PRINT_SHRINK == toPrintFlags(PrintFlags::Shrink));
static_assert(LYD_PRINT_WD_ALL == toPrintFlags(PrintFlags::WithDefaultsAll));
static_assert(LYD_PRINT_WD_ALL_TAG == toPrintFlags(PrintFlags::WithDefaultsAllTag));
static_assert(LYD_PRINT_WD_EXPLICIT == toPrintFlags(PrintFlags::WithDefaultsExplicit));
static_assert(LYD_PRINT_WD_IMPL_TAG == toPrintFlags(PrintFlags::WithDefaultsImplicitTag));
static_assert(LYD_PRINT_WD_MASK == toPrintFlags(PrintFlags::WithDefaultsMask));
static_assert(LYD_PRINT_WD_TRIM == toPrintFlags(PrintFlags::WithDefaultsTrim));
static_assert(LYD_PRINT_SIBLINGS == toPrintFlags(PrintFlags::Siblings));
static_assert(LYD_PRINT_JSON_NO_NESTED_PREFIX == toPrintFlags(PrintFlags::JsonNoNestedPrefix));

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
static_assert(LYD_NEW_VAL_OUTPUT == toCreationOptions(CreationOptions::Output));
static_assert(LYD_NEW_VAL_STORE_ONLY == toCreationOptions(CreationOptions::StoreOnly));
// static_assert(LYD_NEW_PATH_BIN_VALUE == toCreationOptions(CreationOptions::BinaryLyb));
static_assert(LYD_NEW_VAL_CANON == toCreationOptions(CreationOptions::CanonicalValue));
static_assert(LYD_NEW_META_CLEAR_DFLT == toCreationOptions(CreationOptions::ClearDefaultFromParents));
static_assert(LYD_NEW_PATH_UPDATE == toCreationOptions(CreationOptions::Update));
static_assert(LYD_NEW_PATH_OPAQ == toCreationOptions(CreationOptions::Opaque));
static_assert(LYD_NEW_PATH_WITH_OPAQ == toCreationOptions(CreationOptions::PathWithOpaque));

constexpr uint32_t toDuplicationOptions(const DuplicationOptions options)
{
    return static_cast<uint32_t>(options);
}
static_assert(LYD_DUP_NO_META == toDuplicationOptions(DuplicationOptions::NoMeta));
static_assert(LYD_DUP_RECURSIVE == toDuplicationOptions(DuplicationOptions::Recursive));
static_assert(LYD_DUP_WITH_FLAGS == toDuplicationOptions(DuplicationOptions::WithFlags));
static_assert(LYD_DUP_WITH_PARENTS == toDuplicationOptions(DuplicationOptions::WithParents));
static_assert(LYD_DUP_NO_EXT == toDuplicationOptions(DuplicationOptions::NoExt));
static_assert(LYD_DUP_WITH_PRIV == toDuplicationOptions(DuplicationOptions::WithPriv));
static_assert(LYD_DUP_NO_LYDS == toDuplicationOptions(DuplicationOptions::NoLyds));

static_assert((LYD_DUP_NO_META | LYD_DUP_NO_EXT) ==
        toDuplicationOptions(DuplicationOptions::NoMeta | DuplicationOptions::NoExt));

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
static_assert(toContextOptions(ContextOptions::EnableImpFeatures) == LY_CTX_ENABLE_IMP_FEATURES);
static_assert(toContextOptions(ContextOptions::CompileObsolete) == LY_CTX_COMPILE_OBSOLETE);
static_assert(toContextOptions(ContextOptions::LybHashes) == LY_CTX_LYB_HASHES);
static_assert(toContextOptions(ContextOptions::LeafrefExtended) == LY_CTX_LEAFREF_EXTENDED);
static_assert(toContextOptions(ContextOptions::LeafrefLinking) == LY_CTX_LEAFREF_LINKING);
static_assert(toContextOptions(ContextOptions::BuiltinPluginsOnly) == LY_CTX_BUILTIN_PLUGINS_ONLY);
static_assert(toContextOptions(ContextOptions::StaticPluginsOnly) == LY_CTX_STATIC_PLUGINS_ONLY);

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
static_assert(toValidationOptions(ValidationOptions::MultiError) == LYD_VALIDATE_MULTI_ERROR);
static_assert(toValidationOptions(ValidationOptions::Operational) == LYD_VALIDATE_OPERATIONAL);
static_assert(toValidationOptions(ValidationOptions::NoDefaults) == LYD_VALIDATE_NO_DEFAULTS);
static_assert(toValidationOptions(ValidationOptions::NotFinal) == LYD_VALIDATE_NOT_FINAL);

constexpr uint32_t toParseOptions(const ParseOptions opts)
{
    return static_cast<uint32_t>(opts);
}

static_assert(toParseOptions(ParseOptions::ParseOnly) == LYD_PARSE_ONLY);
static_assert(toParseOptions(ParseOptions::Strict) == LYD_PARSE_STRICT);
static_assert(toParseOptions(ParseOptions::Opaque) == LYD_PARSE_OPAQ);
static_assert(toParseOptions(ParseOptions::NoState) == LYD_PARSE_NO_STATE);
static_assert(toParseOptions(ParseOptions::LybSkipCtxCheck) == LYD_PARSE_LYB_SKIP_CTX_CHECK);
static_assert(toParseOptions(ParseOptions::Ordered) == LYD_PARSE_ORDERED);
static_assert(toParseOptions(ParseOptions::Subtree) == LYD_PARSE_SUBTREE);
static_assert(toParseOptions(ParseOptions::WhenTrue) == LYD_PARSE_WHEN_TRUE);
static_assert(toParseOptions(ParseOptions::NoNew) == LYD_PARSE_NO_NEW);
static_assert(toParseOptions(ParseOptions::StoreOnly) == LYD_PARSE_STORE_ONLY);
static_assert(toParseOptions(ParseOptions::JsonNull) == LYD_PARSE_JSON_NULL);
static_assert(toParseOptions(ParseOptions::JsonStringDataTypes) == LYD_PARSE_JSON_STRING_DATATYPES);

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
static_assert(toOpType(OperationType::RpcRestconf) == LYD_TYPE_RPC_RESTCONF);
static_assert(toOpType(OperationType::NotificationRestconf) == LYD_TYPE_NOTIF_RESTCONF);
static_assert(toOpType(OperationType::ReplyRestconf) == LYD_TYPE_REPLY_RESTCONF);

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

constexpr LYS_OUTFORMAT toLysOutFormat(const SchemaOutputFormat format)
{
    return static_cast<LYS_OUTFORMAT>(format);
}

#ifndef _MSC_VER
static_assert(std::is_same_v<std::underlying_type_t<LYS_OUTFORMAT>, std::underlying_type_t<SchemaOutputFormat>>);
#endif
static_assert(toLysOutFormat(SchemaOutputFormat::Unknown) == LYS_OUT_UNKNOWN);
static_assert(toLysOutFormat(SchemaOutputFormat::Yang) == LYS_OUT_YANG);
static_assert(toLysOutFormat(SchemaOutputFormat::CompiledYang) == LYS_OUT_YANG_COMPILED);
static_assert(toLysOutFormat(SchemaOutputFormat::Yin) == LYS_OUT_YIN);
static_assert(toLysOutFormat(SchemaOutputFormat::Tree) == LYS_OUT_TREE);

constexpr uint32_t toSchemaPrintFlags(const SchemaPrintFlags flags)
{
    return static_cast<uint32_t>(flags);
}
static_assert(toSchemaPrintFlags(SchemaPrintFlags::Shrink) == LYS_PRINT_SHRINK);
static_assert(toSchemaPrintFlags(SchemaPrintFlags::NoSubStatements) == LYS_PRINT_NO_SUBSTMT);
static_assert(toSchemaPrintFlags(SchemaPrintFlags::Shrink | SchemaPrintFlags::NoSubStatements) == (LYS_PRINT_SHRINK | LYS_PRINT_NO_SUBSTMT));

constexpr uint32_t toDataCompareOptions(const DataCompare flags)
{
    return static_cast<uint32_t>(flags);
}
static_assert(toDataCompareOptions(DataCompare::DistinguishExplicitDefaults) == LYD_COMPARE_DEFAULTS);
static_assert(toDataCompareOptions(DataCompare::FullRecursion) == LYD_COMPARE_FULL_RECURSION);
static_assert(toDataCompareOptions(DataCompare::OpaqueAsData) == LYD_COMPARE_OPAQ);
static_assert(toDataCompareOptions(DataCompare::FullRecursion | DataCompare::NoOptions) == LYD_COMPARE_FULL_RECURSION);
static_assert(toDataCompareOptions(DataCompare::DistinguishExplicitDefaults | DataCompare::FullRecursion | DataCompare::OpaqueAsData) == (LYD_COMPARE_DEFAULTS | LYD_COMPARE_FULL_RECURSION | LYD_COMPARE_OPAQ));
}
