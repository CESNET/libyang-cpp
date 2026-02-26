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

using namespace std::string_literals;

namespace libyang::impl {
std::optional<DataNode> newPath(lyd_node* node, ly_ctx* ctx, std::shared_ptr<internal_refcount> refs, const std::string& path, const std::optional<std::string>& value, const std::optional<CreationOptions> options)
{
    lyd_node* out;
    auto err = lyd_new_path(node, ctx, path.c_str(), value ? value->c_str() : nullptr, options ? utils::toCreationOptions(*options) : 0, &out);

    throwIfError(err, "Couldn't create a node with path '"s + path + "'");

    if (out) {
        return DataNode{out, refs};
    } else {
        return std::nullopt;
    }
}

CreatedNodes newPath2(lyd_node* node, ly_ctx* ctx, std::shared_ptr<internal_refcount> refs, const std::string& path, const void * const value, const int any_type, const std::optional<CreationOptions> options)
{
    lyd_node* newParent;
    lyd_node* newNode;
    auto err = lyd_new_path2(node, ctx, path.c_str(), value, 0, (const LYD_ANYDATA_VALUETYPE)any_type, options ? utils::toCreationOptions(*options) : 0, &newParent, &newNode);

    throwIfError(err, "Couldn't create a node with path '"s + path + "'");

    return {
        .createdParent = (newParent ? std::optional{DataNode{newParent, refs}} : std::nullopt),
        .createdNode = (newNode ? std::optional{DataNode{newNode, refs}} : std::nullopt),
    };
}

CreatedNodes newPath2(lyd_node* node, ly_ctx* ctx, std::shared_ptr<internal_refcount> refs, const std::string& path, const AnydataBlob& anydata, const std::optional<CreationOptions> options)
{
    LYD_ANYDATA_VALUETYPE valueType;
    const void *value = nullptr;
    if (std::holds_alternative<JSON>(anydata)) {
        valueType = LYD_ANYDATA_JSON;
        value = std::get<JSON>(anydata).content.c_str();
    } else if (std::holds_alternative<XML>(anydata)) {
        valueType = LYD_ANYDATA_XML;
        value = std::get<XML>(anydata).content.c_str();
    } else if (std::holds_alternative<std::string>(anydata)) {
        valueType = LYD_ANYDATA_STRING;
        value = std::get<std::string>(anydata).c_str();
    } else {
        throw std::logic_error{"Unsupported AnydataBlob type for libyang::impl::newPath2"};
    }
    return newPath2(node, ctx, refs, path, value, (const int)valueType, options);
}
}
