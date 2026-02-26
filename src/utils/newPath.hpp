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
namespace libyang::impl {
std::optional<DataNode> newPath(lyd_node* node, ly_ctx* ctx, std::shared_ptr<internal_refcount> refs, const std::string& path, const std::optional<std::string>& value, const std::optional<CreationOptions> options);
CreatedNodes newPath2(lyd_node* node, ly_ctx* ctx, std::shared_ptr<internal_refcount> refs, const std::string& path, const void* value, const AnydataValueType valueType, const std::optional<CreationOptions> options);
}
