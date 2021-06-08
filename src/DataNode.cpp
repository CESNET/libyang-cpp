/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/
#include <libyang/libyang.h>
#include <string>
#include "DataNode.hpp"
#include "utils/enum.hpp"
namespace libyang {
DataNode::DataNode(lyd_node* node)
    : m_node(node)
{
}

DataNode::~DataNode()
{
    lyd_free_all(m_node);
}

String DataNode::printStr(const DataFormat format, const PrintFlags flags) const
{
    char* str;
    lyd_print_mem(&str, m_node, utils::toLydFormat(format), utils::toPrintFlags(flags));

    return String{str};
}
}
