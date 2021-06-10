
/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#include "utils/exception.hpp"

namespace libyang {
LibyangErrorCode::LibyangErrorCode(const std::string& what, LY_ERR errCode)
    : std::runtime_error(what)
    , m_errCode(errCode)
{
}

LY_ERR LibyangErrorCode::code()
{
    return m_errCode;
}
}
