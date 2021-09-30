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
 * Wraps a ly_ctx pointer without taking ownership of it. Use at own risk.
 */
Context createUnmanagedContext(ly_ctx* ctx)
{
    return Context{ctx};
}

/**
 * @brief Creates a new libyang context.
 * @param searchPath Set the search directory for modules. Pass nullptr if you don't want to specify it.
 * @param options Optional context creation options.
 */
Context::Context(const char* searchPath, const std::optional<ContextOptions> options)
{
    ly_ctx* ctx;
    auto err = ly_ctx_new(searchPath, options ? utils::toContextOptions(*options) : 0, &ctx);
    if (err != LY_SUCCESS) {
        throw ErrorWithCode("Can't create libyang context (" + std::to_string(err) + ")", err);
    }

    m_ctx = std::shared_ptr<ly_ctx>(ctx, ly_ctx_destroy);
}

/**
 * Internal use only. Wraps a ly_ctx pointer without taking ownership of it.
 */
Context::Context(ly_ctx* ctx)
    : m_ctx(ctx, [] (ly_ctx*) {})
{
}

/**
 * @brief Set the search directory for the context.
 * @param searchPath The desired search directory.
 */
void Context::setSearchDir(const char* searchDir) const
{
    auto err = ly_ctx_set_searchdir(m_ctx.get(), searchDir);
    if (err != LY_SUCCESS) {
        throw ErrorWithCode("Can't set search directory (" + std::to_string(err) + ")", err);
    }
}

/**
 * @brief Parses module from a string.
 *
 * @param data String containing the module definition.
 * @param format Format of the module definition.
 */
Module Context::parseModuleMem(const char* data, const SchemaFormat format) const
{
    lys_module* mod;
    auto err = lys_parse_mem(m_ctx.get(), data, utils::toLysInformat(format), &mod);
    if (err != LY_SUCCESS) {
        throw ErrorWithCode("Can't parse module (" + std::to_string(err) + ")", err);
    }

    return Module{mod, m_ctx};
}

/**
 * @brief Parses module from a file.
 *
 * @param data String containing the path to the file.
 * @param format Format of the module definition.
 */
Module Context::parseModulePath(const char* path, const SchemaFormat format) const
{
    lys_module* mod;
    auto err = lys_parse_path(m_ctx.get(), path, utils::toLysInformat(format), &mod);
    if (err != LY_SUCCESS) {
        throw ErrorWithCode("Can't parse module (" + std::to_string(err) + ")", err);
    }

    return Module{mod, m_ctx};
}

/**
 * @brief Parses data from a string into libyang.
 *
 * @param data String containing the input data.
 * @param format Format of the input data.
 */
DataNode Context::parseDataMem(const char* data, const DataFormat format) const
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
DataNode Context::newPath(const char* path, const char* value, const std::optional<CreationOptions> options) const
{
    auto out = impl::newPath(nullptr, m_ctx.get(), std::make_shared<internal_refcount>(m_ctx), path, value, options);

    if (!out) {
        throw std::logic_error("Expected a new node to be created");
    }

    return *out;
}

/**
 * @brief Returns the schema definition of a node specified by `dataPath`.
 *
 * @param dataPath A JSON path of the node to get.
 * @return The found schema node.
 */
SchemaNode Context::findPath(const char* dataPath, const OutputNodes output) const
{
    // TODO: allow output nodes
    auto node = lys_find_path(m_ctx.get(), nullptr, dataPath, output == OutputNodes::Yes ? true : false);

    if (!node) {
        throw Error("Couldn't find schema node: "s + dataPath);
    }

    return SchemaNode{node, m_ctx};
}

/**
 * @brief Retrieves module from the context.
 *
 * @param name Name of the wanted module.
 * @param revision Revision of the wanted module. Can be nullptr if you want a module that has no revision specified.
 */
std::optional<Module> Context::getModule(const char* name, const char* revision) const
{
    auto mod = ly_ctx_get_module(m_ctx.get(), name, revision);

    if (!mod) {
        return std::nullopt;
    }

    return Module{mod, m_ctx};
}

/**
 * @brief Retrieves an implemented module from the context.
 *
 * @param name Name of the wanted module.
 * @return The wanted module or std::nullopt if there is no implemented module with the name.
 */
std::optional<Module> Context::getModuleImplemented(const char* name) const
{
    auto mod = ly_ctx_get_module_implemented(m_ctx.get(), name);

    if (!mod) {
        return std::nullopt;
    }

    return Module{mod, m_ctx};
}

Module Context::loadModule(const char* name, const char* revision, const std::vector<std::string>& features) const
{
    auto featuresArray = std::make_unique<const char*[]>(features.size() + 1);
    std::transform(features.begin(), features.end(), featuresArray.get(), [] (const auto& feature) {
        return feature.c_str();
    });

    auto mod = ly_ctx_load_module(m_ctx.get(), name, revision, featuresArray.get());

    if (!mod) {
        throw Error("Can't load module '"s + name + "'");
    }

    return Module{mod, m_ctx};
}

/**
 * Retrieves a vector of all loaded modules.
 */
std::vector<Module> Context::modules() const
{
    std::vector<Module> res;
    uint32_t index = 0;
    while (auto module = ly_ctx_get_module_iter(m_ctx.get(), &index)) {
        res.emplace_back(Module{module, m_ctx});
    }

    return res;
}

namespace {
void impl_freeModuleData(void* moduleData, void*)
{
    std::free(moduleData);
}

LY_ERR impl_callback(const char* modName,
                     const char* modRev,
                     const char* submodName,
                     const char* submodRev,
                     void* userData,
                     LYS_INFORMAT* format,
                     const char** moduleData,
                     ly_module_imp_data_free_clb* moduleFree)
{
    auto cb = reinterpret_cast<std::function<ModuleCallback>*>(userData);
    auto ret = (*cb)(modName, modRev, submodName, submodRev);
    if (!ret) {
        return LY_ENOT;
    }

    *moduleData = strdup(ret->data.c_str());
    *format = utils::toLysInformat(ret->format);
    *moduleFree = impl_freeModuleData;

    return LY_SUCCESS;
}
}

/**
 * Registers a callback for retrieving missing include and import modules. This method is meant to be used when modules
 * are not locally available. If the callback returns std::nullopt, libyang will continue trying to get module data in
 * the default algorithm.
 */
void Context::registerModuleCallback(std::function<ModuleCallback> callback)
{
    if (!callback) {
        throw std::logic_error("Context::registerModuleCallback: callback is empty.");
    }

    m_moduleCallback = std::move(callback);
    ly_ctx_set_module_imp_clb(m_ctx.get(), impl_callback, &m_moduleCallback);
}
}
