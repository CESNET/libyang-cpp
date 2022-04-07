/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <libyang-cpp/ChildInstantiables.hpp>
#include <libyang/libyang.h>

namespace libyang {
ChildInstanstiablesIterator::ChildInstanstiablesIterator(const lysc_node* parent, const lysc_module* module, const ChildInstanstiables* childInstantiables)
    : m_childInstantiables(childInstantiables)
    , m_parent(parent)
    , m_module(module)
    , m_current(nullptr)
{
    operator++();
}

/**
 * @brief Creates an iterator that acts as the `end` iterator.
 */
ChildInstanstiablesIterator::ChildInstanstiablesIterator(const std::nullptr_t, const ChildInstanstiables* childInstantiables)
    : m_childInstantiables(childInstantiables)
    , m_parent(nullptr)
    , m_module(nullptr)
    , m_current(nullptr)
{
}

ChildInstanstiablesIterator& ChildInstanstiablesIterator::operator++()
{
    // TODO: allow options also by asking for them on creation of the ChildInstanstiables class and saving them there
    m_current = lys_getnext(m_current, m_parent, m_module, 0);
    return *this;
}

ChildInstanstiablesIterator ChildInstanstiablesIterator::operator++(int)
{
    auto copy = *this;
    operator++();
    return copy;
}

SchemaNode ChildInstanstiablesIterator::operator*() const
{
    if (!m_current) {
        throw std::out_of_range("Derefenced .end iterator");
    }
    return SchemaNode{m_current, m_childInstantiables->m_ctx};
}

ChildInstanstiablesIterator::SchemaNodeProxy ChildInstanstiablesIterator::operator->() const
{
    if (!m_current) {
        throw std::out_of_range("Derefenced .end iterator");
    }
    return {SchemaNode{m_current, m_childInstantiables->m_ctx}};
}

bool ChildInstanstiablesIterator::operator==(const ChildInstanstiablesIterator& other) const
{
    return m_current == other.m_current && m_childInstantiables == other.m_childInstantiables;
}

ChildInstanstiables::ChildInstanstiables(const lysc_node* parent, const lysc_module* module, std::shared_ptr<ly_ctx> ctx)
    : m_parent(parent)
    , m_module(module)
    , m_ctx(ctx)
{
}

ChildInstanstiablesIterator ChildInstanstiables::begin() const
{
    return ChildInstanstiablesIterator{m_parent, m_module, this};
}

ChildInstanstiablesIterator ChildInstanstiables::end() const
{
    return ChildInstanstiablesIterator{nullptr, this};
}
}
