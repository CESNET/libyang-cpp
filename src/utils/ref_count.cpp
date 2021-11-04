/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#include "ref_count.hpp"

namespace libyang {
internal_refcount::internal_refcount(std::shared_ptr<ly_ctx> ctx)
    : context(ctx)
{
}
}
