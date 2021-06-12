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

struct internal_empty;

class DataNodeTerm;
/**
 * @brief Class representing a node in a libyang tree.
 */
class DataNode {
public:
    DataNode(lyd_node* node);
    DataNode(lyd_node* node, std::shared_ptr<internal_empty> viewCount);
    ~DataNode();

    String printStr(const DataFormat format, const PrintFlags flags) const;
    std::optional<DataNode> findPath(const char* path) const;
    String path() const;
    DataNodeTerm asTerm() const;
    std::optional<DataNode> newPath(const char* path, const char* value = nullptr, const std::optional<CreationOptions> options = std::nullopt);

protected:
    lyd_node* m_node;
private:
    std::shared_ptr<internal_empty> m_viewCount;
};

/**
 * @brief Class representing a term node - leaf or leaf-list.
 */
class DataNodeTerm : DataNode {
public:
    using DataNode::DataNode;
    using DataNode::path;
    using DataNode::newPath;

    std::string_view valueStr() const;
private:
};
}
