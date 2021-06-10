/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include <cstdlib>
#include <memory>
#include <optional>
#include <libyang-cpp/Enum.hpp>
#include <libyang-cpp/String.hpp>

struct lyd_node;
namespace libyang {
class Context;

// Internal
struct Empty;

class DataNodeTerm;
/**
 * @brief Class representing a node in a libyang tree.
 */
class DataNode {
public:
    ~DataNode();

    String printStr(const DataFormat format, const PrintFlags flags) const;
    std::optional<DataNode> findPath(const char* path) const;
    String path() const;
    DataNodeTerm asTerm();

    friend Context;

protected:
    lyd_node* m_node;
private:
    DataNode(lyd_node* node);
    DataNode(lyd_node* node, std::shared_ptr<Empty> viewCount);

    std::shared_ptr<Empty> m_viewCount;
};

/**
 * @brief Class representing a term node - leaf or leaf-list.
 */
class DataNodeTerm : DataNode {
public:
    using DataNode::path;

    std::string_view valueStr() const;

    friend DataNode;
private:
    using DataNode::DataNode;
};
}
