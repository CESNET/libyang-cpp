/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include <libyang-cpp/SchemaNode.hpp>
#include <libyang-cpp/export.h>

struct lysc_module;
namespace libyang {
class ChildInstanstiables;
class Module;

class LIBYANG_CPP_EXPORT ChildInstanstiablesIterator {
public:
    friend ChildInstanstiables;
    SchemaNode operator*() const;
    ChildInstanstiablesIterator& operator++();
    ChildInstanstiablesIterator operator++(int);
    bool operator==(const ChildInstanstiablesIterator&) const;

    struct SchemaNodeProxy {
        SchemaNode node;
        SchemaNode* operator->();
    };
    SchemaNodeProxy operator->() const;

private:
    ChildInstanstiablesIterator(const lysc_node* parent, const lysc_module* module, const ChildInstanstiables* childInstantiables);
    ChildInstanstiablesIterator(const std::nullptr_t parent, const ChildInstanstiables* childInstantiables);

    const ChildInstanstiables* m_childInstantiables;
    const lysc_node* m_parent;
    const lysc_module* m_module;
    const lysc_node* m_current;
};

/**
 * @brief A collection that iterates over children of a schema node that can be instantiated, i.e. they can have a DataNode.
 *
 * Check out Module::childInstantiables and SchemaNode::childInstantiables for usage.
 */
class LIBYANG_CPP_EXPORT ChildInstanstiables {
public:
    friend SchemaNode;
    friend Module;
    friend ChildInstanstiablesIterator;
    ChildInstanstiablesIterator begin() const;
    ChildInstanstiablesIterator end() const;

private:
    ChildInstanstiables(const lysc_node* parent, const lysc_module* module, std::shared_ptr<ly_ctx> ctx);
    const lysc_node* m_parent;
    const lysc_module* m_module;
    std::shared_ptr<ly_ctx> m_ctx;
};
}
