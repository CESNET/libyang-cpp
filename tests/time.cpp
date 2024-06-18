/*
 * Copyright (C) 2024 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Tomáš Pecka <tomas.pecka@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <date/date.h>
#include <date/tz.h>
#include <doctest/doctest.h>
#include <libyang-cpp/Time.hpp>

TEST_CASE("Time utils")
{
    using namespace std::chrono_literals;
    using libyang::Timezone;
    using libyang::yangTimeFormat;
    using libyang::fromYangTimeFormat;

    auto palindromicTimeNoTZ = date::local_days{date::February / 22 / 2022} + 22h + 20min + 22s + 20ms;
    auto palindromicTime = date::make_zoned(date::locate_zone("Europe/Prague"), palindromicTimeNoTZ).get_sys_time();

    DOCTEST_SUBCASE("Print in different timezones")
    {
        REQUIRE(yangTimeFormat(palindromicTime, "Europe/Prague") == "2022-02-22T22:20:22.020+01:00");
        REQUIRE(yangTimeFormat(palindromicTime, "UTC") == "2022-02-22T21:20:22.020+00:00");
        REQUIRE(yangTimeFormat(palindromicTime, "Asia/Tokyo") == "2022-02-23T06:20:22.020+09:00");
        REQUIRE(yangTimeFormat(palindromicTime, "America/New_York") == "2022-02-22T16:20:22.020-05:00");
        REQUIRE(yangTimeFormat(palindromicTime, "Australia/Eucla") == "2022-02-23T06:05:22.020+08:45");
    }

    DOCTEST_SUBCASE("Timezone unspecified")
    {
        auto timeNoTz = date::local_days{date::June / 17 / 2024} + 13h + 58min + 02s + 1234us;
        auto time = date::make_zoned(date::locate_zone("Europe/Prague"), timeNoTz).get_sys_time();

        REQUIRE(yangTimeFormat(timeNoTz, Timezone::Unspecified) == "2024-06-17T13:58:02.001234-00:00");
        REQUIRE(yangTimeFormat<decltype(timeNoTz), std::chrono::seconds>(timeNoTz, Timezone::Unspecified) == "2024-06-17T13:58:02-00:00");
        REQUIRE(yangTimeFormat<decltype(timeNoTz), std::chrono::milliseconds>(timeNoTz, Timezone::Unspecified) == "2024-06-17T13:58:02.001-00:00");
        REQUIRE(yangTimeFormat<decltype(timeNoTz), std::chrono::microseconds>(timeNoTz, Timezone::Unspecified) == "2024-06-17T13:58:02.001234-00:00");
        REQUIRE(yangTimeFormat<decltype(timeNoTz), std::chrono::nanoseconds>(timeNoTz, Timezone::Unspecified) == "2024-06-17T13:58:02.001234000-00:00");

        REQUIRE(yangTimeFormat(time, Timezone::Unspecified) == "2024-06-17T11:58:02.001234-00:00");
        REQUIRE(yangTimeFormat<decltype(time), std::chrono::seconds>(time, Timezone::Unspecified) == "2024-06-17T11:58:02-00:00");
        REQUIRE(yangTimeFormat<decltype(time), std::chrono::milliseconds>(time, Timezone::Unspecified) == "2024-06-17T11:58:02.001-00:00");
        REQUIRE(yangTimeFormat<decltype(time), std::chrono::microseconds>(time, Timezone::Unspecified) == "2024-06-17T11:58:02.001234-00:00");
        REQUIRE(yangTimeFormat<decltype(time), std::chrono::nanoseconds>(time, Timezone::Unspecified) == "2024-06-17T11:58:02.001234000-00:00");
    }

    DOCTEST_SUBCASE("Precision")
    {
        DOCTEST_SUBCASE("Minute resolution")
        {
            auto timeNoTz = date::local_days{date::June / 17 / 2024} + 13h + 58min;
            auto time = date::make_zoned(date::locate_zone("Europe/Prague"), timeNoTz).get_sys_time();

            REQUIRE(yangTimeFormat(time, "Europe/Prague") == "2024-06-17T13:58:00+02:00");
            REQUIRE(yangTimeFormat<decltype(time), std::chrono::seconds>(time, "Europe/Prague") == "2024-06-17T13:58:00+02:00");
            REQUIRE(yangTimeFormat<decltype(time), std::chrono::milliseconds>(time, "Europe/Prague") == "2024-06-17T13:58:00.000+02:00");
            REQUIRE(yangTimeFormat<decltype(time), std::chrono::microseconds>(time, "Europe/Prague") == "2024-06-17T13:58:00.000000+02:00");
            REQUIRE(yangTimeFormat<decltype(time), std::chrono::nanoseconds>(time, "Europe/Prague") == "2024-06-17T13:58:00.000000000+02:00");
        }

        DOCTEST_SUBCASE("Seconds resolution")
        {
            auto timeNoTz = date::local_days{date::June / 17 / 2024} + 13h + 58min + 02s;
            auto time = date::make_zoned(date::locate_zone("Europe/Prague"), timeNoTz).get_sys_time();

            REQUIRE(yangTimeFormat(time, "Europe/Prague") == "2024-06-17T13:58:02+02:00");
            REQUIRE(yangTimeFormat<decltype(time), std::chrono::seconds>(time, "Europe/Prague") == "2024-06-17T13:58:02+02:00");
            REQUIRE(yangTimeFormat<decltype(time), std::chrono::milliseconds>(time, "Europe/Prague") == "2024-06-17T13:58:02.000+02:00");
            REQUIRE(yangTimeFormat<decltype(time), std::chrono::microseconds>(time, "Europe/Prague") == "2024-06-17T13:58:02.000000+02:00");
            REQUIRE(yangTimeFormat<decltype(time), std::chrono::nanoseconds>(time, "Europe/Prague") == "2024-06-17T13:58:02.000000000+02:00");
        }

        DOCTEST_SUBCASE("Milliseconds resolution")
        {
            auto timeNoTz = date::local_days{date::June / 17 / 2024} + 13h + 58min + 02s + 550ms;
            auto time = date::make_zoned(date::locate_zone("Europe/Prague"), timeNoTz).get_sys_time();

            REQUIRE(yangTimeFormat(time, "Europe/Prague") == "2024-06-17T13:58:02.550+02:00");
            REQUIRE(yangTimeFormat<decltype(time), std::chrono::seconds>(time, "Europe/Prague") == "2024-06-17T13:58:02+02:00");
            REQUIRE(yangTimeFormat<decltype(time), std::chrono::milliseconds>(time, "Europe/Prague") == "2024-06-17T13:58:02.550+02:00");
            REQUIRE(yangTimeFormat<decltype(time), std::chrono::microseconds>(time, "Europe/Prague") == "2024-06-17T13:58:02.550000+02:00");
            REQUIRE(yangTimeFormat<decltype(time), std::chrono::nanoseconds>(time, "Europe/Prague") == "2024-06-17T13:58:02.550000000+02:00");
        }

        DOCTEST_SUBCASE("Microseconds resolution")
        {
            auto timeNoTz = date::local_days{date::June / 17 / 2024} + 13h + 58min + 02s + 123456us;
            auto time = date::make_zoned(date::locate_zone("Europe/Prague"), timeNoTz).get_sys_time();

            REQUIRE(yangTimeFormat(time, "Europe/Prague") == "2024-06-17T13:58:02.123456+02:00");
            REQUIRE(yangTimeFormat<decltype(time), std::chrono::seconds>(time, "Europe/Prague") == "2024-06-17T13:58:02+02:00");
            REQUIRE(yangTimeFormat<decltype(time), std::chrono::milliseconds>(time, "Europe/Prague") == "2024-06-17T13:58:02.123+02:00");
            REQUIRE(yangTimeFormat<decltype(time), std::chrono::microseconds>(time, "Europe/Prague") == "2024-06-17T13:58:02.123456+02:00");
            REQUIRE(yangTimeFormat<decltype(time), std::chrono::nanoseconds>(time, "Europe/Prague") == "2024-06-17T13:58:02.123456000+02:00");
        }

        DOCTEST_SUBCASE("Nanoseconds resolution")
        {
            auto timeNoTz = date::local_days{date::June / 17 / 2024} + 13h + 58min + 02s + 123456789ns;
            auto time = date::make_zoned(date::locate_zone("Europe/Prague"), timeNoTz).get_sys_time();

            REQUIRE(yangTimeFormat(time, "Europe/Prague") == "2024-06-17T13:58:02.123456789+02:00");
            REQUIRE(yangTimeFormat<decltype(time), std::chrono::seconds>(time, "Europe/Prague") == "2024-06-17T13:58:02+02:00");
            REQUIRE(yangTimeFormat<decltype(time), std::chrono::milliseconds>(time, "Europe/Prague") == "2024-06-17T13:58:02.123+02:00");
            REQUIRE(yangTimeFormat<decltype(time), std::chrono::microseconds>(time, "Europe/Prague") == "2024-06-17T13:58:02.123456+02:00");
            REQUIRE(yangTimeFormat<decltype(time), std::chrono::nanoseconds>(time, "Europe/Prague") == "2024-06-17T13:58:02.123456789+02:00");
        }
    }

    DOCTEST_SUBCASE("Parsing")
    {
        REQUIRE(fromYangTimeFormat("2022-02-22T22:20:22.020000000+01:00") == palindromicTime);
        REQUIRE(fromYangTimeFormat("2022-02-22T21:20:22.020000000+00:00") == palindromicTime);
        REQUIRE(fromYangTimeFormat("2022-02-23T06:20:22.020000000+09:00") == palindromicTime);
        REQUIRE(fromYangTimeFormat("2022-02-22T16:20:22.020000000-05:00") == palindromicTime);
        REQUIRE(fromYangTimeFormat("2022-02-23T06:05:22.020000000+08:45") == palindromicTime);
    }
}
