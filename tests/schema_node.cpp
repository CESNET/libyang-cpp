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

using namespace std::string_literals;

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

        leaf notKey {
            type string;
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
