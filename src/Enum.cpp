/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#include <ostream>
#include <libyang-cpp/Enum.hpp>

namespace libyang {
std::ostream& operator<<(std::ostream& os, const NodeType& type)
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

std::ostream& operator<<(std::ostream& os, const ErrorCode& err)
{
    switch (err) {
    case ErrorCode::Success:
        return os << "LY_SUCCESS";
    case ErrorCode::MemoryFailure:
        return os << "LY_EMEM";
    case ErrorCode::SyscallFail:
        return os << "LY_ESYS";
    case ErrorCode::InvalidValue:
        return os << "LY_EINVAL";
    case ErrorCode::ItemAlreadyExists:
        return os << "LY_EEXIST";
    case ErrorCode::NotFound:
        return os << "LY_ENOTFOUND";
    case ErrorCode::InternalError:
        return os << "LY_EINT";
    case ErrorCode::ValidationFailure:
        return os << "LY_EVALID";
    case ErrorCode::OperationDenied:
        return os << "LY_EDENIED";
    case ErrorCode::OperationIncomplete:
        return os << "LY_EINCOMPLETE";
    case ErrorCode::RecompileRequired:
        return os << "LY_ERECOMPILE";
    case ErrorCode::Negative:
        return os << "LY_ENOT";
    case ErrorCode::Unknown:
        return os << "LY_OTHER";
    case ErrorCode::PluginError:
        return os << "LY_EPLUGIN";
    }

    return os << "[unknown error code]";
}
}
