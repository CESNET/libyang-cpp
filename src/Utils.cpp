/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <libyang-cpp/Utils.hpp>
#include "utils/enum.hpp"

namespace libyang {
LogOptions setLogOptions(const libyang::LogOptions options)
{
    return static_cast<LogOptions>(ly_log_options(utils::toLogOptions(options)));
}
}
