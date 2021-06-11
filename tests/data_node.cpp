/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <doctest/doctest.h>
#include <experimental/iterator>
#include <libyang-cpp/Context.hpp>
#include <sstream>

using namespace std::string_literals;

namespace libyang {
    namespace {
        struct impl_toStruct {
            std::string operator()(const Binary& value)
            {
                std::ostringstream oss;
                oss << std::hex;
                std::transform(value.data.begin(), value.data.end(), std::experimental::make_ostream_joiner(oss, ", "), [] (uint8_t num) {
                    return std::to_string(static_cast<unsigned int>(num));
                });
                return oss.str();
            }

            std::string operator()(const Empty)
            {
                return "<empty>";
            }

            std::string operator()(const std::string& value)
            {
                return value;
            }

            std::string operator()(const std::optional<DataNode>& value)
            {
                if (!value) {
                    return "std::nullopt (DataNode)";
                }

                return std::string{value->path()};
            }

            std::string operator()(const Decimal64& value)
            {
                return "Decimal64{"s + std::to_string(value.number) + ", " + std::to_string(value.digits) + "}";
            }

            std::string operator()(const std::vector<Bit>& value)
            {
                std::ostringstream oss;
                oss << "Bits{";
                std::transform(value.begin(), value.end(), std::experimental::make_ostream_joiner(oss, ", "), [] (const Bit& bit) {
                    return bit.name + "(" + std::to_string(bit.position) + ")";
                });
                oss << "}";
                return oss.str();
            }

            std::string operator()(const Enum& value)
            {
                return "Enum{"s + value.name + "}";
            }

            std::string operator()(const IdentityRef& value)
            {
                return "IdentityRef{"s + value.module + ":" + value.name + "}";
            }

            template <typename Type>
            std::string operator()(const Type& value)
            {
                return std::to_string(value);
            }
        };
    }
    doctest::String toString(const Value& value) {
        auto str = std::visit(impl_toStruct{}, value);
        return str.c_str();
    }
}

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
  "example-schema:leafDecimal": "23212131231.43242",
  "example-schema:leafBinary": "AAAABBBBCCC=",
  "example-schema:intOrString": 14332,
  "example-schema:person": [
    {
        "name": "Dan"
    }
  ],
  "example-schema:bossPerson": "Dan",
  "example-schema:targetInstance": "/example-schema:leafBool",
  "example-schema:NOtargetInstance": "/example-schema:dummy",
  "example-schema:flagBits": "carry overflow",
  "example-schema:pizzaSize": "large",
  "example-schema:leafFoodTypedef": "hawaii"
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

            DOCTEST_SUBCASE("decimal64") {
                path = "/example-schema:leafDecimal";
                expected = libyang::Decimal64{2321213123143242, 5};
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
                expected = libyang::Binary{{0, 0, 0, 4, 16, 65, 8, 32}, "AAAABBBBCCC="};
            }

            DOCTEST_SUBCASE("intOrString") {
                path = "/example-schema:intOrString";
                expected = int32_t{14332};
            }

            DOCTEST_SUBCASE("leafref") {
                path = "/example-schema:bossPerson";
                expected = std::string{"Dan"};
            }

            DOCTEST_SUBCASE("instance-identifier") {
                DOCTEST_SUBCASE("require-instance = true") {
                    path = "/example-schema:targetInstance";
                    expected = data.findPath("/example-schema:leafBool");
                }

                DOCTEST_SUBCASE("require-instance = false") {
                    path = "/example-schema:NOtargetInstance";
                    expected = std::nullopt;
                }
            }

            DOCTEST_SUBCASE("bits") {
                path = "/example-schema:flagBits";
                expected = std::vector<libyang::Bit>{{0, "carry"}, {2, "overflow"}};
            }

            DOCTEST_SUBCASE("enum") {
                path = "/example-schema:pizzaSize";
                expected = libyang::Enum{"large"};
            }

            DOCTEST_SUBCASE("identityref") {
                path = "/example-schema:leafFoodTypedef";
                expected = libyang::IdentityRef{"example-schema", "hawaii"};
            }
        }

        auto node = data.findPath(path.c_str());
        REQUIRE(node);
        auto term = node->asTerm();
        REQUIRE(term.path() == path);
        REQUIRE(term.value() == expected);
    }

    DOCTEST_SUBCASE("newPath")
    {
        auto node = ctx.newPath("/example-schema:leafInt32", "420");
        auto str = node.printStr(libyang::DataFormat::JSON, libyang::PrintFlags::WithSiblings | libyang::PrintFlags::KeepEmptyCont);
        REQUIRE(str == data);
    }
}
