/*
 * Copyright (C) 2024 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Tomáš Pecka <tomas.pecka@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <chrono>
#include "libyang-cpp/export.h"

#if __cpp_lib_chrono >= 201907L && !LIBYANG_CPP_SKIP_STD_CHRONO_TZ
namespace date_impl = std::chrono;
#define LIBYANG_CPP_TIME_BACKEND_STL
#else
#include <date/tz.h>
namespace date_impl = date;
#define LIBYANG_CPP_TIME_BACKEND_HHDATE
#endif

using namespace std::string_literals;

namespace {
//FIXME
constexpr auto formatStrTz = "{:%Y-%m-%dT%H:%M:%S%Ez}";
constexpr auto formatStrTz2 = "%Y-%m-%dT%H:%M:%S%Ez";
constexpr auto formatStrNoTz = "{:%Y-%m-%dT%H:%M:%S-00:00}";
constexpr auto formatStrNoTz2 = "%Y-%m-%dT%H:%M:%S-00:00";
constexpr auto formatStrUTC = "{:%Y-%m-%dT%H:%M:%S+00:00}";
constexpr auto formatStrUTC2 = "%Y-%m-%dT%H:%M:%S+00:00";
constexpr auto formatStrParseTz = "%Y-%m-%dT%H:%M:%S%Ez";
}

namespace libyang {

/** @brief Interprets the time point in special timezones */
enum class TimezoneInterpretation {
    Unspecified, //* the timezone of the time point is unspecified */
    Local, //* interprets the time point as if local timezone */
};

#ifdef LIBYANG_CPP_TIME_BACKEND_STL
template <typename Duration>
LIBYANG_CPP_EXPORT std::string yangTimeFormat(const std::chrono::local_time<Duration>& timePoint)
{
    return std::format(formatStrNoTz, timePoint);
}
#endif

#ifdef TZ_H
template <typename Duration>
LIBYANG_CPP_EXPORT std::string yangTimeFormat(const date::local_time<Duration>& timePoint)
{
    return date::format(formatStrNoTz2, timePoint);
}
#endif

#ifdef LIBYANG_CPP_TIME_BACKEND_STL
template <typename Duration>
LIBYANG_CPP_EXPORT std::string yangTimeFormat(const std::chrono::zoned_time<Duration>& zonedTime)
{
    return std::format(formatStrTz, zonedTime);
}
#endif

#ifdef TZ_H
template <typename Duration>
LIBYANG_CPP_EXPORT std::string yangTimeFormat(const date::zoned_time<Duration>& zonedTime)
{
    return date::format(formatStrTz2, zonedTime);
}
#endif

template <typename Duration>
LIBYANG_CPP_EXPORT std::string yangTimeFormat(const std::chrono::time_point<std::chrono::system_clock, Duration>& timePoint, TimezoneInterpretation tz)
{
    switch (tz) {
    case TimezoneInterpretation::Unspecified:
#if defined(LIBYANG_CPP_TIME_BACKEND_STL)
        return std::format(formatStrNoTz, timePoint);
#elif defined(LIBYANG_CPP_TIME_BACKEND_HHDATE)
        return date::format(formatStrNoTz2, timePoint);
#endif
    case TimezoneInterpretation::Local:
        const auto* tzdata = date_impl::current_zone();
        return yangTimeFormat(date_impl::zoned_time{tzdata, timePoint});
    }

    __builtin_unreachable();
}

template <typename Duration>
LIBYANG_CPP_EXPORT std::string yangTimeFormat(const std::chrono::time_point<std::chrono::utc_clock, Duration>& timePoint)
{
#if defined(LIBYANG_CPP_TIME_BACKEND_STL)
    return std::format(formatStrUTC, timePoint);
#elif defined(LIBYANG_CPP_TIME_BACKEND_HHDATE)
    return date::format(formatStrUTC2, std::chrono::clock_cast<date::utc_clock>(timePoint));
#endif
}

/** @brief Converts a textual representation yang:date-and-time to std::time_point<std::system_clock> */
template <typename Duration = std::chrono::nanoseconds>
LIBYANG_CPP_EXPORT date_impl::local_time<Duration> fromYangTimeFormat(const std::string& timeStr)
{
    std::istringstream iss(timeStr);
    date_impl::local_time<Duration> timePoint;
    std::chrono::minutes offset;

    if (!(iss >> date_impl::parse(formatStrParseTz, timePoint, offset))) {
        throw std::invalid_argument("Invalid date for format string '"s + formatStrParseTz + "'");
    }

    // TODO: co s offsetem? Jak z toho udelat zoned_time?

    return timePoint;
}

}

#undef LIBYANG_CPP_TIME_BACKEND_STL
#undef LIBYANG_CPP_TIME_BACKEND_HHDATE
