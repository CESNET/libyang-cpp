/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/
#include "DataNode.hpp"
#include <libyang/libyang.h>
namespace libyang {
DataNode::DataNode(lyd_node* node)
    : m_node(node)
{
}

DataNode::~DataNode()
{
    lyd_free_all(m_node);
}
}
