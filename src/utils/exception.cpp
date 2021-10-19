/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#include <libyang-cpp/utils/exception.hpp>
#include <libyang/libyang.h>

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
}
