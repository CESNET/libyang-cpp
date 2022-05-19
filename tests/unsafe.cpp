/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <doctest/doctest.h>
#include <libyang-cpp/Context.hpp>
#include <libyang-cpp/Utils.hpp>
#include <libyang/libyang.h>
#include "example_schema.hpp"
#include "test_vars.hpp"

TEST_CASE("Unsafe methods")
{
    ly_ctx* ctx;
    ly_ctx_new(nullptr, 0, &ctx);
    // When wrapping raw lyd_nodes, the context struct however is not managed and needs to be released manually (for
    // example with a unique_ptr), like below.
    auto ctx_deleter = std::unique_ptr<ly_ctx, decltype(&ly_ctx_destroy)>(ctx, ly_ctx_destroy);
    lys_parse_mem(ctx, example_schema, LYS_IN_YANG, nullptr);
    auto data = R"({ "example-schema:leafInt32": 32 })";

    DOCTEST_SUBCASE("createUnmanagedContext")
    {
        DOCTEST_SUBCASE("Custom deleter")
        {
            ctx_deleter.release();

            auto wrapped = libyang::createUnmanagedContext(ctx, ly_ctx_destroy);

        }

        DOCTEST_SUBCASE("No custom deleter")
        {
            auto wrapped = libyang::createUnmanagedContext(ctx, nullptr);
        }
    }

    DOCTEST_SUBCASE("wrapRawNode")
    {
        lyd_node* node;
        lyd_parse_data_mem(ctx, data, LYD_JSON, 0, LYD_VALIDATE_PRESENT, &node);

        // The wrapped node does take ownership of the lyd_node* and deletes it on destruction of the class.
        auto wrapped = libyang::wrapRawNode(node);
        REQUIRE(wrapped.path() == "/example-schema:leafInt32");

        DOCTEST_SUBCASE("Automatic memory")
        {
            // nothing
        }

        DOCTEST_SUBCASE("Releasing the raw node")
        {
            auto releasedNode = libyang::releaseRawNode(wrapped);
            lyd_free_all(releasedNode);
        }

        DOCTEST_SUBCASE("Retrieving the raw node")
        {
            // Calling this should not make memory problems.
            libyang::getRawNode(wrapped);
        }

        REQUIRE_THROWS(libyang::wrapRawNode(nullptr));
    }

    DOCTEST_SUBCASE("wrapUnmanagedRawNode")
    {
        lyd_node* node;
        lyd_parse_data_mem(ctx, data, LYD_JSON, 0, LYD_VALIDATE_PRESENT, &node);
        // With wrapConstRawNode, the node needs to be released manually.
        auto node_deleter = std::unique_ptr<lyd_node, decltype(&lyd_free_all)>(node, lyd_free_all);

        // The wrapped node does NOT take ownership of the lyd_node*.
        auto wrapped = libyang::wrapUnmanagedRawNode(const_cast<const lyd_node*>(node));
        REQUIRE(wrapped.path() == "/example-schema:leafInt32");
        // The schema should still be accessible.
        REQUIRE(wrapped.schema().name() == "leafInt32");
        for (const auto& node : wrapped.childrenDfs()) {
            node.path();
        }

        // It is possible to create unmanaged sets.
        for (const auto& node : wrapped.findXPath("/example-schema:leafInt32")) {
            node.path();
        }

        // You can do low level tree manipulation with the unmanaged tree.

        // You have two trees `wrapped` and `anotherNodeWrapped` and you want to do some manipulation in C++.
        DOCTEST_SUBCASE("Inserting an UNMANAGED node into an unmanaged node")
        {
            lyd_node* anotherNode;
            lyd_new_path(nullptr, ctx, "/example-schema:leafInt8", "0", LYD_VALIDATE_PRESENT, &anotherNode);
            auto anotherNodeWrapped = libyang::wrapUnmanagedRawNode(const_cast<const lyd_node*>(anotherNode));
            wrapped.insertSibling(anotherNodeWrapped);
            // Both are still unmanaged, both are accessible.
            REQUIRE(wrapped.path() == "/example-schema:leafInt32");
            REQUIRE(anotherNodeWrapped.path() == "/example-schema:leafInt8");
        }

        // You have a C++ managed node and you want to insert that into an unmanaged node.
        DOCTEST_SUBCASE("Inserting a MANAGED node into an unmanaged node")
        {
            lyd_node* anotherNode;
            lyd_new_path(nullptr, ctx, "/example-schema:leafInt8", "0", LYD_VALIDATE_PRESENT, &anotherNode);
            auto anotherNodeWrapped = libyang::wrapRawNode(anotherNode);
            wrapped.insertSibling(anotherNodeWrapped);
            // BOTH are now unmanaged, both are accessible.
            REQUIRE(wrapped.path() == "/example-schema:leafInt32");
            REQUIRE(anotherNodeWrapped.path() == "/example-schema:leafInt8");
        }

        // You have a C++ managed node and you want to insert an unmanaged node into it.
        DOCTEST_SUBCASE("Inserting a UNMANAGED node into a managed node")
        {
            lyd_node* anotherNode;
            lyd_new_path(nullptr, ctx, "/example-schema:leafInt8", "0", LYD_VALIDATE_PRESENT, &anotherNode);
            auto anotherNodeWrapped = libyang::wrapRawNode(anotherNode);
            anotherNodeWrapped.insertSibling(wrapped);
            // BOTH are now managed by C++, both are accessible.
            REQUIRE(wrapped.path() == "/example-schema:leafInt32");
            REQUIRE(anotherNodeWrapped.path() == "/example-schema:leafInt8");
            // Since the original `wrapped` pointer is now managed by C++, we have to release our C management.
            (void)node_deleter.release();
        }

        DOCTEST_SUBCASE("Query identity from unmanaged node")
        {
            lyd_node* node;
            lyd_new_path(nullptr, ctx, "/example-schema:leafFoodTypedef", "example-schema:pizza", LYD_VALIDATE_PRESENT, &node);
            auto wrappedNode = libyang::wrapUnmanagedRawNode(const_cast<const lyd_node*>(node));

            REQUIRE(wrappedNode.path() == "/example-schema:leafFoodTypedef");
            REQUIRE(wrappedNode.asTerm().valueStr() == "example-schema:pizza");

            auto typeIdIdentity = std::get<libyang::IdentityRef>(wrappedNode.asTerm().value()).schema;
            REQUIRE(typeIdIdentity.module().name() == "example-schema");
            REQUIRE(typeIdIdentity.name() == "pizza");
            auto node_deleter = std::unique_ptr<lyd_node, decltype(&lyd_free_all)>(node, lyd_free_all);
        }

        REQUIRE_THROWS(libyang::wrapUnmanagedRawNode(nullptr));
    }

    DOCTEST_SUBCASE("wrapUnmanagedRawNode metadata")
    {
        std::string in = R"(
{
  "example-schema:leafInt8": -127,
  "@example-schema:leafInt8": {
    "ietf-netconf:operation": "merge"
  },
  "example-schema:leafString": "",
  "@example-schema:leafString": {
    "ietf-netconf:operation": "remove"
  }
}
)";

        REQUIRE(ly_ctx_set_searchdir(ctx, TESTS_DIR) == LY_SUCCESS);
        REQUIRE(lys_parse_path(ctx, TESTS_DIR "/ietf-netconf@2011-06-01.yang", LYS_IN_YANG, nullptr) == LY_SUCCESS);

        lyd_node* node;

        REQUIRE(lyd_parse_data_mem(ctx, in.c_str(), LYD_JSON, LYD_PARSE_OPAQ, LYD_VALIDATE_PRESENT, &node) == LY_SUCCESS);

        REQUIRE(!!node);
        auto node_deleter = std::unique_ptr<lyd_node, decltype(&lyd_free_all)>(node, lyd_free_all);

        auto wrapped = libyang::wrapUnmanagedRawNode(const_cast<const lyd_node*>(node));

        REQUIRE(wrapped.path() == "/example-schema:leafInt8");
        REQUIRE(wrapped.nextSibling().has_value());
        REQUIRE(wrapped.nextSibling()->path() == "/example-schema:leafString");

        REQUIRE(!wrapped.meta().empty());
        REQUIRE(wrapped.meta().begin()->name() == "operation");
        REQUIRE(wrapped.meta().begin()->valueStr() == "merge");
        REQUIRE(wrapped.meta().begin()->module().name() == "ietf-netconf");

        REQUIRE(!wrapped.nextSibling()->meta().empty());
        REQUIRE(wrapped.nextSibling()->meta().begin()->name() == "operation");
        REQUIRE(wrapped.nextSibling()->meta().begin()->valueStr() == "remove");
        REQUIRE(wrapped.nextSibling()->meta().begin()->module().name() == "ietf-netconf");
    }
}
