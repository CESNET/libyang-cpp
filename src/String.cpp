/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/
#include <cstdlib>
#include <string>
#include "String.hpp"

String::String(char* str)
    : m_ptr(str, std::free)
{
}

std::shared_ptr<char> String::get()
{
    return m_ptr;
}

std::string String::toStdString()
{
    return m_ptr.get();
}

