/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <doctest/doctest.h>
#include <libyang-cpp/Context.hpp>
#include <libyang-cpp/utils/exception.hpp>
#include "example_schema.hpp"
#include "test_vars.hpp"
#include "pretty_printers.hpp"

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

const auto imported_module = R"(
    module importedModule {
      namespace "http://example.com";
      prefix "ab";

      leaf myLeaf {
        type string;
      }
    }
)";

const auto model_with_import = R"(
    module withImport {
      namespace "http://example.com";
      prefix "t";

      import importedModule {
        prefix "im";
      }

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
                format = libyang::SchemaFormat::YANG;
            }

            DOCTEST_SUBCASE("yin") {
                mod = valid_yin_model;
                format = libyang::SchemaFormat::YIN;
            }

            REQUIRE(ctx->parseModuleMem(mod, format).name() == "test");

            REQUIRE(ctx->getModule("test", nullptr)->name() == "test");
        }

        DOCTEST_SUBCASE("invalid") {
            format = libyang::SchemaFormat::YANG;
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
        ctx->parseModuleMem(valid_yang_model, libyang::SchemaFormat::YANG);

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

    DOCTEST_SUBCASE("Module::features")
    {
        ctx->setSearchDir(TESTS_DIR);
        auto mod = ctx->loadModule("mod1", nullptr, {
            "feature1",
            "feature2"
        });

        std::vector<std::string> expectedFeatures {
            "feature1",
            "feature2",
            "feature3",
        };
        std::vector<std::string> actualFeatures;
        for (const auto& feature : mod.features()) {
            actualFeatures.emplace_back(feature.name());
        }

        REQUIRE(actualFeatures == expectedFeatures);
    }

    DOCTEST_SUBCASE("Module::setImplemented")
    {
        ctx->setSearchDir(TESTS_DIR);
        auto mod = ctx->loadModule("mod1", nullptr, {});
        REQUIRE(!mod.featureEnabled("feature1"));
        REQUIRE(!mod.featureEnabled("feature2"));
        mod.setImplemented({{"feature1"}});
        REQUIRE(mod.featureEnabled("feature1"));
        REQUIRE(!mod.featureEnabled("feature2"));
        mod.setImplemented();
        REQUIRE(mod.featureEnabled("feature1"));
        REQUIRE(!mod.featureEnabled("feature2"));
        mod.setImplemented(libyang::AllFeatures{});
        REQUIRE(mod.featureEnabled("feature1"));
        REQUIRE(mod.featureEnabled("feature2"));
        REQUIRE(mod.featureEnabled("feature3"));
    }

    DOCTEST_SUBCASE("Context::modules")
    {
        ctx->setSearchDir(TESTS_DIR);
        ctx->loadModule("mod1", nullptr, {});
        ctx->parseModuleMem(valid_yang_model, libyang::SchemaFormat::YANG);
        auto modules = ctx->modules();
        REQUIRE(modules.size() == 8);
        REQUIRE(modules.at(0).name() == "ietf-yang-metadata");
        REQUIRE(modules.at(1).name() == "yang");
        REQUIRE(modules.at(2).name() == "ietf-inet-types");
        REQUIRE(modules.at(3).name() == "ietf-yang-types");
        REQUIRE(modules.at(4).name() == "ietf-datastores");
        REQUIRE(modules.at(5).name() == "ietf-yang-library");
        REQUIRE(modules.at(6).name() == "mod1");
        REQUIRE(modules.at(7).name() == "test");
    }

    DOCTEST_SUBCASE("Context::registerModuleCallback")
    {
        auto numCalled = 0;
        ctx->registerModuleCallback([&numCalled] (const char* modName, const char*, const char*, const char*) -> std::optional<libyang::ModuleInfo> {
            numCalled++;
            if (modName == std::string_view{"example-schema"}) {
                return libyang::ModuleInfo{
                    .data = example_schema,
                    .format = libyang::SchemaFormat::YANG
                };
            }

            return std::nullopt;
        });

        REQUIRE(ctx->loadModule("example-schema").name() == "example-schema");
        REQUIRE_THROWS_AS(ctx->loadModule("doesnt-exist"), libyang::Error);
        REQUIRE(numCalled == 2);
    }

    DOCTEST_SUBCASE("Module::implemented")
    {
        ctx->registerModuleCallback([] (const char* modName, const char*, const char*, const char*) -> std::optional<libyang::ModuleInfo> {
            if (modName == std::string_view{"withImport"}) {
                return libyang::ModuleInfo{
                    .data = model_with_import,
                    .format = libyang::SchemaFormat::YANG
                };
            }

            if (modName == std::string_view{"importedModule"}) {
                return libyang::ModuleInfo{
                    .data = imported_module,
                    .format = libyang::SchemaFormat::YANG
                };
            }
            return std::nullopt;
        });

        REQUIRE(ctx->loadModule("withImport").implemented());
        REQUIRE(!ctx->getModule("importedModule")->implemented());
    }

}
