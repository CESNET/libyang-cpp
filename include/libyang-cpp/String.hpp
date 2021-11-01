/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

#include <memory>

namespace libyang {
/**
 * @brief A container for strings created by libyang.
 */
class String {
public:
    explicit String(char* str);
    std::shared_ptr<char> get();

    explicit operator std::string() const;

    bool operator==(const char*) const;
    bool operator==(const std::string_view& str) const;
    bool operator==(const String& str) const;
    std::strong_ordering operator<=>(const String& str) const;

    friend std::ostream& operator<<(std::ostream& os, const String& str);

private:
    std::shared_ptr<char> m_ptr;
};
}
