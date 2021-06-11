/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include <cstdint>
#include <string_view>
#include <variant>

namespace libyang {
struct Empty {
    auto operator<=>(const Empty&) const = default;
};

struct Binary {
    auto operator<=>(const Binary&) const = default;
    std::string_view value;
};

using Value = std::variant<int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t, bool, Empty, Binary, std::string>;
}
