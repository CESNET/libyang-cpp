/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/
#pragma once
#include <cstdlib>
#include <memory>
#include <optional>
#include "Enum.hpp"
#include "String.hpp"

struct lyd_node;
namespace libyang {
class Context;
class DataNode;

class DataView {
public:
    ~DataView();

    String path();

    friend DataNode;
private:
    DataView(lyd_node* node);
    lyd_node* m_node;
};

class DataNode {
public:
    ~DataNode();
    DataNode(const DataNode& src) = delete;
    DataNode& operator=(const DataNode& src) = delete;

    String printStr(const DataFormat format, const PrintFlags flags) const;
    std::optional<DataView> findPath(const char* path) const;

    friend Context;
private:
    DataNode(lyd_node* node);
    lyd_node* m_node;
};
}
