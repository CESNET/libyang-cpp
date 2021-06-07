/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/
#pragma once

#include <memory>

struct lyd_node;
namespace libyang {
class Context;

class DataNode {
public:
    ~DataNode();

    friend Context;
private:
    DataNode(lyd_node* node);
    lyd_node* m_node;
};
}
