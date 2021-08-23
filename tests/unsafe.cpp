/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <doctest/doctest.h>
#include <libyang/libyang.h>
#include <libyang-cpp/Context.hpp>
#include <libyang-cpp/utils/exception.hpp>
#include "example_schema.hpp"

TEST_CASE("Unsafe methods")
{
    DOCTEST_SUBCASE("wrapRawNode")
    {
        ly_ctx* ctx;
        ly_ctx_new(nullptr, 0, &ctx);
        auto ctx_deleter = std::unique_ptr<ly_ctx, decltype(&ly_ctx_destroy)>(ctx, ly_ctx_destroy);
        lys_parse_mem(ctx, example_schema, LYS_IN_YANG, nullptr);

        auto data = R"({ "example-schema:leafInt32": 32 })";

        lyd_node* node;
        lyd_parse_data_mem(ctx, data, LYD_JSON, 0, LYD_VALIDATE_PRESENT, &node);

        // The wrapped node takes ownership of the lyd_node* and deletes it on destruction of the class. The context
        // struct however is not managed and needs to be released manually (for example with a unique_ptr).
        auto wrapped = libyang::wrapRawNode(node);
        REQUIRE(wrapped.path() == "/example-schema:leafInt32");
    }
}
