/*
 * Copyright (C) 2025 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Jan Kundrát <jan.kundrat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include <libyang-cpp/export.h>
#include <string>

namespace libyang {
class Context;

/**
 * @brief A regular expression pattern which uses the YANG-flavored regex engine
 */
class LIBYANG_CPP_EXPORT Regex {
public:
    Regex(const std::string& pattern);
    ~Regex();
    bool matches(const std::string& input);
private:
    void* code;
};

}
