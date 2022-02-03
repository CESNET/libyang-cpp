/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#include <libyang-cpp/Utils.hpp>
#include <libyang/libyang.h>
#include <sstream>
#include "exception.hpp"

namespace libyang {
ErrorWithCode::ErrorWithCode(const std::string& what, unsigned int errCode)
    : Error(what)
    , m_errCode(static_cast<ErrorCode>(errCode))
{
}

ErrorCode ErrorWithCode::code()
{
    return m_errCode;
}

void throwIfError(int code, std::string msg)
{
    if (code != LY_SUCCESS) {
        throwError(code, msg);
    }
}

[[noreturn]] void throwError(int code, std::string msg)
{
    if (code == LY_SUCCESS) {
        throw std::logic_error("Threw with LY_SUCCESS");
    }

    std::ostringstream oss;
    oss << msg << ": " << static_cast<ErrorCode>(code);
    throw ErrorWithCode(oss.str(), code);
}
}
