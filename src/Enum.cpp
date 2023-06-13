/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#include <ostream>
#include <libyang-cpp/Enum.hpp>
#include <libyang/libyang.h>
#include <string>

namespace libyang {
LIBYANG_CPP_EXPORT std::ostream& operator<<(std::ostream& os, const NodeType& type)
{
    switch (type) {
    case NodeType::Action:
        return os << "action";
    case NodeType::AnyData:
        return os << "anydata";
    case NodeType::AnyXML:
        return os << "anyxml";
    case NodeType::Augment:
        return os << "augment";
    case NodeType::Case:
        return os << "case";
    case NodeType::Choice:
        return os << "choice";
    case NodeType::Container:
        return os << "container";
    case NodeType::Grouping:
        return os << "grouping";
    case NodeType::Input:
        return os << "input";
    case NodeType::Leaf:
        return os << "leaf";
    case NodeType::Leaflist:
        return os << "leaflist";
    case NodeType::List:
        return os << "list";
    case NodeType::Notification:
        return os << "notification";
    case NodeType::Output:
        return os << "output";
    case NodeType::RPC:
        return os << "rpc";
    case NodeType::Uses:
        return os << "uses";
    case NodeType::Unknown:
        break;
    }

    return os << "[unknown node type]";
}

#define CHECK_AND_STRINGIFY(CPP_ENUM, C_ENUM) \
    static_assert(static_cast<std::underlying_type_t<decltype(CPP_ENUM)>>(CPP_ENUM) == (C_ENUM)); \
    case CPP_ENUM: \
        return #C_ENUM

std::string stringify(const ErrorCode err)
{
    switch (err) {
    CHECK_AND_STRINGIFY(ErrorCode::Success, LY_SUCCESS);
    CHECK_AND_STRINGIFY(ErrorCode::MemoryFailure, LY_EMEM);
    CHECK_AND_STRINGIFY(ErrorCode::SyscallFail, LY_ESYS);
    CHECK_AND_STRINGIFY(ErrorCode::InvalidValue, LY_EINVAL);
    CHECK_AND_STRINGIFY(ErrorCode::ItemAlreadyExists, LY_EEXIST);
    CHECK_AND_STRINGIFY(ErrorCode::NotFound, LY_ENOTFOUND);
    CHECK_AND_STRINGIFY(ErrorCode::InternalError, LY_EINT);
    CHECK_AND_STRINGIFY(ErrorCode::ValidationFailure, LY_EVALID);
    CHECK_AND_STRINGIFY(ErrorCode::OperationDenied, LY_EDENIED);
    CHECK_AND_STRINGIFY(ErrorCode::OperationIncomplete, LY_EINCOMPLETE);
    CHECK_AND_STRINGIFY(ErrorCode::RecompileRequired, LY_ERECOMPILE);
    CHECK_AND_STRINGIFY(ErrorCode::Negative, LY_ENOT);
    CHECK_AND_STRINGIFY(ErrorCode::Unknown, LY_EOTHER);
    CHECK_AND_STRINGIFY(ErrorCode::PluginError, LY_EPLUGIN);
    }

    return "[unknown error code (" + std::to_string(static_cast<std::underlying_type_t<decltype(err)>>(err)) + ")]";
}

std::string stringify(const ValidationErrorCode err)
{
    switch (err) {
    CHECK_AND_STRINGIFY(ValidationErrorCode::Success, LYVE_SUCCESS);
    CHECK_AND_STRINGIFY(ValidationErrorCode::Syntax, LYVE_SYNTAX);
    CHECK_AND_STRINGIFY(ValidationErrorCode::YangSyntax, LYVE_SYNTAX_YANG);
    CHECK_AND_STRINGIFY(ValidationErrorCode::YinSyntax, LYVE_SYNTAX_YIN);
    CHECK_AND_STRINGIFY(ValidationErrorCode::Reference, LYVE_REFERENCE);
    CHECK_AND_STRINGIFY(ValidationErrorCode::Xpath, LYVE_XPATH);
    CHECK_AND_STRINGIFY(ValidationErrorCode::Semantics, LYVE_SEMANTICS);
    CHECK_AND_STRINGIFY(ValidationErrorCode::XmlSyntax, LYVE_SYNTAX_XML);
    CHECK_AND_STRINGIFY(ValidationErrorCode::JsonSyntax, LYVE_SYNTAX_JSON);
    CHECK_AND_STRINGIFY(ValidationErrorCode::Data, LYVE_DATA);
    CHECK_AND_STRINGIFY(ValidationErrorCode::Other, LYVE_OTHER);
    }

    return "[unknown validation error code (" + std::to_string(static_cast<std::underlying_type_t<decltype(err)>>(err)) + ")]";
}

std::string stringify(const LogLevel level)
{
    switch (level) {
    CHECK_AND_STRINGIFY(LogLevel::Error, LY_LLERR);
    CHECK_AND_STRINGIFY(LogLevel::Warning, LY_LLWRN);
    CHECK_AND_STRINGIFY(LogLevel::Verbose, LY_LLVRB);
    CHECK_AND_STRINGIFY(LogLevel::Debug, LY_LLDBG);
    }

    return "[unknown log level (" + std::to_string(static_cast<std::underlying_type_t<decltype(level)>>(level)) + ")]";
}

LIBYANG_CPP_EXPORT std::ostream& operator<<(std::ostream& os, const ErrorCode& err)
{
    os << stringify(err);
    return os;
}

LIBYANG_CPP_EXPORT std::ostream& operator<<(std::ostream& os, const ValidationErrorCode& err)
{
    os << stringify(err);
    return os;
}

LIBYANG_CPP_EXPORT std::ostream& operator<<(std::ostream& os, const LogLevel& level)
{
    os << stringify(level);
    return os;
}
}
