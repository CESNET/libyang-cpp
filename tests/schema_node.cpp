/*
 * Copyright (container) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <doctest/doctest.h>
#include <libyang-cpp/Context.hpp>
#include <libyang-cpp/Type.hpp>
#include <libyang-cpp/Utils.hpp>
#include "example_schema.hpp"
#include "pretty_printers.hpp"
#include "test_vars.hpp"

using namespace std::string_literals;

const auto type_module = R"(
module type_module {
    yang-version 1.1;
    namespace "http://example.com/custom-prefix";
    prefix custom-prefix;

    leaf leafBinary {
        type binary;
    }

    leaf leafBits {
        type bits {
            bit one;
            bit two;
            bit three;
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

    leaf leafNumber {
        type int32;
    }

    leaf leafRef {
        type leafref {
            path "/custom-prefix:listAdvancedWithOneKey/lol";
        }
    }

    leaf leafString {
        type string;
    }

    leaf leafUnion {
        type union {
            type string;
            type int32;
            type boolean;
        }
    }

    identity food;

    identity fruit {
        base food;
    }

    identity apple {
        base fruit;
    }

    identity meat {
        base food;
    }

    identity fruit-and-meat-mix {
        base fruit;
        base meat;
    }

    leaf meal {
        type identityref {
            base food;
        }
    }

    leaf leafWithConfigFalse {
        config false;
        type string;
    }

    leaf leafWithDefaultValue {
        type string;
        default "custom-prefix";
    }

    leaf leafWithDescription {
        type string;
        description "This is a description.";
    }

    leaf leafWithMandatoryTrue {
        mandatory true;
        type string;
    }

    leaf leafWithStatusDeprecated {
        status deprecated;
        type string;
    }

    leaf leafWithStatusObsolete {
        status obsolete;
        type string;
    }

    leaf leafWithUnits {
        type int32;
        units "s";
    }

    leaf-list leafListBasic {
        type string;
    }

    leaf-list leafListWithMinMaxElements {
        type int32;
        min-elements 1;
        max-elements 5;
    }

    leaf-list leafListWithUnits {
        type int32;
        units "s";
    }

    list listBasic {
        key 'primary-key';

        leaf primary-key {
            type string;
        }
    }

    list listAdvancedWithOneKey {
        key 'lol';
        leaf lol {
            type string;
        }

        leaf notKey1 {
            type string {
              length "10 .. 20 | 50 .. 100 | 255";
              pattern "fo+";
              pattern "XXX" {
                description "yay";
                error-app-tag "x-XXX-failed";
                error-message "hard to fail this one";
                modifier invert-match;
              }
            }
        }

        leaf notKey2 {
            type string {
                length "min .. max" {
                    description "yay";
                    error-app-tag "x-XXX-failed";
                    error-message "hard to fail this one";
              }
            }
        }

        leaf notKey3 {
            type binary {
              length "10 .. 20 | 50 .. 100 | 255";
            }
        }

        leaf notKey4 {
            type binary {
                length "min .. max" {
                    description "yay";
                    error-app-tag "x-XXX-failed";
                    error-message "hard to fail this one";
              }
            }
        }
    }

    list listAdvancedWithTwoKey {
        key 'first second';
        leaf first {
            type string;
        }

        leaf second {
            type string;
        }
    }

    list listWithMinMaxElements {
        key 'primary-key';
        min-elements 1;
        max-elements 5;

        leaf primary-key {
            type string;
        }
    }

    container numeric {
        leaf i8 {
            type int8 {
                range "-20..-10 | -5 | 10..100" {
                    description "yay";
                    error-app-tag "this-is-a-nasty-limit";
                    error-message "flip a coin next time";
                }
            }
        }

        leaf i16 {
            type int16 {
                range "253..254";
            }
        }

        leaf i32 {
            type int32 {
                range "253..254";
            }
        }

        leaf i64 {
            type int64 {
                range "253..254";
            }
        }

        leaf deci {
            type decimal64 {
                fraction-digits 6;
                range "-123.456..666.042";
            }
        }

        leaf deci1 {
            type decimal64 {
                fraction-digits 1;
            }
        }

        leaf u8 {
            type uint8 {
                range "253..254";
            }
        }

        leaf u16 {
            type uint16 {
                range "253..254";
            }
        }

        leaf u32 {
            type uint32 {
                range "253..254";
            }
        }

        leaf u64 {
            type uint64 {
                range "253..254";
            }
        }
    }

    container container {
        container x {
            leaf x1 {
                type string;
            }
            leaf x2 {
                type string;
            }
        }
        container y {
        }
        container z {
            leaf z1 {
                type string;
            }
        }
    }

    container containerWithMandatoryChild {
        leaf leafWithMandatoryTrue {
            mandatory true;
            type string;
        }
    }
}
)"s;

TEST_CASE("SchemaNode")
{
    std::optional<libyang::Context> ctx{std::in_place, std::nullopt, libyang::ContextOptions::NoYangLibrary};
    std::optional<libyang::Context> ctxWithParsed{std::in_place, std::nullopt, libyang::ContextOptions::SetPrivParsed | libyang::ContextOptions::NoYangLibrary};
    ctx->parseModule(example_schema, libyang::SchemaFormat::YANG);
    ctx->parseModule(type_module, libyang::SchemaFormat::YANG);
    ctxWithParsed->parseModule(example_schema, libyang::SchemaFormat::YANG);
    ctxWithParsed->parseModule(type_module, libyang::SchemaFormat::YANG);

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
            ],
            "type_module:containerWithMandatoryChild": {
                "leafWithMandatoryTrue": "test-string"
            },
            "type_module:leafWithMandatoryTrue": "test-string",
            "type_module:leafListWithMinMaxElements": [123],
            "type_module:listWithMinMaxElements": [{"primary-key": "123"}]
        }
        )"s;
        auto node = ctx->parseData(data, libyang::DataFormat::JSON);
        REQUIRE(node->path() == "/example-schema:person[name='Dan']");
        REQUIRE(node->schema().path() == "/example-schema:person");
    }

    DOCTEST_SUBCASE("SchemaNode::child")
    {
        REQUIRE(ctx->findPath("/type_module:listAdvancedWithTwoKey").child()->name() == "first");
        REQUIRE(!ctx->findPath("/type_module:leafString").child().has_value());
    }

    DOCTEST_SUBCASE("SchemaNode::config")
    {
        REQUIRE(ctx->findPath("/type_module:leafString").config() == libyang::Config::True);
        REQUIRE(ctx->findPath("/type_module:leafWithConfigFalse").config() == libyang::Config::False);
    }

    DOCTEST_SUBCASE("SchemaNode::description")
    {
        REQUIRE(ctx->findPath("/type_module:leafWithDescription").description() == "This is a description.");
        REQUIRE(ctx->findPath("/type_module:leafString").description() == std::nullopt);
    }

    DOCTEST_SUBCASE("SchemaNode::findXPath")
    {
        auto list = ctx->findXPath("/type_module:listAdvancedWithTwoKey");
        REQUIRE(list.begin()->path() == "/type_module:listAdvancedWithTwoKey");
        REQUIRE(++list.begin() == list.end());

        // You don't have to specify key predicates
        REQUIRE(ctx->findXPath("/type_module:listAdvancedWithTwoKey/first").front().path() == "/type_module:listAdvancedWithTwoKey/first");
    }

    DOCTEST_SUBCASE("SchemaNode::isInput")
    {
        REQUIRE(ctx->findPath("/example-schema:myRpc/inputLeaf").isInput());
        REQUIRE(!ctx->findPath("/example-schema:myRpc/outputLeaf", libyang::OutputNodes::Yes).isInput());
        REQUIRE(!ctx->findPath("/type_module:leafString").isInput());
    }

    DOCTEST_SUBCASE("SchemaNode::module")
    {
        REQUIRE(ctx->findPath("/type_module:leafString").module().name() == "type_module");
    }


    DOCTEST_SUBCASE("SchemaNode::name")
    {
        REQUIRE(ctx->findPath("/example-schema:presenceContainer").name() == "presenceContainer");

        ctx->setSearchDir(TESTS_DIR);
        ctx->loadModule("augmentModule");

        REQUIRE(ctx->findPath("/importThis:myCont/augmentModule:myLeaf").name() == "myLeaf");
    }

    DOCTEST_SUBCASE("SchemaNode::nodetype")
    {
        libyang::NodeType expected;
        const char* path;
        DOCTEST_SUBCASE("leaf")
        {
            path = "/type_module:leafString";
            expected = libyang::NodeType::Leaf;
        }

        DOCTEST_SUBCASE("list")
        {
            path = "/type_module:listAdvancedWithOneKey";
            expected = libyang::NodeType::List;
        }
        // TODO: add tests for other nodetypes, specifically schema-only nodetypes

        REQUIRE(ctx->findPath(path).nodeType() == expected);
    }

    DOCTEST_SUBCASE("SchemaNode::parent")
    {
        REQUIRE(ctx->findPath("/type_module:listAdvancedWithTwoKey/first").parent()->name() == "listAdvancedWithTwoKey");
        REQUIRE(!ctx->findPath("/type_module:listAdvancedWithTwoKey").parent().has_value());
    }

    DOCTEST_SUBCASE("SchemaNode::status")
    {
        REQUIRE(ctx->findPath("/type_module:leafString").status() == libyang::Status::Current);
        REQUIRE(ctx->findPath("/type_module:leafWithStatusDeprecated").status() == libyang::Status::Deprecated);
        REQUIRE(ctx->findPath("/type_module:leafWithStatusObsolete").status() == libyang::Status::Obsolete);
    }

    DOCTEST_SUBCASE("childInstantiables")
    {
        std::vector<std::string> expectedPaths;
        std::optional<libyang::ChildInstanstiables> children;

        DOCTEST_SUBCASE("SchemaNode::childInstantiables")
        {
            expectedPaths = {
                "/type_module:listAdvancedWithOneKey/lol",
                "/type_module:listAdvancedWithOneKey/notKey1",
                "/type_module:listAdvancedWithOneKey/notKey2",
                "/type_module:listAdvancedWithOneKey/notKey3",
                "/type_module:listAdvancedWithOneKey/notKey4",
            };

            children = ctx->findPath("/type_module:listAdvancedWithOneKey").childInstantiables();
        }

        DOCTEST_SUBCASE("Module::childInstantiables")
        {
            expectedPaths = {
                "/type_module:leafBinary",
                "/type_module:leafBits",
                "/type_module:leafEnum",
                "/type_module:leafEnum2",
                "/type_module:leafNumber",
                "/type_module:leafRef",
                "/type_module:leafString",
                "/type_module:leafUnion",
                "/type_module:meal",
                "/type_module:leafWithConfigFalse",
                "/type_module:leafWithDefaultValue",
                "/type_module:leafWithDescription",
                "/type_module:leafWithMandatoryTrue",
                "/type_module:leafWithStatusDeprecated",
                "/type_module:leafWithStatusObsolete",
                "/type_module:leafWithUnits",
                "/type_module:leafListBasic",
                "/type_module:leafListWithMinMaxElements",
                "/type_module:leafListWithUnits",
                "/type_module:listBasic",
                "/type_module:listAdvancedWithOneKey",
                "/type_module:listAdvancedWithTwoKey",
                "/type_module:listWithMinMaxElements",
                "/type_module:numeric",
                "/type_module:container",
                "/type_module:containerWithMandatoryChild",
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

        DOCTEST_SUBCASE("listAdvancedWithTwoKey")
        {
            expectedPaths = {
                "/type_module:listAdvancedWithTwoKey",
                "/type_module:listAdvancedWithTwoKey/first",
                "/type_module:listAdvancedWithTwoKey/second",
            };

            path = "/type_module:listAdvancedWithTwoKey";
        }

        DOCTEST_SUBCASE("DFS on a leaf")
        {
            expectedPaths = {
                "/type_module:leafString",
            };

            path = "/type_module:leafString";
        }

        std::vector<std::string> actualPaths;
        for (const auto& it : ctx->findPath(path).childrenDfs()) {
            actualPaths.emplace_back(it.path());
        }

        REQUIRE(actualPaths == expectedPaths);
    }

    DOCTEST_SUBCASE("SchemaNode::immediateChildren")
    {
        std::vector<std::string> expectedPaths;
        const char* path;
        DOCTEST_SUBCASE("listAdvancedWithTwoKey")
        {
            expectedPaths = {
                "/type_module:listAdvancedWithTwoKey/first",
                "/type_module:listAdvancedWithTwoKey/second",
            };
            path = "/type_module:listAdvancedWithTwoKey";
        }
        DOCTEST_SUBCASE("leaf")
        {
            expectedPaths = {
            };
            path = "/type_module:leafString";
        }
        DOCTEST_SUBCASE("no recursion")
        {
            expectedPaths = {
                "/type_module:container/x",
                "/type_module:container/y",
                "/type_module:container/z",
            };
            path = "/type_module:container";
        }
        DOCTEST_SUBCASE("empty container")
        {
            expectedPaths = {
            };
            path = "/type_module:container/y";
        }
        DOCTEST_SUBCASE("one item")
        {
            expectedPaths = {
                "/type_module:container/z/z1",
            };
            path = "/type_module:container/z";
        }
        DOCTEST_SUBCASE("two items")
        {
            expectedPaths = {
                "/type_module:container/x/x1",
                "/type_module:container/x/x2",
            };
            path = "/type_module:container/x";
        }
        std::vector<std::string> actualPaths;
        for (const auto& it : ctx->findPath(path).immediateChildren()) {
            actualPaths.emplace_back(it.path());
        }
        REQUIRE(actualPaths == expectedPaths);
    }

    DOCTEST_SUBCASE("SchemaNode::siblings")
    {
        std::vector<std::string> expectedPaths;

        const char* path;

        DOCTEST_SUBCASE("leafListWithUnits")
        {
            expectedPaths = {
                "/type_module:leafListWithUnits",
                "/type_module:listBasic",
                "/type_module:listAdvancedWithOneKey",
                "/type_module:listAdvancedWithTwoKey",
                "/type_module:listWithMinMaxElements",
                "/type_module:numeric",
                "/type_module:container",
                "/type_module:containerWithMandatoryChild",
            };

            path = "/type_module:leafListWithUnits";
        }

        DOCTEST_SUBCASE("the last item")
        {
            expectedPaths = {
                "/type_module:containerWithMandatoryChild",
            };

            path = "/type_module:containerWithMandatoryChild";
        }

        std::vector<std::string> actualPaths;
        for (const auto& it : ctx->findPath(path).siblings()) {
            actualPaths.emplace_back(it.path());
        }

        REQUIRE(actualPaths == expectedPaths);
    }

    DOCTEST_SUBCASE("Container::isMandatory")
    {
        REQUIRE(ctx->findPath("/type_module:containerWithMandatoryChild").asContainer().isMandatory());
        REQUIRE(!ctx->findPath("/type_module:container").asContainer().isMandatory());
    }

    DOCTEST_SUBCASE("Container::isPresence")
    {
        REQUIRE(ctx->findPath("/example-schema:presenceContainer").asContainer().isPresence());
        REQUIRE(!ctx->findPath("/example-schema:first").asContainer().isPresence());
    }

    DOCTEST_SUBCASE("Leaf::defaultValueStr")
    {
        REQUIRE(ctx->findPath("/type_module:leafWithDefaultValue").asLeaf().defaultValueStr() == "custom-prefix");
        REQUIRE(!ctx->findPath("/type_module:leafString").asLeaf().defaultValueStr());
    }

    DOCTEST_SUBCASE("Leaf::isKey")
    {
        REQUIRE(ctx->findPath("/type_module:listAdvancedWithOneKey/lol").asLeaf().isKey());
        REQUIRE(!ctx->findPath("/type_module:leafString").asLeaf().isKey());
    }

    DOCTEST_SUBCASE("Leaf::isMandatory")
    {
        REQUIRE(!ctx->findPath("/type_module:leafString").asLeaf().isMandatory());
        REQUIRE(ctx->findPath("/type_module:leafWithMandatoryTrue").asLeaf().isMandatory());
    }

    DOCTEST_SUBCASE("Leaf::type")
    {
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

        DOCTEST_SUBCASE("binary")
        {
            auto type = ctx->findPath("/type_module:leafBinary").asLeaf().valueType();
            REQUIRE(type.base() == libyang::LeafBaseType::Binary);
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

            std::vector<std::pair<std::string, std::string>> expectedDerived{
                {"type_module", "fruit"},
                {"type_module", "meat"},
            };
            std::vector<std::pair<std::string, std::string>> actualDerived;
            for (const auto& it : bases) {
                for (const auto& der : it.derived()) {
                    actualDerived.emplace_back(der.module().name(), der.name());
                }
            }

            REQUIRE(expectedDerived == actualDerived);

            std::vector<std::pair<std::string, std::string>> expectedRecursivelyDerived{
                {"type_module", "apple"},
                {"type_module", "food"},
                {"type_module", "fruit"},
                {"type_module", "fruit-and-meat-mix"},
                {"type_module", "meat"},
            };
            std::vector<std::pair<std::string, std::string>> actualRecursivelyDerived;
            for (const auto& it : bases) {
                for (const auto& der : it.derivedRecursive()) {
                    actualRecursivelyDerived.emplace_back(der.module().name(), der.name());
                }
            }

            REQUIRE(expectedRecursivelyDerived == actualRecursivelyDerived);
        }

        DOCTEST_SUBCASE("leafref")
        {
            auto lref = ctx->findPath("/type_module:leafRef").asLeaf().valueType().asLeafRef();
            REQUIRE(lref.path() == "/custom-prefix:listAdvancedWithOneKey/lol");
            REQUIRE(lref.resolvedType().base() == libyang::LeafBaseType::String);
        }

        DOCTEST_SUBCASE("string")
        {
            auto type = ctx->findPath("/type_module:listAdvancedWithOneKey/lol").asLeaf().valueType();
            REQUIRE(type.base() == libyang::LeafBaseType::String);

            auto typeWithParsed = ctxWithParsed->findPath("/type_module:listAdvancedWithOneKey/lol").asLeaf().valueType();
            REQUIRE(typeWithParsed.base() == libyang::LeafBaseType::String);
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

    DOCTEST_SUBCASE("Leaf::units")
    {
        REQUIRE(ctx->findPath("/type_module:leafWithUnits").asLeaf().units() == "s");
        REQUIRE(ctx->findPath("/type_module:leafNumber").asLeaf().units() == std::nullopt);
    }

    DOCTEST_SUBCASE("LeafList::isMandatory")
    {
        REQUIRE(ctx->findPath("/type_module:leafListWithMinMaxElements").asLeafList().isMandatory());
        REQUIRE(!ctx->findPath("/type_module:leafListBasic").asLeafList().isMandatory());
    }

    DOCTEST_SUBCASE("LeafList::maxElements")
    {
        REQUIRE(ctx->findPath("/type_module:leafListWithMinMaxElements").asLeafList().maxElements() == 5);
        REQUIRE(ctx->findPath("/type_module:leafListBasic").asLeafList().maxElements() == std::numeric_limits<uint32_t>::max());
    }

    DOCTEST_SUBCASE("LeafList::minElements")
    {
        REQUIRE(ctx->findPath("/type_module:leafListWithMinMaxElements").asLeafList().minElements() == 1);
        REQUIRE(ctx->findPath("/type_module:leafListBasic").asLeafList().minElements() == 0);
    }

    DOCTEST_SUBCASE("LeafList::type")
    {
        REQUIRE(ctx->findPath("/type_module:leafListBasic").asLeafList().valueType().base() == libyang::LeafBaseType::String);
    }

    DOCTEST_SUBCASE("LeafList::units")
    {
        REQUIRE(ctx->findPath("/type_module:leafListWithUnits").asLeafList().units() == "s");
        REQUIRE(ctx->findPath("/type_module:leafListBasic").asLeafList().units() == std::nullopt);
    }

    DOCTEST_SUBCASE("List::isMandatory")
    {
        REQUIRE(ctx->findPath("/type_module:listWithMinMaxElements").asList().isMandatory());
        REQUIRE(!ctx->findPath("/type_module:listBasic").asList().isMandatory());
    }

    DOCTEST_SUBCASE("List::maxElements")
    {
        REQUIRE(ctx->findPath("/type_module:listWithMinMaxElements").asList().maxElements() == 5);
        REQUIRE(ctx->findPath("/type_module:listBasic").asList().maxElements() == std::numeric_limits<uint32_t>::max());
    }

    DOCTEST_SUBCASE("List::minElements")
    {
        REQUIRE(ctx->findPath("/type_module:listWithMinMaxElements").asList().minElements() == 1);
        REQUIRE(ctx->findPath("/type_module:listBasic").asList().minElements() == 0);
    }

    DOCTEST_SUBCASE("List::keys")
    {
        auto keys = ctx->findPath("/type_module:listAdvancedWithOneKey").asList().keys();
        REQUIRE(keys.size() == 1);
        REQUIRE(keys.front().path() == "/type_module:listAdvancedWithOneKey/lol");

        keys = ctx->findPath("/type_module:listAdvancedWithTwoKey").asList().keys();
        REQUIRE(keys.size() == 2);
        REQUIRE(keys[0].path() == "/type_module:listAdvancedWithTwoKey/first");
        REQUIRE(keys[1].path() == "/type_module:listAdvancedWithTwoKey/second");
    }

    DOCTEST_SUBCASE("RPC")
    {
        auto rpc = ctx->findPath("/example-schema:myRpc");
        REQUIRE(rpc.asActionRpc().child()->name() == "input");
        REQUIRE(rpc.asActionRpc().input().child()->name() == "inputLeaf");
        REQUIRE(rpc.asActionRpc().output().child()->name() == "outputLeaf");
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

    DOCTEST_SUBCASE("Binary::length")
    {
        REQUIRE_THROWS_WITH_AS(ctx->findPath("/type_module:listAdvancedWithOneKey/notKey3").asLeaf().valueType().asBinary().length(), "Context not created with libyang::ContextOptions::SetPrivParsed", libyang::Error);
        REQUIRE_THROWS_WITH_AS(ctxWithParsed->findPath("/example-schema:typedefedLeafInt").asLeaf().valueType().asBinary().length(), "Type is not a binary", libyang::Error);
        auto s_type1 = ctxWithParsed->findPath("/type_module:listAdvancedWithOneKey/notKey3").asLeaf().valueType().asBinary();
        REQUIRE(s_type1.length().parts.size() == 3);
        REQUIRE(s_type1.length().parts[0].min == 10);
        REQUIRE(s_type1.length().parts[0].max == 20);
        REQUIRE(s_type1.length().parts[1].min == 50);
        REQUIRE(s_type1.length().parts[1].max == 100);
        REQUIRE(s_type1.length().parts[2].min == 255);
        REQUIRE(s_type1.length().parts[2].max == 255);
        REQUIRE(!s_type1.length().description);
        REQUIRE(!s_type1.length().errorAppTag);
        REQUIRE(!s_type1.length().errorMessage);
        auto s_type2 = ctxWithParsed->findPath("/type_module:listAdvancedWithOneKey/notKey4").asLeaf().valueType().asBinary();
        REQUIRE(s_type2.length().parts.size() == 1);
        REQUIRE(s_type2.length().parts[0].min == 0);
        REQUIRE(s_type2.length().parts[0].max == std::numeric_limits<uint64_t>::max());
        REQUIRE(s_type2.length().description == "yay");
        REQUIRE(s_type2.length().errorAppTag == "x-XXX-failed");
        REQUIRE(s_type2.length().errorMessage == "hard to fail this one");

        auto noLength = ctxWithParsed->findPath("/type_module:leafBinary").asLeaf().valueType().asBinary();
        REQUIRE(noLength.length().parts.size() == 0);
    }

    DOCTEST_SUBCASE("String::length")
    {
        REQUIRE_THROWS_WITH_AS(ctx->findPath("/type_module:listAdvancedWithOneKey/notKey1").asLeaf().valueType().asString().length(), "Context not created with libyang::ContextOptions::SetPrivParsed", libyang::Error);
        REQUIRE_THROWS_WITH_AS(ctxWithParsed->findPath("/example-schema:typedefedLeafInt").asLeaf().valueType().asString().length(), "Type is not a string", libyang::Error);
        auto s_type1 = ctxWithParsed->findPath("/type_module:listAdvancedWithOneKey/notKey1").asLeaf().valueType().asString();
        REQUIRE(s_type1.length().parts.size() == 3);
        REQUIRE(s_type1.length().parts[0].min == 10);
        REQUIRE(s_type1.length().parts[0].max == 20);
        REQUIRE(s_type1.length().parts[1].min == 50);
        REQUIRE(s_type1.length().parts[1].max == 100);
        REQUIRE(s_type1.length().parts[2].min == 255);
        REQUIRE(s_type1.length().parts[2].max == 255);
        REQUIRE(!s_type1.length().description);
        REQUIRE(!s_type1.length().errorAppTag);
        REQUIRE(!s_type1.length().errorMessage);
        auto s_type2 = ctxWithParsed->findPath("/type_module:listAdvancedWithOneKey/notKey2").asLeaf().valueType().asString();
        REQUIRE(s_type2.length().parts.size() == 1);
        REQUIRE(s_type2.length().parts[0].min == 0);
        REQUIRE(s_type2.length().parts[0].max == std::numeric_limits<uint64_t>::max());
        REQUIRE(s_type2.length().description == "yay");
        REQUIRE(s_type2.length().errorAppTag == "x-XXX-failed");
        REQUIRE(s_type2.length().errorMessage == "hard to fail this one");

        auto noLength = ctxWithParsed->findPath("/type_module:listAdvancedWithOneKey/lol").asLeaf().valueType().asString();
        REQUIRE(noLength.length().parts.size() == 0);
    }

    DOCTEST_SUBCASE("String::patterns")
    {
        REQUIRE_THROWS_WITH_AS(ctx->findPath("/type_module:listAdvancedWithOneKey/notKey1").asLeaf().valueType().asString().patterns(), "Context not created with libyang::ContextOptions::SetPrivParsed", libyang::Error);
        REQUIRE_THROWS_WITH_AS(ctxWithParsed->findPath("/example-schema:typedefedLeafInt").asLeaf().valueType().asString().patterns(), "Type is not a string", libyang::Error);
        auto s_type = ctxWithParsed->findPath("/type_module:listAdvancedWithOneKey/notKey1").asLeaf().valueType().asString();
        REQUIRE(s_type.patterns().size() == 2);
        REQUIRE(s_type.patterns()[0].pattern == "fo+");
        REQUIRE(!s_type.patterns()[0].isInverted);
        REQUIRE(!s_type.patterns()[0].description);
        REQUIRE(!s_type.patterns()[0].errorAppTag);
        REQUIRE(!s_type.patterns()[0].errorMessage);
        REQUIRE(s_type.patterns()[1].pattern == "XXX");
        REQUIRE(s_type.patterns()[1].isInverted);
        REQUIRE(s_type.patterns()[1].description == "yay");
        REQUIRE(s_type.patterns()[1].errorAppTag == "x-XXX-failed");
        REQUIRE(s_type.patterns()[1].errorMessage == "hard to fail this one");
    }

    DOCTEST_SUBCASE("numeric limits")
    {
        REQUIRE_THROWS_WITH_AS(ctx->findPath("/type_module:numeric/i8").asLeaf().valueType().asNumeric().range(), "Context not created with libyang::ContextOptions::SetPrivParsed", libyang::Error);
        REQUIRE_THROWS_WITH_AS(ctxWithParsed->findPath("/type_module:listAdvancedWithOneKey/notKey1").asLeaf().valueType().asNumeric(), "Type is not a numeric type", libyang::Error);

        {
            auto t = ctxWithParsed->findPath("/type_module:numeric/i8").asLeaf().valueType().asNumeric();
            REQUIRE(t.fractionDigits() == 0);
            REQUIRE(t.range().parts.size() == 3);
            REQUIRE(t.range().parts[0].first == libyang::Value{int8_t{-20}});
            REQUIRE(t.range().parts[0].second == libyang::Value{int8_t{-10}});
            REQUIRE(t.range().parts[1].first == libyang::Value{int8_t{-5}});
            REQUIRE(t.range().parts[1].second == libyang::Value{int8_t{-5}});
            REQUIRE(t.range().parts[2].first == libyang::Value{int8_t{10}});
            REQUIRE(t.range().parts[2].second == libyang::Value{int8_t{100}});
        }

        {
            auto t = ctxWithParsed->findPath("/type_module:numeric/i16").asLeaf().valueType().asNumeric();
            REQUIRE(t.fractionDigits() == 0);
            REQUIRE(t.range().parts.size() == 1);
            REQUIRE(t.range().parts[0].first == libyang::Value{int16_t{253}});
            REQUIRE(t.range().parts[0].second == libyang::Value{int16_t{254}});
        }

        {
            auto t = ctxWithParsed->findPath("/type_module:numeric/i32").asLeaf().valueType().asNumeric();
            REQUIRE(t.fractionDigits() == 0);
            REQUIRE(t.range().parts.size() == 1);
            REQUIRE(t.range().parts[0].first == libyang::Value{int32_t{253}});
            REQUIRE(t.range().parts[0].second == libyang::Value{int32_t{254}});
        }

        {
            auto t = ctxWithParsed->findPath("/type_module:numeric/i64").asLeaf().valueType().asNumeric();
            REQUIRE(t.fractionDigits() == 0);
            REQUIRE(t.range().parts.size() == 1);
            REQUIRE(t.range().parts[0].first == libyang::Value{int64_t{253}});
            REQUIRE(t.range().parts[0].second == libyang::Value{int64_t{254}});
        }

        {
            using namespace libyang::literals;
            auto t = ctxWithParsed->findPath("/type_module:numeric/deci").asLeaf().valueType().asNumeric();
            REQUIRE(t.fractionDigits() == 6);
            REQUIRE(t.range().parts.size() == 1);
            REQUIRE(t.range().parts[0].first == libyang::Value{-123.456000_decimal64});
            REQUIRE(t.range().parts[0].second == libyang::Value{666.042000_decimal64});
        }

        {
            auto t = ctxWithParsed->findPath("/type_module:numeric/deci1").asLeaf().valueType().asNumeric();
            REQUIRE(t.fractionDigits() == 1);
            REQUIRE(t.range().parts.size() == 0);
        }

        {
            auto t = ctxWithParsed->findPath("/type_module:numeric/u8").asLeaf().valueType().asNumeric();
            REQUIRE(t.fractionDigits() == 0);
            REQUIRE(t.range().parts.size() == 1);
            REQUIRE(t.range().parts[0].first == libyang::Value{uint8_t{253}});
            REQUIRE(t.range().parts[0].second == libyang::Value{uint8_t{254}});
        }

        {
            auto t = ctxWithParsed->findPath("/type_module:numeric/u16").asLeaf().valueType().asNumeric();
            REQUIRE(t.fractionDigits() == 0);
            REQUIRE(t.range().parts.size() == 1);
            REQUIRE(t.range().parts[0].first == libyang::Value{uint16_t{253}});
            REQUIRE(t.range().parts[0].second == libyang::Value{uint16_t{254}});
        }

        {
            auto t = ctxWithParsed->findPath("/type_module:numeric/u32").asLeaf().valueType().asNumeric();
            REQUIRE(t.fractionDigits() == 0);
            REQUIRE(t.range().parts.size() == 1);
            REQUIRE(t.range().parts[0].first == libyang::Value{uint32_t{253}});
            REQUIRE(t.range().parts[0].second == libyang::Value{uint32_t{254}});
        }

        {
            auto t = ctxWithParsed->findPath("/type_module:numeric/u64").asLeaf().valueType().asNumeric();
            REQUIRE(t.fractionDigits() == 0);
            REQUIRE(t.range().parts.size() == 1);
            REQUIRE(t.range().parts[0].first == libyang::Value{uint64_t{253}});
            REQUIRE(t.range().parts[0].second == libyang::Value{uint64_t{254}});
        }

        {
            auto t = ctxWithParsed->findPath("/type_module:leafWithUnits").asLeaf().valueType().asNumeric();
            REQUIRE(t.fractionDigits() == 0);
            REQUIRE(t.range().parts.size() == 0);
        }
    }
}
