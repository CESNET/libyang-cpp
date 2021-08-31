/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <libyang/libyang.h>
#include <libyang-cpp/DataNode.hpp>
#include <libyang-cpp/Set.hpp>
#include <span>

namespace libyang {
DataNodeSet::DataNodeSet(ly_set* set, std::shared_ptr<internal_refcount> refs)
    : m_set(set)
    , m_refs(refs)
{
}

DataNodeSet::~DataNodeSet()
{
    ly_set_free(m_set, nullptr);
}

std::vector<DataNode> DataNodeSet::asVector()
{
    std::vector<DataNode> res;
    for (const auto& item : std::span(m_set->dnodes, m_set->count)) {
        res.emplace_back(DataNode{item, m_refs});
    }

    return res;
}
}
