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

    DOCTEST_SUBCASE("schema path doesn't exist")
    {
        REQUIRE_THROWS_WITH_AS(ctx->findPath("/example-schema:hi"), "Couldn't find schema node: /example-schema:hi", libyang::Error);
        REQUIRE_THROWS_WITH_AS(ctx->findPath("/oof:leafBinary"), "Couldn't find schema node: /oof:leafBinary", libyang::Error);
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
}
