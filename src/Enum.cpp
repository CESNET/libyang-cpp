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
}
