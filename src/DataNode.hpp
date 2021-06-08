/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/
#pragma once
#include <cstdlib>
#include <memory>
#include "Enum.hpp"
#include "String.hpp"

struct lyd_node;
namespace libyang {
class Context;

class DataNode {
public:
    ~DataNode();

    String printStr(const DataFormat format, const PrintFlags flags) const;

    friend Context;
private:
    DataNode(lyd_node* node);
    lyd_node* m_node;
};
}
