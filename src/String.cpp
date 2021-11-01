/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#include <cstdlib>
#include <cstring>
#include <libyang-cpp/String.hpp>
#include <string>

namespace libyang {
/**
 * @brief Wraps a new C-string.
 */
String::String(char* str)
    : m_ptr(str, std::free)
{
}

/**
 * @brief Retrieves the C-string.
 */
std::shared_ptr<char> String::get()
{
    return m_ptr;
}

/**
 * @brief Converts the string to an std::string.
 */
String::operator std::string() const
{
    return m_ptr.get();
}

bool String::operator==(const char* str) const
{
    return !std::strcmp(m_ptr.get(), str);
}

/**
 * @brief Compares String with a null-terminated std::string_view.
 */
bool String::operator==(const std::string_view& str) const
{
    return m_ptr.get() == str;
}

std::weak_ordering String::operator<=>(const String& str) const
{
    auto res = std::strcmp(m_ptr.get(), str.m_ptr.get());
    switch (res) {
    case -1:
        return std::weak_ordering::less;
    case 0:
        return std::weak_ordering::equivalent;
    case 1:
        return std::weak_ordering::greater;
    }

    __builtin_unreachable();
}

std::ostream& operator<<(std::ostream& os, const String& str)
{
    os << str.m_ptr.get();
    return os;
}
}
