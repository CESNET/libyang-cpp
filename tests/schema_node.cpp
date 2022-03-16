/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <doctest/doctest.h>
#include <libyang-cpp/Context.hpp>
#include <libyang-cpp/Utils.hpp>
#include <sstream>
#include "example_schema.hpp"
#include "pretty_printers.hpp"
#include "test_vars.hpp"

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

    leaf leafBits {
        type bits {
            bit one;
            bit two;
            bit three;
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

    leaf leafUnion {
        type union {
            type string;
            type int32;
            type boolean;
        }
    }
}
)";

TEST_CASE("SchemaNode")
{
    std::optional<libyang::Context> ctx{std::in_place, std::nullopt, libyang::ContextOptions::NoYangLibrary};
    std::optional<libyang::Context> ctxWithParsed{std::in_place, std::nullopt, libyang::ContextOptions::SetPrivParsed | libyang::ContextOptions::NoYangLibrary};
    ctx->parseModuleMem(example_schema, libyang::SchemaFormat::YANG);
    ctx->parseModuleMem(type_module, libyang::SchemaFormat::YANG);
    ctxWithParsed->parseModuleMem(example_schema, libyang::SchemaFormat::YANG);
    ctxWithParsed->parseModuleMem(type_module, libyang::SchemaFormat::YANG);

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
        REQUIRE_THROWS(ctx->findPath("/example-schema:myRpc/outputLeaf", libyang::OutputNodes::No));
        REQUIRE(ctx->findPath("/example-schema:myRpc/outputLeaf", libyang::OutputNodes::Yes).nodeType() == libyang::NodeType::Leaf);
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
        REQUIRE(node->path() == "/example-schema:person[name='Dan']");
        REQUIRE(node->schema().path() == "/example-schema:person");
    }

    DOCTEST_SUBCASE("SchemaNode::nodetype")
    {
        libyang::NodeType expected;
        const char* path;
        DOCTEST_SUBCASE("leaf")
        {
            path = "/type_module:myLeaf";
            expected = libyang::NodeType::Leaf;
        }

        DOCTEST_SUBCASE("list")
        {
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
        REQUIRE(ctx->findPath("/type_module:leafWithDescription").description() == "This is a description.");
        REQUIRE(ctx->findPath("/type_module:leafWithoutDescription").description() == std::nullopt);
    }

    DOCTEST_SUBCASE("SchemaNode::status")
    {
        REQUIRE(ctx->findPath("/type_module:currentLeaf").status() == libyang::Status::Current);
        REQUIRE(ctx->findPath("/type_module:deprecatedLeaf").status() == libyang::Status::Deprecated);
        REQUIRE(ctx->findPath("/type_module:obsoleteLeaf").status() == libyang::Status::Obsolete);
    }

    DOCTEST_SUBCASE("SchemaNode::config")
    {
        REQUIRE(ctx->findPath("/type_module:configTrueLeaf").config() == libyang::Config::True);
        REQUIRE(ctx->findPath("/type_module:configFalseLeaf").config() == libyang::Config::False);
    }

    DOCTEST_SUBCASE("SchemaNode::child")
    {
        REQUIRE(ctx->findPath("/type_module:twoKeyList").child()->name() == "first");
        REQUIRE(!ctx->findPath("/type_module:myLeaf").child().has_value());
    }

    DOCTEST_SUBCASE("SchemaNode::findXPath")
    {
        auto list = ctx->findXPath("/type_module:twoKeyList");
        REQUIRE(list.begin()->path() == "/type_module:twoKeyList");
        REQUIRE(++list.begin() == list.end());

        // You don't have to specify key predicates
        REQUIRE(ctx->findXPath("/type_module:twoKeyList/first").front().path() == "/type_module:twoKeyList/first");
    }

    DOCTEST_SUBCASE("SchemaNode::parent")
    {
        REQUIRE(ctx->findPath("/type_module:twoKeyList/first").parent()->name() == "twoKeyList");
        REQUIRE(!ctx->findPath("/type_module:twoKeyList").parent().has_value());
    }

    DOCTEST_SUBCASE("Container::isPresence")
    {
        REQUIRE(ctx->findPath("/example-schema:presenceContainer").asContainer().isPresence());
        REQUIRE(!ctx->findPath("/example-schema:first").asContainer().isPresence());
    }

    DOCTEST_SUBCASE("Leaf::isKey")
    {
        REQUIRE(ctx->findPath("/type_module:myList/lol").asLeaf().isKey());
        REQUIRE(!ctx->findPath("/type_module:myLeaf").asLeaf().isKey());
    }

    DOCTEST_SUBCASE("Leaf::defaultValueStr")
    {
        REQUIRE(ctx->findPath("/type_module:withDefaultValue").asLeaf().defaultValueStr() == "AHOJ");
        REQUIRE(!ctx->findPath("/type_module:withoutDefaultValue").asLeaf().defaultValueStr());
    }

    DOCTEST_SUBCASE("Leaf::type")
    {
        DOCTEST_SUBCASE("string")
        {
            auto type = ctx->findPath("/type_module:myList/lol").asLeaf().valueType();
            REQUIRE(type.base() == libyang::LeafBaseType::String);

            auto typeWithParsed = ctxWithParsed->findPath("/type_module:myList/lol").asLeaf().valueType();
            REQUIRE(typeWithParsed.base() == libyang::LeafBaseType::String);
        }

        DOCTEST_SUBCASE("enum")
        {
            auto enums = ctx->findPath("/type_module:leafEnum").asLeaf().valueType().asEnum().items();

            REQUIRE(enums.at(0).name == "A");
            REQUIRE(enums.at(0).value == 2);
            REQUIRE(enums.at(1).name == "B");
            REQUIRE(enums.at(1).value == 5);
            enums = ctx->findPath("/type_module:leafEnum2").asLeaf().valueType().asEnum().items();

            REQUIRE(enums.at(0).name == "A");
            REQUIRE(enums.at(0).value == 0);
            REQUIRE(enums.at(1).name == "B");
            REQUIRE(enums.at(1).value == 1);
        }

        DOCTEST_SUBCASE("bits")
        {
            auto bits = ctx->findPath("/type_module:leafBits").asLeaf().valueType().asBits().items();
            REQUIRE(bits.size() == 3);
            REQUIRE(bits.at(0).name == "one");
            REQUIRE(bits.at(1).name == "two");
            REQUIRE(bits.at(2).name == "three");
            REQUIRE(bits.at(0).position == 0);
            REQUIRE(bits.at(1).position == 1);
            REQUIRE(bits.at(2).position == 2);
        }

        DOCTEST_SUBCASE("identityref")
        {
            auto bases = ctx->findPath("/type_module:meal").asLeaf().valueType().asIdentityRef().bases();
            std::vector<std::pair<std::string, std::string>> expectedBases{{"type_module", "food"}};
            std::vector<std::pair<std::string, std::string>> actualBases;
            for (const auto& it : bases) {
                actualBases.emplace_back(it.module().name(), it.name());
            }

            REQUIRE(expectedBases == actualBases);

            std::vector<std::pair<std::string, std::string>> expectedDerived{{"type_module", "fruit"}};
            std::vector<std::pair<std::string, std::string>> actualDerived;
            for (const auto& it : bases) {
                for (const auto& der : it.derived()) {
                    actualDerived.emplace_back(der.module().name(), der.name());
                }
            }

            REQUIRE(expectedDerived == actualDerived);
        }

        DOCTEST_SUBCASE("leafref")
        {
            auto lref = ctx->findPath("/type_module:leafLref").asLeaf().valueType().asLeafRef();
            REQUIRE(lref.path() == "/ahoj:myList/lol");
            REQUIRE(lref.resolvedType().base() == libyang::LeafBaseType::String);
        }

        DOCTEST_SUBCASE("union")
        {
            auto types = ctx->findPath("/type_module:leafUnion").asLeaf().valueType().asUnion().types();
            REQUIRE(types.at(0).base() == libyang::LeafBaseType::String);
            REQUIRE(types.at(1).base() == libyang::LeafBaseType::Int32);
            REQUIRE(types.at(2).base() == libyang::LeafBaseType::Bool);

            // Also check whether the types in the vector have the parsed info filled in.
            auto typesWithParsed = ctxWithParsed->findPath("/type_module:leafUnion").asLeaf().valueType().asUnion().types();
            REQUIRE(typesWithParsed.at(0).name() == "string");
            REQUIRE(typesWithParsed.at(1).name() == "int32");
            REQUIRE(typesWithParsed.at(2).name() == "boolean");
        }
    }

    DOCTEST_SUBCASE("LeafList::type")
    {
        REQUIRE(ctx->findPath("/type_module:leafListString").asLeafList().valueType().base() == libyang::LeafBaseType::String);
    }

    DOCTEST_SUBCASE("Leaf::units")
    {
        REQUIRE(ctx->findPath("/type_module:leafWithUnits").asLeaf().units() == "s");
        REQUIRE(ctx->findPath("/type_module:leafWithoutUnits").asLeaf().units() == std::nullopt);
    }

    DOCTEST_SUBCASE("LeafList::units")
    {
        REQUIRE(ctx->findPath("/type_module:leaflistWithUnits").asLeafList().units() == "s");
        REQUIRE(ctx->findPath("/type_module:leaflistWithoutUnits").asLeafList().units() == std::nullopt);
    }

    DOCTEST_SUBCASE("List::keys")
    {
        auto keys = ctx->findPath("/type_module:myList").asList().keys();
        REQUIRE(keys.size() == 1);
        REQUIRE(keys.front().path() == "/type_module:myList/lol");

        keys = ctx->findPath("/type_module:twoKeyList").asList().keys();
        REQUIRE(keys.size() == 2);
        REQUIRE(keys[0].path() == "/type_module:twoKeyList/first");
        REQUIRE(keys[1].path() == "/type_module:twoKeyList/second");
    }

    DOCTEST_SUBCASE("RPC")
    {
        auto rpc = ctx->findPath("/example-schema:myRpc");
        REQUIRE(rpc.asActionRpc().child()->name() == "input");
        REQUIRE(rpc.asActionRpc().input().child()->name() == "inputLeaf");
        REQUIRE(rpc.asActionRpc().output().child()->name() == "outputLeaf");
    }

    DOCTEST_SUBCASE("childInstantiables")
    {
        std::vector<std::string> expectedPaths;
        std::optional<libyang::ChildInstanstiables> children;

        DOCTEST_SUBCASE("SchemaNode::childInstantiables")
        {
            expectedPaths = {
                "/type_module:myList/lol",
                "/type_module:myList/notKey"
            };

            children = ctx->findPath("/type_module:myList").childInstantiables();

        }

        DOCTEST_SUBCASE("Module::childInstantiables")
        {
            expectedPaths = {
                "/type_module:leafWithDescription",
                "/type_module:leafWithoutDescription",
                "/type_module:myLeaf",
                "/type_module:myList",
                "/type_module:leafLref",
                "/type_module:twoKeyList",
                "/type_module:leafEnum",
                "/type_module:leafEnum2",
                "/type_module:leafBits",
                "/type_module:meal",
                "/type_module:currentLeaf",
                "/type_module:deprecatedLeaf",
                "/type_module:obsoleteLeaf",
                "/type_module:configTrueLeaf",
                "/type_module:configFalseLeaf",
                "/type_module:withDefaultValue",
                "/type_module:withoutDefaultValue",
                "/type_module:leafListString",
                "/type_module:leafWithUnits",
                "/type_module:leafWithoutUnits",
                "/type_module:leaflistWithUnits",
                "/type_module:leaflistWithoutUnits",
                "/type_module:leafUnion"
            };
            children = ctx->getModule("type_module")->childInstantiables();
        }

        std::vector<std::string> actualPaths;
        for (const auto& child : *children) {
            actualPaths.emplace_back(child.path());
        }

        REQUIRE(expectedPaths == actualPaths);
    }

    DOCTEST_SUBCASE("SchemaNode::childrenDfs")
    {
        std::vector<std::string> expectedPaths;

        const char* path;

        DOCTEST_SUBCASE("twoKeyList")
        {
            expectedPaths = {
                "/type_module:twoKeyList",
                "/type_module:twoKeyList/first",
                "/type_module:twoKeyList/second"
            };

            path = "/type_module:twoKeyList";
        }

        DOCTEST_SUBCASE("DFS on a leaf")
        {
            expectedPaths = {
                "/type_module:currentLeaf",
            };

            path = "/type_module:currentLeaf";
        }

        std::vector<std::string> actualPaths;
        for (const auto& it : ctx->findPath(path).childrenDfs()) {
            actualPaths.emplace_back(it.path());
        }

        REQUIRE(actualPaths == expectedPaths);
    }

    DOCTEST_SUBCASE("SchemaNode::module")
    {
        REQUIRE(ctx->findPath("/type_module:currentLeaf").module().name() == "type_module");
    }

    DOCTEST_SUBCASE("Type::name")
    {
        REQUIRE(ctxWithParsed->findPath("/example-schema:typedefedLeafInt").asLeaf().valueType().name() == "myTypeInt");
        REQUIRE_THROWS_WITH_AS(ctx->findPath("/example-schema:typedefedLeafInt").asLeaf().valueType().name(), "Context not created with libyang::ContextOptions::SetPrivParsed", libyang::Error);
    }

    DOCTEST_SUBCASE("Type::description")
    {
        REQUIRE(ctxWithParsed->findPath("/example-schema:typedefedLeafInt").asLeaf().valueType().description() == "An int32 typedef.");
        REQUIRE_THROWS_WITH_AS(ctx->findPath("/example-schema:typedefedLeafInt").asLeaf().valueType().description(), "Context not created with libyang::ContextOptions::SetPrivParsed", libyang::Error);
    }
}
