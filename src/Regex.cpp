/*
 * Copyright (C) 2025 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Jan Kundrát <jan.kundrat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <libyang-cpp/Regex.hpp>
#include <libyang/tree_data.h>
#include "utils/exception.hpp"

namespace libyang {

Regex::Regex(const std::string& pattern)
    : code(nullptr)
{
    auto res = ly_pattern_compile(nullptr, pattern.c_str(), &code);
    throwIfError(res, ly_last_logmsg());
}

Regex::~Regex()
{
    ly_pattern_free(code);
    code = nullptr;
}

bool Regex::matches(const std::string& input)
{
    auto res = ly_pattern_match(nullptr, nullptr /* we have a precompiled pattern */, input.c_str(), input.size(), &code);
    if (res == LY_SUCCESS) {
        return true;
    } else if (res == LY_ENOT) {
        return false;
    } else {
        throwError(res, ly_last_logmsg());
    }
}
}
