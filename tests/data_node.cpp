/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <algorithm>
#include <doctest/doctest.h>
#include <libyang-cpp/Context.hpp>
#include <libyang-cpp/Utils.hpp>
#include <libyang/libyang.h>
#include <stdexcept>
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
)"s;

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
)"s;

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
)"s;

const auto data3 = R"({
  "example-schema2:contWithTwoNodes": {
    "one": 123,
    "two": 456
  }
}
)"s;

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
)"s;

libyang::IdentityRef createIdentityRefValue(libyang::Context ctx, const std::string& module, const std::string& name)
{
    auto modIdentities = ctx.getModuleImplemented(module)->identities();

    auto identSchema = [&] {
        for (const auto& identity : modIdentities) {
            if (identity.name() == name) {
                return identity;
            }
        }

        throw std::logic_error("createIdentityRefValue: Couldn't find the wanted identity");
    }();

    return {.module = module, .name = name, .schema = identSchema};
}

auto dataTypeFor(const std::string& payload)
{
    auto xml = payload.find('<');
    auto json = payload.find('{');
    if (xml == std::string::npos && json == std::string::npos)
        throw std::runtime_error{"tests: Cannot guess JSON/XML payload type"};

    if (xml == std::string::npos)
        return libyang::DataFormat::JSON;
    if (json == std::string::npos)
        return libyang::DataFormat::XML;
    return json < xml ? libyang::DataFormat::JSON : libyang::DataFormat::XML;
}

TEST_CASE("Data Node manipulation")
{
    libyang::Context ctx(std::nullopt, libyang::ContextOptions::NoYangLibrary | libyang::ContextOptions::DisableSearchCwd);
    ctx.parseModule(example_schema, libyang::SchemaFormat::YANG);
    ctx.parseModule(example_schema2, libyang::SchemaFormat::YANG);
    ctx.parseModule(example_schema3, libyang::SchemaFormat::YANG);

    DOCTEST_SUBCASE("Printing")
    {
        auto node = ctx.parseData(data, libyang::DataFormat::JSON);
        auto str = node->printStr(libyang::DataFormat::JSON, libyang::PrintFlags::WithSiblings | libyang::PrintFlags::KeepEmptyCont);
        const auto expected = R"({
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
  },
  "ietf-yang-schema-mount:schema-mounts": {}
}
)";

        REQUIRE(str == expected);

        auto emptyCont = ctx.newPath("/example-schema:first");
        REQUIRE(emptyCont.printStr(libyang::DataFormat::XML, libyang::PrintFlags::WithSiblings) == std::nullopt);
    }

    DOCTEST_SUBCASE("Overwriting a tree with a different tree")
    {
        // The original tree must be freed.
        auto node = ctx.parseData(data, libyang::DataFormat::JSON);
        node = ctx.parseData(data, libyang::DataFormat::JSON);
    }

    DOCTEST_SUBCASE("findPath")
    {
        auto node = ctx.parseData(data, libyang::DataFormat::JSON);

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
                node = ctx.parseData(data, libyang::DataFormat::JSON);
                REQUIRE(nodeLeafInt32->path() == "/example-schema:leafInt32");
            }
        }

        DOCTEST_SUBCASE("Invalid node")
        {
            REQUIRE_THROWS_WITH_AS(node->findPath("/mod:nein"), "Error in DataNode::findPath: LY_EVALID", std::runtime_error);
        }

        DOCTEST_SUBCASE("Node doesn't exist in the tree")
        {
            REQUIRE(node->findPath("/example-schema:active") == std::nullopt);
        }

        DOCTEST_SUBCASE("Finding RPC output nodes")
        {
            auto node = ctx.newPath("/example-schema:myRpc/outputLeaf", "AHOJ", libyang::CreationOptions::Output);
            REQUIRE_THROWS_WITH_AS(node.findPath("/example-schema:myRpc/outputLeaf", libyang::InputOutputNodes::Input),
                                   "Error in DataNode::findPath: LY_EVALID",
                                   libyang::ErrorWithCode);
            REQUIRE(node.findPath("/example-schema:myRpc/outputLeaf", libyang::InputOutputNodes::Output).has_value());
        }
    }

    DOCTEST_SUBCASE("DataNodeTerm")
    {
        auto data = ctx.parseData(dataTypes, libyang::DataFormat::JSON);
        std::string path;
        libyang::Value expected;
        std::string expectedPrinter;

        DOCTEST_SUBCASE("value types")
        {
            DOCTEST_SUBCASE("int8")
            {
                path = "/example-schema:leafInt8";
                expected = int8_t{-43};
                expectedPrinter = "-43";
            }

            DOCTEST_SUBCASE("int16")
            {
                path = "/example-schema:leafInt16";
                expected = int16_t{3000};
                expectedPrinter = "3000";
            }

            DOCTEST_SUBCASE("int32")
            {
                path = "/example-schema:leafInt32";
                expected = int32_t{-391203910};
                expectedPrinter = "-391203910";
            }

            DOCTEST_SUBCASE("int64")
            {
                path = "/example-schema:leafInt64";
                expected = int64_t{-234214214928};
                expectedPrinter = "-234214214928";
            }

            DOCTEST_SUBCASE("uint8")
            {
                path = "/example-schema:leafUInt8";
                expected = uint8_t{43};
                expectedPrinter = "43";
            }

            DOCTEST_SUBCASE("uint16")
            {
                path = "/example-schema:leafUInt16";
                expected = uint16_t{2333};
                expectedPrinter = "2333";
            }

            DOCTEST_SUBCASE("uint32")
            {
                path = "/example-schema:leafUInt32";
                expected = uint32_t{23423422};
                expectedPrinter = "23423422";
            }

            DOCTEST_SUBCASE("uint64")
            {
                path = "/example-schema:leafUInt64";
                expected = uint64_t{453545335344};
                expectedPrinter = "453545335344";
            }

            DOCTEST_SUBCASE("decimal64")
            {
                path = "/example-schema:leafDecimal";
                using namespace libyang::literals;
                expected = 23212131231.43242_decimal64;
                expectedPrinter = "23212131231.43242";
            }

            DOCTEST_SUBCASE("boolean")
            {
                path = "/example-schema:leafBool";
                expected = bool{false};
                expectedPrinter = "false";
            }

            DOCTEST_SUBCASE("string")
            {
                path = "/example-schema:leafString";
                expected = std::string{"AHOJ"};
                expectedPrinter = "AHOJ";
            }

            DOCTEST_SUBCASE("empty")
            {
                path = "/example-schema:leafEmpty";
                expected = libyang::Empty{};
                expectedPrinter = "empty";
            }

            DOCTEST_SUBCASE("binary")
            {
                path = "/example-schema:leafBinary";
                expected = libyang::Binary{{0, 0, 0, 4, 16, 65, 8, 32}, "AAAABBBBCCC="};
                expectedPrinter = "AAAABBBBCCC=";
            }

            DOCTEST_SUBCASE("intOrString")
            {
                path = "/example-schema:intOrString";
                expected = int32_t{14332};
                expectedPrinter = "14332";
            }

            DOCTEST_SUBCASE("leafref")
            {
                path = "/example-schema:bossPerson";
                expected = std::string{"Dan"};
                expectedPrinter = "Dan";
            }

            DOCTEST_SUBCASE("schema-specific union members")
            {
                using namespace libyang::literals;
                path = "/example-schema:pwnedUnion";

                DOCTEST_SUBCASE("decimal64")
                {
                    // the first decimal64 has fraction-digits: 1
                    data = ctx.parseData(R"({"example-schema:pwnedUnion": "123.4"})"s, libyang::DataFormat::JSON);
                    expected = 123.4_decimal64;
                    expectedPrinter = "123.4";
                }

                DOCTEST_SUBCASE("nested union")
                {
                    // this one is nested, and it has fraction-digits: 3
                    data = ctx.parseData(R"({"example-schema:pwnedUnion": "123.456"})"s, libyang::DataFormat::JSON);
                    expected = 123.456_decimal64;
                    expectedPrinter = "123.456";
                }

                DOCTEST_SUBCASE("leafref")
                {
                    data = ctx.parseData(R"({
                    "example-schema:pwnedUnion": "Dan",
                    "example-schema:person": [
                      {
                        "name": "Dan"
                      }
                    ]})"s, libyang::DataFormat::JSON);
                    expected = "Dan"s;
                    expectedPrinter = "Dan";
                }

                DOCTEST_SUBCASE("instance-identifier")
                {
                    // a nice self-referencing instance-identified that goes back to the union
                    data = ctx.parseData(R"({"example-schema:pwnedUnion": "/example-schema:pwnedUnion"})"s, libyang::DataFormat::JSON);
                    expected = libyang::InstanceIdentifier{"/example-schema:pwnedUnion", data->findPath("/example-schema:pwnedUnion")};
                    expectedPrinter = "InstanceIdentifier{/example-schema:pwnedUnion}";
                }

                DOCTEST_SUBCASE("identityref")
                {
                    data = ctx.parseData(R"({"example-schema:pwnedUnion": "example-schema:pizza"})"s, libyang::DataFormat::JSON);
                    expected = createIdentityRefValue(ctx, "example-schema", "pizza");
                    expectedPrinter = "example-schema:pizza";
                }

                DOCTEST_SUBCASE("binary")
                {
                    data = ctx.parseData(R"({"example-schema:pwnedUnion": "AA=="})"s, libyang::DataFormat::JSON);
                    expected = libyang::Binary{{0}, "AA=="};
                    expectedPrinter = "AA==";
                }
            }

            DOCTEST_SUBCASE("instance-identifier")
            {
                DOCTEST_SUBCASE("require-instance = true")
                {
                    path = "/example-schema:targetInstance";
                    expected = libyang::InstanceIdentifier{"/example-schema:leafBool", data->findPath("/example-schema:leafBool")};
                    expectedPrinter = "InstanceIdentifier{/example-schema:leafBool}";
                }

                DOCTEST_SUBCASE("require-instance = false")
                {
                    path = "/example-schema:NOtargetInstance";
                    expected = libyang::InstanceIdentifier{"/example-schema:dummy", std::nullopt};
                    expectedPrinter = "InstanceIdentifier{no-instance, /example-schema:dummy}";
                }
            }

            DOCTEST_SUBCASE("bits")
            {
                path = "/example-schema:flagBits";
                expected = std::vector<libyang::Bit>{{0, "carry"}, {2, "overflow"}};
                expectedPrinter = "carry overflow";
            }

            DOCTEST_SUBCASE("enum")
            {
                path = "/example-schema:pizzaSize";
                expected = libyang::Enum{"large", 0};
                expectedPrinter = "large";
            }

            DOCTEST_SUBCASE("identityref")
            {
                path = "/example-schema:leafFoodTypedef";
                expected = createIdentityRefValue(ctx, "example-schema", "hawaii");
                expectedPrinter = "example-schema:hawaii";
            }

            auto node = data->findPath(path.c_str());
            REQUIRE(node);
            auto term = node->asTerm();
            REQUIRE(term.path() == path);
            REQUIRE(term.value() == expected);
            REQUIRE(std::visit(libyang::ValuePrinter{}, term.value()) == expectedPrinter);
        }

        DOCTEST_SUBCASE("querying Identity schema from a value")
        {
            auto node = data->findPath("/example-schema:leafFoodTypedef");
            auto schema = std::get<libyang::IdentityRef>(node->asTerm().value()).schema;
        }

        DOCTEST_SUBCASE("instance-identifier hasInstance")
        {
            REQUIRE(std::get<libyang::InstanceIdentifier>(data->findPath("/example-schema:targetInstance")
                        ->asTerm().value()).hasInstance());
            REQUIRE(!std::get<libyang::InstanceIdentifier>(data->findPath("/example-schema:NOtargetInstance")
                        ->asTerm().value()).hasInstance());
            REQUIRE_THROWS_WITH_AS(libyang::InstanceIdentifier("/ietf-interfaces:interface", data->findPath("/example-schema:leafBool")),
                    "instance-identifier: got path /ietf-interfaces:interface, but the node points to /example-schema:leafBool", libyang::Error);
        }
    }

    DOCTEST_SUBCASE("default values")
    {
        auto data = ctx.parseData(data4, libyang::DataFormat::JSON);

        auto node = data->findPath("/example-schema3:leafWithDefault")->asTerm();
        REQUIRE(node.hasDefaultValue());
        REQUIRE(node.isImplicitDefault());

        data->newPath("/example-schema3:leafWithDefault", "not-default-value", libyang::CreationOptions::Update);
        node = data->findPath("/example-schema3:leafWithDefault")->asTerm();
        REQUIRE(!node.hasDefaultValue());
        REQUIRE(!node.isImplicitDefault());

        data->newPath("/example-schema3:leafWithDefault", "AHOJ", libyang::CreationOptions::Update);
        node = data->findPath("/example-schema3:leafWithDefault")->asTerm();
        REQUIRE(node.hasDefaultValue());
        REQUIRE(!node.isImplicitDefault());
    }

    DOCTEST_SUBCASE("isTerm")
    {
        REQUIRE(ctx.newPath("/example-schema:leafInt32", "420").isTerm());
        REQUIRE(ctx.newPath("/example-schema3:values", "420").isTerm());
        REQUIRE(!ctx.newPath("/example-schema:bigTree").isTerm());
    }

    DOCTEST_SUBCASE("newPath")
    {
        auto node = std::optional{ctx.newPath("/example-schema:leafInt32", "420")};
        libyang::validateAll(node, libyang::ValidationOptions::NoState);
        auto str = node->printStr(libyang::DataFormat::JSON, libyang::PrintFlags::WithSiblings | libyang::PrintFlags::KeepEmptyCont);
        REQUIRE(str == data);
    }

    DOCTEST_SUBCASE("validateAll throws when you have more references to the node")
    {
        auto node = std::optional{ctx.newPath("/example-schema:leafInt32", "420")};
        auto node2 = node;
        REQUIRE_THROWS_WITH_AS(libyang::validateAll(node, libyang::ValidationOptions::NoState), "validateAll: Node is not a unique reference", libyang::Error);
    }

    DOCTEST_SUBCASE("unlink")
    {
        auto root = ctx.parseData(data2, libyang::DataFormat::JSON);
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
            auto node = ctx.parseData(dataTypes, libyang::DataFormat::JSON)->findPath("/example-schema:leafInt32");

            node->unlinkWithSiblings();
        }

        DOCTEST_SUBCASE("Nodes have a parent")
        {
            auto node = ctx.parseData(data3, libyang::DataFormat::JSON);

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
        auto root = ctx.parseData(data3, libyang::DataFormat::JSON);

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

    DOCTEST_SUBCASE("insert of a parentless node reparents its following siblings")
    {
        auto data = ctx.newPath2("/example-schema2:contWithTwoNodes/one", "333").createdNode;
        data->newPath("/example-schema2:contWithTwoNodes/two", "666");
        auto cont = ctx.newPath2("/example-schema2:contWithTwoNodes").createdNode;
        data->unlinkWithSiblings();
        cont->insertChild(*data);
        REQUIRE(*cont->printStr(libyang::DataFormat::JSON, libyang::PrintFlags::WithSiblings) == R"({
  "example-schema2:contWithTwoNodes": {
    "one": 333,
    "two": 666
  }
}
)");
    }

    DOCTEST_SUBCASE("DataNode::merge")
    {
        auto root = ctx.parseData(data2, libyang::DataFormat::JSON);

        DOCTEST_SUBCASE("Merge the same thing")
        {
            auto root2 = ctx.parseData(data2, libyang::DataFormat::JSON);
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
        auto root = ctx.parseData(data4, libyang::DataFormat::JSON);
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

    DOCTEST_SUBCASE("DataNode::duplicate")
    {
        auto root = ctx.parseData(data2, libyang::DataFormat::JSON);
        std::optional<libyang::DataNode> dup;
        DOCTEST_SUBCASE("Just dup")
        {
            dup = root->duplicate();
        }

        DOCTEST_SUBCASE("dup and free the original")
        {
            dup = root->duplicate();
            root = std::nullopt;
        }

        if (root) {
            REQUIRE(root->path() == "/example-schema:leafInt8");
        }

        REQUIRE(dup->nextSibling() == std::nullopt);
        REQUIRE(dup->path() == "/example-schema:leafInt8");

    }

    DOCTEST_SUBCASE("DataNode::duplicateWithSiblings")
    {
        auto root = ctx.parseData(data2, libyang::DataFormat::JSON);
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

        REQUIRE(dup->nextSibling()->path() == "/example-schema:first");
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
        )"s;

        auto node = ctx.parseData(dataToIter, libyang::DataFormat::JSON)->findPath("/example-schema:bigTree");

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
                "/example-schema:bigTree/two/myList[thekey='213']",
                "/example-schema:bigTree/two/myList[thekey='213']/thekey",
                "/example-schema:bigTree/two/myList[thekey='432']",
                "/example-schema:bigTree/two/myList[thekey='432']/thekey",
                "/example-schema:bigTree/two/myList[thekey='43221']",
                "/example-schema:bigTree/two/myList[thekey='43221']/thekey",
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

        DOCTEST_SUBCASE("Assigning iterators")
        {
            auto coll = node->childrenDfs();
            auto iter1 = coll.begin();
            REQUIRE(iter1->path() == "/example-schema:bigTree");
            auto iter2 = coll.begin();
            REQUIRE(iter2->path() == "/example-schema:bigTree");
            iter1++;
            REQUIRE(iter1->path() == "/example-schema:bigTree/one");
            iter2 = iter1;
            REQUIRE(iter2->path() == "/example-schema:bigTree/one");
        }

        DOCTEST_SUBCASE("range-for loop over immediateChildren")
        {
            std::string path;
            std::vector<std::string> expected;
            DOCTEST_SUBCASE("some children") {
                expected = {
                    "/example-schema:bigTree/one",
                    "/example-schema:bigTree/two",
                };
                path = "/example-schema:bigTree";
            }
            DOCTEST_SUBCASE("no recursion") {
                expected = {
                    "/example-schema:first/second",
                };
                path = "/example-schema:first";
            }
            DOCTEST_SUBCASE("empty") {
                expected = {
                };
                path = "/example-schema:first/second/third/fourth";
            }
            std::vector<std::string> res;
            node = ctx.parseData(dataToIter, libyang::DataFormat::JSON)->findPath(path);
            for (const auto& it : node->immediateChildren()) {
                res.emplace_back(it.path());
            }
            REQUIRE(res == expected);

            // verify that copy constructor works properly
            auto c1 = node->immediateChildren();
            auto c2 = c1;
            res.clear();
            for (const auto& it : c2) {
                res.emplace_back(it.path());
            }
            REQUIRE(res == expected);
        }
    }

    DOCTEST_SUBCASE("DataNode::siblings")
    {
        auto root = ctx.parseData(data2, libyang::DataFormat::JSON);
        auto siblings = root->siblings();

        DOCTEST_SUBCASE("No freeing")
        {
            auto iter = siblings.begin();
            REQUIRE((iter++)->path() == "/example-schema:leafInt8");
            REQUIRE((iter++)->path() == "/example-schema:first");
            REQUIRE((iter++)->path() == "/example-schema:bigTree");
            REQUIRE((iter++)->path() == "/example-schema3:leafWithDefault");
            REQUIRE((iter++)->path() == "/ietf-yang-schema-mount:schema-mounts");
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
        // libyang v3 sorts these alphabetically
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
        )"s;

        auto node = ctx.parseData(data3, libyang::DataFormat::JSON);

        DOCTEST_SUBCASE("Copying DataNodeSet")
        {
            auto set = node->findXPath("/example-schema:person[name='Dan']");
            auto copy = set;
        }

        DOCTEST_SUBCASE("Standard algorithms")
        {
            auto set = node->findXPath("/example-schema:person[name='Dan']");

            std::any_of(set.begin(), set.end(), [](const auto& node) { node.path(); return true; });
        }

        DOCTEST_SUBCASE("find one node")
        {
            auto set = node->findXPath("/example-schema:person[name='Dan']");
            REQUIRE(set.size() == 1);
            auto iter = set.begin();
            REQUIRE((iter++)->path() == "/example-schema:person[name='Dan']");
            REQUIRE(iter == set.end());
            REQUIRE_THROWS_WITH_AS(*iter, "Dereferenced an .end() iterator", std::out_of_range);
        }

        DOCTEST_SUBCASE("find zero nodes")
        {
            auto set = node->findXPath("/example-schema:person[name='non-existent']");
            REQUIRE(set.begin() == set.end());
            REQUIRE(set.size() == 0);
            REQUIRE(set.empty());
        }

        DOCTEST_SUBCASE("find all list nodes")
        {
            auto set = node->findXPath("/example-schema:person");
            REQUIRE(set.size() == 3);

            REQUIRE(set.front().path() == "/example-schema:person[name='Dan']");
            REQUIRE(set.back().path() == "/example-schema:person[name='John']");

            auto iter = set.begin();
            REQUIRE((iter++)->path() == "/example-schema:person[name='Dan']");
            REQUIRE((iter++)->path() == "/example-schema:person[name='David']");
            REQUIRE((iter++)->path() == "/example-schema:person[name='John']");
            REQUIRE(iter == set.end());
            REQUIRE_THROWS_WITH_AS(*iter, "Dereferenced an .end() iterator", std::out_of_range);
        }

        DOCTEST_SUBCASE("Iterator arithmetic operators")
        {
            auto set = node->findXPath("/example-schema:person");

            REQUIRE((set.begin() + 0) == set.begin());
            REQUIRE((set.begin() + 0)->path() == "/example-schema:person[name='Dan']");
            REQUIRE((set.begin() + 1)->path() == "/example-schema:person[name='David']");
            REQUIRE((set.begin() + 2)->path() == "/example-schema:person[name='John']");
            REQUIRE((set.begin() + 3) == set.end());
            REQUIRE_THROWS(set.begin() + 4);

            REQUIRE((set.end() - 0) == set.end());
            REQUIRE((set.end() - 1)->path() == "/example-schema:person[name='John']");
            REQUIRE((set.end() - 2)->path() == "/example-schema:person[name='David']");
            REQUIRE((set.end() - 3)->path() == "/example-schema:person[name='Dan']");
            REQUIRE((set.end() - 3) == set.begin());
            REQUIRE_THROWS(set.end() - 4);

            auto iter = set.end();
            REQUIRE((--iter)->path() == "/example-schema:person[name='John']");
            REQUIRE((--iter)->path() == "/example-schema:person[name='David']");
            REQUIRE((--iter)->path() == "/example-schema:person[name='Dan']");
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
        auto root = ctx.parseData(data4, libyang::DataFormat::JSON);
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
            ctx.setSearchDir(TESTS_DIR / "yang");
            ctx.loadModule("ietf-datastores");
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
                        <leafInt32 xmlns="http://example.com/coze">123</leafInt32>
                    </data>
                </rpc-reply>
            )";

            REQUIRE(parsedOp.op.has_value());
            // DataNode::parseOp directly changes the original node, no need to use the return value.
            parsedOp.op->parseOp(ncRPCreply, libyang::DataFormat::XML, libyang::OperationType::ReplyNetconf);
            auto anydataNode = parsedOp.op->findPath("/ietf-netconf-nmda:get-data/data", libyang::InputOutputNodes::Output);

            // anydataValue is now the leafInt32 inside the RPC reply
            auto anydataValue = std::get<libyang::DataNode>(*anydataNode->asAny().releaseValue());
            REQUIRE(anydataValue.path() == "/example-schema:leafInt32");
            REQUIRE(std::get<int32_t>(anydataValue.asTerm().value()) == 123);
        }

        DOCTEST_SUBCASE("JSON")
        {
            // FIXME: libyang no longer accepts JSON arrays as strings for anydata
            /* DOCTEST_SUBCASE("Context::newPath2") */
            /* { */
            /*     auto jsonAnyDataNode = ctx.newPath2("/example-schema:myData", libyang::JSON{"[1,2,3]"}); */
            /*     REQUIRE(std::get<libyang::JSON>(jsonAnyDataNode.createdNode->asAny().releaseValue().value()).content == "[1,2,3]"); */
            /* } */

            DOCTEST_SUBCASE("DataNode::newPath2")
            {
                auto node = ctx.newPath("/example-schema:leafInt32", "123");
                auto jsonAnyDataNode = node.newPath2("/example-schema:myData", libyang::JSON{R"({"key": "value"})"}).createdNode;
                REQUIRE(!!jsonAnyDataNode);
                auto rawVal = jsonAnyDataNode->asAny().releaseValue().value();
                REQUIRE(std::holds_alternative<libyang::DataNode>(rawVal));
                auto retrieved = std::get<libyang::DataNode>(rawVal);
                REQUIRE(retrieved.path() == "/key");
                REQUIRE(*retrieved.printStr(libyang::DataFormat::JSON, libyang::PrintFlags::Shrink | libyang::PrintFlags::WithSiblings)
                        == R"|({"key":"value"})|");
                REQUIRE(*retrieved.printStr(libyang::DataFormat::XML, libyang::PrintFlags::Shrink | libyang::PrintFlags::WithSiblings)
                        == R"|(<key>value</key>)|");
            }
        }

        DOCTEST_SUBCASE("XML")
        {
            DOCTEST_SUBCASE("Context::newPath2")
            {
                auto xmlAnyDataNode = ctx.newPath2("/example-schema:myData", libyang::XML{"<something>lol</something>"});
                REQUIRE(*std::get<libyang::DataNode>(xmlAnyDataNode.createdNode->asAny().releaseValue().value()).printStr(libyang::DataFormat::XML, libyang::PrintFlags::Shrink) == R"|(<something>lol</something>)|");
            }

            DOCTEST_SUBCASE("DataNode::newPath2")
            {
                auto node = ctx.newPath("/example-schema:leafInt32", "123");
                auto xmlAnyDataNode = node.newPath2("/example-schema:myData", libyang::XML{R"(<something>lol</something>)"}).createdNode;
                REQUIRE(!!xmlAnyDataNode);
                auto rawVal = xmlAnyDataNode->asAny().releaseValue().value();
                REQUIRE(std::holds_alternative<libyang::DataNode>(rawVal));
                auto retrieved = std::get<libyang::DataNode>(rawVal);
                REQUIRE(retrieved.path() == "/something");
                REQUIRE(*retrieved.printStr(libyang::DataFormat::XML, libyang::PrintFlags::Shrink | libyang::PrintFlags::WithSiblings)
                        == R"|(<something>lol</something>)|");
                REQUIRE(*retrieved.printStr(libyang::DataFormat::JSON, libyang::PrintFlags::Shrink | libyang::PrintFlags::WithSiblings)
                        == R"|({"something":"lol"})|");
            }
        }
    }

    DOCTEST_SUBCASE("anyxml")
    {
        libyang::AnydataValue val;

        DOCTEST_SUBCASE("raw array")
        {
            // This is something which can only be reproduced in JSON, and not in XML.
            // Quoting https://datatracker.ietf.org/doc/html/rfc7950#section-7.11:
            //
            //   An anyxml node exists in zero or one instance in the data tree.
            //
            // That means that the XML serialization of multiple <ax/> elements is just not valid.

            auto origJSON = R"|([1,2,3])|"s;

            {
                libyang::CreatedNodes jsonAnyXmlNode;

                DOCTEST_SUBCASE("Context::newPath2")
                {
                    jsonAnyXmlNode = ctx.newPath2("/example-schema:ax", libyang::JSON{origJSON});
                    val = jsonAnyXmlNode.createdNode->asAny().releaseValue();
                }

                DOCTEST_SUBCASE("DataNode::newPath2")
                {
                    auto node = ctx.newPath("/example-schema:leafInt32", "123");
                    jsonAnyXmlNode = node.newPath2("/example-schema:ax", libyang::JSON{origJSON});
                    val = jsonAnyXmlNode.createdNode->asAny().releaseValue();
                }

                DOCTEST_SUBCASE("Context::parseData")
                {
                    auto root = ctx.parseData(R"|({"example-schema:ax":)|"s + origJSON + "}", libyang::DataFormat::JSON);
                    REQUIRE(!!root);
                    jsonAnyXmlNode.createdNode = root->findPath("/example-schema:ax");
                    val = jsonAnyXmlNode.createdNode->asAny().releaseValue();
                }

                REQUIRE(*jsonAnyXmlNode.createdNode->printStr(libyang::DataFormat::JSON, libyang::PrintFlags::Shrink | libyang::PrintFlags::WithSiblings)
                        == R"|({"example-schema:ax":[1,2,3]})|"s);
                REQUIRE(*jsonAnyXmlNode.createdNode->printStr(libyang::DataFormat::XML, libyang::PrintFlags::Shrink | libyang::PrintFlags::WithSiblings)
                        == R"|(<ax xmlns="http://example.com/coze"/>)|"s);
            }

            REQUIRE(!!val);
            REQUIRE(std::holds_alternative<libyang::JSON>(*val));
            auto retrieved = std::get<libyang::JSON>(*val);
            val.reset();
            REQUIRE(retrieved.content == origJSON);
        }

        DOCTEST_SUBCASE("wrapped array")
        {
            // Unlike the raw JSON array above, this thing is already wrapped into an extra element,
            // and therefore it can be represented both in XML and in JSON.
            auto origXML = R"|(<ax xmlns="http://example.com/coze"><x>1</x><x>2</x><x>3</x></ax>)|"s;
            auto origJSON = R"|({"example-schema:ax":{"x":[1,2,3]}})|"s;

            std::optional<libyang::DataNode> root;

            DOCTEST_SUBCASE("XML") {
                root = ctx.parseData(origXML, libyang::DataFormat::XML);
            }

            DOCTEST_SUBCASE("JSON") {
                root = ctx.parseData(origJSON, libyang::DataFormat::JSON);
            }

            REQUIRE(root);
            REQUIRE(*root->printStr(libyang::DataFormat::XML, libyang::PrintFlags::Shrink | libyang::PrintFlags::WithSiblings)
                    == origXML);
            REQUIRE(*root->printStr(libyang::DataFormat::JSON, libyang::PrintFlags::Shrink | libyang::PrintFlags::WithSiblings)
                    == origJSON);

            auto node = root->findPath("/example-schema:ax");
            REQUIRE(node);
            val = node->asAny().releaseValue();
            REQUIRE(!!val);
            REQUIRE(std::holds_alternative<libyang::DataNode>(*val));
            auto innerNode = std::get<libyang::DataNode>(*val);

            std::vector<std::string> values;

            for (const auto& x: innerNode.siblings()) {
                REQUIRE(x.path() == "/example-schema:x");
                REQUIRE(x.isOpaque());
                REQUIRE(!x.isTerm());
                values.push_back(x.asOpaque().value().data());
                REQUIRE_THROWS_WITH_AS(x.asAny(), "Node is not anydata/anyxml", libyang::Error);
            }
            REQUIRE(values == std::vector<std::string>{"1", "2", "3"});
        }

        DOCTEST_SUBCASE("elements")
        {
            auto origXML = R"|(<a/><b/><c/>)|"s;
            auto origJSON = R"|({"a":[null],"b":[null],"c":[null]})|"s;

            DOCTEST_SUBCASE("XML") {
                DOCTEST_SUBCASE("Context::newPath2")
                {
                    auto xmlAnyXmlNode = ctx.newPath2("/example-schema:ax", libyang::XML{origXML});
                    val = xmlAnyXmlNode.createdNode->asAny().releaseValue();
                }

                DOCTEST_SUBCASE("DataNode::newPath2")
                {
                    auto node = ctx.newPath("/example-schema:leafInt32", "123");
                    val = node.newPath2("/example-schema:ax", libyang::XML{"<a/><b/><c/>"}).createdNode->asAny().releaseValue();
                }
            }

            DOCTEST_SUBCASE("JSON") {
                DOCTEST_SUBCASE("Context::newPath2")
                {
                    auto xmlAnyXmlNode = ctx.newPath2("/example-schema:ax", libyang::JSON{origJSON});
                    val = xmlAnyXmlNode.createdNode->asAny().releaseValue();
                }

                DOCTEST_SUBCASE("DataNode::newPath2")
                {
                    auto node = ctx.newPath("/example-schema:leafInt32", "123");
                    val = node.newPath2("/example-schema:ax", libyang::JSON{origJSON}).createdNode->asAny().releaseValue();
                }
            }

            REQUIRE(!!val);
            REQUIRE(std::holds_alternative<libyang::DataNode>(*val));
            auto retrieved = std::get<libyang::DataNode>(*val);
            val.reset();
            REQUIRE(retrieved.path() == "/a");
            REQUIRE(*retrieved.printStr(libyang::DataFormat::XML, libyang::PrintFlags::Shrink | libyang::PrintFlags::WithSiblings)
                    == origXML);
            REQUIRE(*retrieved.printStr(libyang::DataFormat::JSON, libyang::PrintFlags::Shrink | libyang::PrintFlags::WithSiblings)
                    == origJSON);
        }
    }

    DOCTEST_SUBCASE("DataNode::newPath2 normal node")
    {
        auto node = ctx.newPath("/example-schema:leafInt32", "123");
        auto createdNodes = node.newPath2("/example-schema:bigTree/one");
        REQUIRE(createdNodes.createdNode->path() == "/example-schema:bigTree/one");
        REQUIRE(createdNodes.createdParent->path() == "/example-schema:bigTree");
    }

    DOCTEST_SUBCASE("DataNode::next, DataNode::prev, DataNode::firstSibling, DataNode::parent, and DataNode::child")
    {
        auto root = ctx.parseData(data2, libyang::DataFormat::JSON);
        REQUIRE(root->path() == "/example-schema:leafInt8");

        DOCTEST_SUBCASE("use nextSibling to go to last sibling")
        {
            REQUIRE(root->nextSibling()->path() == "/example-schema:first");
            REQUIRE(root->nextSibling()->nextSibling()->path() == "/example-schema:bigTree");
            REQUIRE(root->nextSibling()->nextSibling()->nextSibling()->path() == "/example-schema3:leafWithDefault");
            REQUIRE(root->nextSibling()->nextSibling()->nextSibling()->nextSibling()->path() == "/ietf-yang-schema-mount:schema-mounts");
            REQUIRE(root->nextSibling()->nextSibling()->nextSibling()->nextSibling()->nextSibling() == std::nullopt);
        }

        DOCTEST_SUBCASE("previousSibling wraps around")
        {
            REQUIRE(root->previousSibling().path() == "/ietf-yang-schema-mount:schema-mounts");
            REQUIRE(root->previousSibling().previousSibling().path() == "/example-schema3:leafWithDefault");
            REQUIRE(root->previousSibling().previousSibling().previousSibling().path() == "/example-schema:bigTree");
            REQUIRE(root->previousSibling().previousSibling().previousSibling().previousSibling().path() == "/example-schema:first");
            REQUIRE(root->previousSibling().previousSibling().previousSibling().previousSibling().previousSibling().path() == "/example-schema:leafInt8");
            REQUIRE(root->previousSibling().previousSibling().previousSibling().previousSibling().previousSibling().previousSibling().path() == "/ietf-yang-schema-mount:schema-mounts");
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

        DOCTEST_SUBCASE("parent")
        {
            REQUIRE(root->findPath("/example-schema:bigTree/one")->parent()->path() == "/example-schema:bigTree");
            REQUIRE(!root->findPath("/example-schema:leafInt8")->parent().has_value());
        }
    }

    DOCTEST_SUBCASE("XPath evaluation")
    {
        auto leaf = ctx.parseData(data2, libyang::DataFormat::JSON);
        REQUIRE(leaf->path() == "/example-schema:leafInt8");
        auto third = leaf->findPath("/example-schema:first/second/third");
        REQUIRE(!!third);
        REQUIRE(third->path() == "/example-schema:first/second/third");
        REQUIRE(!!third->child());
        REQUIRE(third->child()->path() == "/example-schema:first/second/third/fourth");

        // When the context is empty, then relative lookups always start at the context node,
        // no matter wheter that "forest" node is a top-level leaf...
        REQUIRE(findXPathAt(std::nullopt, *leaf, "example-schema:first").size() == 1);
        REQUIRE(findXPathAt(std::nullopt, *leaf, "example-schema:first").front().path() == "/example-schema:first");
        // ...or something that's nested somewhere deep in the forest...
        REQUIRE(findXPathAt(std::nullopt, *third, "example-schema:first").size() == 1);
        REQUIRE(findXPathAt(std::nullopt, *third, "example-schema:first").front().path() == "/example-schema:first");
        // ...which means that direct children of the "forest node" aren't found...
        REQUIRE(findXPathAt(std::nullopt, *third, "example-schema:fourth").empty());

        // Just a sanity check that this is indeed a direct child:
        REQUIRE(!!third->findPath("fourth"));
        // When the "context node" exists, then it's used properly, which means that the context is not at the root:
        REQUIRE(findXPathAt(leaf, *leaf, "example-schema:first").empty());
        REQUIRE(findXPathAt(leaf, *third, "example-schema:first").empty());
        REQUIRE(findXPathAt(third, *leaf, "example-schema:first").empty());
        REQUIRE(findXPathAt(third, *third, "example-schema:first").empty());
        // ...and that means that it can find the direct children:
        REQUIRE(findXPathAt(third, *third, "fourth").size() == 1);
        REQUIRE(findXPathAt(third, *third, "fourth").front().path() == "/example-schema:first/second/third/fourth");
        // ...even if the forest is something "else":
        REQUIRE(findXPathAt(third, *leaf, "fourth").size() == 1);
        REQUIRE(findXPathAt(third, *leaf, "fourth").front().path() == "/example-schema:first/second/third/fourth");

        // Absolute paths should keep working of course
        REQUIRE(findXPathAt(std::nullopt, *leaf, "/example-schema:first/second/third/fourth").size() == 1);
        REQUIRE(findXPathAt(std::nullopt, *leaf, "/example-schema:first/second/third/fourth").front().path() == "/example-schema:first/second/third/fourth");
        REQUIRE(findXPathAt(std::nullopt, *third, "/example-schema:first/second/third/fourth").size() == 1);
        REQUIRE(findXPathAt(std::nullopt, *third, "/example-schema:first/second/third/fourth").front().path() == "/example-schema:first/second/third/fourth");
        // ...even when provided with some arbitrary context:
        REQUIRE(findXPathAt(leaf, *leaf, "/example-schema:first/second/third/fourth").size() == 1);
        REQUIRE(findXPathAt(leaf, *leaf, "/example-schema:first/second/third/fourth").front().path() == "/example-schema:first/second/third/fourth");
        REQUIRE(findXPathAt(leaf, *third, "/example-schema:first/second/third/fourth").size() == 1);
        REQUIRE(findXPathAt(leaf, *third, "/example-schema:first/second/third/fourth").front().path() == "/example-schema:first/second/third/fourth");
        REQUIRE(findXPathAt(third, *leaf, "/example-schema:first/second/third/fourth").size() == 1);
        REQUIRE(findXPathAt(third, *leaf, "/example-schema:first/second/third/fourth").front().path() == "/example-schema:first/second/third/fourth");
        REQUIRE(findXPathAt(third, *third, "/example-schema:first/second/third/fourth").size() == 1);
        REQUIRE(findXPathAt(third, *third, "/example-schema:first/second/third/fourth").front().path() == "/example-schema:first/second/third/fourth");
    }

    DOCTEST_SUBCASE("subtree parsing")
    {
        ctx.parseModule(example_schema5, libyang::SchemaFormat::YANG);
        auto nodeX = ctx.newPath("/example-schema5:x");
        REQUIRE(nodeX.path() == "/example-schema5:x");

        DOCTEST_SUBCASE("leaf in a container in an empty parent")
        {
            std::string data;

            DOCTEST_SUBCASE("namespace-qualified")
            {
                data = R"({
    "example-schema5:x_b": {
        "x_b_leaf": 666
    }
}
)"s;
            }

            DOCTEST_SUBCASE("implicit namespace")
            {
                data = R"({
    "x_b": {
        "x_b_leaf": 666
    }
}
)"s;
            }

            nodeX.parseSubtree(data, libyang::DataFormat::JSON,
                    libyang::ParseOptions::Strict | libyang::ParseOptions::NoState | libyang::ParseOptions::ParseOnly);
            REQUIRE(*nodeX.printStr(libyang::DataFormat::JSON, libyang::PrintFlags::WithSiblings) == R"({
  "example-schema5:x": {
    "x_b": {
      "x_b_leaf": 666
    }
  }
}
)");
            auto x_b = nodeX.findPath("/example-schema5:x/x_b");
            REQUIRE(x_b);
            auto x_b_leaf = nodeX.findPath("/example-schema5:x/x_b/x_b_leaf");
            REQUIRE(x_b_leaf);
            REQUIRE(std::visit(libyang::ValuePrinter{}, x_b_leaf->asTerm().value()) == "666");
        }

        DOCTEST_SUBCASE("parse two children")
        {
            nodeX.parseSubtree(R"({"example-schema5:x_b": {"x_b_leaf": 666}, "example-schema5:x_a": 42})", libyang::DataFormat::JSON, libyang::ParseOptions::Strict | libyang::ParseOptions::NoState | libyang::ParseOptions::ParseOnly);
            REQUIRE(nodeX.findPath("/example-schema5:x/x_a"));
            REQUIRE(nodeX.findPath("/example-schema5:x/x_b"));
            REQUIRE(nodeX.findPath("/example-schema5:x/x_b/x_b_leaf"));
        }

        DOCTEST_SUBCASE("malformed")
        {
            std::string data;
            DOCTEST_SUBCASE("wrong leaf format")
            {
                data = R"({"example-schema5:x_b":{"x_b_leaf":"wtf"}})";
            }

            DOCTEST_SUBCASE("unknown element")
            {
                data = R"({"example-schema5:x_b":{"yay":"wtf"}})";
            }

            DOCTEST_SUBCASE("JSON array")
            {
                data = "[]";
            }

            REQUIRE_THROWS_WITH_AS(nodeX.parseSubtree(data,
                        libyang::DataFormat::JSON,
                        libyang::ParseOptions::Strict | libyang::ParseOptions::NoState | libyang::ParseOptions::ParseOnly),
                    "DataNode::parseSubtree: lyd_parse_data failed: LY_EVALID",
                    libyang::ErrorWithCode);
        }

        DOCTEST_SUBCASE("empty data")
        {
            libyang::DataFormat format;
            std::string data;

            DOCTEST_SUBCASE("JSON dict")
            {
                format = libyang::DataFormat::JSON;
                data = "{}";
            }

            DOCTEST_SUBCASE("XML")
            {
                format = libyang::DataFormat::XML;
                data = "";
            }
            nodeX.parseSubtree(data, format,
                    libyang::ParseOptions::Strict | libyang::ParseOptions::NoState | libyang::ParseOptions::ParseOnly);
            REQUIRE(*nodeX.printStr(libyang::DataFormat::JSON, libyang::PrintFlags{}) == "{\n\n}\n");
        }
    }

    DOCTEST_SUBCASE("Creating a container of DataNodes")
    {
        std::set<libyang::DataNode, libyang::SomeOrder> set;
    }

    DOCTEST_SUBCASE("DataNode metadata")
    {
        ctx.setSearchDir(TESTS_DIR);
        auto netconf = ctx.loadModule("ietf-netconf", "2011-06-01");
        auto ietfOrigin = ctx.loadModule("ietf-origin", "2018-02-14"); // So that we have another attribute to work with.
        auto netconfDeletePresenceCont = ctx.newPath("/example-schema:presenceContainer");

        DOCTEST_SUBCASE("invalid attribute")
        {
            REQUIRE_THROWS(netconfDeletePresenceCont.newMeta(netconf, "invalid", "no"));
        }

        DOCTEST_SUBCASE("iterating metadata")
        {
            netconfDeletePresenceCont.newMeta(netconf, "operation", "delete");
            netconfDeletePresenceCont.newMeta(ietfOrigin, "origin", "ietf-origin:default");

            std::vector<std::pair<std::string, std::string>> expected;
            std::vector<std::pair<std::string, std::string>> actual;

            DOCTEST_SUBCASE("Don't remove anything")
            {
                expected = {
                    {"operation", "delete"},
                    {"origin", "ietf-origin:default"}
                };
            }

            DOCTEST_SUBCASE("Remove one attribute")
            {
                std::string toErase;
                DOCTEST_SUBCASE("erase the first one")
                {
                    toErase = "operation";
                    expected = {
                        {"origin", "ietf-origin:default"}
                    };
                }

                DOCTEST_SUBCASE("erase the second one")
                {
                    toErase = "origin";
                    expected = {
                        {"operation", "delete"},
                    };
                }

                auto meta = netconfDeletePresenceCont.meta();
                for (auto it = meta.begin(); it != meta.end(); /* nothing */) {
                    if (it->name() == toErase) {
                        it = meta.erase(it);
                    } else {
                        it++;
                    }
                }
            }

            auto meta = netconfDeletePresenceCont.meta();
            std::transform(meta.begin(), meta.end(), std::back_inserter(actual), [] (const auto& it) { return std::pair{it.name(), it.valueStr()}; });
            REQUIRE(actual == expected);
        }

        DOCTEST_SUBCASE("valid attribute")
        {
            netconfDeletePresenceCont.newMeta(netconf, "operation", "delete");
            netconfDeletePresenceCont.newMeta(ietfOrigin, "origin", "ietf-origin:default");
            REQUIRE(*netconfDeletePresenceCont.printStr(libyang::DataFormat::XML, libyang::PrintFlags::WithSiblings)
                    == R"(<presenceContainer xmlns="http://example.com/coze" xmlns:nc="urn:ietf:params:xml:ns:netconf:base:1.0" nc:operation="delete" xmlns:or="urn:ietf:params:xml:ns:yang:ietf-origin" or:origin="or:default"/>)" "\n");
        }

        DOCTEST_SUBCASE("opaque nodes")
        {
            auto opaqueLeaf = ctx.newPath("/example-schema:leafInt32", std::nullopt, libyang::CreationOptions::Opaque);
            REQUIRE_THROWS(opaqueLeaf.newMeta(netconf, "operation", "delete"));
            opaqueLeaf.newAttrOpaqueJSON("ietf-netconf", "operation", "delete");
            REQUIRE(*opaqueLeaf.printStr(libyang::DataFormat::JSON, libyang::PrintFlags::WithSiblings)
                    == R"({
  "example-schema:leafInt32": "",
  "@example-schema:leafInt32": {
    "ietf-netconf:operation": "delete"
  }
}
)");
        }

        DOCTEST_SUBCASE("opaque nodes for sysrepo ops data discard")
        {
            auto discard1 = ctx.newOpaqueJSON("sysrepo", "discard-items", libyang::JSON{"/example-schema:a"});
            REQUIRE(!!discard1);
            auto discard2 = ctx.newOpaqueJSON("sysrepo", "discard-items", libyang::JSON{"/example-schema:b"});
            REQUIRE(!!discard2);
            discard1->insertSibling(*discard2);
            REQUIRE(*discard1->printStr(libyang::DataFormat::JSON, libyang::PrintFlags::WithSiblings)
                    == R"({
  "sysrepo:discard-items": "/example-schema:a",
  "sysrepo:discard-items": "/example-schema:b"
}
)");
        }

        DOCTEST_SUBCASE("RESTCONF RPC output")
        {
            std::optional<libyang::DataNode> out;
            auto expectedJson = R"({
  "example-schema:output": {
    "outputLeaf": "AHOJ",
    "another": "yay"
  }
}
)"s;
            auto expectedXml = R"(<output xmlns="http://example.com/coze">
  <outputLeaf>AHOJ</outputLeaf>
  <another>yay</another>
</output>
)"s;

            auto data = ctx.newPath2("/example-schema:myRpc/outputLeaf", "AHOJ", libyang::CreationOptions::Output).createdNode;
            REQUIRE(data);
            data->newPath("/example-schema:myRpc/another", "yay", libyang::CreationOptions::Output);

            DOCTEST_SUBCASE("JSON") {
                out = ctx.newOpaqueJSON(std::string(data->schema().module().name()), "output", std::nullopt);
            }

            DOCTEST_SUBCASE("XML") {
                out = ctx.newOpaqueXML(std::string(data->schema().module().ns()), "output", std::nullopt);
            }

            REQUIRE(out);
            data->unlinkWithSiblings();
            out->insertChild(*data);

            REQUIRE(*out->printStr(libyang::DataFormat::JSON, libyang::PrintFlags::WithSiblings) == expectedJson);
            REQUIRE(*out->printStr(libyang::DataFormat::XML, libyang::PrintFlags::WithSiblings) == expectedXml);
        }
    }

    DOCTEST_SUBCASE("Extension nodes")
    {
        ctx.setSearchDir(TESTS_DIR);
        auto mod = ctx.loadModule("ietf-restconf", "2017-01-26");
        auto ext = mod.extensionInstance("yang-errors");

        auto node = ctx.newExtPath("/ietf-restconf:errors", std::nullopt, ext, std::nullopt);
        REQUIRE(node);
        REQUIRE(node->schema().name() == "errors");
        REQUIRE(*node->printStr(libyang::DataFormat::JSON, libyang::PrintFlags::WithSiblings | libyang::PrintFlags::KeepEmptyCont) == R"({
  "ietf-restconf:errors": {}
}
)");

        REQUIRE(node->newPath("ietf-restconf:error[1]/error-type", "protocol"));
        REQUIRE(node->newPath("ietf-restconf:error[1]/error-tag", "invalid-attribute"));
        REQUIRE(node->newExtPath("/ietf-restconf:errors/error[1]/error-message", "ahoj", ext));
        REQUIRE_THROWS_WITH(node->newPath("ietf-restconf:error[1]/error-message", "duplicate create"), "Couldn't create a node with path 'ietf-restconf:error[1]/error-message': LY_EEXIST");
        REQUIRE(*node->printStr(libyang::DataFormat::JSON, libyang::PrintFlags::WithSiblings | libyang::PrintFlags::KeepEmptyCont) == R"({
  "ietf-restconf:errors": {
    "error": [
      {
        "error-type": "protocol",
        "error-tag": "invalid-attribute",
        "error-message": "ahoj"
      }
    ]
  }
}
)");

        REQUIRE(node->newExtPath("/ietf-restconf:errors/error[2]/error-type", "transport", ext));
        REQUIRE(node->newExtPath("/ietf-restconf:errors/error[2]/error-tag", "invalid-attribute", ext));
        REQUIRE(node->newPath("ietf-restconf:error[2]/error-message", "aaa"));
        REQUIRE(*node->printStr(libyang::DataFormat::JSON, libyang::PrintFlags::WithSiblings | libyang::PrintFlags::KeepEmptyCont) == R"({
  "ietf-restconf:errors": {
    "error": [
      {
        "error-type": "protocol",
        "error-tag": "invalid-attribute",
        "error-message": "ahoj"
      },
      {
        "error-type": "transport",
        "error-tag": "invalid-attribute",
        "error-message": "aaa"
      }
    ]
  }
}
)");
    }

    DOCTEST_SUBCASE("operations")
    {
        ctx.setSearchDir(TESTS_DIR);
        ctx.loadModule("ietf-netconf");
        ctx.loadModule("ietf-datastores");
        ctx.loadModule("ietf-netconf-nmda");

        DOCTEST_SUBCASE("notifications") {
            std::string payload;
            auto opType = libyang::OperationType::DataYang;

            DOCTEST_SUBCASE("RESTCONF JSON") {
                payload = R"(
                {
                  "ietf-restconf:notification" : {
                    "eventTime" : "2013-12-21T00:01:00Z",
                    "example-schema:event" : {
                      "event-class" : "fault"
                    }
                  }
                }
                )";
                opType = libyang::OperationType::NotificationRestconf;
            }

            DOCTEST_SUBCASE("NETCONF XML") {
                payload = R"(
                <notification
                   xmlns="urn:ietf:params:xml:ns:netconf:notification:1.0">
                   <eventTime>2013-12-21T00:01:00Z</eventTime>
                   <event xmlns="http://example.com/coze">
                      <event-class>fault</event-class>
                    </event>
                </notification>
                )";
                opType = libyang::OperationType::NotificationNetconf;
            }

            auto notif = ctx.parseOp(payload, dataTypeFor(payload), opType);
            REQUIRE(notif.tree);
            REQUIRE(notif.tree->path() == "/notification");
            auto node = notif.tree->child();
            REQUIRE(node);
            REQUIRE(node->path() == "/notification/eventTime");
            REQUIRE(node->asOpaque().value() == "2013-12-21T00:01:00Z");

            REQUIRE(notif.op);
            node = notif.op->findPath("/example-schema:event/event-class");
            REQUIRE(!!node);
            REQUIRE(std::visit(libyang::ValuePrinter{}, node->asTerm().value()) == "fault");
        }

        DOCTEST_SUBCASE("invalid notification") {
            REQUIRE_THROWS_WITH_AS(ctx.parseOp("", libyang::DataFormat::JSON, libyang::OperationType::NotificationRestconf),
                    "Can't parse a standalone rpc/action/notification into operation data tree: LY_EVALID", libyang::Error);

            REQUIRE_THROWS_WITH_AS(ctx.parseOp("{}", libyang::DataFormat::JSON, libyang::OperationType::NotificationRestconf),
                    "Can't parse a standalone rpc/action/notification into operation data tree: LY_EVALID", libyang::Error);

            REQUIRE_THROWS_WITH_AS(ctx.parseOp("", libyang::DataFormat::XML, libyang::OperationType::NotificationNetconf),
                    "Can't parse a standalone rpc/action/notification into operation data tree: LY_ENOT", libyang::Error);

            /* libyang::setLogOptions(libyang::LogOptions::Log | libyang::LogOptions::Store); */
            REQUIRE_THROWS_WITH_AS(ctx.parseOp(R"(
                {
                  "ietf-restconf:notification" : {
                    "eventTime" : "2013-12-21T00:01:00Z",
                    "WTF:is-this" : {
                    }
                  }
                }
            )", libyang::DataFormat::JSON, libyang::OperationType::NotificationRestconf),
                    "Can't parse a standalone rpc/action/notification into operation data tree: LY_EVALID", libyang::Error);
        }

        DOCTEST_SUBCASE("RESTCONF RPCs") {
            // NETCONF RPCs are tested separately (the setup for generating them is different):
            // libyang::OperationType::RpcNetconf a.k.a. LYD_TYPE_RPC_NETCONF expects the envelope for parsing,
            // whereas with RESTCONF, libyang::OperationType::RpcRestconf a.k.a. LYD_TYPE_RPC_RESTCONF the actual RPC
            // name is encoded in the URL, and therefore passed out-of-band to the libyang API.

            std::string rpcInput, rpcOutput;

            DOCTEST_SUBCASE("JSON") {
                rpcInput = R"(
                {
                  "example-schema:input": {
                    "inputLeaf": "foo bar baz"
                  }
                }
                )";
                rpcOutput = R"(
                {
                    "example-schema:output" : {
                        "outputLeaf": "666 42"
                    }
                }
                )";
            }

            DOCTEST_SUBCASE("XML") {
                rpcInput = R"(
                <input xmlns="http://example.com/coze">
                  <inputLeaf>foo bar baz</inputLeaf>
                </input>
                )";
                rpcOutput = R"(
                <output xmlns="http://example.com/coze">
                  <outputLeaf>666 42</outputLeaf>
                </output>
                )";
            }

            auto rpcTree = ctx.newPath("/example-schema:myRpc");
            auto rpcOp = rpcTree.parseOp(rpcInput, dataTypeFor(rpcInput), libyang::OperationType::RpcRestconf);
            REQUIRE(!rpcOp.op); // as per docs -- this argument is not used by the C API
            REQUIRE(rpcOp.tree);
            REQUIRE(rpcOp.tree->path() == "/example-schema:input");
            REQUIRE(rpcOp.tree->isOpaque());
            REQUIRE(!rpcOp.tree->child()); // nothing gets "parsed" here, the result is in the input tree (!)

            auto node = rpcTree.findPath("/example-schema:myRpc/inputLeaf");
            REQUIRE(!!node);
            REQUIRE(std::visit(libyang::ValuePrinter{}, node->asTerm().value()) == "foo bar baz");
            REQUIRE(!node->child());

            auto replyTree = ctx.newPath("/example-schema:myRpc");
            auto response = replyTree.parseOp(rpcOutput, dataTypeFor(rpcOutput), libyang::OperationType::ReplyRestconf);
            REQUIRE(!response.op); // as per docs -- this argument is not used by the C API
            REQUIRE(response.tree);
            REQUIRE(response.tree->path() == "/example-schema:output");
            REQUIRE(response.tree->isOpaque());
            REQUIRE(!response.tree->child()); // nothing gets "parsed" here, the result is put into the tree that parseOp() operated on (!)
            CAPTURE(*response.tree->printStr(libyang::DataFormat::JSON, libyang::PrintFlags::WithSiblings | libyang::PrintFlags::KeepEmptyCont));
            CAPTURE(*replyTree.printStr(libyang::DataFormat::JSON, libyang::PrintFlags::WithSiblings | libyang::PrintFlags::KeepEmptyCont));

            node = replyTree.findPath("/example-schema:myRpc/outputLeaf", libyang::InputOutputNodes::Output);
            REQUIRE(!!node);
            REQUIRE(std::visit(libyang::ValuePrinter{}, node->asTerm().value()) == "666 42");
            REQUIRE(!node->child());
        }

        DOCTEST_SUBCASE("malformed input") {
            auto rpcTree = ctx.newPath("/example-schema:myRpc");

            DOCTEST_SUBCASE("empty string") {
                REQUIRE_THROWS_WITH_AS(rpcTree.parseOp("", libyang::DataFormat::JSON, libyang::OperationType::RpcRestconf),
                        "Can't parse into operation data tree: LY_EVALID", libyang::Error);
            }

            DOCTEST_SUBCASE("empty JSON") {
                REQUIRE_THROWS_WITH_AS(rpcTree.parseOp("{}", libyang::DataFormat::JSON, libyang::OperationType::RpcRestconf),
                        "Can't parse into operation data tree: LY_EVALID", libyang::Error);
            }

            DOCTEST_SUBCASE("invalid data") {
                REQUIRE_THROWS_WITH_AS(rpcTree.parseOp(R"(
                    {
                      "example-schema:input": {
                        "WTF": "foo bar baz"
                      }
                    }
                    )", libyang::DataFormat::JSON, libyang::OperationType::RpcRestconf),
                        "Can't parse into operation data tree: LY_EVALID", libyang::Error);
            }
        }
    }

    DOCTEST_SUBCASE("comparing") {
        libyang::Context ctxB(std::nullopt, libyang::ContextOptions::NoYangLibrary | libyang::ContextOptions::DisableSearchCwd);
        ctxB.parseModule(example_schema, libyang::SchemaFormat::YANG);

        // Differences from `data2`:
        // - `/example-schema:leafInt8` is missing
        // - `/example-schema:first/second/third/fourth/fifth` has a different value
        const auto fifth666 = R"({
  "example-schema:first": {
    "second": {
      "third": {
        "fourth": {
            "fifth": "666"
        }
      }
    }
  }
}
)"s;

        auto rootA = ctx.parseData(data2, libyang::DataFormat::JSON);
        auto rootB = ctxB.parseData(data2, libyang::DataFormat::JSON);
        auto rootC = ctxB.parseData(fifth666, libyang::DataFormat::JSON);

        auto secondA = *rootA->findPath("/example-schema:first/second");
        auto secondB = *rootB->findPath("/example-schema:first/second");
        auto secondC = *rootC->findPath("/example-schema:first/second");
        REQUIRE(secondA.isEqual(secondB));
        REQUIRE(secondA.siblingsEqual(secondB));
        REQUIRE(secondA.isEqual(secondC));
        REQUIRE(!secondA.siblingsEqual(secondC));
        REQUIRE(!secondA.isEqual(secondC, libyang::DataCompare::FullRecursion));
    }
}

TEST_CASE("union data types")
{
    std::optional<libyang::Context> ctxWithParsed{std::in_place, std::nullopt,
        libyang::ContextOptions::SetPrivParsed | libyang::ContextOptions::NoYangLibrary | libyang::ContextOptions::DisableSearchCwd};
    ctxWithParsed->parseModule(with_inet_types_module, libyang::SchemaFormat::YANG);
    std::string input, expectedPlugin;

    DOCTEST_SUBCASE("IPv6")
    {
        DOCTEST_SUBCASE("no zone")
        {
            input = "::1";
        }
        DOCTEST_SUBCASE("with zone")
        {
            input = "::1%lo";
        }
        DOCTEST_SUBCASE("mapped IPv4")
        {
            input = "::ffff:192.0.2.1";
        }
        expectedPlugin = "ipv6";
    }

    DOCTEST_SUBCASE("IPv4")
    {
        input = "127.0.0.1";
        expectedPlugin = "ipv4";
    }

    DOCTEST_SUBCASE("string")
    {
        input = "foo-bar.example.org";
        expectedPlugin = "string";
    }

    auto node = ctxWithParsed->newPath("/with-inet-types:hostname", input);
    REQUIRE(node.asTerm().valueType().internalPluginId().find(expectedPlugin) != std::string::npos);
    REQUIRE_THROWS_AS(node.asTerm().valueType().name(), libyang::ParsedInfoUnavailable);
}
