/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include "enum.hpp"
#include "libyang-cpp/utils/exception.hpp"
#include "newPath.hpp"

namespace libyang::impl {
std::optional<DataNode> newPath(lyd_node* node, ly_ctx* ctx, std::shared_ptr<internal_refcount> refs, const char* path, const char* value, const std::optional<CreationOptions> options)
{
    using namespace std::string_literals;
    lyd_node* out;
    auto err = lyd_new_path(node, ctx, path, value, options ? utils::toCreationOptions(*options) : 0, &out);

    if (err != LY_SUCCESS) {
        throw ErrorWithCode("Couldn't create a node with path '"s + path + "' (" + std::to_string(err) + ")", err);
    }

    if (out) {
        return DataNode{out, refs};
    } else {
        return std::nullopt;
    }
}
}
