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

namespace libyang {
class Context;

using ContextDeleter = std::function<void(ly_ctx*)>;

Context createUnmanagedContext(ly_ctx* ctx, ContextDeleter);
ly_ctx* retrieveContext(Context ctx);

struct ModuleInfo {
    std::string data;
    SchemaFormat format;
};
/**
 * Callback for supplying module data.
 * FIXME: add more info about what args can be optional
 */
using ModuleCallback = std::optional<ModuleInfo>(const char* modName,
                                                 const char* modRevision,
                                                 const char* submodName,
                                                 const char* submodRev);

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
    Context(const char* searchPath = nullptr, const std::optional<ContextOptions> options = std::nullopt);
    Module parseModuleMem(const char* data, const SchemaFormat format) const;
    Module parseModulePath(const char* path, const SchemaFormat format) const;
    std::optional<DataNode> parseDataMem(
            const char* data,
            const DataFormat format,
            const std::optional<ParseOptions> parseOpts = std::nullopt,
            const std::optional<ValidationOptions> validationOpts = std::nullopt) const;
    std::optional<DataNode> parseDataPath(
            const std::filesystem::path& path,
            const DataFormat format,
            const std::optional<ParseOptions> parseOpts = std::nullopt,
            const std::optional<ValidationOptions> validationOpts = std::nullopt) const;
    Module loadModule(const char* name, const char* revision = nullptr, const std::vector<std::string>& = {}) const;
    void setSearchDir(const char* searchDir) const;
    std::optional<Module> getModule(const char* name, const char* revision = nullptr) const;
    std::optional<Module> getModuleImplemented(const char* name) const;
    std::vector<Module> modules() const;
    void registerModuleCallback(std::function<ModuleCallback> callback);

    ParsedOp parseOp(const char* input, const DataFormat format, const OperationType opType) const;

    DataNode newPath(const char* path, const char* value = nullptr, const std::optional<CreationOptions> options = std::nullopt) const;
    CreatedNodes newPath2(const char* path, const char* value = nullptr, const std::optional<CreationOptions> options = std::nullopt) const;
    CreatedNodes newPath2(const char* path, libyang::JSON json, const std::optional<CreationOptions> options = std::nullopt) const;
    SchemaNode findPath(const char* dataPath, const OutputNodes output = OutputNodes::No) const;
    Set<SchemaNode> findXPath(const char* path) const;

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
