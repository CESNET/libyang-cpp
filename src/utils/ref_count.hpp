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
#include <optional>
#include <set>

struct ly_ctx;
namespace libyang {
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

struct data_refcounting {
    std::set<DataNode*> nodes;
    std::set<Collection<DataNode, IterationType::Dfs>*> dataCollectionsDfs;
    std::set<Collection<DataNode, IterationType::Sibling>*> dataCollectionsSibling;
    std::set<Set<DataNode>*> dataSets;
};

struct internal_refcount {
    explicit internal_refcount(std::shared_ptr<ly_ctx> ctx);
    explicit internal_refcount(std::shared_ptr<ly_ctx> ctx, const unmanaged_tag);
    std::shared_ptr<ly_ctx> context;
    std::optional<data_refcounting> data;
};
}
