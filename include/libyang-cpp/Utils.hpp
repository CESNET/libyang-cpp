/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include <libyang-cpp/Enum.hpp>
#include <stdexcept>

namespace libyang {
LogOptions setLogOptions(const libyang::LogOptions options);
LogLevel setLogLevel(const LogLevel level);

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
class ErrorWithCode : public Error {
public:
    explicit ErrorWithCode(const std::string& what, uint32_t errCode);

    ErrorCode code();

private:
    ErrorCode m_errCode;
};
}
