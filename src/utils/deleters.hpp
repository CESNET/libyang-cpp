/*
 * Copyright (C) 2023 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Jan Kundrát <jan.kundrat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#include <memory>
#include <libyang/libyang.h>
#include "exception.hpp"

namespace libyang {
namespace {
using deleter_free_t = decltype([](auto *p) constexpr {
        std::free(p);
    });
using deleter_ly_in_free_false_t = decltype([](auto* in) constexpr {
        ly_in_free(in, false);
    });

inline auto wrap_ly_in_new_memory(const std::string& buf)
{
    struct ly_in *in;
    auto ret = ly_in_new_memory(buf.c_str(), &in);
    throwIfError(ret, "ly_in_new_memory failed");
    return std::unique_ptr<ly_in, deleter_ly_in_free_false_t>{in};
}
}
}
