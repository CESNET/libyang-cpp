/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/
#pragma once

#include <libyang/libyang.h>
namespace libyang {
enum class SchemaFormat {
    Detect = 0,
    Yang = 1,
    Yin = 3
};

enum class DataFormat {
    Detect = 0,
    XML,
    JSON
};
}
