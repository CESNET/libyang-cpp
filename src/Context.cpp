/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <libyang/libyang.h>
#include <stdexcept>
#include <libyang-cpp/Context.hpp>
#include <libyang-cpp/DataNode.hpp>
#include <libyang-cpp/SchemaNode.hpp>
#include <libyang-cpp/utils/exception.hpp>
#include "utils/enum.hpp"
#include "utils/newPath.hpp"

using namespace std::string_literals;

namespace libyang {
/**
 * @brief Creates a new libyang context.
 */
Context::Context()
{
    ly_ctx* ctx;
    auto err = ly_ctx_new(nullptr, 0, &ctx);
    if (err != LY_SUCCESS) {
        throw ErrorWithCode("Can't create libyang context (" + std::to_string(err) + ")", err);
    }

    m_ctx = std::shared_ptr<ly_ctx>(ctx, ly_ctx_destroy);
}

/**
 * @brief Parses module from a string.
 *
 * @param data String containing the module definition.
 * @param format Format of the module definition.
 */
void Context::parseModuleMem(const char* data, const SchemaFormat format)
{
    // FIXME: Return the module handle that lys_parse_mem gives.
    auto err = lys_parse_mem(m_ctx.get(), data, utils::toLysInformat(format), nullptr);
    if (err != LY_SUCCESS) {
        throw ErrorWithCode("Can't parse module (" + std::to_string(err) + ")", err);
    }
}

/**
 * @brief Parses data from a string into libyang.
 *
 * @param data String containing the input data.
 * @param format Format of the input data.
 */
DataNode Context::parseDataMem(const char* data, const DataFormat format)
{
    lyd_node* tree;
    // TODO: Allow specifying all the arguments.
    auto err = lyd_parse_data_mem(m_ctx.get(), data, utils::toLydFormat(format), 0, LYD_VALIDATE_PRESENT, &tree);
    if (err != LY_SUCCESS) {
        throw ErrorWithCode("Can't parse data (" + std::to_string(err) + ")", err);
    }

    return DataNode{tree, m_ctx};
}

/**
 * @brief Creates a new node with the supplied path, creating a completely new tree.
 *
 * @param path Path of the new node.
 * @param value String representation of the value. Use nullptr for non-leaf nodes and the `empty` type.
 * @param options Options that change the behavior of this method.
 * @return Returns the newly created node.
 */
DataNode Context::newPath(const char* path, const char* value, const std::optional<CreationOptions> options)
{
    auto out = impl::newPath(nullptr, m_ctx.get(), std::make_shared<internal_refcount>(m_ctx), path, value, options);

    if (!out) {
        throw std::logic_error("Expected a new node to be created");
    }

    return *out;
}

/**
 * @brief Returns the definition of the pointed specified by `dataPath`.
 *
 * @param dataPath A JSON path of the node to get.
 * @return The found schema node.
 */
SchemaNode Context::findPath(const char* dataPath)
{
    // TODO: allow output nodes
    auto node = lys_find_path(m_ctx.get(), nullptr, dataPath, false);

    if (!node) {
        throw Error("Couldn't find schema node: "s + dataPath);
    }

    return SchemaNode{node, m_ctx};
}
}
