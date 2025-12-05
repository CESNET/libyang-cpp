/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#include <cstdint>
#include <iosfwd>
#include <libyang-cpp/export.h>
#include <type_traits>
namespace libyang {
/**
 * Controls whether findPath should consider input or output nodes
 */
enum class InputOutputNodes {
    Input,
    Output,
};

/*
 * Iteration type for Collection and Iterator.
 */
enum class IterationType {
    Dfs,
    Meta,
    Sibling
};

/**
 * Wraps LYS_INFORMAT.
 */
enum class SchemaFormat {
    YANG = 1,
    YIN = 3
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
 * Wraps `lyd_type`.
 */
enum class OperationType : uint32_t {
    DataYang = 0,
    RpcYang,
    NotificationYang,
    ReplyYang,
    RpcNetconf,
    NotificationNetconf,
    ReplyNetconf,
    RpcRestconf,
    NotificationRestconf,
    ReplyRestconf,
};

/**
 * Wraps LYD_PRINT_* flags.
 */
enum class PrintFlags : uint32_t {
    WithDefaultsExplicit = 0x00,
    Siblings = 0x01,
    Shrink = 0x02,
    EmptyContainers = 0x04,
    WithDefaultsTrim = 0x10,
    WithDefaultsAll = 0x20,
    WithDefaultsAllTag = 0x40,
    WithDefaultsImplicitTag = 0x80,
    WithDefaultsMask = 0xF0,
    JsonNoNestedPrefix = 0x100,
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

/**
 * Wraps LY_VECODE.
 */
enum class ValidationErrorCode : uint32_t {
    Success,
    Syntax,
    YangSyntax,
    YinSyntax,
    Reference,
    Xpath,
    Semantics,
    XmlSyntax,
    JsonSyntax,
    Data,
    Other
};

enum class CreationOptions : uint32_t {
    Output = 0x01,
    StoreOnly = 0x02,
    // BinaryLyb = 0x04, TODO
    CanonicalValue = 0x08,
    ClearDefaultFromParents = 0x10,
    Update = 0x20,
    Opaque = 0x40,
    PathWithOpaque = 0x80,
    // LYD_NEW_ANY_USE_VALUE is not relevant
};

/**
 * Wraps LYD_DUP_* flags. Supports operator|.
 */
enum class DuplicationOptions : uint32_t {
    Recursive   = 0x01,
    NoMeta      = 0x02,
    WithParents = 0x04,
    WithFlags   = 0x08,
    NoExt       = 0x10,
    WithPriv    = 0x20,
    NoLyds      = 0x40,
};

enum class NodeType : uint16_t {
    Unknown      = 0x0000,
    Container    = 0x0001,
    Choice       = 0x0002,
    Leaf         = 0x0004,
    Leaflist     = 0x0008,
    List         = 0x0010,
    AnyXML       = 0x0020,
    AnyData      = 0x0060,
    Case         = 0x0080,
    RPC          = 0x0100,
    Action       = 0x0200,
    Notification = 0x0400,
    Uses         = 0x0800,
    Input        = 0x1000,
    Output       = 0x2000,
    Grouping     = 0x4000,
    Augment      = 0x8000,
};

enum class ContextOptions : uint16_t {
    AllImplemented    = 0x01,
    RefImplemented    = 0x02,
    NoYangLibrary     = 0x04,
    DisableSearchDirs = 0x08,
    DisableSearchCwd  = 0x10,
    PreferSearchDirs  = 0x20,
    SetPrivParsed     = 0x40,
    ExplicitCompile   = 0x80,
    EnableImpFeatures = 0x100,
    CompileObsolete   = 0x200,
    LybHashes         = 0x400,
    LeafrefExtended   = 0x800,
    LeafrefLinking    = 0x1000,
    BuiltinPluginsOnly = 0x2000,
    StaticPluginsOnly = 0x4000,
};

/**
 * Wraps LY_DATA_TYPE.
 */
enum class LeafBaseType : uint32_t {
    Unknown = 0,
    Binary,
    Uint8,
    Uint16,
    Uint32,
    Uint64,
    String,
    Bits,
    Bool,
    Dec64,
    Empty,
    Enum,
    IdentityRef,
    InstanceIdentifier,
    Leafref,
    Union,
    Int8,
    Int16,
    Int32,
    Int64
};

/**
 * Wraps LYD_ANYDATA_VALUETYPE.
 */
enum class AnydataValueType : uint32_t {
    DataTree,
    String,
    XML,
    JSON,
};

/**
 * Wraps LY_LO* flags. Supports operator|.
 */
enum class LogOptions : uint32_t {
    NoLog     = 0x00,
    Log       = 0x01,
    Store     = 0x02,
    StoreLast = 0x06,
};

/**
 * Wraps LY_LOG_LEVEL.
 */
enum class LogLevel : uint32_t {
    Error,
    Warning,
    Verbose,
    Debug
};

/**
 * Enum for the YANG `status` statement.
 */
enum class Status {
    Current,
    Deprecated,
    Obsolete
};

/**
 * Enum for the YANG `config` statement.
 */
enum class Config {
    True,
    False
};

/**
 * Wraps LYD_VALIDATE_* flags.
 */
enum class ValidationOptions {
    NoState = 0x0001,
    Present = 0x0002,
    MultiError = 0x0004,
    Operational = 0x0008,
    NoDefaults = 0x0010,
    NotFinal = 0x0020,
};

/**
 * Wraps LYD_PARSE_* flags.
 */
enum class ParseOptions {
    ParseOnly    = 0x010000,
    Strict       = 0x020000,
    Opaque       = 0x040000,
    NoState      = 0x080000,
    LybSkipCtxCheck = 0x100000,
    Ordered      = 0x200000,
    Subtree      = 0x400000, /**< Do not use this one for parsing of data subtrees */
    WhenTrue     = 0x800000,
    NoNew        = 0x1000000,
    StoreOnly    = 0x2010000,
    JsonNull     = 0x4000000,
    JsonStringDataTypes = 0x8000000,
};

/**
 * @brief Wraps LYS_OUT_* schema output format flags
 */
enum class SchemaOutputFormat : unsigned int {
    Unknown = 0,
    Yang = 1,
    CompiledYang = 2,
    Yin = 3,
    Tree = 4,
};

/**
 * @brief Wraps LYS_PRINT_* flags.
 */
enum class SchemaPrintFlags : uint32_t {
    NoSubStatements = 0x10,
    Shrink = 0x02,
};

/**
 * @brief Wraps LYD_COMPARE_* flags.
 */
enum class DataCompare : uint32_t {
    NoOptions = 0x00, /**< Equivalent of a raw C 0 to say "no flags given" in a typesafe manner */
    DistinguishExplicitDefaults = 0x02, /**< LYD_COMPARE_DEFAULTS */
    FullRecursion = 0x01, /**< LYD_COMPARE_FULL_RECURSION*/
    OpaqueAsData = 0x04, /**< LYD_COMPARE_OPAQ */
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

constexpr ContextOptions operator|(const ContextOptions a, const ContextOptions b)
{
    return implEnumBitOr(a, b);
}

constexpr DuplicationOptions operator|(const DuplicationOptions a, const DuplicationOptions b)
{
    return implEnumBitOr(a, b);
}

constexpr LogOptions operator|(const LogOptions a, const LogOptions b)
{
    return implEnumBitOr(a, b);
}

constexpr ValidationOptions operator|(const ValidationOptions a, const ValidationOptions b)
{
    return implEnumBitOr(a, b);
}

constexpr ParseOptions operator|(const ParseOptions a, const ParseOptions b)
{
    return implEnumBitOr(a, b);
}

constexpr SchemaPrintFlags operator|(const SchemaPrintFlags a, const SchemaPrintFlags b)
{
    return implEnumBitOr(a, b);
}

constexpr DataCompare operator|(const DataCompare a, const DataCompare b)
{
    return implEnumBitOr(a, b);
}

LIBYANG_CPP_EXPORT std::ostream& operator<<(std::ostream& os, const NodeType& type);
LIBYANG_CPP_EXPORT std::ostream& operator<<(std::ostream& os, const ErrorCode& err);
LIBYANG_CPP_EXPORT std::ostream& operator<<(std::ostream& os, const ValidationErrorCode& err);
LIBYANG_CPP_EXPORT std::ostream& operator<<(std::ostream& os, const LogLevel& level);
}
