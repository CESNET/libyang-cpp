/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/
#include <memory>

/**
 * @brief A container for strings created by libyang.
 */
class String {
public:
    String(char* str);
    std::shared_ptr<char> get();
    std::string toStdString();

private:
    std::shared_ptr<char> m_ptr;
};

