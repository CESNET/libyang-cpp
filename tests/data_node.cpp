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
#include <libyang-cpp/utils/exception.hpp>
#include <sstream>
#include "example_schema.hpp"

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
    doctest::String toString(const std::vector<libyang::DataNode>& nodes) {
        std::ostringstream oss;
        std::transform(nodes.begin(), nodes.end(), std::experimental::make_ostream_joiner(oss, ", "), [] (const DataNode& node) {
            return "DataNode -> " + std::string{node.path()};
        });

        return oss.str().c_str();
    }
}

namespace std {
doctest::String toString(const std::vector<std::string>& vec) {
    std::ostringstream oss;
    oss << "std::vector<std::string>{\n    ";
    std::copy(vec.begin(), vec.end(), std::experimental::make_ostream_joiner(oss, ",\n    "));

    oss << "\n}";

    return oss.str().c_str();
}
}

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

const auto data2 = R"({
  "example-schema:leafInt8": -43,
  "example-schema:first": {
    "second": {
      "third": {
        "fourth": {
            "fifth": "430"
        }
      }
    }
  }
}
)";

namespace libyang {
doctest::String toString(const String& value) {
    return std::string{value}.c_str();
}
}

TEST_CASE("Data Node manipulation")
{
    libyang::Context ctx;
    ctx.parseModuleMem(example_schema, libyang::SchemaFormat::YANG);

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

        DOCTEST_SUBCASE("Finding RPC output nodes")
        {
            auto node = ctx.newPath("/example-schema:myRpc/outputLeaf", "AHOJ", libyang::CreationOptions::Output);
            REQUIRE_THROWS_WITH_AS(node.findPath("/example-schema:myRpc/outputLeaf", libyang::OutputNodes::No),
                    "Error in DataNode::findPath (7)",
                    libyang::ErrorWithCode);
            REQUIRE(node.findPath("/example-schema:myRpc/outputLeaf", libyang::OutputNodes::Yes).has_value());
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
                using namespace libyang::literals;
                expected = 23212131231.43242_decimal64;
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

    DOCTEST_SUBCASE("unlink")
    {
        std::optional<libyang::DataNode> root = ctx.parseDataMem(data2, libyang::DataFormat::JSON);
        std::vector<libyang::DataNode> refs;

        auto createRef = [&] (const auto* path) {
            auto ref = root->findPath(path);
            REQUIRE(ref);
            refs.emplace_back(*ref);
        };

        DOCTEST_SUBCASE("just unlink root")
        {
            // This checks that the top-level sibling is properly freed.
            root->unlink();
        }

        DOCTEST_SUBCASE("get a ref to root and unlink that")
        {
            createRef("/example-schema:first");

            DOCTEST_SUBCASE("also unref root") {
                root = std::nullopt;
            }

            DOCTEST_SUBCASE("do nothing with root") {
            }

            refs.front().unlink();
        }

        DOCTEST_SUBCASE("ref fifth, unref root, and unlink")
        {
            createRef("/example-schema:first/second/third/fourth/fifth");
            root = std::nullopt;
            refs.front().unlink();
        }

        DOCTEST_SUBCASE("unlinked nodes aren't reachable")
        {
            createRef("/example-schema:first/second/third");
            refs.front().unlink();
            REQUIRE(!root->findPath("/example-schema:first/second/third").has_value());
            REQUIRE(!root->findPath("/example-schema:first/second/third/fourth").has_value());
            REQUIRE(!root->findPath("/example-schema:first/second/third/fourth/fifth").has_value());

            // The parent of the unlinked node should still be reachable from `root`.
            REQUIRE(root->findPath("/example-schema:first/second").has_value());
            // The children of the unlinked node should still be reachable from the ref.
            REQUIRE(refs.front().findPath("fourth").has_value());
            REQUIRE(refs.front().findPath("fourth/fifth").has_value());
        }

        DOCTEST_SUBCASE("ref everything")
        {
            createRef("/example-schema:first");
            createRef("/example-schema:first/second");
            createRef("/example-schema:first/second/third");
            createRef("/example-schema:first/second/third/fourth");
            createRef("/example-schema:first/second/third/fourth/fifth");

            DOCTEST_SUBCASE("unlink everything") {
                for (auto& ref : refs) {
                    ref.unlink();
                }
            }

            DOCTEST_SUBCASE("unlink first") {
                refs.at(0).unlink();

                DOCTEST_SUBCASE("... then nothing") { }

                DOCTEST_SUBCASE("... then second") {
                    refs.at(1).unlink();
                }
            }

            DOCTEST_SUBCASE("unlink second") {
                refs.at(1).unlink();

                DOCTEST_SUBCASE("... then nothing") { }

                DOCTEST_SUBCASE("... then first") {
                    refs.at(0).unlink();
                }
            }

            DOCTEST_SUBCASE("unlink third") {
                refs.at(2).unlink();
            }

            DOCTEST_SUBCASE("unlink fourth") {
                refs.at(3).unlink();
            }

            DOCTEST_SUBCASE("unlink fifth") {
                refs.at(4).unlink();
            }

            DOCTEST_SUBCASE("unlink second, then first") {
                refs.at(1).unlink();
                refs.at(0).unlink();
            }

        }

        DOCTEST_SUBCASE("ref 1, 2, 3")
        {
            createRef("/example-schema:first");
            createRef("/example-schema:first/second/third");
            createRef("/example-schema:first/second/third/fourth/fifth");

            DOCTEST_SUBCASE("unlink all") {
                for (auto& ref : refs) {
                    ref.unlink();
                }
            }

            DOCTEST_SUBCASE("unlink first, then third") {
                refs.at(0).unlink();
                refs.at(1).unlink();
            }

            DOCTEST_SUBCASE("unlink third, then first") {
                refs.at(1).unlink();
                refs.at(0).unlink();
            }

            DOCTEST_SUBCASE("unlink first, then fifth") {
                refs.at(0).unlink();
                refs.at(2).unlink();
            }

            DOCTEST_SUBCASE("unlink third, then fifth") {
                refs.at(1).unlink();
                refs.at(2).unlink();
            }
        }

        if (root) {
            root->path();
        }

        // Check if all refs are still valid.
        for (auto& ref : refs) {
            ref.path();
        }

    }

    DOCTEST_SUBCASE("DataNode::childrenDfs")
    {
        const auto dataToIter = R"(
        {
            "example-schema:bigTree": {
                "one": {
                    "myLeaf": "AHOJ"
                },
                "two": {
                    "myList": [
                    {
                        "thekey": 43221
                    },
                    {
                        "thekey": 432
                    },
                    {
                        "thekey": 213
                    }
                    ]
                }
            }
        }
        )";

        auto node = ctx.parseDataMem(dataToIter, libyang::DataFormat::JSON).findPath("/example-schema:bigTree").value();

        DOCTEST_SUBCASE("range-for loop") {
            auto coll = node.childrenDfs();
            std::vector<std::string> res;
            for (const auto& it : coll) {
                res.emplace_back(it.path());
            }

            std::vector<std::string> expected = {
                "/example-schema:bigTree",
                "/example-schema:bigTree/one",
                "/example-schema:bigTree/one/myLeaf",
                "/example-schema:bigTree/two",
                "/example-schema:bigTree/two/myList[thekey='43221']",
                "/example-schema:bigTree/two/myList[thekey='43221']/thekey",
                "/example-schema:bigTree/two/myList[thekey='432']",
                "/example-schema:bigTree/two/myList[thekey='432']/thekey",
                "/example-schema:bigTree/two/myList[thekey='213']",
                "/example-schema:bigTree/two/myList[thekey='213']/thekey"
            };

            REQUIRE(res == expected);
        }

        DOCTEST_SUBCASE("standard algorithms") {
            auto coll = node.childrenDfs();
            REQUIRE(std::find_if(coll.begin(), coll.end(), [] (const auto& node) {
                return node.path() == "/example-schema:bigTree/two/myList[thekey='432']/thekey";
            }) != coll.end());
        }

        DOCTEST_SUBCASE("incrementing") {
            auto coll = node.childrenDfs();
            auto iter = coll.begin();

            DOCTEST_SUBCASE("prefix increment") {
                REQUIRE(iter->path() == "/example-schema:bigTree");
                auto newIter = ++iter;
                // Both iterators point to the next element.
                REQUIRE(iter->path() == "/example-schema:bigTree/one");
                REQUIRE(newIter->path() == "/example-schema:bigTree/one");
            }

            DOCTEST_SUBCASE("postfix increment") {
                REQUIRE(iter->path() == "/example-schema:bigTree");
                auto newIter = iter++;
                // Only the original iterator points to the next element.
                REQUIRE(iter->path() == "/example-schema:bigTree/one");
                REQUIRE(newIter->path() == "/example-schema:bigTree");
            }
        }

        DOCTEST_SUBCASE("invalidating iterators") {
            std::vector<std::string> expectedPaths;
            std::vector<std::string> actualPaths;

            auto coll = node.findPath("/example-schema:bigTree/two")->childrenDfs();
            auto iter = coll.begin();

            DOCTEST_SUBCASE("unlink starting node") {

                DOCTEST_SUBCASE("don't free") {
                    auto toUnlink = node.findPath("/example-schema:bigTree/two");
                    toUnlink->unlink();

                    REQUIRE_THROWS(coll.begin());
                    REQUIRE_THROWS(*iter);
                }

                DOCTEST_SUBCASE("also free the starting node") {
                    node.findPath("/example-schema:bigTree/two")->unlink();

                    REQUIRE_THROWS(coll.begin());
                    REQUIRE_THROWS(*iter);
                }

            }

            DOCTEST_SUBCASE("unlink node from different subtree") {
                node.findPath("/example-schema:bigTree/one")->unlink();

                REQUIRE(coll.begin()->path() == "/example-schema:bigTree/two");
                REQUIRE(iter->path() == "/example-schema:bigTree/two");
            }

            DOCTEST_SUBCASE("iterator outlives collection") {
                coll = node.findPath("/example-schema:bigTree/two")->childrenDfs();
                REQUIRE_THROWS(*iter);
            }
        }
    }
}
