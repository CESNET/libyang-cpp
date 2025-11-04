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
#include <libyang-cpp/export.h>
#include <memory>

struct ly_ctx;

/**
 * @brief The libyang-cpp namespace.
 */
namespace libyang {
class Context;

using ContextDeleter = std::function<void(ly_ctx*)>;

LIBYANG_CPP_EXPORT Context createUnmanagedContext(ly_ctx* ctx, ContextDeleter);
LIBYANG_CPP_EXPORT ly_ctx* retrieveContext(Context ctx);

/**
 * @brief A structure containing a module as a string and its format.
 *
 * Used as the return value for module retrieval callback.
 */
struct LIBYANG_CPP_EXPORT ModuleInfo {
    std::string data;
    SchemaFormat format;
};
/**
 * @brief Callback for supplying module data.
 *
 * This callback is used for supplying both module and submodule data.
 *
 * @param modName The name of the missing module.
 * @param modRevision Optional missing module revision. std::nullopt can mean two things
 *   - latest revision is requested
 *   - a submodule is requested, in that case submodName won't be std::nullopt
 * @param submodName Optional missing submodule name. std::nullopt if requesting the main module
 * @param submodRev Optional missing submodule revision. std::nullopt if requesting the latest submodule revision.
 */
using ModuleCallback = std::optional<ModuleInfo>(const std::string& modName,
                                                 const std::optional<std::string>& modRevision,
                                                 const std::optional<std::string>& submodName,
                                                 const std::optional<std::string>& submodRev);

/**
 * @brief Contains detailed libyang error.
 *
 * Wraps `ly_err_item`.
 */
struct LIBYANG_CPP_EXPORT ErrorInfo {
    bool operator==(const ErrorInfo& other) const = default;
    std::optional<std::string> appTag;
    LogLevel level;
    std::string message;
    ErrorCode code;
    std::optional<std::string> dataPath;
    std::optional<std::string> schemaPath;
    uint64_t line;
    ValidationErrorCode validationCode;
};

/**
 * @brief libyang context class.
 */
class LIBYANG_CPP_EXPORT Context {
public:
    Context(const std::optional<std::filesystem::path>& searchPath = std::nullopt, const std::optional<ContextOptions> options = std::nullopt);
    Module parseModule(const std::string& data, const SchemaFormat format, const std::vector<std::string>& features = {}) const;
    Module parseModule(const std::filesystem::path& path, const SchemaFormat format, const std::vector<std::string>& features = {}) const;
    std::optional<DataNode> parseData(
            const std::string& data,
            const DataFormat format,
            const std::optional<ParseOptions> parseOpts = std::nullopt,
            const std::optional<ValidationOptions> validationOpts = std::nullopt) const;
    std::optional<DataNode> parseData(
            const std::filesystem::path& path,
            const DataFormat format,
            const std::optional<ParseOptions> parseOpts = std::nullopt,
            const std::optional<ValidationOptions> validationOpts = std::nullopt) const;
    std::optional<DataNode> parseExtData(
        const ExtensionInstance& ext,
        const std::string& data,
        const DataFormat format,
        const std::optional<ParseOptions> parseOpts = std::nullopt,
        const std::optional<ValidationOptions> validationOpts = std::nullopt) const;
    std::optional<DataNode> parseValueFragment(
        const std::string& path,
        const std::string& data,
        const DataFormat format,
        const std::optional<CreationOptions> createOpts = std::nullopt,
        const std::optional<ParseOptions> parseOpts = std::nullopt,
        const std::optional<ValidationOptions> validationOpts = std::nullopt) const;
    Module loadModule(const std::string& name, const std::optional<std::string>& revision = std::nullopt, const std::vector<std::string>& = {}) const;
    void setSearchDir(const std::filesystem::path& searchDir) const;
    std::optional<Module> getModule(const std::string& name, const std::optional<std::string>& revision) const;
    std::optional<Module> getModuleImplemented(const std::string& name) const;
    std::optional<Module> getModuleLatest(const std::string& name) const;
    std::vector<Module> modules() const;
    std::optional<SubmoduleParsed> getSubmodule(const std::string& name, const std::optional<std::string>& revision) const;
    void registerModuleCallback(std::function<ModuleCallback> callback);

    ParsedOp parseOp(
            const std::string& input,
            const DataFormat format,
            const OperationType opType,
            const std::optional<ParseOptions> parseOpts = std::nullopt) const;

    DataNode newPath(const std::string& path, const std::optional<std::string>& value = std::nullopt, const std::optional<CreationOptions> options = std::nullopt) const;
    CreatedNodes newPath2(const std::string& path, const std::optional<std::string>& value = std::nullopt, const std::optional<CreationOptions> options = std::nullopt) const;
    CreatedNodes newPath2(const std::string& path, libyang::JSON json, const std::optional<CreationOptions> options = std::nullopt) const;
    CreatedNodes newPath2(const std::string& path, libyang::XML xml, const std::optional<CreationOptions> options = std::nullopt) const;
    std::optional<DataNode> newExtPath(const ExtensionInstance& ext, const std::string& path, const std::optional<std::string>& value, const std::optional<CreationOptions> options = std::nullopt) const;
    std::optional<DataNode> newOpaqueJSON(const OpaqueName& name, const std::optional<libyang::JSON>& value) const;
    std::optional<DataNode> newOpaqueXML(const OpaqueName& name, const std::optional<libyang::XML>& value) const;
    SchemaNode findPath(const std::string& dataPath, const InputOutputNodes inputOutputNodes = InputOutputNodes::Input) const;
    Set<SchemaNode> findXPath(const std::string& path) const;

    std::vector<ErrorInfo> getErrors() const;
    void cleanAllErrors();

    friend LIBYANG_CPP_EXPORT Context createUnmanagedContext(ly_ctx* ctx, ContextDeleter);
    friend LIBYANG_CPP_EXPORT ly_ctx* retrieveContext(Context ctx);

private:
    Context(ly_ctx* ctx, ContextDeleter = nullptr);
    std::shared_ptr<ly_ctx> m_ctx;

    std::function<ModuleCallback> m_moduleCallback;
};
}
