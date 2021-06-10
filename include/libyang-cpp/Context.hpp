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
#include <libyang-cpp/Enum.hpp>

struct ly_ctx;

namespace libyang {
/**
 * @brief libyang context class.
 */
class Context {
public:
    Context();
    void parseModuleMem(const char* data, const SchemaFormat format);
    DataNode parseDataMem(const char* data, const DataFormat format);
private:
    std::unique_ptr<ly_ctx, void(*)(ly_ctx*)> m_ctx;
};
}
