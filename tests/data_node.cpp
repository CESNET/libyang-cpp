/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <doctest/doctest.h>
#include <libyang-cpp/Context.hpp>


const auto example_schema = R"(
module example-schema {
    yang-version 1.1;
    namespace "http://example.com/";
    prefix coze;

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

    leaf leafBinary {
        description "A binary value.";
        type binary;
    }

    leaf active {
        type boolean;
    }
})";

const auto data = R"({
  "example-schema:leafInt32": 420
}
)";

const auto dataTypes = R"({
  "example-schema:leafInt8": -43,
  "example-schema:leafInt16": 3000,
  "example-schema:leafInt32": -391203910,
  "example-schema:leafInt64": "-234214214928",
  "example-schema:leafUInt8": 43,
  "example-schema:leafUInt16": 2333,
  "example-schema:leafUInt32": 23423422,
  "example-schema:leafUInt64": "453545335344",
  "example-schema:leafBool": false,
  "example-schema:leafString": "AHOJ",
  "example-schema:leafEmpty": [null],
  "example-schema:leafBinary": "AAAABBBBCCC="
}
)";

TEST_CASE("Data Node manipulation")
{
    libyang::Context ctx;
    ctx.parseModuleMem(example_schema, libyang::SchemaFormat::Yang);

    DOCTEST_SUBCASE("Printing")
    {
        auto node = ctx.parseDataMem(data, libyang::DataFormat::JSON);
        auto str = node.printStr(libyang::DataFormat::JSON, libyang::PrintFlags::WithSiblings | libyang::PrintFlags::KeepEmptyCont);
        REQUIRE(str == data);
    }

    DOCTEST_SUBCASE("findPath")
    {
        // Need optional here, because I need to delete the tree at some point.
        std::optional<libyang::DataNode> node{ctx.parseDataMem(data, libyang::DataFormat::JSON)};

        DOCTEST_SUBCASE("Node exists")
        {
            auto nodeLeafInt32 = node->findPath("/example-schema:leafInt32");
            REQUIRE(nodeLeafInt32);
            REQUIRE(nodeLeafInt32->path() == "/example-schema:leafInt32");

            DOCTEST_SUBCASE("Do nothing") { }

            DOCTEST_SUBCASE("Remove the original node") {
                node = std::nullopt;
                REQUIRE(nodeLeafInt32->path() == "/example-schema:leafInt32");
            }

            DOCTEST_SUBCASE("Replace the original node with another one") {
                node = ctx.parseDataMem(data, libyang::DataFormat::JSON);
                REQUIRE(nodeLeafInt32->path() == "/example-schema:leafInt32");
            }
        }

        DOCTEST_SUBCASE("Invalid node")
        {
            REQUIRE_THROWS_WITH_AS(node->findPath("/mod:nein"), "Error in DataNode::findPath (7)", std::runtime_error);
        }

        DOCTEST_SUBCASE("Node doesn't exist in the tree")
        {
            REQUIRE(node->findPath("/example-schema:active") == std::nullopt);
        }
    }

    DOCTEST_SUBCASE("DataNodeTerm")
    {
        auto data = ctx.parseDataMem(dataTypes, libyang::DataFormat::JSON);
        std::string path;
        libyang::Value expected;

        DOCTEST_SUBCASE("value types") {
            DOCTEST_SUBCASE("int8") {
                path = "/example-schema:leafInt8";
                expected = int8_t{-43};
            }

            DOCTEST_SUBCASE("int16") {
                path = "/example-schema:leafInt16";
                expected = int16_t{3000};
            }

            DOCTEST_SUBCASE("int32") {
                path = "/example-schema:leafInt32";
                expected = int32_t{-391203910};
            }

            DOCTEST_SUBCASE("int64") {
                path = "/example-schema:leafInt64";
                expected = int64_t{-234214214928};
            }

            DOCTEST_SUBCASE("uint8") {
                path = "/example-schema:leafUInt8";
                expected = uint8_t{43};
            }

            DOCTEST_SUBCASE("uint16") {
                path = "/example-schema:leafUInt16";
                expected = uint16_t{2333};
            }

            DOCTEST_SUBCASE("uint32") {
                path = "/example-schema:leafUInt32";
                expected = uint32_t{23423422};
            }

            DOCTEST_SUBCASE("uint64") {
                path = "/example-schema:leafUInt64";
                expected = uint64_t{453545335344};
            }

            DOCTEST_SUBCASE("boolean") {
                path = "/example-schema:leafBool";
                expected = bool{false};
            }

            DOCTEST_SUBCASE("string") {
                path = "/example-schema:leafString";
                expected = std::string{"AHOJ"};
            }

            DOCTEST_SUBCASE("empty") {
                path = "/example-schema:leafEmpty";
                expected = libyang::Empty{};
            }

            DOCTEST_SUBCASE("binary") {
                path = "/example-schema:leafBinary";
                expected = libyang::Binary{"AAAABBBBCCC="};
            }
        }

        auto node = data.findPath(path.c_str());
        REQUIRE(node);
        auto term = node->asTerm();
        REQUIRE(term.path() == path);
        REQUIRE(term.value() == expected);
    }
}
