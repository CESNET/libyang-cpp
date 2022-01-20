/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include <libyang-cpp/Enum.hpp>
#include <memory>
#include <set>

struct ly_ctx;
namespace libyang {
/**
 * @brief Internal use only.
 */
struct unmanaged_tag {
};

class DataNode;
template <typename NodeType>
class Set;
class SchemaNode;
template <typename NodeType, IterationType ITER_TYPE>
class Collection;
template <typename NodeType, IterationType ITER_TYPE>
class Iterator;
/**
 * @brief A structure containing info needed for automatic memory management. Internal use only.
 */
struct internal_refcount {
    explicit internal_refcount(std::shared_ptr<ly_ctx> ctx, std::shared_ptr<void> customContext = nullptr);
    std::set<DataNode*> nodes;
    std::set<Collection<DataNode, IterationType::Dfs>*> dataCollectionsDfs;
    std::set<Collection<DataNode, IterationType::Sibling>*> dataCollectionsSibling;
    std::set<Set<DataNode>*> dataSets;
    std::shared_ptr<ly_ctx> context;
    std::shared_ptr<void> customContext;
};
}
