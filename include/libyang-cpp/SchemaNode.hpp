/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#include <memory>
#include <libyang-cpp/String.hpp>

struct lysc_node;
struct ly_ctx;
namespace libyang {
class Context;
class DataNode;

/**
 * @brief Class representing a schema definition of a node.
 */
class SchemaNode {
public:
    String path() const;

    friend Context;
    friend DataNode;
private:
    SchemaNode(const lysc_node* node, std::shared_ptr<ly_ctx> ctx);

    const lysc_node* m_node;
    std::shared_ptr<ly_ctx> m_ctx;
};
}
