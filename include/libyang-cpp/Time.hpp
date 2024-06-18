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
namespace libyang {
namespace date_impl = std::chrono;
}
#define LIBYANG_CPP_TIME_BACKEND_STL
#else
#include <date/tz.h>
namespace libyang {
namespace date_impl = date;
}
#define LIBYANG_CPP_TIME_BACKEND_HHDATE
#endif

#if !defined(LIBYANG_CPP_TIME_BACKEND_STL) && !defined(LIBYANG_CPP_TIME_BACKEND_HHDATE)
#error "Neither compatible STL backend nor HowardHinnant/date found"
#endif

namespace libyang {
using namespace std::string_literals;

#define LIBYANG_CPP_TIME_FORMAT_DATETIME_BASE "%Y-%m-%dT%H:%M:%S"
#define LIBYANG_CPP_TIME_FORMAT_TZ LIBYANG_CPP_TIME_FORMAT_DATETIME_BASE "%Ez"
#define LIBYANG_CPP_TIME_FORMAT_NO_TZ LIBYANG_CPP_TIME_FORMAT_DATETIME_BASE "-00:00"
#define LIBYANG_CPP_TIME_FORMAT_UTC LIBYANG_CPP_TIME_FORMAT_DATETIME_BASE "+00:00"

/** @brief Interprets the time point in special timezones */
enum class TimezoneInterpretation {
    Unspecified, //* the timezone of the time point is unspecified */
    Local, //* interprets the time point as if local timezone */
};

#ifdef LIBYANG_CPP_TIME_BACKEND_STL
/** @brief Converts a time point of local time to a string representing yang:date-and-time with unspecified TZ. */
template <typename Duration>
LIBYANG_CPP_EXPORT std::string yangTimeFormat(const std::chrono::local_time<Duration>& timePoint)
{
    return std::format("{:" LIBYANG_CPP_TIME_FORMAT_NO_TZ "}", timePoint);
}
#endif

#ifdef TZ_H
/** @brief Converts a date::local_time to a string representing yang:date-and-time with unspecified TZ. */
template <typename Duration>
LIBYANG_CPP_EXPORT std::string yangTimeFormat(const date::local_time<Duration>& timePoint)
{
    return date::format(LIBYANG_CPP_TIME_FORMAT_NO_TZ, timePoint);
}
#endif

#ifdef LIBYANG_CPP_TIME_BACKEND_STL
/** @brief Converts a time point of a time with timezone to a string representing yang:date-and-time. */
template <typename Duration>
LIBYANG_CPP_EXPORT std::string yangTimeFormat(const std::chrono::zoned_time<Duration>& zonedTime)
{
    return std::format("{:" LIBYANG_CPP_TIME_FORMAT_TZ "}", zonedTime);
}
#endif

#ifdef TZ_H
/** @brief Converts a time point of a time with timezone (from date library) to a string representing yang:date-and-time. */
template <typename Duration>
LIBYANG_CPP_EXPORT std::string yangTimeFormat(const date::zoned_time<Duration>& zonedTime)
{
    return date::format(LIBYANG_CPP_TIME_FORMAT_TZ, zonedTime);
}
#endif

template <typename Duration>
/** @brief Converts a system_clock time to a string representing yang:date-and-time. */
LIBYANG_CPP_EXPORT std::string yangTimeFormat(const std::chrono::time_point<std::chrono::system_clock, Duration>& timePoint, TimezoneInterpretation tz)
{
    switch (tz) {
    case TimezoneInterpretation::Unspecified:
#if defined(LIBYANG_CPP_TIME_BACKEND_STL)
        return std::format("{:" LIBYANG_CPP_TIME_FORMAT_NO_TZ "}", timePoint);
#elif defined(LIBYANG_CPP_TIME_BACKEND_HHDATE)
        return date::format(LIBYANG_CPP_TIME_FORMAT_NO_TZ, timePoint);
#endif
    case TimezoneInterpretation::Local:
        const auto* tzdata = date_impl::current_zone();
        return yangTimeFormat(date_impl::zoned_time{tzdata, timePoint});
    }

    __builtin_unreachable();
}

/** @brief Converts a utc_clock time to a string representing yang:date-and-time. */
template <typename Duration>
LIBYANG_CPP_EXPORT std::string yangTimeFormat(const std::chrono::time_point<std::chrono::utc_clock, Duration>& timePoint)
{
#if defined(LIBYANG_CPP_TIME_BACKEND_STL)
    return std::format("{:" LIBYANG_CPP_TIME_FORMAT_UTC "}", timePoint);
#elif defined(LIBYANG_CPP_TIME_BACKEND_HHDATE)
    return date::format(LIBYANG_CPP_TIME_FORMAT_UTC, std::chrono::clock_cast<date::utc_clock>(timePoint));
#endif
}

/** @brief Converts a textual representation yang:date-and-time to std::time_point<Clock> */
template <typename Clock, typename Duration = Clock::duration>
LIBYANG_CPP_EXPORT std::chrono::time_point<Clock, Duration> fromYangTimeFormat(const std::string& timeStr)
{
    std::chrono::time_point<Clock, Duration> timePoint;
    std::istringstream iss(timeStr);

    if (!(iss >> date_impl::parse(LIBYANG_CPP_TIME_FORMAT_TZ, timePoint))) {
        throw std::invalid_argument("Invalid date for format string '"s + LIBYANG_CPP_TIME_FORMAT_TZ + "'");
    }

    return timePoint;
}

}
