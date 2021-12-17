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

/**
 * Sets a new log level for libyang. Returns the old log level.
 */
LogLevel setLogLevel(const LogLevel level)
{
    return utils::toLogLevel(ly_log_level(utils::toLogLevel(level)));
}

bool PointerCompare::operator()(const DataNode& a, const DataNode& b) const
{
    return getRawNode(a) < getRawNode(b);
}
}
