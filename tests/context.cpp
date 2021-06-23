/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <doctest/doctest.h>
#include <libyang-cpp/Context.hpp>

const auto valid_yin_model = R"(
<?xml version="1.0" encoding="UTF-8"?>
<module name="test"
        xmlns="urn:ietf:params:xml:ns:yang:yin:1"
        xmlns:t="http://example.com">
  <namespace uri="http://example.com"/>
  <prefix value="t"/>
</module>
)";

const auto valid_yang_model = R"(
    module test {
      namespace "http://example.com";
      prefix "t";

      leaf someLeaf {
        type string;
      }
    }
)";

TEST_CASE("context")
{
    std::optional<libyang::Context> ctx{std::in_place};

    DOCTEST_SUBCASE("parseModuleMem")
    {
        const char* mod;
        libyang::SchemaFormat format;
        DOCTEST_SUBCASE("valid") {
            DOCTEST_SUBCASE("yang") {
                mod = valid_yang_model;
                format = libyang::SchemaFormat::Yang;
            }

            DOCTEST_SUBCASE("yin") {
                mod = valid_yin_model;
                format = libyang::SchemaFormat::Yin;
            }

            ctx->parseModuleMem(mod, format);
        }

        DOCTEST_SUBCASE("invalid") {
            format = libyang::SchemaFormat::Yang;
            mod = "blablabla";
            REQUIRE_THROWS_WITH_AS(ctx->parseModuleMem(mod, format), "Can't parse module (7)", std::runtime_error);
        }
    }

    DOCTEST_SUBCASE("context lifetime")
    {
        ctx->parseModuleMem(valid_yang_model, libyang::SchemaFormat::Yang);
        auto node = ctx->newPath("/test:someLeaf", "123");
        ctx.reset();
        // Node is still reachable.
        REQUIRE(node.path() == "/test:someLeaf");
    }
}
