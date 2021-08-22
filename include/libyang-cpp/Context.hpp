/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#include <functional>
#include <memory>
#include <libyang-cpp/ChildInstantiables.hpp>
#include <libyang-cpp/DataNode.hpp>
#include <libyang-cpp/Module.hpp>
#include <libyang-cpp/SchemaNode.hpp>
#include <libyang-cpp/Enum.hpp>

struct ly_ctx;

namespace libyang {
class Context;

Context createUnmanagedContext(ly_ctx* ctx);

struct ModuleInfo {
    const char* data;
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
/**
 * @brief libyang context class.
 */
class Context {
public:
    Context(const char* searchPath = nullptr, const std::optional<ContextOptions> options = std::nullopt);
    Module parseModuleMem(const char* data, const SchemaFormat format) const;
    Module parseModulePath(const char* path, const SchemaFormat format) const;
    DataNode parseDataMem(const char* data, const DataFormat format) const;
    Module loadModule(const char* name, const char* revision = nullptr, const std::vector<std::string>& = {}) const;
    void setSearchDir(const char* searchDir) const;
    std::optional<Module> getModule(const char* name, const char* revision = nullptr) const;
    std::vector<Module> modules() const;
    void registerModuleCallback(std::function<ModuleCallback> callback, std::function<void(void*)> deleter = nullptr);

    DataNode newPath(const char* path, const char* value = nullptr, const std::optional<CreationOptions> options = std::nullopt) const;
    SchemaNode findPath(const char* dataPath, const OutputNodes output = OutputNodes::No) const;

    struct ModuleCallbacks {
        std::function<ModuleCallback> callback;
        std::function<void(void*)> deleter;
    };

    friend Context createUnmanagedContext(ly_ctx* ctx);

private:
    Context(ly_ctx* ctx);
    std::shared_ptr<ly_ctx> m_ctx;

    ModuleCallbacks m_callbacks;
};
}
