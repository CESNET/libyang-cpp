/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include <memory>
#include <set>

struct ly_ctx;
namespace libyang {
class DataNode;
class SchemaNode;
template <typename NodeType>
class DfsCollection;
template <typename NodeType>
class DfsIterator;
struct internal_refcount {
    explicit internal_refcount(std::shared_ptr<ly_ctx> ctx, std::set<DataNode*> nodes = {});
    std::set<DataNode*> nodes;
    std::set<DfsCollection<DataNode>*> dataCollections;
    std::set<DfsIterator<DataNode>*> dataIterators;
    std::shared_ptr<ly_ctx> context;
};
}
