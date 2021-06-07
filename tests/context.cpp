/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#include "trompeloeil_doctest.hpp"
#include "Context.hpp"

const auto valid_yin_model = R"(
<?xml version="1.0" encoding="UTF-8"?>
<module name="test"
        xmlns="urn:ietf:params:xml:ns:yang:yin:1"
        xmlns:t="http://example.com">
  <namespace uri="http://example.com"/>
  <prefix value="t"/>
</module>
)";

const auto valid_yang_model = R"(
    module test {
      namespace "http://example.com";
      prefix "t";
    }
)";

TEST_CASE("context")
{
    libyang::Context ctx;

    DOCTEST_SUBCASE("parseModuleMem")
    {
        const char* mod;
        libyang::SchemaFormat format;
        DOCTEST_SUBCASE("valid") {
            DOCTEST_SUBCASE("yang") {
                mod = valid_yang_model;
                format = libyang::SchemaFormat::Yang;
            }

            DOCTEST_SUBCASE("yin") {
                mod = valid_yin_model;
                format = libyang::SchemaFormat::Yin;
            }

            // FIXME: The detect function currently doesn't work
            // https://github.com/CESNET/libyang/issues/1610
            // DOCTEST_SUBCASE("detect") {
            //     format = libyang::SchemaFormat::Detect;

            //     DOCTEST_SUBCASE("yang") {
            //         mod = valid_yang_model;
            //     }

            //     DOCTEST_SUBCASE("yin") {
            //         mod = valid_yin_model;
            //     }

            // }
            ctx.parseModuleMem(mod, format);
        }

        DOCTEST_SUBCASE("invalid") {
            format = libyang::SchemaFormat::Yang;
            mod = "blablabla";
            REQUIRE_THROWS_WITH_AS(ctx.parseModuleMem(mod, format), "Can't parse module (7)", std::runtime_error);
        }
    }
}
