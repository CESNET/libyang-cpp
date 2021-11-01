/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <cstring>
#include <doctest/doctest.h>
#include <libyang-cpp/String.hpp>
#include "pretty_printers.hpp"


TEST_CASE("libyang::String")
{
    auto createString = [] (const char* str) { return libyang::String{strdup(str)}; };
    REQUIRE((createString("aaa") <=> createString("bbb")) == std::strong_ordering::less);
    REQUIRE((createString("bbb") <=> createString("aaa") == std::strong_ordering::greater));
    REQUIRE((createString("aaa") <=> createString("aaa") == std::strong_ordering::equal));
}
