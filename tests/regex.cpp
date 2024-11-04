/*
 * Copyright (C) 2025 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Jan Kundrát <jan.kundrat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <doctest/doctest.h>
#include <libyang-cpp/Regex.hpp>
#include <libyang-cpp/Utils.hpp>

TEST_CASE("regex")
{
    using libyang::Regex;
    using namespace std::string_literals;

    // FIXME: this can be inlined once we're on doctest>=2.4.9. Older versions fail the exception match.
    const auto err = R"(Regular expression "\" is not valid ("": \ at end of pattern).: LY_EVALID)"s;
    REQUIRE_THROWS_WITH_AS(Regex{"\\"}, err.c_str(), libyang::ErrorWithCode);

    Regex re{"ahoj"};
    REQUIRE(re.matches("ahoj"));
    REQUIRE(!re.matches("cau"));
    REQUIRE(re.matches("ahoj")); // test repeated calls as well
    REQUIRE(!re.matches("oj"));
    REQUIRE(!re.matches("aho"));

    // Testing runtime errors during pattern *matching* is tricky. There's a limit on backtracking,
    // so testing a pattern like x+x+y on an obscenely long string of "x" characters *will* do the trick, eventually,
    // but the PCRE2 library has a default limit of 10M attempts. That's a VERY big number to hit during a test :(.
}
