/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/
#include <libyang/libyang.h>
#include <stdexcept>
#include <string>
#include "DataNode.hpp"
#include "utils/enum.hpp"
namespace libyang {
DataNode::DataNode(lyd_node* node)
    : m_node(node)
    , m_viewCount(std::make_shared<Empty>())
{
}

DataNode::DataNode(lyd_node* node, std::shared_ptr<Empty> viewCount)
    : m_node(node)
    , m_viewCount(viewCount)
{
}

DataNode::~DataNode()
{
    if (m_viewCount.use_count() == 1) {
        lyd_free_all(m_node);
    }
}

String DataNode::printStr(const DataFormat format, const PrintFlags flags) const
{
    char* str;
    lyd_print_mem(&str, m_node, utils::toLydFormat(format), utils::toPrintFlags(flags));

    return String{str};
}

/**
 * Returns a view of the node specified by `path`.
 * If the node is not found, returns std::nullopt.
 * Throws on errors.
 *
 * @param path Node to search for.
 * @return DataView is the node is found, other std::nullopt.
 */
std::optional<DataNode> DataNode::findPath(const char* path) const
{
    lyd_node* node;
    auto err = lyd_find_path(m_node, path, false, &node);

    switch (err) {
    case LY_SUCCESS:
        return DataNode{node, m_viewCount};
    case LY_ENOTFOUND:
    case LY_EINCOMPLETE: // TODO: is this really important?
        return std::nullopt;
    default:
        throw std::runtime_error("Error in DataNode::findPath (" + std::to_string(err) + ")");
    }
}

String DataNode::path() const
{
    // TODO: handle all path types, not just LYD_PATH_STD

    auto str = lyd_path(m_node, LYD_PATH_STD, nullptr, 0);
    if (!str) {
        throw std::runtime_error("DataView::path memory allocation error");
    }

    return String{str};
}
}
