/*
 * Copyright (C) 2025 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Jan Kundrát <jan.kundrat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

// clang-format off
// The following header MUST be included before anything else which "might" use PCRE2
// because that library uses the preprocessor to prepare the "correct" versions of symbols.
#include <libyang/tree_data.h>
// clang-format on
#include <libyang-cpp/Regex.hpp>
#include <pcre2.h>
#include "utils/exception.hpp"

#define THE_PCRE2_CODE_P reinterpret_cast<pcre2_code *>(this->code)
#define THE_PCRE2_CODE_P_P reinterpret_cast<pcre2_code **>(&this->code)

namespace libyang {

Regex::Regex(const std::string& pattern)
    : code(nullptr)
{
    auto res = ly_pattern_compile(nullptr, pattern.c_str(), THE_PCRE2_CODE_P_P);
    throwIfError(res, ly_last_logmsg());
}

Regex::~Regex()
{
    pcre2_code_free(THE_PCRE2_CODE_P);
}

bool Regex::matches(const std::string& input)
{
    auto res = ly_pattern_match(nullptr, nullptr /* we have a precompiled pattern */, input.c_str(), input.size(), THE_PCRE2_CODE_P_P);
    if (res == LY_SUCCESS) {
        return true;
    } else if (res == LY_ENOT) {
        return false;
    } else {
        throwError(res, ly_last_logmsg());
    }
}
}
