/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <algorithm>
#include <libyang-cpp/Context.hpp>
#include <libyang-cpp/DataNode.hpp>
#include <libyang-cpp/SchemaNode.hpp>
#include <libyang-cpp/Utils.hpp>
#include <libyang/libyang.h>
#include <span>
#include <stdexcept>
#include "utils/deleters.hpp"
#include "utils/enum.hpp"
#include "utils/exception.hpp"
#include "utils/newPath.hpp"

using namespace std::string_literals;

namespace libyang {

/**
 * @brief Wraps a ly_ctx pointer with specifying an optional custom deleter. The pointer is not managed further by
 * libyang-cpp's automatic memory management. Use at own risk.
 */
Context createUnmanagedContext(ly_ctx* ctx, ContextDeleter deleter)
{
    return Context{ctx, deleter};
}

/**
 * @brief Retreives a raw pointer to the context. Use at own risk.
 */
ly_ctx* retrieveContext(Context ctx)
{
    return ctx.m_ctx.get();
}

/**
 * @brief Creates a new libyang context.
 * @param searchPath Optional the search directory for modules.
 * @param options Optional context creation options.
 */
Context::Context(const std::optional<std::filesystem::path>& searchPath, const std::optional<ContextOptions> options)
{
    ly_ctx* ctx;
    auto err = ly_ctx_new(searchPath ? PATH_TO_LY_STRING(*searchPath) : nullptr, options ? utils::toContextOptions(*options) : 0, &ctx);
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
void Context::setSearchDir(const std::filesystem::path& searchDir) const
{
    auto err = ly_ctx_set_searchdir(m_ctx.get(), PATH_TO_LY_STRING(searchDir));
    throwIfError(err, "Can't set search directory");
}

namespace {
// this might be dangerous if invoked on a temporary
std::vector<const char*> toCStringArray(const std::vector<std::string>& vec)
{
    std::vector<const char*> res;
    res.reserve(vec.size() + 1 /* trailing nullptr */);
    std::transform(vec.begin(), vec.end(), std::back_inserter(res), [](const auto& x) { return x.c_str(); });
    res.push_back(nullptr);
    return res;
}
}


/**
 * @brief Parses module from a string.
 *
 * @param data String containing the module definition.
 * @param features List of features to enable. Leave empty to not enable any feature.
 * @param format Format of the module definition.
 */
Module Context::parseModule(const std::string& data, const SchemaFormat format, const std::vector<std::string>& features) const
{
    auto in = wrap_ly_in_new_memory(data);
    lys_module* mod;
    auto err = lys_parse(m_ctx.get(), in.get(), utils::toLysInformat(format), toCStringArray(features).data(), &mod);
    throwIfError(err, "Can't parse module");

    return Module{mod, m_ctx};
}

/**
 * @brief Parses module from a file.
 *
 * @param data String containing the path to the file.
 * @param features List of features to enable. Leave empty to not enable any feature.
 * @param format Format of the module definition.
 */
Module Context::parseModule(const std::filesystem::path& path, const SchemaFormat format, const std::vector<std::string>& features) const
{
    auto in = wrap_ly_in_new_file(path);
    lys_module* mod;
    auto err = lys_parse(m_ctx.get(), in.get(), utils::toLysInformat(format), toCStringArray(features).data(), &mod);
    throwIfError(err, "Can't parse module");

    return Module{mod, m_ctx};
}

/**
 * @brief Parses data from a string into libyang.
 *
 * @param data String containing the input data.
 * @param format Format of the input data.
 */
std::optional<DataNode> Context::parseData(
        const std::string& data,
        const DataFormat format,
        const std::optional<ParseOptions> parseOpts,
        const std::optional<ValidationOptions> validationOpts) const
{
    lyd_node* tree;
    auto err = lyd_parse_data_mem(
            m_ctx.get(),
            data.c_str(),
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
std::optional<DataNode> Context::parseData(
        const std::filesystem::path& path,
        const DataFormat format,
        const std::optional<ParseOptions> parseOpts,
        const std::optional<ValidationOptions> validationOpts) const
{
    lyd_node* tree;
    ly_log_level(LY_LLDBG);
    auto err = lyd_parse_data_path(
            m_ctx.get(),
            PATH_TO_LY_STRING(path),
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
 * @brief Parses data from a string representing extension data tree node.
 *
 * Wraps `lyd_parse_ext_data`.
 */
std::optional<DataNode> Context::parseExtData(
    const ExtensionInstance& ext,
    const std::string& data,
    const DataFormat format,
    const std::optional<ParseOptions> parseOpts,
    const std::optional<ValidationOptions> validationOpts) const
{
    auto in = wrap_ly_in_new_memory(data);

    lyd_node* tree = nullptr;
    auto err = lyd_parse_ext_data(
        ext.m_instance,
        nullptr,
        in.get(),
        utils::toLydFormat(format),
        parseOpts ? utils::toParseOptions(*parseOpts) : 0,
        validationOpts ? utils::toValidationOptions(*validationOpts) : 0,
        &tree);
    throwIfError(err, "Can't parse extension data");

    if (!tree) {
        return std::nullopt;
    }

    return DataNode{tree, m_ctx};
}


/**
 * @brief Parses YANG data into an operation data tree.
 *
 * Use this method to parse standalone "operation elements", which are:
 *
 *   - a NETCONF RPC,
 *   - a NETCONF notification,
 *   - a RESTCONF notification,
 *   - a YANG notification.
 *
 * Parsing any of these requires just the schema (which is available through the Context), and the textual payload.
 * All the other information are encoded in the textual payload as per the standard.
 *
 * Parsing a RESTCONF RPC is different because the RPC name is encoded out-of-band in the requested URL.
 * Parsing a RPC reply (regardless on NETCONF/RESTCONF calling convention) requires additional information, too.
 * Use DataNode::parseOp() for these.
 *
 * The returned value's `tree` contains some metadata extracted from the envelope (e.g., a notification's `eventTime`)
 * stored into opaque data nodes. The returned value's `op` contains the actual payload (e.g., notificaiton data).
 *
 * To parse a NETCONF RPC, firstly use this method supplying the RPC as the `input` argument. After that, to parse
 * a NETCONF RPC reply, use DataNode::parseOp on the `ParsedOp::op` field with OperationType::ReplyNetconf.
 * Note: to parse a NETCONF RPC reply, you MUST parse the original NETCONF RPC request (that is, you have to use
 * this method with OperationType::RpcNetconf).
 */
ParsedOp Context::parseOp(const std::string& input, const DataFormat format, const OperationType opType) const
{
    auto in = wrap_ly_in_new_memory(input);

    switch (opType) {
    case OperationType::RpcYang:
    case OperationType::RpcNetconf:
    case OperationType::NotificationNetconf:
    case OperationType::NotificationRestconf:
    case OperationType::NotificationYang: {
        lyd_node* op = nullptr;
        lyd_node* tree = nullptr;
        auto err = lyd_parse_op(m_ctx.get(), nullptr, in.get(), utils::toLydFormat(format), utils::toOpType(opType), &tree, &op);

        ParsedOp res;
        res.tree = tree ? std::optional{libyang::wrapRawNode(tree)} : std::nullopt;

        if ((opType == OperationType::NotificationYang) || (opType == OperationType::RpcYang)) {
            res.op = op && tree ? std::optional{DataNode(op, res.tree->m_refs)} : std::nullopt;
        } else {
            res.op = op ? std::optional{libyang::wrapRawNode(op)} : std::nullopt;
        }

        throwIfError(err, "Can't parse a standalone rpc/action/notification into operation data tree");
        return res;
    }
    case OperationType::ReplyNetconf:
    case OperationType::ReplyRestconf:
        throw Error("To parse a NETCONF/RESTCONF reply to an RPC, use DataNode::parseOp");
    case OperationType::RpcRestconf:
        throw Error("To parse a RESTCONF RPC, use DataNode::parseOp (to specify the RPC envelope)");
    default:
        throw Error("Context::parseOp: unsupported op");
    }
}

/**
 * @brief Creates a new node with the supplied path, creating a completely new tree.
 *
 * @param path Path of the new node.
 * @param value String representation of the value. Use std::nullopt for non-leaf nodes and the `empty` type.
 * @param options Options that change the behavior of this method.
 * @return Returns the first created parent.
 */
DataNode Context::newPath(const std::string& path, const std::optional<std::string>& value, const std::optional<CreationOptions> options) const
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
 * @param value String representation of the value. Use std::nullopt for non-leaf nodes and the `empty` type.
 * @param options Options that change the behavior of this method.
 * @return Returns the first created parent and also the node specified by `path`. These might be the same node.
 */
CreatedNodes Context::newPath2(const std::string& path, const std::optional<std::string>& value, const std::optional<CreationOptions> options) const
{
    // The AnydataValueType here doesn't matter, because this overload creates a classic node and not an `anydata` node.
    // TODO: Make overloads for all of the AnydataValueType values.
    auto out = impl::newPath2(nullptr, m_ctx.get(), std::make_shared<internal_refcount>(m_ctx), path, value ? value->c_str() : nullptr, AnydataValueType::String, options);

    if (!out.createdNode) {
        throw std::logic_error("Expected a new node to be created");
    }

    return out;
}

/**
 * @brief Creates a new anyxml node with the supplied path, creating a completely new tree.
 *
 * @param path Path of the new node.
 * @param xml An XML value.
 * @param options Options that change the behavior of this method.
 * @return Returns the first created parent and also the node specified by `path`. These might be the same node.
 */
CreatedNodes Context::newPath2(const std::string& path, libyang::XML xml, const std::optional<CreationOptions> options) const
{
    auto out = impl::newPath2(nullptr, m_ctx.get(), std::make_shared<internal_refcount>(m_ctx), path, xml.content.data(), AnydataValueType::XML, options);

    if (!out.createdNode) {
        throw std::logic_error("Expected a new node to be created");
    }

    return out;
}

/**
 * @brief Creates a new anydata node with the supplied path with a JSON value, creating a completely new tree.
 *
 * @param path Path of the new node.
 * @param json JSON value.
 * @param options Options that change the behavior of this method.
 * @return Returns the first created parent and also the node specified by `path`. These might be the same node.
 */
CreatedNodes Context::newPath2(const std::string& path, libyang::JSON json, const std::optional<CreationOptions> options) const
{
    auto out = impl::newPath2(nullptr, m_ctx.get(), std::make_shared<internal_refcount>(m_ctx), path, json.content.data(), AnydataValueType::JSON, options);

    if (!out.createdNode) {
        throw std::logic_error("Expected a new node to be created");
    }

    return out;
}

/**
 * @brief Creates a new extension node with the supplied path, creating a completely new tree.
 *
 * @param ext Extension instance where the node being created is defined.
 * @param path Path of the new node.
 * @param value String representation of the value. Use std::nullopt for non-leaf nodes and the `empty` type.
 * @param options Options that change the behavior of this method.
 * @return Returns the first created parent.
 */
std::optional<DataNode> Context::newExtPath(const ExtensionInstance& ext, const std::string& path, const std::optional<std::string>& value, const std::optional<CreationOptions> options) const
{
    auto out = impl::newExtPath(nullptr, ext.m_instance, std::make_shared<internal_refcount>(m_ctx), path, value, options);

    if (!out) {
        throw std::logic_error("Expected a new node to be created");
    }

    return *out;
}

/**
 * @brief Create a new JSON opaque node
 *
 * Wraps `lyd_new_opaq`.
 *
 * @param moduleName Node module name, used as a prefix as well
 * @param name Name of the created node
 * @param value JSON data blob, if any
 * @return Returns the newly created node (if created)
 */
std::optional<DataNode> Context::newOpaqueJSON(const std::string& moduleName, const std::string& name, const std::optional<libyang::JSON>& value) const
{
    lyd_node* out;
    auto err = lyd_new_opaq(nullptr, m_ctx.get(), name.c_str(), value ? value->content.c_str() : nullptr, nullptr, moduleName.c_str(), &out);

    throwIfError(err, "Couldn't create an opaque JSON node '"s + moduleName + ':' + name + "'");

    if (out) {
        return DataNode{out, std::make_shared<internal_refcount>(m_ctx)};
    } else {
        return std::nullopt;
    }

}

/**
 * @brief Create a new XML opaque node
 *
 * Wraps `lyd_new_opaq2`.
 *
 * @param xmlNamespace Node module namespace
 * @param name Name of the created node
 * @param value XML data blob, if any
 * @return Returns the newly created node (if created)
 */
std::optional<DataNode> Context::newOpaqueXML(const std::string& xmlNamespace, const std::string& name, const std::optional<libyang::XML>& value) const
{
    lyd_node* out;
    auto err = lyd_new_opaq2(nullptr, m_ctx.get(), name.c_str(), value ? value->content.c_str() : nullptr, nullptr, xmlNamespace.c_str(), &out);

    throwIfError(err, "Couldn't create an opaque XML node '"s + name +"' from namespace '" + xmlNamespace + "'");

    if (out) {
        return DataNode{out, std::make_shared<internal_refcount>(m_ctx)};
    } else {
        return std::nullopt;
    }

}

/**
 * @brief Returns the schema definition of a node specified by `dataPath`.
 *
 * @param dataPath A JSON path of the node to get.
 * @param inputOutputNodes Consider input or output nodes
 * @return The found schema node.
 */
SchemaNode Context::findPath(const std::string& dataPath, const InputOutputNodes inputOutputNodes) const
{
    // TODO: allow output nodes
    auto node = lys_find_path(m_ctx.get(), nullptr, dataPath.c_str(), inputOutputNodes == InputOutputNodes::Output ? true : false);

    if (!node) {
        throw Error("Couldn't find schema node: "s + dataPath);
    }

    return SchemaNode{node, m_ctx};
}

/**
 * @brief Returns a set of schema nodes matching an XPath.
 *
 * Wraps `lys_find_xpath`.
 */
Set<SchemaNode> Context::findXPath(const std::string& path) const
{
    ly_set* set;
    auto err = lys_find_xpath(m_ctx.get(), nullptr, path.c_str(), 0, &set);
    throwIfError(err, "Context::findXPath: couldn't find node with path '"s + path + "'");

    return Set<SchemaNode>{set, m_ctx};
}

/**
 * @brief Retrieves module from the context.
 *
 * @param name Name of the wanted module.
 * @param revision Revision of the wanted module. Use std::nullopt if you want a module that has no revision specified.
 */
std::optional<Module> Context::getModule(const std::string& name, const std::optional<std::string>& revision) const
{
    auto mod = ly_ctx_get_module(m_ctx.get(), name.c_str(), revision ? revision->c_str() : nullptr);

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
std::optional<Module> Context::getModuleImplemented(const std::string& name) const
{
    auto mod = ly_ctx_get_module_implemented(m_ctx.get(), name.c_str());

    if (!mod) {
        return std::nullopt;
    }

    return Module{mod, m_ctx};
}

/**
 * @brief Retrieves the latest version of a module from the context.
 *
 * Wraps `ly_ctx_get_module_latest`.
 *
 * @param name Name of the wanted module.
 * @return The wanted module or std::nullopt if there is no module with the name.
 */
std::optional<Module> Context::getModuleLatest(const std::string& name) const
{
    auto mod = ly_ctx_get_module_latest(m_ctx.get(), name.c_str());

    if (!mod) {
        return std::nullopt;
    }

    return Module{mod, m_ctx};
}

/**
 * @brief Loads a module through its name and revision.
 *
 * @param name The name of the module to be loaded.
 * @param revision Optional revision of the module to be loaded.
 * @param features Optional features array to be enabled. Pass {"*"} to enable all of them.
 *
 * Wraps `ly_ctx_load_module`.
 */
Module Context::loadModule(const std::string& name, const std::optional<std::string>& revision, const std::vector<std::string>& features) const
{
    auto mod = ly_ctx_load_module(m_ctx.get(), name.c_str(), revision ? revision->c_str() : nullptr, toCStringArray(features).data());

    if (!mod) {
        throw Error("Can't load module '"s + name + "'");
    }

    return Module{mod, m_ctx};
}

/**
 * @brief Retrieves a vector of all loaded modules.
 *
 * Wraps `ly_ctx_get_module_iter`.
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

/**
 * @brief Retrieves a submodule from the context.
 *
 * @param name Name of the wanted submodule.
 * @param revision Revision of the wanted submodule. Use std::nullopt if you want a submodule that has no revision specified.
 */
std::optional<SubmoduleParsed> Context::getSubmodule(const std::string& name, const std::optional<std::string>& revision) const
{
    auto mod = ly_ctx_get_submodule(m_ctx.get(), name.c_str(), revision ? revision->c_str() : nullptr);

    if (!mod) {
        return std::nullopt;
    }

    return SubmoduleParsed{mod, m_ctx};
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
    auto ret = (*cb)(
            modName,
            modRev ? std::optional{modRev} : std::nullopt,
            submodName ? std::optional{submodName} : std::nullopt,
            submodRev ? std::optional{submodRev} : std::nullopt);
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
            .code = static_cast<ErrorCode>(errIt->err),
            .dataPath = errIt->data_path ? std::optional{errIt->data_path} : std::nullopt,
            .schemaPath = errIt->schema_path ? std::optional{errIt->schema_path} : std::nullopt,
            .line = errIt->line,
            .validationCode = utils::toValidationErrorCode(errIt->vecode)
        });

        errIt = errIt->next;
    }

    return res;
}

/**
 * @brief Clears up all errors within the context.
 *
 * Wraps `ly_err_clean`.
 */
void Context::cleanAllErrors()
{
    ly_err_clean(m_ctx.get(), nullptr);
}
}
