/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <libyang-cpp/Utils.hpp>
#include "enum.hpp"
#include "exception.hpp"
#include "newPath.hpp"

namespace libyang::impl {
std::optional<DataNode> newPath(lyd_node* node, ly_ctx* ctx, std::shared_ptr<internal_refcount> refs, const char* path, const char* value, const std::optional<CreationOptions> options)
{
    using namespace std::string_literals;
    lyd_node* out;
    auto err = lyd_new_path(node, ctx, path, value, options ? utils::toCreationOptions(*options) : 0, &out);

    throwIfError(err, "Couldn't create a node with path '"s + path + "'");

    if (out) {
        return DataNode{out, refs};
    } else {
        return std::nullopt;
    }
}

CreatedNodes newPath2(lyd_node* node, ly_ctx* ctx, std::shared_ptr<internal_refcount> refs, const char* path, const void* value, const AnydataValueType valueType, const std::optional<CreationOptions> options)
{
    using namespace std::string_literals;

    lyd_node* newParent;
    lyd_node* newNode;
    auto err = lyd_new_path2(node, ctx, path, value, 0, utils::toAnydataValueType(valueType), options ? utils::toCreationOptions(*options) : 0, &newParent, &newNode);

    throwIfError(err, "Couldn't create a node with path '"s + path + "'");

    return {
        .createdParent = (newParent ? std::optional{DataNode{newParent, refs}} : std::nullopt),
        .createdNode = (newNode ? std::optional{DataNode{newNode, refs}} : std::nullopt),
    };
}
}
