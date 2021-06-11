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
#include <libyang-cpp/Value.hpp>

struct lyd_node;
struct ly_ctx;
namespace libyang {
class Context;
class DataNode;

struct internal_empty;

class DataNodeTerm;

namespace impl {
std::optional<DataNode> newPath(lyd_node* node, ly_ctx* parent, std::shared_ptr<internal_empty> viewCount, const char* path, const char* value, const std::optional<CreationOptions> options);
}
/**
 * @brief Class representing a node in a libyang tree.
 */
class DataNode {
public:
    ~DataNode();

    String printStr(const DataFormat format, const PrintFlags flags) const;
    std::optional<DataNode> findPath(const char* path) const;
    String path() const;
    DataNodeTerm asTerm() const;
    std::optional<DataNode> newPath(const char* path, const char* value = nullptr, const std::optional<CreationOptions> options = std::nullopt);

    friend Context;
    friend DataNodeTerm;

    bool operator==(const DataNode& node) const;

    friend std::optional<DataNode> impl::newPath(lyd_node* node, ly_ctx* parent, std::shared_ptr<internal_empty> viewCount, const char* path, const char* value, const std::optional<CreationOptions> options);

protected:
    lyd_node* m_node;
private:
    DataNode(lyd_node* node);
    DataNode(lyd_node* node, std::shared_ptr<internal_empty> viewCount);

    std::shared_ptr<internal_empty> m_viewCount;
};

/**
 * @brief Class representing a term node - leaf or leaf-list.
 */
class DataNodeTerm : DataNode {
public:
    using DataNode::path;
    using DataNode::newPath;

    std::string_view valueStr() const;

    friend DataNode;
    Value value() const;

private:
    using DataNode::DataNode;
};
}
