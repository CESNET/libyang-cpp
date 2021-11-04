/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#include <libyang-cpp/DataNode.hpp>
#include "ref_count.hpp"

struct ly_ctx;
namespace libyang {

/**
 * This struct represents the return value for newPath2.
 */
struct NewPath2Ret {
    /**
     * Contains the first created parent. Will be the same as `newNode` if only one was created.
     */
    std::optional<DataNode> newParent;
    /**
     * Contains the node specified by `path` from the original newPath2 call.
     */
    std::optional<DataNode> newNode;
};

namespace impl {
std::optional<DataNode> newPath(lyd_node* node, ly_ctx* parent, std::shared_ptr<internal_refcount> refs, const char* path, const char* value, const std::optional<CreationOptions> options);
NewPath2Ret newPath2(lyd_node* node, ly_ctx* parent, std::shared_ptr<internal_refcount> refs, const char* path, const void* value, const AnydataValueType valueType, const std::optional<CreationOptions> options);
}
}
