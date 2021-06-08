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
})";
using namespace std::string_view_literals;

const auto data = R"({
  "example-schema:leafInt32": 420
}
)"sv;

TEST_CASE("Data Node manipulation")
{
    libyang::Context ctx;
    ctx.parseModuleMem(example_schema, libyang::SchemaFormat::Yang);

    auto node = ctx.parseDataMem(data.data(), libyang::DataFormat::JSON);
    auto str = node.printStr(libyang::DataFormat::JSON, libyang::PrintFlags::WithSiblings | libyang::PrintFlags::KeepEmptyCont);
    REQUIRE(data == std::string_view(str.get().get()));
}
