/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#include <libyang/libyang.h>
#include <stdexcept>
#include "Context.hpp"

namespace libyang {

namespace {
constexpr LYS_INFORMAT toLysInformat(const SchemaFormat format)
{
    return static_cast<LYS_INFORMAT>(format);
}
// These tests ensure that I used the right numbers when defining my enum.
static_assert(LYS_INFORMAT::LYS_IN_UNKNOWN == toLysInformat(SchemaFormat::Detect));
static_assert(LYS_INFORMAT::LYS_IN_YANG == toLysInformat(SchemaFormat::Yang));
static_assert(LYS_INFORMAT::LYS_IN_YIN == toLysInformat(SchemaFormat::Yin));

constexpr LYD_FORMAT toLydFormat(const DataFormat format)
{
    return static_cast<LYD_FORMAT>(format);
}
// These tests ensure that I used the right numbers when defining my enum.
static_assert(LYD_FORMAT::LYD_UNKNOWN == toLydFormat(DataFormat::Invalid));
static_assert(LYD_FORMAT::LYD_XML == toLydFormat(DataFormat::XML));
static_assert(LYD_FORMAT::LYD_JSON == toLydFormat(DataFormat::JSON));
}

Context::Context()
    : m_ctx(nullptr, nullptr) // fun-ptr deleter deletes the default constructor
{
    ly_ctx* ctx;
    auto err = ly_ctx_new(nullptr, 0, &ctx);
    if (err != LY_SUCCESS) {
        throw std::runtime_error("Can't create libyang context (" + std::to_string(err) + ")");
    }

    m_ctx = std::unique_ptr<ly_ctx, decltype(&ly_ctx_destroy)>(ctx, ly_ctx_destroy);
}

void Context::parseModuleMem(const char* data, const SchemaFormat format)
{
    // FIXME: Return the module handle that lys_parse_mem gives.
    auto err = lys_parse_mem(m_ctx.get(), data, toLysInformat(format), nullptr);
    if (err != LY_SUCCESS) {
        throw std::runtime_error("Can't parse module (" + std::to_string(err) + ")");
    }
}
}
