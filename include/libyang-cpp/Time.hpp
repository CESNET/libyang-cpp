/*
 * Copyright (C) 2024 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Tomáš Pecka <tomas.pecka@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once
#include <chrono>
#include <date/date.h>
#include <date/tz.h>
#include "libyang-cpp/export.h"

namespace {
// cannot static_asssert(false) directly in constexpr else
template <bool flag = false>
void error_wrong_precision()
{
    static_assert(flag, "Wrong precision for fraction-digit time");
}

constexpr auto formatStr = "%Y-%m-%dT%H:%M:%S%Ez";
}

namespace libyang {

/** @brief Converts a time_point to a textual representation yang:date-and-time with timezone.
 *         If timezone is missing then local timezone is used.
 *         The fractional part of a second is auto-determined by the time_point resolution but can override with Precision template parameter.
 *
 *  @tparam Precision Precision of the fractional part. Can be std::chrono::{seconds,milliseconds,nanoseconds,microseconds}.
 *
 *  @throws std::runtime_error when local timezone could not be retrieved
 *  @throws std::runtime_error when tz not found in timezone database
 * */
template <typename TimePoint, typename Precision = TimePoint::duration>
LIBYANG_CPP_EXPORT std::string yangTimeFormat(const TimePoint& timePoint, const std::optional<std::string>& timezone = std::nullopt)
{
    if constexpr (
        !std::is_same_v<Precision, std::chrono::seconds>
        && !std::is_same_v<Precision, std::chrono::milliseconds>
        && !std::is_same_v<Precision, std::chrono::microseconds>
        && !std::is_same_v<Precision, std::chrono::nanoseconds>) {
        error_wrong_precision();
    }

    auto tz = timezone ? date::locate_zone(*timezone) : date::current_zone();
    return date::format(formatStr, date::make_zoned(tz, std::chrono::time_point_cast<Precision>(timePoint)));
}
}
