/*
 * Copyright (C) 2024 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Tomáš Pecka <tomas.pecka@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <date/tz.h>
#include <doctest/doctest.h>
#include <libyang-cpp/Time.hpp>

TEST_CASE("Time utils")
{
    using libyang::TimezoneInterpretation;
    using libyang::yangTimeFormat;
    using libyang::fromYangTimeFormat;

    using namespace std::chrono_literals;

    DOCTEST_SUBCASE("STL structures")
    {
        using namespace std::chrono;

        const auto sys_time = std::chrono::sys_days{year(2021) / January / day(23)} + 06h + 5min + 23s + 20ms;
        REQUIRE(yangTimeFormat(sys_time, TimezoneInterpretation::Unspecified) == "2021-01-23T06:05:23.020-00:00");

        const auto utc_time = std::chrono::clock_cast<std::chrono::utc_clock>(sys_time);
        REQUIRE(yangTimeFormat(utc_time) == "2021-01-23T06:05:23.020+00:00");

        REQUIRE(fromYangTimeFormat<std::chrono::system_clock>("2021-01-23T06:05:23.020-00:00") == sys_time);
    }

    DOCTEST_SUBCASE("HowardHinnant/date")
    {
        using namespace date::literals;

        const auto loc_time = date::local_days{2021_y / date::January / 23} + 06h + 5min + 23s + 20ms;
        REQUIRE(yangTimeFormat(loc_time) == "2021-01-23T06:05:23.020-00:00");
        REQUIRE(yangTimeFormat(date::make_zoned(date::locate_zone("Europe/Prague"), loc_time)) == "2021-01-23T06:05:23.020+01:00");
    }

#if __cpp_lib_chrono >= 201907L && !LIBYANG_CPP_SKIP_STD_CHRONO_TZ
    DOCTEST_SUBCASE("Only C++20 with calendar and tz support")
    {
        using namespace std::chrono;

        const auto loc_time = std::chrono::local_days{year(2021) / June / day(23)} + 06h + 5min + 23s + 20ms;
        REQUIRE(yangTimeFormat(std::chrono::zoned_time{"Europe/Prague", loc_time}) == "2021-06-23T06:05:23.020+02:00");
        REQUIRE(yangTimeFormat(std::chrono::zoned_time{"Australia/Eucla", loc_time}) == "2021-06-23T06:05:23.020+08:45");
        REQUIRE(yangTimeFormat(loc_time) == "2021-06-23T06:05:23.020-00:00");
        REQUIRE(fromYangTimeFormat<std::chrono::local_t, std::chrono::nanoseconds>("2021-06-23T06:05:23.020-00:00") == loc_time);

        const auto sys_time = std::chrono::sys_days{year(2021) / January / day(23)} + 06h + 5min + 23s + 20ms;
        const auto utc_time = std::chrono::clock_cast<std::chrono::utc_clock>(sys_time);
        REQUIRE(fromYangTimeFormat<std::chrono::utc_clock>("2021-01-23T06:05:23.020+00:00") == utc_time);
    }
#endif
}
