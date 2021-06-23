/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#include <memory>
#include <libyang-cpp/DataNode.hpp>
#include <libyang-cpp/Module.hpp>
#include <libyang-cpp/SchemaNode.hpp>
#include <libyang-cpp/Enum.hpp>

struct ly_ctx;

namespace libyang {
/**
 * @brief libyang context class.
 */
class Context {
public:
    Context(const char* searchPath = nullptr, const std::optional<ContextOptions> options = std::nullopt);
    void parseModuleMem(const char* data, const SchemaFormat format);
    void parseModulePath(const char* path, const SchemaFormat format);
    DataNode parseDataMem(const char* data, const DataFormat format);
    Module loadModule(const char* name, const char* revision = nullptr, const std::vector<std::string>& = {});
    void setSearchDir(const char* searchDir);
    std::optional<Module> getModule(const char* name, const char* revision = nullptr) const;

    DataNode newPath(const char* path, const char* value = nullptr, const std::optional<CreationOptions> options = std::nullopt);
    SchemaNode findPath(const char* dataPath);
private:
    std::shared_ptr<ly_ctx> m_ctx;
};
}
