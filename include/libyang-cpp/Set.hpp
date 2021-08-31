/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include <memory>
#include <vector>

struct ly_set;
struct ly_ctx;

namespace libyang {
class Context;
class DataNode;

struct internal_refcount;

class DataNodeSet {
public:
    // TODO: add more methods
    ~DataNodeSet();
    std::vector<DataNode> asVector();

private:
    DataNodeSet(ly_set* set, std::shared_ptr<internal_refcount> refs);
    friend DataNode;

    ly_set* m_set;
    std::shared_ptr<internal_refcount> m_refs;
};
}
