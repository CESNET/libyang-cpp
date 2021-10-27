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
#include <libyang/libyang.h>
#include "example_schema.hpp"
#include "pretty_printers.hpp"
#include "test_vars.hpp"

const auto data = R"({
  "example-schema:leafInt32": 420,
  "example-schema:first": {
    "second": {
      "third": {
        "fourth": {}
      }
    }
  },
  "example-schema:bigTree": {
    "one": {},
    "two": {}
  }
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

const auto data3 = R"({
  "example-schema2:contWithTwoNodes": {
    "one": 123,
    "two": 456
  }
}
)";

const auto data4 = R"({
  "example-schema3:values": [ 10,20,30,40 ],
  "example-schema3:person": [
    {
        "name": "Dan"
    },
    {
        "name": "George"
    }
  ]
}
)";

TEST_CASE("Data Node manipulation")
{
    libyang::Context ctx;
    ctx.parseModuleMem(example_schema, libyang::SchemaFormat::YANG);
    ctx.parseModuleMem(example_schema2, libyang::SchemaFormat::YANG);
    ctx.parseModuleMem(example_schema3, libyang::SchemaFormat::YANG);

    DOCTEST_SUBCASE("Printing")
    {
        auto node = ctx.parseDataMem(data, libyang::DataFormat::JSON);
        auto str = node->printStr(libyang::DataFormat::JSON, libyang::PrintFlags::WithSiblings | libyang::PrintFlags::KeepEmptyCont);

        REQUIRE(str == data);
    }

    DOCTEST_SUBCASE("Overwriting a tree with a different tree")
    {
        // The original tree must be freed.
        auto node = ctx.parseDataMem(data, libyang::DataFormat::JSON);
        node = ctx.parseDataMem(data, libyang::DataFormat::JSON);
    }

    DOCTEST_SUBCASE("findPath")
    {
        auto node = ctx.parseDataMem(data, libyang::DataFormat::JSON);

        DOCTEST_SUBCASE("Node exists")
        {
            auto nodeLeafInt32 = node->findPath("/example-schema:leafInt32");
            REQUIRE(nodeLeafInt32);
            REQUIRE(nodeLeafInt32->path() == "/example-schema:leafInt32");

            DOCTEST_SUBCASE("Do nothing") { }

            DOCTEST_SUBCASE("Remove the original node")
            {
                node = std::nullopt;
                REQUIRE(nodeLeafInt32->path() == "/example-schema:leafInt32");
            }

            DOCTEST_SUBCASE("Replace the original node with another one")
            {
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

        DOCTEST_SUBCASE("value types")
        {
            DOCTEST_SUBCASE("int8")
            {
                path = "/example-schema:leafInt8";
                expected = int8_t{-43};
            }

            DOCTEST_SUBCASE("int16")
            {
                path = "/example-schema:leafInt16";
                expected = int16_t{3000};
            }

            DOCTEST_SUBCASE("int32")
            {
                path = "/example-schema:leafInt32";
                expected = int32_t{-391203910};
            }

            DOCTEST_SUBCASE("int64")
            {
                path = "/example-schema:leafInt64";
                expected = int64_t{-234214214928};
            }

            DOCTEST_SUBCASE("uint8")
            {
                path = "/example-schema:leafUInt8";
                expected = uint8_t{43};
            }

            DOCTEST_SUBCASE("uint16")
            {
                path = "/example-schema:leafUInt16";
                expected = uint16_t{2333};
            }

            DOCTEST_SUBCASE("uint32")
            {
                path = "/example-schema:leafUInt32";
                expected = uint32_t{23423422};
            }

            DOCTEST_SUBCASE("uint64")
            {
                path = "/example-schema:leafUInt64";
                expected = uint64_t{453545335344};
            }

            DOCTEST_SUBCASE("decimal64")
            {
                path = "/example-schema:leafDecimal";
                using namespace libyang::literals;
                expected = 23212131231.43242_decimal64;
            }

            DOCTEST_SUBCASE("boolean")
            {
                path = "/example-schema:leafBool";
                expected = bool{false};
            }

            DOCTEST_SUBCASE("string")
            {
                path = "/example-schema:leafString";
                expected = std::string{"AHOJ"};
            }

            DOCTEST_SUBCASE("empty")
            {
                path = "/example-schema:leafEmpty";
                expected = libyang::Empty{};
            }

            DOCTEST_SUBCASE("binary")
            {
                path = "/example-schema:leafBinary";
                expected = libyang::Binary{{0, 0, 0, 4, 16, 65, 8, 32}, "AAAABBBBCCC="};
            }

            DOCTEST_SUBCASE("intOrString")
            {
                path = "/example-schema:intOrString";
                expected = int32_t{14332};
            }

            DOCTEST_SUBCASE("leafref")
            {
                path = "/example-schema:bossPerson";
                expected = std::string{"Dan"};
            }

            DOCTEST_SUBCASE("instance-identifier")
            {
                DOCTEST_SUBCASE("require-instance = true")
                {
                    path = "/example-schema:targetInstance";
                    expected = data->findPath("/example-schema:leafBool");
                }

                DOCTEST_SUBCASE("require-instance = false")
                {
                    path = "/example-schema:NOtargetInstance";
                    expected = std::nullopt;
                }
            }

            DOCTEST_SUBCASE("bits")
            {
                path = "/example-schema:flagBits";
                expected = std::vector<libyang::Bit>{{0, "carry"}, {2, "overflow"}};
            }

            DOCTEST_SUBCASE("enum")
            {
                path = "/example-schema:pizzaSize";
                expected = libyang::Enum{"large"};
            }

            DOCTEST_SUBCASE("identityref")
            {
                path = "/example-schema:leafFoodTypedef";
                expected = libyang::IdentityRef{"example-schema", "hawaii"};
            }
        }

        auto node = data->findPath(path.c_str());
        REQUIRE(node);
        auto term = node->asTerm();
        REQUIRE(term.path() == path);
        REQUIRE(term.value() == expected);
    }

    DOCTEST_SUBCASE("newPath")
    {
        auto node = std::optional{ctx.newPath("/example-schema:leafInt32", "420")};
        libyang::validateAll(node, libyang::ValidationOptions::NoState);
        auto str = node->printStr(libyang::DataFormat::JSON, libyang::PrintFlags::WithSiblings | libyang::PrintFlags::KeepEmptyCont);
        REQUIRE(str == data);
    }

    DOCTEST_SUBCASE("unlink")
    {
        auto root = ctx.parseDataMem(data2, libyang::DataFormat::JSON);
        std::vector<libyang::DataNode> refs;

        auto createRef = [&](const auto* path) {
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

            DOCTEST_SUBCASE("also unref root")
            {
                root = std::nullopt;
            }

            DOCTEST_SUBCASE("do nothing with root")
            {
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

            DOCTEST_SUBCASE("unlink everything")
            {
                for (auto& ref : refs) {
                    ref.unlink();
                }
            }

            DOCTEST_SUBCASE("unlink first")
            {
                refs.at(0).unlink();

                DOCTEST_SUBCASE("... then nothing") { }

                DOCTEST_SUBCASE("... then second")
                {
                    refs.at(1).unlink();
                }
            }

            DOCTEST_SUBCASE("unlink second")
            {
                refs.at(1).unlink();

                DOCTEST_SUBCASE("... then nothing") { }

                DOCTEST_SUBCASE("... then first")
                {
                    refs.at(0).unlink();
                }
            }

            DOCTEST_SUBCASE("unlink third")
            {
                refs.at(2).unlink();
            }

            DOCTEST_SUBCASE("unlink fourth")
            {
                refs.at(3).unlink();
            }

            DOCTEST_SUBCASE("unlink fifth")
            {
                refs.at(4).unlink();
            }

            DOCTEST_SUBCASE("unlink second, then first")
            {
                refs.at(1).unlink();
                refs.at(0).unlink();
            }
        }

        DOCTEST_SUBCASE("ref 1, 2, 3")
        {
            createRef("/example-schema:first");
            createRef("/example-schema:first/second/third");
            createRef("/example-schema:first/second/third/fourth/fifth");

            DOCTEST_SUBCASE("unlink all")
            {
                for (auto& ref : refs) {
                    ref.unlink();
                }
            }

            DOCTEST_SUBCASE("unlink first, then third")
            {
                refs.at(0).unlink();
                refs.at(1).unlink();
            }

            DOCTEST_SUBCASE("unlink third, then first")
            {
                refs.at(1).unlink();
                refs.at(0).unlink();
            }

            DOCTEST_SUBCASE("unlink first, then fifth")
            {
                refs.at(0).unlink();
                refs.at(2).unlink();
            }

            DOCTEST_SUBCASE("unlink third, then fifth")
            {
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

    DOCTEST_SUBCASE("DataNode::unlinkWithSiblings")
    {
        DOCTEST_SUBCASE("Nodes have no parent")
        {
            auto node = ctx.parseDataMem(dataTypes, libyang::DataFormat::JSON)->findPath("/example-schema:leafInt32");

            node->unlinkWithSiblings();
        }

        DOCTEST_SUBCASE("Nodes have a parent")
        {
            auto node = ctx.parseDataMem(data3, libyang::DataFormat::JSON);

            DOCTEST_SUBCASE("Keep ref to parent")
            {
                node->findPath("/example-schema2:contWithTwoNodes/two")->unlinkWithSiblings();
            }

            DOCTEST_SUBCASE("Keep ref to `two`")
            {
                auto two = *node->findPath("/example-schema2:contWithTwoNodes/two");
                two.unlinkWithSiblings();
                REQUIRE(two.path() == "/example-schema2:two");
            }

            DOCTEST_SUBCASE("Unlink `one`")
            {
                auto one = *node->findPath("/example-schema2:contWithTwoNodes/one");
                one.unlinkWithSiblings();
                REQUIRE(one.path() == "/example-schema2:one");
            }

            DOCTEST_SUBCASE("Don't keep ref to parent")
            {
                node = *node->findPath("/example-schema2:contWithTwoNodes/two");
                node->unlinkWithSiblings();
            }

            // The original tree should still be accesible.
            node->printStr(libyang::DataFormat::JSON, libyang::PrintFlags::WithSiblings);
        }
    }

    DOCTEST_SUBCASE("low-level manipulation")
    {
        auto root = ctx.parseDataMem(data3, libyang::DataFormat::JSON);

        DOCTEST_SUBCASE("Transplant a tree")
        {
            // Before:
            // example-schema2:contWithTwoNodes (node)   example-schema:contWithTwoNodes (root)
            //                                                         |
            //                                                        one
            //
            // After:
            // example-schema2:contWithTwoNodes (node)   example-schema:contWithTwoNodes (root)
            //                   |
            //                  one
            //
            auto node = ctx.newPath("/example-schema2:contWithTwoNodes");
            // Transplant "one" into the new tree.
            auto one = root->findPath("/example-schema2:contWithTwoNodes/one");

            DOCTEST_SUBCASE("Don't unlink the original tree")
            {
                // *nothing*
            }

            DOCTEST_SUBCASE("Also unlink the original tree")
            {
                // Should not make a difference since insertChild calls unlink anyway.
                one->unlink();
            }
            node.insertChild(*one);
            one.reset();

            // "one" is now reachable from the new tree (`node`).
            REQUIRE(node.findPath("/example-schema2:contWithTwoNodes/one"));
            // "one" is no longer reachable from the original (`root`).
            REQUIRE(!root->findPath("/example-schema2:contWithTwoNodes/one"));
        }

        DOCTEST_SUBCASE("Just insert a node to the same place")
        {
            // Before:
            // example-schema:first (root)
            //          |
            //        second
            //
            // After:
            // example-schema:first (root)
            //          |
            //        second
            //
            auto one = root->findPath("/example-schema2:contWithTwoNodes/one");
            root->findPath("/example-schema2:contWithTwoNodes")->insertChild(*one);

            // `one` is still reachable after freeing `root`.
            root = std::nullopt;
            REQUIRE(one->schema().path() == "/example-schema2:contWithTwoNodes/one");
        }

        DOCTEST_SUBCASE("Unlink and insert a node to the same place")
        {
            // Before:
            // example-schema:first (root)
            //          |
            //        second
            //
            //  ... unlink second ...
            //
            // After:
            // example-schema:first (root)
            //          |
            //        second
            //
            auto one = root->findPath("/example-schema2:contWithTwoNodes/one");
            one->unlink();
            root->findPath("/example-schema2:contWithTwoNodes")->insertChild(*one);
        }

        DOCTEST_SUBCASE("Unlink two children separately, connect them as siblings and reconnect to parent")
        {
            // Beginning:
            // example-schema:contWithTwoNodes
            //         |    |
            //        one  two
            auto one = root->findPath("/example-schema2:contWithTwoNodes/one");
            auto two = root->findPath("/example-schema2:contWithTwoNodes/two");
            one->unlink();
            two->unlink();
            // Both are now unlinked:
            // example-schema2:contWithTwoNodes
            //
            //                                one  two
            //
            // `one` and `two` are now unreachable from root and from each other:
            REQUIRE(!root->findPath("/example-schema2:contWithTwoNodes/one"));
            REQUIRE(!root->findPath("/example-schema2:contWithTwoNodes/two"));
            REQUIRE(one->previousSibling().schema().path() == "/example-schema2:contWithTwoNodes/one");
            REQUIRE(two->previousSibling().schema().path() == "/example-schema2:contWithTwoNodes/two");

            one->insertSibling(*two);
            // Now `one` and `two` are connected (they are siblings)
            // example-schema2:contWithTwoNodes
            //
            //                                one - two
            //
            // They are reachable from each other but not from the parent.
            REQUIRE(one->previousSibling().schema().path() == "/example-schema2:contWithTwoNodes/two");
            REQUIRE(two->previousSibling().schema().path() == "/example-schema2:contWithTwoNodes/one");
            REQUIRE(!root->findPath("/example-schema2:contWithTwoNodes/one"));
            REQUIRE(!root->findPath("/example-schema2:contWithTwoNodes/two"));
            // Now we connect `one` (which also connects `two` because it's a sibling of `one` and has no parent) again
            // to get the original tree.
            root->findPath("/example-schema2:contWithTwoNodes")->insertChild(*one);
            // Both `one` and `two` are now reachable from `root` again.
            REQUIRE(root->findPath("/example-schema2:contWithTwoNodes/one"));
            REQUIRE(root->findPath("/example-schema2:contWithTwoNodes/two"));
        }
    }

    DOCTEST_SUBCASE("DataNode::merge")
    {
        auto root = ctx.parseDataMem(data2, libyang::DataFormat::JSON);

        DOCTEST_SUBCASE("Merge the same thing")
        {
            auto root2 = ctx.parseDataMem(data2, libyang::DataFormat::JSON);
            root->merge(*root2);
            // Both trees are still reachable.
            REQUIRE(root->findPath("/example-schema:leafInt8")->asTerm().valueStr() == "-43");
            REQUIRE(root2->findPath("/example-schema:leafInt8")->asTerm().valueStr() == "-43");

            DOCTEST_SUBCASE("Delete `root`")
            {
                root = std::nullopt;
                REQUIRE(root2->findPath("/example-schema:leafInt8")->asTerm().valueStr() == "-43");
            }

            DOCTEST_SUBCASE("Delete `root2`")
            {
                root2 = std::nullopt;
                REQUIRE(root->findPath("/example-schema:leafInt8")->asTerm().valueStr() == "-43");
            }
        }

        DOCTEST_SUBCASE("Merge a leaf")
        {
            auto leaf = std::optional{ctx.newPath("/example-schema:leafInt8", "10")};
            REQUIRE(root->findPath("/example-schema:leafInt8")->asTerm().valueStr() == "-43");
            root->merge(*leaf);
            REQUIRE(root->findPath("/example-schema:leafInt8")->asTerm().valueStr() == "10");
            REQUIRE(leaf->asTerm().valueStr() == "10");

            DOCTEST_SUBCASE("Delete `leaf`")
            {
                leaf = std::nullopt;
                REQUIRE(root->findPath("/example-schema:leafInt8")->asTerm().valueStr() == "10");
            }

            DOCTEST_SUBCASE("Delete `root`")
            {
                root = std::nullopt;
                REQUIRE(leaf->asTerm().valueStr() == "10");
            }
        }
    }

    DOCTEST_SUBCASE("user-ordered stuff")
    {
        auto root = ctx.parseDataMem(data4, libyang::DataFormat::JSON);
        std::vector<int32_t> expected;
        auto getNumberOrder = [&root] {
            std::vector<int32_t> res;
            auto siblings = root->firstSibling().siblings();
            for (const auto& sibling : siblings)
            {
                if (sibling.schema().path() != "/example-schema3:values") {
                    continue;
                }
                res.emplace_back(std::get<int32_t>(sibling.asTerm().value()));

            }
            return res;
        };

        DOCTEST_SUBCASE("Don't change initial order")
        {
            expected = {10, 20, 30, 40};
        }

        DOCTEST_SUBCASE("insert at the beginning")
        {
            root->findPath("/example-schema3:values[.='10']")->insertBefore(*root->findPath("/example-schema3:values[.='20']"));
            expected = {20, 10, 30, 40};
        }

        DOCTEST_SUBCASE("insert at the end")
        {
            root->findPath("/example-schema3:values[.='40']")->insertAfter(*root->findPath("/example-schema3:values[.='20']"));
            expected = {10, 30, 40, 20};
        }

        DOCTEST_SUBCASE("insertBefore in the middle")
        {
            root->findPath("/example-schema3:values[.='20']")->insertBefore(*root->findPath("/example-schema3:values[.='40']"));
            expected = {10, 40, 20, 30};
        }

        DOCTEST_SUBCASE("insertAfter in the middle")
        {
            root->findPath("/example-schema3:values[.='20']")->insertAfter(*root->findPath("/example-schema3:values[.='40']"));
            expected = {10, 20, 40, 30};
        }

        DOCTEST_SUBCASE("insertBefore in the same place")
        {
            root->findPath("/example-schema3:values[.='20']")->insertBefore(*root->findPath("/example-schema3:values[.='10']"));
            expected = {10, 20, 30, 40};
        }

        DOCTEST_SUBCASE("insertAfter in the same place")
        {
            root->findPath("/example-schema3:values[.='20']")->insertAfter(*root->findPath("/example-schema3:values[.='30']"));
            expected = {10, 20, 30, 40};
        }

        REQUIRE(getNumberOrder() == expected);
    }

    DOCTEST_SUBCASE("DataNode::duplicateWithSiblings")
    {
        auto root = ctx.parseDataMem(data2, libyang::DataFormat::JSON);
        std::optional<libyang::DataNode> dup;
        DOCTEST_SUBCASE("Just dup")
        {
            dup = root->duplicateWithSiblings();
        }

        DOCTEST_SUBCASE("dup and free the original")
        {
            dup = root->duplicateWithSiblings();
            root = std::nullopt;
        }

        if (root) {
            REQUIRE(root->path() == "/example-schema:leafInt8");
        }

        REQUIRE(dup->path() == "/example-schema:leafInt8");
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

        auto node = ctx.parseDataMem(dataToIter, libyang::DataFormat::JSON)->findPath("/example-schema:bigTree");

        DOCTEST_SUBCASE("range-for loop")
        {
            std::vector<std::string> res;
            for (const auto& it : node->childrenDfs()) {
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

        DOCTEST_SUBCASE("DFS on a leaf")
        {
            node = *node->findPath("/example-schema:bigTree/one/myLeaf");
            for (const auto& it : node->childrenDfs()) {
                REQUIRE(it.path() == "/example-schema:bigTree/one/myLeaf");
            }
        }

        DOCTEST_SUBCASE("standard algorithms")
        {
            auto coll = node->childrenDfs();
            REQUIRE(std::find_if(coll.begin(), coll.end(), [] (const auto& node) {
                return node.path() == "/example-schema:bigTree/two/myList[thekey='432']/thekey";
            }) != coll.end());
        }

        DOCTEST_SUBCASE("incrementing")
        {
            auto coll = node->childrenDfs();
            auto iter = coll.begin();

            DOCTEST_SUBCASE("prefix increment")
            {
                REQUIRE(iter->path() == "/example-schema:bigTree");
                auto newIter = ++iter;
                // Both iterators point to the next element.
                REQUIRE(iter->path() == "/example-schema:bigTree/one");
                REQUIRE(newIter->path() == "/example-schema:bigTree/one");
            }

            DOCTEST_SUBCASE("postfix increment")
            {
                REQUIRE(iter->path() == "/example-schema:bigTree");
                auto newIter = iter++;
                // Only the original iterator points to the next element.
                REQUIRE(iter->path() == "/example-schema:bigTree/one");
                REQUIRE(newIter->path() == "/example-schema:bigTree");
            }
        }

        DOCTEST_SUBCASE("invalidating iterators")
        {
            std::vector<std::string> expectedPaths;
            std::vector<std::string> actualPaths;

            auto coll = node->findPath("/example-schema:bigTree/two")->childrenDfs();
            auto iter = coll.begin();

            DOCTEST_SUBCASE("unlink starting node")
            {

                DOCTEST_SUBCASE("don't free")
                {
                    auto toUnlink = node->findPath("/example-schema:bigTree/two");
                    toUnlink->unlink();

                    REQUIRE_THROWS(coll.begin());
                    REQUIRE_THROWS(*iter);
                }

                DOCTEST_SUBCASE("also free the starting node")
                {
                    node->findPath("/example-schema:bigTree/two")->unlink();

                    REQUIRE_THROWS(coll.begin());
                    REQUIRE_THROWS(*iter);
                }
            }

            DOCTEST_SUBCASE("unlink node from different subtree")
            {
                node->findPath("/example-schema:bigTree/one")->unlink();

                REQUIRE(coll.begin()->path() == "/example-schema:bigTree/two");
                REQUIRE(iter->path() == "/example-schema:bigTree/two");
            }

            DOCTEST_SUBCASE("unlink child of the starting node")
            {
                node->findPath("/example-schema:bigTree/two/myList[thekey='43221']")->unlink();

                REQUIRE_THROWS(coll.begin());
                REQUIRE_THROWS(*iter);
            }

            DOCTEST_SUBCASE("unlink parent of the starting node")
            {
                DOCTEST_SUBCASE("don't free")
                {
                    auto toUnlink = node->findPath("/example-schema:bigTree");
                    toUnlink->unlink();

                    REQUIRE_THROWS(coll.begin());
                    REQUIRE_THROWS(*iter);
                }

                DOCTEST_SUBCASE("also free")
                {
                    node->findPath("/example-schema:bigTree/two")->unlink();

                    REQUIRE_THROWS(coll.begin());
                    REQUIRE_THROWS(*iter);
                }
            }

            DOCTEST_SUBCASE("iterator outlives collection")
            {
                coll = node->findPath("/example-schema:bigTree/two")->childrenDfs();
                REQUIRE_THROWS(*iter);
            }

            DOCTEST_SUBCASE("free the whole tree")
            {
                node = std::nullopt;
                REQUIRE_THROWS(coll.begin());
            }
        }
    }

    DOCTEST_SUBCASE("DataNode::siblings")
    {
        auto root = ctx.parseDataMem(data2, libyang::DataFormat::JSON);
        auto siblings = root->siblings();

        DOCTEST_SUBCASE("No freeing")
        {
            auto iter = siblings.begin();
            REQUIRE((iter++)->path() == "/example-schema:leafInt8");
            REQUIRE((iter++)->path() == "/example-schema:first");
            REQUIRE((iter++)->path() == "/example-schema:bigTree");
            REQUIRE_THROWS(*iter);
        }

        DOCTEST_SUBCASE("Free the original tree")
        {
            root = std::nullopt;
            REQUIRE_THROWS(siblings.begin());
        }

        DOCTEST_SUBCASE("unlinking something that an iterator points to")
        {
            //           A
            //         /   \   .
            //        B     C
            //   iter-^     ^-iter2
            auto iter = siblings.begin();
            auto iter2 = siblings.begin()++;
            // Get a reference to C.
            auto node_C = *iter2;
            // Get rid of the reference to
            root = std::nullopt;
            // Now we unlink C. This means that B and A are freed, because no other references are held. C is not freed,
            // because we have a reference to it through node_C.
            node_C.unlink();

            // `iter` now points to a node that's freed. Everything must be invalidated.
            REQUIRE_THROWS(siblings.begin());
            REQUIRE_THROWS(*iter);
            REQUIRE_THROWS(*iter2);
        }
    }

    DOCTEST_SUBCASE("DataNode::findXPath")
    {
        const auto data3 = R"({
            "example-schema:person": [
                {
                    "name": "John"
                },
                {
                    "name": "Dan"
                },
                {
                    "name": "David"
                }
            ]
        }
        )";

        auto node = ctx.parseDataMem(data3, libyang::DataFormat::JSON);

        DOCTEST_SUBCASE("Copying DataNodeSet")
        {
            auto set = node->findXPath("/example-schema:person[name='Dan']");
            auto copy = set;
        }

        DOCTEST_SUBCASE("find one node")
        {
            auto set = node->findXPath("/example-schema:person[name='Dan']");
            auto iter = set.begin();
            REQUIRE((iter++)->path() == "/example-schema:person[name='Dan']");
            REQUIRE(iter == set.end());
            REQUIRE_THROWS_WITH_AS(*iter, "Dereferenced an .end() iterator", std::out_of_range);
        }

        DOCTEST_SUBCASE("find all list nodes")
        {
            auto set = node->findXPath("/example-schema:person");
            REQUIRE(set.front().path() == "/example-schema:person[name='John']");
            REQUIRE(set.back().path() == "/example-schema:person[name='David']");

            auto iter = set.begin();
            REQUIRE((iter++)->path() == "/example-schema:person[name='John']");
            REQUIRE((iter++)->path() == "/example-schema:person[name='Dan']");
            REQUIRE((iter++)->path() == "/example-schema:person[name='David']");
            REQUIRE(iter == set.end());
            REQUIRE_THROWS_WITH_AS(*iter, "Dereferenced an .end() iterator", std::out_of_range);
        }

        DOCTEST_SUBCASE("Iterator arithmetic operators")
        {
            auto set = node->findXPath("/example-schema:person");

            REQUIRE((set.begin() + 0) == set.begin());
            REQUIRE((set.begin() + 0)->path() == "/example-schema:person[name='John']");
            REQUIRE((set.begin() + 1)->path() == "/example-schema:person[name='Dan']");
            REQUIRE((set.begin() + 2)->path() == "/example-schema:person[name='David']");
            REQUIRE((set.begin() + 3) == set.end());
            REQUIRE_THROWS(set.begin() + 4);

            REQUIRE((set.end() - 0) == set.end());
            REQUIRE((set.end() - 1)->path() == "/example-schema:person[name='David']");
            REQUIRE((set.end() - 2)->path() == "/example-schema:person[name='Dan']");
            REQUIRE((set.end() - 3)->path() == "/example-schema:person[name='John']");
            REQUIRE((set.end() - 3) == set.begin());
            REQUIRE_THROWS(set.end() - 4);

            auto iter = set.end();
            REQUIRE((--iter)->path() == "/example-schema:person[name='David']");
            REQUIRE((--iter)->path() == "/example-schema:person[name='Dan']");
            REQUIRE((--iter)->path() == "/example-schema:person[name='John']");
            REQUIRE_THROWS(--iter);
        }

        DOCTEST_SUBCASE("Set class and iterator invalidation")
        {
            auto set = node->findXPath("/example-schema:person[name='John']");

            DOCTEST_SUBCASE("Set invalidation on freeing the tree")
            {
                node = std::nullopt;
                REQUIRE_THROWS_WITH_AS(set.begin(), "Set is invalid", std::out_of_range);
            }

            DOCTEST_SUBCASE("Iterator invalidation on freeing the tree")
            {
                auto iter = set.begin();
                node = std::nullopt;
                REQUIRE_THROWS_WITH_AS(*iter, "Iterator is invalid", std::out_of_range);
            }

            DOCTEST_SUBCASE("Set invalidation on unlink")
            {
                auto iter = set.begin();
                auto john = node->findPath("/example-schema:person[name='John']");
                john->unlink();
                node = std::nullopt;
                REQUIRE_THROWS_WITH_AS(*iter, "Iterator is invalid", std::out_of_range);
            }
        }
    }

    DOCTEST_SUBCASE("DataNode::findSiblingVal")
    {
        auto root = ctx.parseDataMem(data4, libyang::DataFormat::JSON);
        DOCTEST_SUBCASE("leaflist")
        {
            REQUIRE(root->findSiblingVal(ctx.findPath("/example-schema3:values"), "10")->path() == "/example-schema3:values[.='10']");
            REQUIRE(root->findSiblingVal(ctx.findPath("/example-schema3:values"), "20")->path() == "/example-schema3:values[.='20']");
            REQUIRE(!root->findSiblingVal(ctx.findPath("/example-schema3:values"), "0").has_value());
            REQUIRE_THROWS(root->findSiblingVal(ctx.findPath("/example-schema3:values"), "invalid-value"));
        }

        DOCTEST_SUBCASE("list")
        {
            REQUIRE(root->findSiblingVal(ctx.findPath("/example-schema3:person"), "[name='Dan']")->path() == "/example-schema3:person[name='Dan']");
            REQUIRE(root->findSiblingVal(ctx.findPath("/example-schema3:person"), "[name='George']")->path() == "/example-schema3:person[name='George']");
            REQUIRE(!root->findSiblingVal(ctx.findPath("/example-schema3:person"), "[name='non-existent']"));
            REQUIRE_THROWS(root->findSiblingVal(ctx.findPath("/example-schema3:person"), "invalid-format"));
        }
    }

    DOCTEST_SUBCASE("Working with anydata")
    {
        DOCTEST_SUBCASE("DataNode")
        {
            ctx.setSearchDir(TESTS_DIR);
            ctx.loadModule("ietf-netconf-nmda");
            // To parse a <get-data> reply, I also need to parse the request RPC.
            auto ncRPC = R"(
               <rpc message-id="101" xmlns="urn:ietf:params:xml:ns:netconf:base:1.0">
                   <get-data xmlns="urn:ietf:params:xml:ns:yang:ietf-netconf-nmda" xmlns:ds="urn:ietf:params:xml:ns:yang:ietf-datastores">
                     <datastore>ds:running</datastore>
                   </get-data>
               </rpc>
            )";
            auto parsedOp = ctx.parseOp(ncRPC, libyang::DataFormat::XML, libyang::OperationType::RpcNetconf);

            // Now, let's parse the reply.
            auto ncRPCreply = R"(
                <rpc-reply message-id="101" xmlns="urn:ietf:params:xml:ns:netconf:base:1.0">
                    <data xmlns="urn:ietf:params:xml:ns:yang:ietf-netconf-nmda">
                        <leafInt32 xmlns="http://example.com/">123</leafInt32>
                    </data>
                </rpc-reply>
            )";

            // DataNode::parseOp directly changes the original node, no need to use the return value.
            parsedOp.op->parseOp(ncRPCreply, libyang::DataFormat::XML, libyang::OperationType::ReplyNetconf);
            auto anydataNode = parsedOp.op->findPath("/ietf-netconf-nmda:get-data/data", libyang::OutputNodes::Yes);

            // anydataValue is now the leafInt32 inside the RPC reply
            auto anydataValue = std::get<libyang::DataNode>(*anydataNode->asAny().releaseValue());
            REQUIRE(anydataValue.path() == "/example-schema:leafInt32");
            REQUIRE(std::get<int32_t>(anydataValue.asTerm().value()) == 123);
        }

        DOCTEST_SUBCASE("JSON")
        {
            lyd_node* node;
            lyd_new_path2(nullptr, libyang::retrieveContext(ctx), "/example-schema:myData", "[1,2,3]", 0, LYD_ANYDATA_JSON, 0, nullptr, &node);

            auto wrapped = libyang::wrapRawNode(node);

            REQUIRE(std::get<libyang::JSON>(wrapped.asAny().releaseValue().value()).content == "[1,2,3]");
        }
    }

    DOCTEST_SUBCASE("DataNode::next, DataNode::prev, DataNode::firstSibling, and DataNode::child")
    {
        auto root = ctx.parseDataMem(data2, libyang::DataFormat::JSON);
        REQUIRE(root->path() == "/example-schema:leafInt8");

        DOCTEST_SUBCASE("use nextSibling to go to last sibling")
        {
            REQUIRE(root->nextSibling()->path() == "/example-schema:first");
            REQUIRE(root->nextSibling()->nextSibling()->path() == "/example-schema:bigTree");
            REQUIRE(root->nextSibling()->nextSibling()->nextSibling() == std::nullopt);
        }

        DOCTEST_SUBCASE("previousSibling wraps around")
        {
            REQUIRE(root->previousSibling().path() == "/example-schema:bigTree");
            REQUIRE(root->previousSibling().previousSibling().path() == "/example-schema:first");
            REQUIRE(root->previousSibling().previousSibling().previousSibling().path() == "/example-schema:leafInt8");
        }

        DOCTEST_SUBCASE("first sibling")
        {
            REQUIRE(root->firstSibling() == root);
            REQUIRE(root->previousSibling().firstSibling() == root);
            REQUIRE(root->nextSibling()->firstSibling() == root);
        }

        DOCTEST_SUBCASE("child")
        {
            REQUIRE(root->findPath("/example-schema:bigTree")->child()->path() == "/example-schema:bigTree/one");
            REQUIRE(!root->findPath("/example-schema:leafInt8")->child().has_value());
        }
    }
}
