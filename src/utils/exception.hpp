/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#include <libyang-cpp/Utils.hpp>

namespace libyang {
    void throwIfError(int code, std::string msg);
    [[noreturn]] void throwError(int code, std::string msg);
}
