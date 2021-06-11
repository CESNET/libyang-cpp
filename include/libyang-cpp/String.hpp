/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#include <memory>

/**
 * @brief A container for strings created by libyang.
 */
class String {
public:
    explicit String(char* str);
    std::shared_ptr<char> get();
    std::string toStdString();

    bool operator==(const char*) const;
    bool operator==(const std::string&) const;

private:
    std::shared_ptr<char> m_ptr;
};

