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
}
