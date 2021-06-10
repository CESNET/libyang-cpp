/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#pragma once
#include <stdexcept>

namespace libyang {
/**
 * A generic libyang error. All other libyang errors inherit from this exception type.
 */
class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/**
 * A libyang error containing a message and an error code.
 */
class ErrorCode : public Error {
public:
    explicit ErrorCode(const std::string& what, unsigned int errCode);

    unsigned int code();
private:
    int m_errCode;
};
}
