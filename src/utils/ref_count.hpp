/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include <set>

namespace libyang {
class DataNode;
struct internal_refcount {
    std::set<DataNode*> m_refs;
};
}
