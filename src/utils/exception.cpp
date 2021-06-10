/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#include <libyang/libyang.h>
#include "utils/exception.hpp"

namespace libyang {
ErrorCode::ErrorCode(const std::string& what, unsigned int errCode)
    : Error(what)
    , m_errCode(errCode)
{
}

unsigned int ErrorCode::code()
{
    return m_errCode;
}

static_assert(std::is_same_v<std::underlying_type_t<LY_ERR>, unsigned int>);
}
