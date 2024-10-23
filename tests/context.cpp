/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <doctest/doctest.h>
#include <fstream>
#include <libyang-cpp/Context.hpp>
#include <libyang-cpp/Utils.hpp>
#include "example_schema.hpp"
#include "pretty_printers.hpp"
#include "test_vars.hpp"

using namespace std::literals;

const auto valid_yin_model = R"(
<?xml version="1.0" encoding="UTF-8"?>
<module name="test"
        xmlns="urn:ietf:params:xml:ns:yang:yin:1"
        xmlns:t="http://example.com">
  <namespace uri="http://example.com"/>
  <prefix value="t"/>
</module>
)"s;

const auto valid_yang_model = R"(
    module test {
      namespace "http://example.com";
      prefix "t";

      leaf someLeaf {
        type string;
      }
    }
)"s;

const auto imported_module = R"(
    module importedModule {
      namespace "http://example.com/importedModule";
      prefix "ab";

      leaf myLeaf {
        type string;
      }
    }
)"s;

const auto model_with_import = R"(
    module withImport {
      namespace "http://example.com/withImport";
      prefix "t";

      import importedModule {
        prefix "im";
      }

      leaf someLeaf {
        type string;
      }
    }
)"s;

TEST_CASE("context")
{
    std::optional<libyang::Context> ctx{std::in_place, std::nullopt, libyang::ContextOptions::NoYangLibrary | libyang::ContextOptions::DisableSearchCwd};

    DOCTEST_SUBCASE("parseModule")
    {
        std::string mod;
        libyang::SchemaFormat format;
        DOCTEST_SUBCASE("valid")
        {
            DOCTEST_SUBCASE("yang")
            {
                mod = valid_yang_model;
                format = libyang::SchemaFormat::YANG;
            }

            DOCTEST_SUBCASE("yin")
            {
                mod = valid_yin_model;
                format = libyang::SchemaFormat::YIN;
            }

            REQUIRE(ctx->parseModule(mod, format).name() == "test");

            REQUIRE(ctx->getModule("test", std::nullopt)->name() == "test");
        }

        DOCTEST_SUBCASE("invalid")
        {
            format = libyang::SchemaFormat::YANG;
            mod = "blablabla";
            REQUIRE_THROWS_WITH_AS(ctx->parseModule(mod, format), "Can't parse module: LY_EVALID", std::runtime_error);
        }
    }

    DOCTEST_SUBCASE("Loading modules by name")
    {
        DOCTEST_SUBCASE("module exists")
        {
            ctx->setSearchDir(TESTS_DIR / "yang");
            auto mod = ctx->loadModule("mod1", std::nullopt, {
                "feature1",
                "feature2"
            });

            REQUIRE(mod.name() == "mod1");
            REQUIRE(mod.featureEnabled("feature1"));
            REQUIRE(mod.featureEnabled("feature2"));
            REQUIRE(!mod.featureEnabled("feature3"));
            REQUIRE_THROWS(mod.featureEnabled("invalid"));
        }

        DOCTEST_SUBCASE("module does not exist")
        {
            REQUIRE_THROWS(ctx->loadModule("invalid"));
        }
    }

    DOCTEST_SUBCASE("Get module extensions")
    {
        ctx->setSearchDir(TESTS_DIR / "yang");
        auto modYangPatch = ctx->loadModule("ietf-yang-patch", std::nullopt);
        auto modRestconf = ctx->getModule("ietf-restconf", "2017-01-26");
        REQUIRE(modRestconf);
        REQUIRE(modRestconf->name() == "ietf-restconf");

        REQUIRE(!modRestconf->implemented());
        REQUIRE_THROWS_WITH_AS(modRestconf->extensionInstances(), "Module \"ietf-restconf\" not implemented", libyang::Error);
        REQUIRE_THROWS_WITH_AS(modRestconf->extensionInstance("yang-errors"), "Module \"ietf-restconf\" not implemented", libyang::Error);

        modRestconf->setImplemented();
        REQUIRE(modRestconf->implemented());
        REQUIRE(modRestconf->extensionInstances().size() == 2);

        REQUIRE(modRestconf->extensionInstances()[0].argument() == "yang-errors");
        REQUIRE(modRestconf->extensionInstances()[0].definition().name() == "yang-data");
        REQUIRE(modRestconf->extensionInstance("yang-errors").argument() == "yang-errors");
        REQUIRE(modRestconf->extensionInstance("yang-errors").definition().name() == "yang-data");

        REQUIRE(modRestconf->extensionInstances()[1].argument() == "yang-api");
        REQUIRE(modRestconf->extensionInstances()[1].definition().name() == "yang-data");
        REQUIRE(modRestconf->extensionInstance("yang-api").argument() == "yang-api");
        REQUIRE(modRestconf->extensionInstance("yang-api").definition().name() == "yang-data");

        REQUIRE_THROWS_WITH_AS(modRestconf->extensionInstance("yay"), "Extension \"yay\" not defined in module \"ietf-restconf\"", libyang::Error);
    }

    DOCTEST_SUBCASE("Module RPC nodes")
    {
        auto mod = ctx->parseModule(example_schema, libyang::SchemaFormat::YANG);
        auto rpcs = mod.actionRpcs();
        REQUIRE(rpcs.size() == 1);
        REQUIRE(rpcs[0].module().name() == "example-schema");
        REQUIRE(rpcs[0].name() == "myRpc");

        REQUIRE(ctx->parseModule(example_schema2, libyang::SchemaFormat::YANG).actionRpcs().empty());
    }

    DOCTEST_SUBCASE("context lifetime")
    {
        ctx->parseModule(valid_yang_model, libyang::SchemaFormat::YANG);

        DOCTEST_SUBCASE("Data nodes")
        {
            auto node = ctx->newPath("/test:someLeaf", "123");
            ctx.reset();
            // Node is still reachable.
            REQUIRE(node.path() == "/test:someLeaf");
        }

        DOCTEST_SUBCASE("Modules")
        {
            auto mod = ctx->getModule("test", std::nullopt);
            ctx.reset();
            // Module is still reachable.
            REQUIRE(mod->name() == "test");
        }
    }

    DOCTEST_SUBCASE("Context::newPath2")
    {
        ctx->parseModule(example_schema2, libyang::SchemaFormat::YANG);
        auto nodes = ctx->newPath2("/example-schema2:contWithTwoNodes/one", "1");
        REQUIRE(nodes.createdNode->path() == "/example-schema2:contWithTwoNodes/one");
        REQUIRE(nodes.createdParent->path() == "/example-schema2:contWithTwoNodes");

        nodes = ctx->newPath2("/example-schema2:contWithTwoNodes");
        REQUIRE(nodes.createdNode->path() == "/example-schema2:contWithTwoNodes");
        REQUIRE(nodes.createdParent->path() == "/example-schema2:contWithTwoNodes");
    }

    DOCTEST_SUBCASE("Module::identities")
    {
        auto module = ctx->parseModule(example_schema, libyang::SchemaFormat::YANG);
        auto identities = module.identities();
        REQUIRE(identities.size() == 4);
        REQUIRE(identities.at(0).name() == "food");
        REQUIRE(identities.at(1).name() == "fruit");
        REQUIRE(identities.at(2).name() == "pizza");
        REQUIRE(identities.at(3).name() == "hawaii");

        auto module4 = ctx->parseModule(example_schema4, libyang::SchemaFormat::YANG);
        auto identities4 = module4.identities();
        REQUIRE(identities4.size() == 3);
        REQUIRE(identities4.at(0).name() == "pizza");
        REQUIRE(identities4.at(1).name() == "carpaccio");
        REQUIRE(identities4.at(2).name() == "another-carpaccio");

        REQUIRE(identities.at(0) == identities.at(0));
        REQUIRE(identities.at(0) != identities.at(1));
        REQUIRE(identities.at(2) != identities4.at(0)); // it's a different pizza

        std::set<libyang::Identity, libyang::SomeOrder> allIdentities;
        for (const auto& mod : {module, module4}) {
            for (const auto& identity : mod.identities()) {
                allIdentities.insert(identity);
            }
        }

        std::vector<std::string> expectedNames {
            {"example-schema:food"},
            {"example-schema:fruit"},
            {"example-schema:hawaii"},
            {"example-schema:pizza"},
            {"example-schema4:another-carpaccio"},
            {"example-schema4:carpaccio"},
            {"example-schema4:pizza"},
        };
        std::vector<std::string> actual;
        std::transform(allIdentities.begin(), allIdentities.end(), std::back_inserter(actual), libyang::qualifiedName);
        REQUIRE(actual == expectedNames);
    }

    DOCTEST_SUBCASE("Module::features")
    {
        std::vector<std::string> expectedAllFeatures{
            "feature1",
            "feature2",
            "feature3",
        };
        std::vector<std::string> expectedEnabledFeatures;
        std::optional<libyang::Module> mod;
        auto filename = TESTS_DIR / "yang" / "mod1.yang";
        auto yangContent = [&filename]() {
            std::ifstream ifs(filename);
            std::istreambuf_iterator<char> begin(ifs), end;
            return std::string(begin, end);
        }();

        DOCTEST_SUBCASE("feature subset")
        {
            expectedEnabledFeatures = {
                "feature1",
                "feature2",
            };

            DOCTEST_SUBCASE("loadModule")
            {
                ctx->setSearchDir(TESTS_DIR / "yang");
                mod = ctx->loadModule("mod1", std::nullopt, expectedEnabledFeatures);
            }

            DOCTEST_SUBCASE("parseModule path")
            {
                mod = ctx->parseModule(filename, libyang::SchemaFormat::YANG, expectedEnabledFeatures);
            }

            DOCTEST_SUBCASE("parseModule data")
            {
                mod = ctx->parseModule(yangContent, libyang::SchemaFormat::YANG, expectedEnabledFeatures);
            }
        }

        DOCTEST_SUBCASE("wildcarded features")
        {
            expectedEnabledFeatures = {
                "feature1",
                "feature2",
                "feature3",
            };

            DOCTEST_SUBCASE("loadModule")
            {
                ctx->setSearchDir(TESTS_DIR / "yang");
                mod = ctx->loadModule("mod1", std::nullopt, {"*"});
            }

            DOCTEST_SUBCASE("parseModule path")
            {
                mod = ctx->parseModule(filename, libyang::SchemaFormat::YANG, {"*"});
            }

            DOCTEST_SUBCASE("parseModule data")
            {
                mod = ctx->parseModule(yangContent, libyang::SchemaFormat::YANG, {"*"});
            }
        }

        DOCTEST_SUBCASE("no features")
        {
            DOCTEST_SUBCASE("loadModule")
            {
                ctx->setSearchDir(TESTS_DIR / "yang");
                mod = ctx->loadModule("mod1", std::nullopt, {});
            }

            DOCTEST_SUBCASE("parseModule path")
            {
                mod = ctx->parseModule(filename, libyang::SchemaFormat::YANG, {});
            }

            DOCTEST_SUBCASE("parseModule data")
            {
                mod = ctx->parseModule(yangContent, libyang::SchemaFormat::YANG, {});
            }
        }

        std::vector<std::string> allFeatures, enabledFeatures;
        REQUIRE(!!mod);
        for (const auto& feature : mod->features()) {
            allFeatures.emplace_back(feature.name());
            if (feature.isEnabled()) {
                enabledFeatures.emplace_back(feature.name());
            }
        }

        REQUIRE(allFeatures == expectedAllFeatures);
        REQUIRE(enabledFeatures == expectedEnabledFeatures);
    }

    DOCTEST_SUBCASE("Module::setImplemented")
    {
        ctx->setSearchDir(TESTS_DIR / "yang");
        auto mod = ctx->loadModule("mod1", std::nullopt, {});
        REQUIRE(!mod.featureEnabled("feature1"));
        REQUIRE(!mod.featureEnabled("feature2"));
        mod.setImplemented({{"feature1"}});
        REQUIRE(mod.featureEnabled("feature1"));
        REQUIRE(!mod.featureEnabled("feature2"));
        mod.setImplemented();
        REQUIRE(mod.featureEnabled("feature1"));
        REQUIRE(!mod.featureEnabled("feature2"));
        mod.setImplemented(libyang::AllFeatures{});
        REQUIRE(mod.featureEnabled("feature1"));
        REQUIRE(mod.featureEnabled("feature2"));
        REQUIRE(mod.featureEnabled("feature3"));

        REQUIRE_THROWS_AS(mod.setImplemented({{"nonexisting"}}), libyang::ErrorWithCode);
    }

    DOCTEST_SUBCASE("Context::modules")
    {
        ctx->setSearchDir(TESTS_DIR / "yang");
        ctx->loadModule("mod1", std::nullopt, {});
        ctx->parseModule(valid_yang_model, libyang::SchemaFormat::YANG);
        auto modules = ctx->modules();
        REQUIRE(modules.size() == 8);
        REQUIRE(modules.at(0).name() == "ietf-yang-metadata");
        REQUIRE(modules.at(0).ns() == "urn:ietf:params:xml:ns:yang:ietf-yang-metadata");
        REQUIRE(modules.at(1).name() == "yang");
        REQUIRE(modules.at(1).ns() == "urn:ietf:params:xml:ns:yang:1");
        REQUIRE(modules.at(2).name() == "ietf-inet-types");
        REQUIRE(modules.at(2).ns() == "urn:ietf:params:xml:ns:yang:ietf-inet-types");
        REQUIRE(modules.at(3).name() == "ietf-yang-types");
        REQUIRE(modules.at(3).ns() == "urn:ietf:params:xml:ns:yang:ietf-yang-types");
        REQUIRE(modules.at(4).name() == "ietf-yang-schema-mount");
        REQUIRE(modules.at(4).ns() == "urn:ietf:params:xml:ns:yang:ietf-yang-schema-mount");
        REQUIRE(modules.at(5).name() == "ietf-yang-structure-ext");
        REQUIRE(modules.at(5).ns() == "urn:ietf:params:xml:ns:yang:ietf-yang-structure-ext");
        REQUIRE(modules.at(6).name() == "mod1");
        REQUIRE(modules.at(6).ns() == "http://example.com");
        REQUIRE(*modules.at(6).revision() == "2021-11-15");
        REQUIRE(modules.at(7).name() == "test");
        REQUIRE(modules.at(7).ns() == "http://example.com");
        REQUIRE(modules.at(7).revision() == std::nullopt);
    }

    DOCTEST_SUBCASE("Module comparison")
    {
        ctx->setSearchDir(TESTS_DIR / "yang");
        auto mod = ctx->loadModule("mod1", std::nullopt, {});

        DOCTEST_SUBCASE("Same module loaded later")
        {
            REQUIRE(mod == ctx->loadModule("mod1", std::nullopt, {}));
        }

        DOCTEST_SUBCASE("Same module, different contexts")
        {
            std::optional<libyang::Context> ctx2{std::in_place, std::nullopt, libyang::ContextOptions::NoYangLibrary | libyang::ContextOptions::DisableSearchCwd};
            ctx2->setSearchDir(TESTS_DIR / "yang");

            REQUIRE(mod != ctx2->loadModule("mod1", std::nullopt, {}));
        }
    }

    DOCTEST_SUBCASE("Context::registerModuleCallback")
    {
        auto numCalled = 0;
        ctx->registerModuleCallback([&numCalled](auto modName, auto, auto, auto) -> std::optional<libyang::ModuleInfo> {
            numCalled++;
            if (modName == "example-schema") {
                return libyang::ModuleInfo{
                    .data = example_schema,
                    .format = libyang::SchemaFormat::YANG
                };
            }

            return std::nullopt;
        });

        REQUIRE(ctx->loadModule("example-schema").name() == "example-schema");
        REQUIRE_THROWS_AS(ctx->loadModule("doesnt-exist"), libyang::Error);
        REQUIRE(numCalled == 2);
    }

    DOCTEST_SUBCASE("Implemented modules")
    {
        ctx->registerModuleCallback([](auto modName, auto, auto, auto) -> std::optional<libyang::ModuleInfo> {
            if (modName == "withImport") {
                return libyang::ModuleInfo{
                    .data = model_with_import,
                    .format = libyang::SchemaFormat::YANG
                };
            }

            if (modName == "importedModule") {
                return libyang::ModuleInfo{
                    .data = imported_module,
                    .format = libyang::SchemaFormat::YANG
                };
            }
            return std::nullopt;
        });

        REQUIRE(ctx->loadModule("withImport").implemented());
        REQUIRE(!ctx->getModule("importedModule", std::nullopt)->implemented());

        REQUIRE(ctx->getModuleImplemented("withImport").has_value());
        REQUIRE(!ctx->getModuleImplemented("importedModule").has_value());

        REQUIRE(ctx->getModuleLatest("withImport"));
        REQUIRE(ctx->getModuleLatest("importedModule"));
    }

    DOCTEST_SUBCASE("Context::parseData")
    {
        ctx->parseModule(example_schema2, libyang::SchemaFormat::YANG);
        auto parsed = ctx->parseData("{}"s, libyang::DataFormat::JSON);
        REQUIRE(parsed.has_value());
        REQUIRE(parsed->schema().path() == "/ietf-yang-schema-mount:schema-mounts");
    }

    DOCTEST_SUBCASE("Context::parseData")
    {
        ctx->parseModule(example_schema, libyang::SchemaFormat::YANG);
        auto data = ctx->parseData(TESTS_DIR / "test_data.json", libyang::DataFormat::JSON);
        REQUIRE(data);
        REQUIRE(data->findPath("/example-schema:leafInt8")->asTerm().valueStr() == "-43");
    }

    DOCTEST_SUBCASE("Context::parseOp")
    {
        DOCTEST_SUBCASE("OperationType::RpcYang")
        {
            ctx->parseModule(example_schema, libyang::SchemaFormat::YANG);
            std::string data = R"({"example-schema:myRpc":{"inputLeaf":"str"}})";
            auto node = ctx->parseOp(data, libyang::DataFormat::JSON, libyang::OperationType::RpcYang);
            REQUIRE(node.tree.has_value());
            std::string out = *node.tree->printStr(libyang::DataFormat::JSON, libyang::PrintFlags::Shrink);
            REQUIRE(data == out);
        }
    }


    DOCTEST_SUBCASE("Context::parseExt")
    {
        ctx->setSearchDir(TESTS_DIR / "yang");

        DOCTEST_SUBCASE("ietf-restconf")
        {
            auto mod = ctx->loadModule("ietf-restconf", "2017-01-26");
            auto ext = mod.extensionInstance("yang-errors");

            auto node = ctx->parseExtData(ext, R"({"ietf-restconf:errors": {"error": [{"error-type": "protocol", "error-tag": "invalid-attribute", "error-message": "hi"}]}})", libyang::DataFormat::JSON);
            REQUIRE(node);

            auto errorsNode = node->findXPath("/ietf-restconf:errors");
            REQUIRE(errorsNode.size() == 1);
            REQUIRE(errorsNode.begin()->path() == "/ietf-restconf:errors");
            REQUIRE(*errorsNode.begin()->printStr(libyang::DataFormat::JSON, libyang::PrintFlags::WithSiblings | libyang::PrintFlags::KeepEmptyCont) == R"({
  "ietf-restconf:errors": {
    "error": [
      {
        "error-type": "protocol",
        "error-tag": "invalid-attribute",
        "error-message": "hi"
      }
    ]
  },
  "ietf-yang-schema-mount:schema-mounts": {}
}
)");
        }

        DOCTEST_SUBCASE("ietf-yang-patch")
        {
            auto mod = ctx->parseModule(example_schema, libyang::SchemaFormat::YANG);
            auto ext = ctx->loadModule("ietf-yang-patch", "2017-02-22").extensionInstance("yang-patch");

            std::string data;
            libyang::DataFormat dataFormat;

            DOCTEST_SUBCASE("XML")
            {
                dataFormat = libyang::DataFormat::XML;
                data = R"(
<yang-patch xmlns="urn:ietf:params:xml:ns:yang:ietf-yang-patch">
  <patch-id>add-songs-patch</patch-id>
  <edit>
    <edit-id>edit1</edit-id>
    <operation>create</operation>
    <target>/person=John</target>
    <value>
      <person xmlns="http://example.com/coze">
        <name>John</name>
      </person>
    </value>
  </edit>
  <edit>
    <edit-id>edit2</edit-id>
    <operation>create</operation>
    <target>/dummy</target>
    <value>
      <dummy xmlns="http://example.com/coze">I am a dummy</dummy>
    </value>
  </edit>
</yang-patch>
)";
            }
            DOCTEST_SUBCASE("JSON")
            {
                dataFormat = libyang::DataFormat::JSON;
                data = R"({
  "ietf-yang-patch:yang-patch": {
    "patch-id": "add-songs-patch",
    "edit": [
      {
        "edit-id": "edit1",
        "operation": "create",
        "target": "/person=John",
        "value": {
          "example-schema:person": {
            "name": "John"
          }
        }
      },
      {
        "edit-id": "edit2",
        "operation": "create",
        "target": "/dummy",
        "value": {
          "example-schema:dummy": "I am a dummy"
        }
      }
    ]
  }
}
)";
            }

            auto node = ctx->parseExtData(ext, data, dataFormat);
            REQUIRE(node);
            auto edits = node->findXPath("/ietf-yang-patch:yang-patch/edit");
            REQUIRE(edits.size() == 2);

            auto firstValue = edits.begin()->findPath("value");
            REQUIRE(firstValue);
            REQUIRE(*firstValue->printStr(libyang::DataFormat::JSON, libyang::PrintFlags::KeepEmptyCont) == R"({
  "ietf-yang-patch:value": {
    "example-schema:person": {
      "name": "John"
    }
  }
}
)");
            REQUIRE(*firstValue->printStr(libyang::DataFormat::XML, libyang::PrintFlags::KeepEmptyCont) == R"(<value xmlns="urn:ietf:params:xml:ns:yang:ietf-yang-patch">
  <person xmlns="http://example.com/coze">
    <name>John</name>
  </person>
</value>
)");

            auto secondValueNode = (edits.begin() + 1)->findPath("value");
            REQUIRE(secondValueNode);
            auto secondValue = std::get<libyang::DataNode>(secondValueNode->asAny().releaseValue().value());
            REQUIRE(*secondValue.printStr(libyang::DataFormat::JSON, libyang::PrintFlags::KeepEmptyCont) == "{\n  \"example-schema:dummy\": \"I am a dummy\"\n}\n");
            REQUIRE(*secondValue.printStr(libyang::DataFormat::XML, libyang::PrintFlags::KeepEmptyCont) == "<dummy xmlns=\"http://example.com/coze\">I am a dummy</dummy>\n");
        }
    }

    DOCTEST_SUBCASE("Log level")
    {
        REQUIRE(libyang::setLogLevel(libyang::LogLevel::Error) == libyang::LogLevel::Debug);
        REQUIRE(libyang::setLogLevel(libyang::LogLevel::Warning) == libyang::LogLevel::Error);
        REQUIRE(libyang::setLogLevel(libyang::LogLevel::Verbose) == libyang::LogLevel::Warning);
    }

    DOCTEST_SUBCASE("Error info")
    {
        std::vector<libyang::ErrorInfo> expected;
        DOCTEST_SUBCASE("No errors")
        {
        }

        DOCTEST_SUBCASE("Trying to parse invalid module")
        {
            REQUIRE_THROWS(ctx->parseModule("invalid module"s, libyang::SchemaFormat::YANG));
            expected = {
                libyang::ErrorInfo {
                    .appTag = std::nullopt,
                    .level = libyang::LogLevel::Error,
                    .message = "Invalid character sequence \"invalid\", expected a keyword.",
                    .code = libyang::ErrorCode::ValidationFailure,
                    .dataPath = std::nullopt,
                    .schemaPath = std::nullopt,
                    .line = 1,
                    .validationCode = libyang::ValidationErrorCode::Syntax,
                }
            };
        }

        DOCTEST_SUBCASE("Data restriction failure - multiple errors")
        {
            ctx->parseModule(example_schema, libyang::SchemaFormat::YANG);

            DOCTEST_SUBCASE("Store only last error")
            {
                libyang::setLogOptions(libyang::LogOptions::Log | libyang::LogOptions::StoreLast);
                expected = {
                    libyang::ErrorInfo {
                        .appTag = std::nullopt,
                        .level = libyang::LogLevel::Error,
                        .message = "Value \"9001\" is out of type int8 min/max bounds.",
                        .code = libyang::ErrorCode::ValidationFailure,
                        .dataPath = std::nullopt,
                        .schemaPath = "/example-schema:leafInt8",
                        .line = 0,
                        .validationCode = libyang::ValidationErrorCode::Data,
                    }
                };
            }

            DOCTEST_SUBCASE("Store multiple errors")
            {
                libyang::setLogOptions(libyang::LogOptions::Log | libyang::LogOptions::Store);
                expected = {
                    libyang::ErrorInfo {
                        .appTag = std::nullopt,
                        .level = libyang::LogLevel::Error,
                        .message = "Invalid type int8 empty value.",
                        .code = libyang::ErrorCode::ValidationFailure,
                        .dataPath = std::nullopt,
                        .schemaPath = "/example-schema:leafInt8",
                        .line = 0,
                        .validationCode = libyang::ValidationErrorCode::Data,
                    },
                    libyang::ErrorInfo {
                        .appTag = std::nullopt,
                        .level = libyang::LogLevel::Error,
                        .message = "Value \"9001\" is out of type int8 min/max bounds.",
                        .code = libyang::ErrorCode::ValidationFailure,
                        .dataPath = std::nullopt,
                        .schemaPath = "/example-schema:leafInt8",
                        .line = 0,
                        .validationCode = libyang::ValidationErrorCode::Data,
                    }
                };
            }

            REQUIRE_THROWS(ctx->newPath("/example-schema:leafInt8"));
            REQUIRE_THROWS(ctx->newPath("/example-schema:leafInt8", "9001"));

            DOCTEST_SUBCASE("clear errors")
            {
                ctx->cleanAllErrors();
                expected = {};
            }
        }

        REQUIRE(ctx->getErrors() == expected);
    }

    DOCTEST_SUBCASE("schema printing")
    {
        std::optional<libyang::Context> ctx_pp{std::in_place, std::nullopt, libyang::ContextOptions::NoYangLibrary | libyang::ContextOptions::DisableSearchCwd | libyang::ContextOptions::SetPrivParsed};
        auto mod = ctx_pp->parseModule(type_module, libyang::SchemaFormat::YANG);

        REQUIRE(mod.printStr(libyang::SchemaOutputFormat::Tree) == R"(module: type_module
  +--rw anydataBasic?                  anydata
  +--rw anydataWithMandatoryChild      anydata
  +--rw anyxmlBasic?                   anyxml
  +--rw anyxmlWithMandatoryChild       anyxml
  +--rw leafBinary?                    binary
  +--rw leafBits?                      bits
  +--rw leafEnum?                      enumeration
  +--rw leafEnum2?                     enumeration
  +--rw leafNumber?                    int32
  +--rw leafRef?                       -> /custom-prefix:listAdvancedWithOneKey/lol
  +--rw leafRefRelaxed?                -> /custom-prefix:listAdvancedWithOneKey/lol
  +--rw leafString?                    string
  +--rw leafUnion?                     union
  +--rw meal?                          identityref
  +--ro leafWithConfigFalse?           string
  +--rw leafWithDefaultValue?          string
  +--rw leafWithDescription?           string
  +--rw leafWithMandatoryTrue          string
  x--rw leafWithStatusDeprecated?      string
  o--rw leafWithStatusObsolete?        string
  +--rw leafWithUnits?                 int32
  +--rw iid-valid?                     instance-identifier
  +--rw iid-relaxed?                   instance-identifier
  +--rw leafListBasic*                 string
  +--rw leafListWithMinMaxElements*    int32
  +--rw leafListWithUnits*             int32
  +--rw listBasic* [primary-key]
  |  +--rw primary-key    string
  +--rw listAdvancedWithOneKey* [lol]
  |  +--rw lol        string
  |  +--rw notKey1?   string
  |  +--rw notKey2?   string
  |  +--rw notKey3?   binary
  |  +--rw notKey4?   binary
  +--rw listAdvancedWithTwoKey* [first second]
  |  +--rw first     string
  |  +--rw second    string
  +--rw listWithMinMaxElements* [primary-key]
  |  +--rw primary-key    string
  +--rw numeric
  |  +--rw i8?      int8
  |  +--rw i16?     int16
  |  +--rw i32?     int32
  |  +--rw i64?     int64
  |  +--rw deci?    decimal64
  |  +--rw deci1?   decimal64
  |  +--rw u8?      uint8
  |  +--rw u16?     uint16
  |  +--rw u32?     uint32
  |  +--rw u64?     uint64
  +--rw container
  |  +--rw x
  |  |  +--rw x1?    string
  |  |  +--rw x2?    string
  |  |  +---x aaa
  |  +--rw y
  |  +--rw z
  |     +--rw z1?   string
  +--rw containerWithMandatoryChild
     +--rw leafWithMandatoryTrue    string
)");

        // the actual string is not preserved, stuff is reformatted
        REQUIRE(mod.printStr(libyang::SchemaOutputFormat::Yang).substr(0, 131) == R"(module type_module {
  yang-version 1.1;
  namespace "http://example.com/custom-prefix";
  prefix custom-prefix;

  identity food;
)");

        REQUIRE(mod.printStr(libyang::SchemaOutputFormat::CompiledYang).substr(0, 130) == R"(module type_module {
  namespace "http://example.com/custom-prefix";
  prefix custom-prefix;

  identity food {
    derived fruit;)");

        REQUIRE(mod.printStr(libyang::SchemaOutputFormat::Yin).substr(0, 333) == R"(<?xml version="1.0" encoding="UTF-8"?>
<module name="type_module"
        xmlns="urn:ietf:params:xml:ns:yang:yin:1"
        xmlns:custom-prefix="http://example.com/custom-prefix">
  <yang-version value="1.1"/>
  <namespace uri="http://example.com/custom-prefix"/>
  <prefix value="custom-prefix"/>
  <identity name="food"/>
  <identi)");
    }

    DOCTEST_SUBCASE("submodules")
    {
        ctx->setSearchDir(TESTS_DIR / "yang");
        ctx->loadModule("root-mod");

        auto rootMod = ctx->getModule("root-mod", std::nullopt);
        REQUIRE(rootMod);
        REQUIRE(rootMod->name() == "root-mod");

        auto subModNoRevs = ctx->getSubmodule("root-submod-no-revs", std::nullopt);
        REQUIRE(subModNoRevs);
        REQUIRE(subModNoRevs->name() == "root-submod-no-revs");
        REQUIRE(subModNoRevs->module().name() == rootMod->name());

        auto subModRevs = ctx->getSubmodule("root-submod-revs", "2022-11-11");
        REQUIRE(subModRevs);
        REQUIRE(subModRevs->name() == "root-submod-revs");
        REQUIRE(subModRevs->module().name() == rootMod->name());

        REQUIRE(!ctx->getSubmodule("root-mod", std::nullopt));
        REQUIRE(!ctx->getModule("root-submod-no-revs", std::nullopt));

        REQUIRE(subModNoRevs->printStr(libyang::SchemaOutputFormat::Yang) == R"(submodule root-submod-no-revs {
  belongs-to root-mod {
    prefix rm;
  }

  leaf r-s {
    type string;
  }
}
)");
    }
}

TEST_CASE("decimal64")
{
    using namespace libyang;
    REQUIRE(std::string(0_decimal64) == "0.0");
    REQUIRE(std::string(123_decimal64) == "123.0");
    REQUIRE(std::string(123.0_decimal64) == "123.0");
    REQUIRE(std::string(123.00_decimal64) == "123.00");
    REQUIRE(std::string(123.000_decimal64) == "123.000");
    REQUIRE(std::string(123.987_decimal64) == "123.987");
    REQUIRE(std::string(1.000000000000000000_decimal64) == "1.000000000000000000");
    REQUIRE(std::string(-1.000000000000000000_decimal64) == "-1.000000000000000000");
    REQUIRE(std::string(-922337203685477580.7_decimal64) == "-922337203685477580.7");
    REQUIRE(std::string(-922337203685477580.8_decimal64) == "-922337203685477580.8");
    REQUIRE(std::string(922337203685477580.7_decimal64) == "922337203685477580.7");
    REQUIRE(std::string(-92233720368547758.08_decimal64) == "-92233720368547758.08");
    REQUIRE(std::string(92233720368547758.07_decimal64) == "92233720368547758.07");
    REQUIRE(std::string(-92.23372036854775808_decimal64) == "-92.23372036854775808");
    REQUIRE(std::string(92.23372036854775807_decimal64) == "92.23372036854775807");
    REQUIRE(std::string(-9.223372036854775808_decimal64) == "-9.223372036854775808");
    REQUIRE(std::string(9.223372036854775807_decimal64) == "9.223372036854775807");
}
