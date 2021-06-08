/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#include "trompeloeil_doctest.hpp"
#include "Context.hpp"


const auto example_schema = R"(
module example-schema {
    yang-version 1.1;
    namespace "http://example.com/";
    prefix coze;

    leaf leafInt32 {
        description "A 32-bit integer leaf.";
        type int32;
    }

    leaf active {
        type boolean;
    }
})";

const auto data = R"({
  "example-schema:leafInt32": 420
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

    DOCTEST_SUBCASE("Creating views")
    {
        auto node = ctx.parseDataMem(data, libyang::DataFormat::JSON);

        DOCTEST_SUBCASE("Node exists")
        {
            auto view = node.findPath("/example-schema:leafInt32");
            REQUIRE(view);
            REQUIRE(view->path() == "/example-schema:leafInt32");
        }

        DOCTEST_SUBCASE("Invalid node")
        {
            REQUIRE_THROWS_WITH_AS(node.findPath("/mod:nein"), "Error in DataNode::findPath (7)", std::runtime_error);
        }

        DOCTEST_SUBCASE("Node doesn't exist in the tree")
        {
            REQUIRE(node.findPath("/example-schema:active") == std::nullopt);
        }
    }
}
