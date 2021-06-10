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
#include "Enum.hpp"
#include "String.hpp"

struct lyd_node;
namespace libyang {
class Context;

// Internal
struct Empty;

/**
 * @brief Class representing a node in a libyang tree.
 */
class DataNode {
public:
    ~DataNode();

    String printStr(const DataFormat format, const PrintFlags flags) const;
    std::optional<DataNode> findPath(const char* path) const;
    String path() const;

    friend Context;
private:
    DataNode(lyd_node* node);
    DataNode(lyd_node* node, std::shared_ptr<Empty> viewCount);

    lyd_node* m_node;
    std::shared_ptr<Empty> m_viewCount;
};
}
