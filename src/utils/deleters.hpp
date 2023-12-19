/*
 * Copyright (C) 2023 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Jan Kundrát <jan.kundrat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#include <memory>
#include <string>
#include <libyang/libyang.h>
#include <sys/types.h>
#include "exception.hpp"

extern "C" {
static ssize_t libyang_cpp_out_string_cb(void* user_data, const void* buf, size_t count)
{
    std::string& str = *reinterpret_cast<std::string*>(user_data);
    str.append(reinterpret_cast<const char*>(buf), static_cast<std::string::size_type>(count));
    return count;
}
}

namespace libyang {
namespace {
using deleter_free_t = decltype([](auto* p) constexpr {
    std::free(p);
});
using deleter_ly_in_free_false_t = decltype([](auto* in) constexpr {
    ly_in_free(in, false);
});
using deleter_ly_out_free_cb_false_t = decltype([](auto* out) constexpr {
    ly_out_free(out, nullptr, false);
});

inline auto wrap_ly_in_new_memory(const std::string& buf)
{
    struct ly_in* in;
    auto ret = ly_in_new_memory(buf.c_str(), &in);
    throwIfError(ret, "ly_in_new_memory failed");
    return std::unique_ptr<ly_in, deleter_ly_in_free_false_t>{in};
}

inline auto wrap_ly_out_new_buf(std::string& buf)
{
    struct ly_out* out;
    auto ret = ly_out_new_clb(&libyang_cpp_out_string_cb, &buf, &out);
    throwIfError(ret, "ly_out_new_clb failed");
    return std::unique_ptr<ly_out, deleter_ly_out_free_cb_false_t>{out};
}
}
}
