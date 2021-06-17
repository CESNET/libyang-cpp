/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <doctest/doctest.h>
#include <libyang-cpp/Context.hpp>
#include <sstream>

using namespace std::string_literals;

const auto example_schema = R"(
module example-schema {
    yang-version 1.1;
    namespace "http://example.com/";
    prefix coze;

    leaf dummy {
        type string;
    }

    leaf leafInt8 {
        description "A 8-bit integer leaf.";
        type int8;
    }

    leaf leafInt16 {
        description "A 16-bit integer leaf.";
        type int16;
    }

    leaf leafInt32 {
        description "A 32-bit integer leaf.";
        type int32;
    }

    leaf leafInt64 {
        description "A 64-bit integer leaf.";
        type int64;
    }

    leaf leafUInt8 {
        description "A 8-bit unsigned integer leaf.";
        type uint8;
    }

    leaf leafUInt16 {
        description "A 16-bit unsigned integer leaf.";
        type uint16;
    }

    leaf leafUInt32 {
        description "A 32-bit unsigned integer leaf.";
        type uint32;
    }

    leaf leafUInt64 {
        description "A 64-bit unsigned integer leaf.";
        type uint64;
    }

    leaf leafBool {
        description "A boolean.";
        type boolean;
    }

    leaf leafString {
        description "A string.";
        type string;
    }

    leaf leafEmpty {
        description "An `empty` leaf.";
        type empty;
    }

    leaf leafDecimal {
        description "A decimal value.";
        type decimal64 {
            fraction-digits 5;
        }
    }

    leaf leafBinary {
        description "A binary value.";
        type binary;
    }

    leaf intOrString {
        description "An int or a string.";
        type union {
            type int32;
            type string;
        }
    }

    list person {
        key 'name';
        leaf name {
            type string;
        }
    }

    leaf bossPerson {
        type leafref {
            path '../person/name';
        }
    }

    leaf targetInstance {
        type instance-identifier;
    }

    leaf NOtargetInstance {
        type instance-identifier {
            require-instance false;
        }
    }

    leaf active {
        type boolean;
    }

    leaf flagBits {
        type bits {
            bit carry;
            bit sign;
            bit overflow;
        }
    }

    leaf pizzaSize {
        type enumeration {
            enum large;
            enum medium;
            enum small;
        }
    }

    identity food {
    }

    identity fruit {
        base "food";
    }

    identity pizza {
        base "food";
    }

    identity hawaii {
        base "pizza";
    }

    typedef foodTypedef {
        type identityref {
            base food;
        }
    }

    leaf leafFoodTypedef {
        type foodTypedef;
    }
})";

const auto type_module = R"(
module type_module {
    yang-version 1.1;
    namespace "http://example.com/";
    prefix ahoj;

    leaf myLeaf {
        type string;
    }

    list myList {
        key 'lol';
        leaf lol {
            type string;
        }
    }
}
)";

TEST_CASE("SchemaNode")
{
    std::optional<libyang::Context> ctx{std::in_place};
    ctx->parseModuleMem(example_schema, libyang::SchemaFormat::Yang);
    ctx->parseModuleMem(type_module, libyang::SchemaFormat::Yang);

    DOCTEST_SUBCASE("context lifetime")
    {
        auto node = ctx->findPath("/example-schema:leafBinary");
        REQUIRE(node.path() == "/example-schema:leafBinary");
        ctx.reset();
        REQUIRE(node.path() == "/example-schema:leafBinary");
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

    DOCTEST_SUBCASE("DataNode::nodetype")
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
        // TODO: add tests for other nodetypes...

        REQUIRE(ctx->findPath(path).nodeType() == expected);
    }
}
