/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#include <filesystem>
#include <functional>
#include <libyang-cpp/Collection.hpp>
#include <libyang-cpp/ChildInstantiables.hpp>
#include <libyang-cpp/DataNode.hpp>
#include <libyang-cpp/Enum.hpp>
#include <libyang-cpp/Module.hpp>
#include <libyang-cpp/SchemaNode.hpp>
#include <libyang-cpp/Set.hpp>
#include <memory>

struct ly_ctx;

/**
 * @brief The libyang-cpp namespace.
 */
namespace libyang {
class Context;

using ContextDeleter = std::function<void(ly_ctx*)>;

Context createUnmanagedContext(ly_ctx* ctx, ContextDeleter);
ly_ctx* retrieveContext(Context ctx);

/**
 * @brief A structure containing a module as a string and its format.
 *
 * Used as the return value for module retrieval callback.
 */
struct ModuleInfo {
    std::string data;
    SchemaFormat format;
};
/**
 * Callback for supplying module data.
 * TODO: add more info about what args can be optional by changing the args to std::string_view
 * and std::optional<std::string_view.
 */
using ModuleCallback = std::optional<ModuleInfo>(const char* modName,
                                                 const char* modRevision,
                                                 const char* submodName,
                                                 const char* submodRev);

/**
 * @brief Contains detailed libyang error.
 *
 * Wraps `ly_err_item`.
 */
struct ErrorInfo {
    bool operator==(const ErrorInfo& other) const = default;
    std::optional<std::string> appTag;
    LogLevel level;
    std::string message;
    ErrorCode code;
    std::optional<std::string> path;
    ValidationErrorCode validationCode;
};

/**
 * @brief libyang context class.
 */
class Context {
public:
    Context(const std::optional<std::string>& searchPath = std::nullopt, const std::optional<ContextOptions> options = std::nullopt);
    Module parseModuleMem(const std::string& data, const SchemaFormat format) const;
    Module parseModulePath(const std::string& path, const SchemaFormat format) const;
    std::optional<DataNode> parseDataMem(
            const std::string& data,
            const DataFormat format,
            const std::optional<ParseOptions> parseOpts = std::nullopt,
            const std::optional<ValidationOptions> validationOpts = std::nullopt) const;
    std::optional<DataNode> parseDataPath(
            const std::filesystem::path& path,
            const DataFormat format,
            const std::optional<ParseOptions> parseOpts = std::nullopt,
            const std::optional<ValidationOptions> validationOpts = std::nullopt) const;
    Module loadModule(const std::string& name, const std::optional<std::string>& revision = std::nullopt, const std::vector<std::string>& = {}) const;
    void setSearchDir(const std::string& searchDir) const;
    std::optional<Module> getModule(const std::string& name, const std::optional<std::string>& revision = std::nullopt) const;
    std::optional<Module> getModuleImplemented(const std::string& name) const;
    std::vector<Module> modules() const;
    void registerModuleCallback(std::function<ModuleCallback> callback);

    ParsedOp parseOp(const std::string& input, const DataFormat format, const OperationType opType) const;

    DataNode newPath(const std::string& path, const std::optional<std::string>& value = std::nullopt, const std::optional<CreationOptions> options = std::nullopt) const;
    CreatedNodes newPath2(const std::string& path, const std::optional<std::string>& value = std::nullopt, const std::optional<CreationOptions> options = std::nullopt) const;
    CreatedNodes newPath2(const std::string& path, libyang::JSON json, const std::optional<CreationOptions> options = std::nullopt) const;
    CreatedNodes newPath2(const std::string& path, libyang::XML xml, const std::optional<CreationOptions> options = std::nullopt) const;
    SchemaNode findPath(const std::string& dataPath, const OutputNodes output = OutputNodes::No) const;
    Set<SchemaNode> findXPath(const std::string& path) const;

    std::vector<ErrorInfo> getErrors() const;
    void cleanAllErrors();

    friend Context createUnmanagedContext(ly_ctx* ctx, ContextDeleter);
    friend ly_ctx* retrieveContext(Context ctx);

private:
    Context(ly_ctx* ctx, ContextDeleter = nullptr);
    std::shared_ptr<ly_ctx> m_ctx;

    std::function<ModuleCallback> m_moduleCallback;
};
}
