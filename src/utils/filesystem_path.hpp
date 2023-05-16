/*
 * Copyright (C) 2021-2023 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Jan Kundrát <jan.kundrat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

/**
 * Convert std::filesystem::path to a `const char *` for libyang representaiton of filesystem path names
 *
 * This is because of Windows where std::filesystem::path::value_type is a `wchar_t`, not just the plain old `char`.
 * */
#define PATH_TO_LY_STRING(PATH) (PATH).string().c_str()
