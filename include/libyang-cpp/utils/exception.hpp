/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#pragma once
// FIXME: including this here means I'm potentially leaking something from libyang directly into the user's global
// namespace (and it's only because I need the LY_ERR enum, maybe I can wrap that too?)
#include <libyang/libyang.h>
#include <stdexcept>

namespace libyang {
class LibyangErrorCode : public std::runtime_error {
public:
    explicit LibyangErrorCode(const std::string& what, LY_ERR errCode);

    LY_ERR code();
private:
    LY_ERR m_errCode;
};

class LibyangError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};
}
