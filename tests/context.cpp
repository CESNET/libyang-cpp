/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <doctest/doctest.h>
#include <libyang-cpp/Context.hpp>
#include "test_vars.hpp"

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

            REQUIRE(ctx->parseModuleMem(mod, format).name() == "test");

            REQUIRE(ctx->getModule("test", nullptr)->name() == "test");
        }

        DOCTEST_SUBCASE("invalid") {
            format = libyang::SchemaFormat::Yang;
            mod = "blablabla";
            REQUIRE_THROWS_WITH_AS(ctx->parseModuleMem(mod, format), "Can't parse module (7)", std::runtime_error);
        }
    }

    DOCTEST_SUBCASE("Loading modules by name")
    {
        DOCTEST_SUBCASE("module exists") {
            ctx->setSearchDir(TESTS_DIR);
            auto mod = ctx->loadModule("mod1", nullptr, {
                "feature1",
                "feature2"
            });

            REQUIRE(mod.name() == "mod1");
            REQUIRE(mod.featureEnabled("feature1"));
            REQUIRE(mod.featureEnabled("feature2"));
            REQUIRE(!mod.featureEnabled("feature3"));
            REQUIRE_THROWS(mod.featureEnabled("invalid"));
        }

        DOCTEST_SUBCASE("module does not exist") {
            REQUIRE_THROWS(ctx->loadModule("invalid"));
        }
    }

    DOCTEST_SUBCASE("context lifetime")
    {
        ctx->parseModuleMem(valid_yang_model, libyang::SchemaFormat::Yang);

        DOCTEST_SUBCASE("Data nodes")
        {
            auto node = ctx->newPath("/test:someLeaf", "123");
            ctx.reset();
            // Node is still reachable.
            REQUIRE(node.path() == "/test:someLeaf");

        }

        DOCTEST_SUBCASE("Modules")
        {
            auto mod = ctx->getModule("test");
            ctx.reset();
            // Module is still reachable.
            REQUIRE(mod->name() == "test");
        }
    }
}
