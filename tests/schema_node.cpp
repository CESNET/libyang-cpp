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
#include <sstream>
#include "example_schema.hpp"
#include "test_vars.hpp"
#include "pretty_printers.hpp"

using namespace std::string_literals;

const auto type_module = R"(
module type_module {
    yang-version 1.1;
    namespace "http://example.com/";
    prefix ahoj;

    leaf leafWithDescription {
        type string;
        description "This is a description.";
    }

    leaf leafWithoutDescription {
        type string;
    }

    leaf myLeaf {
        type string;
    }

    list myList {
        key 'lol';
        leaf lol {
            type string;
        }

        leaf notKey {
            type string;
        }
    }

    leaf leafLref {
        type leafref {
            path "/ahoj:myList/lol";
        }
    }

    list twoKeyList {
        key 'first second';
        leaf first {
            type string;
        }

        leaf second {
            type string;
        }
    }

    leaf leafEnum {
        type enumeration {
            enum A {
                value 2;
            }

            enum B {
                value 5;
            }
        }
    }

    leaf leafEnum2 {
        type enumeration {
            enum A;
            enum B;
        }
    }

    identity food;

    identity fruit {
        base food;
    }

    identity apple {
        base fruit;
    }

    leaf meal {
        type identityref {
            base food;
        }
    }

    leaf currentLeaf {
        type string;
    }

    leaf deprecatedLeaf {
        status deprecated;
        type string;
    }

    leaf obsoleteLeaf {
        status obsolete;
        type string;
    }

    leaf configTrueLeaf {
        type string;
    }

    leaf configFalseLeaf {
        config false;
        type string;
    }

    leaf withDefaultValue {
        type string;
        default "AHOJ";
    }

    leaf withoutDefaultValue {
        type string;
    }

    leaf-list leafListString {
        type string;
    }

    leaf leafWithUnits {
        type int32;
        units "s";
    }

    leaf leafWithoutUnits {
        type int32;
    }

    leaf-list leaflistWithUnits {
        type int32;
        units "s";
    }

    leaf-list leaflistWithoutUnits {
        type int32;
    }
}
)";

TEST_CASE("SchemaNode")
{
    std::optional<libyang::Context> ctx{std::in_place};
    ctx->parseModuleMem(example_schema, libyang::SchemaFormat::YANG);
    ctx->parseModuleMem(type_module, libyang::SchemaFormat::YANG);

    DOCTEST_SUBCASE("context lifetime")
    {
        auto node = ctx->findPath("/example-schema:leafBinary");
        REQUIRE(node.path() == "/example-schema:leafBinary");
        ctx.reset();
        REQUIRE(node.path() == "/example-schema:leafBinary");
    }

    DOCTEST_SUBCASE("schema path doesn't exist")
    {
        REQUIRE_THROWS_WITH_AS(ctx->findPath("/example-schema:hi"), "Couldn't find schema node: /example-schema:hi", libyang::Error);
        REQUIRE_THROWS_WITH_AS(ctx->findPath("/oof:leafBinary"), "Couldn't find schema node: /oof:leafBinary", libyang::Error);
    }

    DOCTEST_SUBCASE("finding RPC output nodes")
    {
        REQUIRE_THROWS(ctx->findPath("example-schema:myRpc/outputLeaf", libyang::OutputNodes::No));
        REQUIRE(ctx->findPath("example-schema:myRpc/outputLeaf", libyang::OutputNodes::Yes).nodeType() == libyang::NodeType::Leaf);
    }

    DOCTEST_SUBCASE("DataNode::schema")
    {
        auto data = R"(
        {
            "example-schema:person": [
                {
                    "name": "Dan"
                }
            ]
        }
        )";
        auto node = ctx->parseDataMem(data, libyang::DataFormat::JSON);
        REQUIRE(node.path() == "/example-schema:person[name='Dan']");
        REQUIRE(node.schema().path() == "/example-schema:person");
    }

    DOCTEST_SUBCASE("SchemaNode::nodetype")
    {
        libyang::NodeType expected;
        const char* path;
        DOCTEST_SUBCASE("leaf") {
            path = "/type_module:myLeaf";
            expected = libyang::NodeType::Leaf;
        }

        DOCTEST_SUBCASE("list") {
            path = "/type_module:myList";
            expected = libyang::NodeType::List;
        }
        // TODO: add tests for other nodetypes, specifically schema-only nodetypes

        REQUIRE(ctx->findPath(path).nodeType() == expected);
    }

    DOCTEST_SUBCASE("SchemaNode::name")
    {
        REQUIRE(ctx->findPath("/example-schema:presenceContainer").name() == "presenceContainer");

        ctx->setSearchDir(TESTS_DIR);
        ctx->loadModule("augmentModule");

        REQUIRE(ctx->findPath("/importThis:myCont/augmentModule:myLeaf").name() == "myLeaf");
    }

    DOCTEST_SUBCASE("SchemaNode::description")
    {
        REQUIRE(ctx->findPath("type_module:leafWithDescription").description() == "This is a description.");
        REQUIRE(ctx->findPath("type_module:leafWithoutDescription").description() == std::nullopt);
    }

    DOCTEST_SUBCASE("SchemaNode::status")
    {
        REQUIRE(ctx->findPath("type_module:currentLeaf").status() == libyang::Status::Current);
        REQUIRE(ctx->findPath("type_module:deprecatedLeaf").status() == libyang::Status::Deprecated);
        REQUIRE(ctx->findPath("type_module:obsoleteLeaf").status() == libyang::Status::Obsolete);
    }

    DOCTEST_SUBCASE("SchemaNode::config")
    {
        REQUIRE(ctx->findPath("type_module:configTrueLeaf").config() == libyang::Config::True);
        REQUIRE(ctx->findPath("type_module:configFalseLeaf").config() == libyang::Config::False);
    }

    DOCTEST_SUBCASE("Container::isPresence")
    {
        REQUIRE(ctx->findPath("/example-schema:presenceContainer").asContainer().isPresence());
        REQUIRE(!ctx->findPath("/example-schema:first").asContainer().isPresence());
    }

    DOCTEST_SUBCASE("Leaf::isKey")
    {
        REQUIRE(ctx->findPath("type_module:myList/lol").asLeaf().isKey());
        REQUIRE(!ctx->findPath("type_module:myLeaf").asLeaf().isKey());
    }

    DOCTEST_SUBCASE("Leaf::defaultValueStr")
    {
        REQUIRE(ctx->findPath("type_module:withDefaultValue").asLeaf().defaultValueStr() == "AHOJ");
        REQUIRE(!ctx->findPath("type_module:withoutDefaultValue").asLeaf().defaultValueStr());
    }

    DOCTEST_SUBCASE("Leaf::type")
    {
        DOCTEST_SUBCASE("string") {
            auto type = ctx->findPath("type_module:myList/lol").asLeaf().valueType();
            REQUIRE(type.base() == libyang::LeafBaseType::String);
        }

        DOCTEST_SUBCASE("enum") {
            auto enums = ctx->findPath("type_module:leafEnum").asLeaf().valueType().asEnum().items();

            REQUIRE(enums.at(0).name == "A");
            REQUIRE(enums.at(0).value == 2);
            REQUIRE(enums.at(1).name == "B");
            REQUIRE(enums.at(1).value == 5);
            enums = ctx->findPath("type_module:leafEnum2").asLeaf().valueType().asEnum().items();

            REQUIRE(enums.at(0).name == "A");
            REQUIRE(enums.at(0).value == 0);
            REQUIRE(enums.at(1).name == "B");
            REQUIRE(enums.at(1).value == 1);
        }

        DOCTEST_SUBCASE("identityref") {
            auto bases = ctx->findPath("type_module:meal").asLeaf().valueType().asIdentityRef().bases();
            std::vector<std::string> expectedBases{"food"};
            std::vector<std::string> actualBases;
            for (const auto& it : bases) {
                actualBases.emplace_back(it.name());
            }

            REQUIRE(expectedBases == actualBases);

            std::vector<std::string> expectedDerived{"fruit"};
            std::vector<std::string> actualDerived;
            for (const auto& it : bases) {
                for (const auto& der : it.derived()) {
                    actualDerived.emplace_back(der.name());
                }
            }

            REQUIRE(expectedDerived == actualDerived);
        }

        DOCTEST_SUBCASE("leafref") {
            REQUIRE(ctx->findPath("/type_module:leafLref").asLeaf().valueType().asLeafRef().path() == "/ahoj:myList/lol");
        }
    }

    DOCTEST_SUBCASE("LeafList::type")
    {
        REQUIRE(ctx->findPath("type_module:leafListString").asLeafList().valueType().base() == libyang::LeafBaseType::String);
    }

    DOCTEST_SUBCASE("Leaf::units")
    {
        REQUIRE(ctx->findPath("type_module:leafWithUnits").asLeaf().units() == "s");
        REQUIRE(ctx->findPath("type_module:leafWithoutUnits").asLeaf().units() == std::nullopt);
    }

    DOCTEST_SUBCASE("LeafList::units")
    {
        REQUIRE(ctx->findPath("type_module:leaflistWithUnits").asLeafList().units() == "s");
        REQUIRE(ctx->findPath("type_module:leaflistWithoutUnits").asLeafList().units() == std::nullopt);
    }

    DOCTEST_SUBCASE("List::keys")
    {
        auto keys = ctx->findPath("type_module:myList").asList().keys();
        REQUIRE(keys.size() == 1);
        REQUIRE(keys.front().path() == "/type_module:myList/lol");

        keys = ctx->findPath("type_module:twoKeyList").asList().keys();
        REQUIRE(keys.size() == 2);
        REQUIRE(keys[0].path() == "/type_module:twoKeyList/first");
        REQUIRE(keys[1].path() == "/type_module:twoKeyList/second");
    }
}
