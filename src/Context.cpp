/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <libyang-cpp/Context.hpp>
#include <libyang-cpp/DataNode.hpp>
#include <libyang-cpp/SchemaNode.hpp>
#include <libyang-cpp/Utils.hpp>
#include <libyang/libyang.h>
#include <span>
#include <stdexcept>
#include "utils/enum.hpp"
#include "utils/newPath.hpp"
#include "utils/exception.hpp"

using namespace std::string_literals;

namespace libyang {
/**
 * Wraps a ly_ctx pointer with specifying an optional custom deleter. The pointer is not managed further by
 * libyang-cpp's automatic memory management. Use at own risk.
 */
Context createUnmanagedContext(ly_ctx* ctx, ContextDeleter deleter)
{
    return Context{ctx, deleter};
}

ly_ctx* retrieveContext(Context ctx)
{
    return ctx.m_ctx.get();
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
    throwIfError(err, "Can't create libyang context");

    m_ctx = std::shared_ptr<ly_ctx>(ctx, ly_ctx_destroy);
}

/**
 * Internal use only. Wraps a ly_ctx pointer without taking ownership of it, while specifying a custom deleter. The
 * pointer is not managed further by libyang-cpp's automatic memory management.
 */
Context::Context(ly_ctx* ctx, ContextDeleter deleter)
    : m_ctx(ctx, deleter ? deleter : ContextDeleter([] (ly_ctx*) {}))
{
}

/**
 * @brief Set the search directory for the context.
 * @param searchPath The desired search directory.
 */
void Context::setSearchDir(const char* searchDir) const
{
    auto err = ly_ctx_set_searchdir(m_ctx.get(), searchDir);
    throwIfError(err, "Can't set search directory");
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
    throwIfError(err, "Can't parse module");

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
    throwIfError(err, "Can't parse module");

    return Module{mod, m_ctx};
}

/**
 * @brief Parses data from a string into libyang.
 *
 * @param data String containing the input data.
 * @param format Format of the input data.
 */
std::optional<DataNode> Context::parseDataMem(
        const char* data,
        const DataFormat format,
        const std::optional<ParseOptions> parseOpts,
        const std::optional<ValidationOptions> validationOpts) const
{
    lyd_node* tree;
    auto err = lyd_parse_data_mem(
            m_ctx.get(),
            data,
            utils::toLydFormat(format),
            parseOpts ? utils::toParseOptions(*parseOpts) : 0,
            validationOpts ? utils::toValidationOptions(*validationOpts) : 0,
            &tree);
    throwIfError(err, "Can't parse data");


    if (!tree) {
        return std::nullopt;
    }

    return DataNode{tree, m_ctx};
}

/**
 * @brief Parses data from a string into libyang.
 *
 * @param data String containing the input data.
 * @param format Format of the input data.
 */
std::optional<DataNode> Context::parseDataPath(
        const std::filesystem::path& path,
        const DataFormat format,
        const std::optional<ParseOptions> parseOpts,
        const std::optional<ValidationOptions> validationOpts) const
{
    lyd_node* tree;
    ly_log_level(LY_LLDBG);
    auto err = lyd_parse_data_path(
            m_ctx.get(),
            path.c_str(),
            utils::toLydFormat(format),
            parseOpts ? utils::toParseOptions(*parseOpts) : 0,
            validationOpts ? utils::toValidationOptions(*validationOpts) : 0,
            &tree);
    throwIfError(err, "Can't parse data");

    if (!tree) {
        return std::nullopt;
    }

    return DataNode{tree, m_ctx};
}

/*
 * Parses YANG data into an operation data tree.
 *
 * Currently only supports OperationType::RpcNetconf. To parse a NETCONF RPC, firstly use this method supplying the RPC
 * as the `input` argument. After that, to parse a NETCONF RPC reply, use DataNode::parseOp on the `ParsedOp::op` field
 * with OperationType::ReplyNetconf. Note: to parse a NETCONF RPC reply, you MUST parse the original NETCONF RPC request
 * (that is, you have to use this method with OperationType::RpcNetconf).
 */
ParsedOp Context::parseOp(const char* input, const DataFormat format, const OperationType opType) const
{
    ly_in* in;
    ly_in_new_memory(input, &in);
    auto deleteFunc = [](auto* in) {
        ly_in_free(in, false);
    };
    auto deleter = std::unique_ptr<ly_in, decltype(deleteFunc)>(in, deleteFunc);

    lyd_node* op = nullptr;
    lyd_node* tree = nullptr;

    switch (opType) {
    case OperationType::RpcNetconf:
        lyd_parse_op(m_ctx.get(), nullptr, in, utils::toLydFormat(format), utils::toOpType(opType), &tree, &op);
        break;
    case OperationType::ReplyNetconf:
        throw Error("To parse a NETCONF reply, use DataNode::parseOp on a parsed NETCONF RPC");
    default:
        throw Error("Context::parseOp: unsupported op");
    }

    return {
        .tree = tree ? std::optional{libyang::wrapRawNode(tree)} : std::nullopt,
        .op = op ? std::optional{libyang::wrapRawNode(op)} : std::nullopt
    };
}

/**
 * @brief Creates a new node with the supplied path, creating a completely new tree.
 *
 * @param path Path of the new node.
 * @param value String representation of the value. Use nullptr for non-leaf nodes and the `empty` type.
 * @param options Options that change the behavior of this method.
 * @return Returns the first created parent.
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
 * @brief Creates a new node with the supplied path, creating a completely new tree.
 *
 * @param path Path of the new node.
 * @param value String representation of the value. Use nullptr for non-leaf nodes and the `empty` type.
 * @param options Options that change the behavior of this method.
 * @return Returns the first created parent and also the node specified by `path`. These might be the same node.
 */
CreatedNodes Context::newPath2(const char* path, const char* value, const std::optional<CreationOptions> options) const
{
    // The AnydataValueType here doesn't matter, because this overload creates a classic node and not an `anydata` node.
    // TODO: Make overloads for all of the AnydataValueType values.
    auto out = impl::newPath2(nullptr, m_ctx.get(), std::make_shared<internal_refcount>(m_ctx), path, value, AnydataValueType::String, options);

    if (!out.createdNode) {
        throw std::logic_error("Expected a new node to be created");
    }

    return out;
}

/**
 * @brief Creates a new anyxml node with the supplied path, creating a completely new tree.
 *
 * @param path Path of the new node.
 * @param json JSON value.
 * @param options Options that change the behavior of this method.
 * @return Returns the first created parent and also the node specified by `path`. These might be the same node.
 */
CreatedNodes Context::newPath2(const char* path, libyang::XML xml, const std::optional<CreationOptions> options) const
{
    auto out = impl::newPath2(nullptr, m_ctx.get(), std::make_shared<internal_refcount>(m_ctx), path, xml.content.data(), AnydataValueType::XML, options);

    if (!out.createdNode) {
        throw std::logic_error("Expected a new node to be created");
    }

    return out;
}

CreatedNodes Context::newPath2(const char* path, libyang::JSON json, const std::optional<CreationOptions> options) const
{
    auto out = impl::newPath2(nullptr, m_ctx.get(), std::make_shared<internal_refcount>(m_ctx), path, json.content.data(), AnydataValueType::JSON, options);

    if (!out.createdNode) {
        throw std::logic_error("Expected a new node to be created");
    }

    return out;
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

Set<SchemaNode> Context::findXPath(const char* path) const
{
    ly_set* set;
    auto err = lys_find_xpath(m_ctx.get(), nullptr, path, 0, &set);
    throwIfError(err, "Context::findXPath: couldn't find node with path '"s + path + "'");

    return Set<SchemaNode>{set, m_ctx};
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
    std::transform(features.begin(), features.end(), featuresArray.get(), [](const auto& feature) {
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

/**
 * Retrieves specific information about errors.
 */
std::vector<ErrorInfo> Context::getErrors() const
{
    std::vector<ErrorInfo> res;

    auto errIt = ly_err_first(m_ctx.get());
    while (errIt) {
        res.push_back(ErrorInfo{
            .appTag = errIt->apptag ? std::optional{errIt->apptag} : std::nullopt,
            .level = utils::toLogLevel(errIt->level),
            .message = errIt->msg,
            .code = static_cast<ErrorCode>(errIt->no),
            .path = errIt->path ? std::optional{errIt->path} : std::nullopt,
            .validationCode = utils::toValidationErrorCode(errIt->vecode)
        });

        errIt = errIt->next;
    }

    return res;
}

void Context::cleanAllErrors()
{
    ly_err_clean(m_ctx.get(), nullptr);
}
}
