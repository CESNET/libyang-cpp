/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
*/

#include <memory>

struct ly_ctx;

namespace libyang {
class Context {
public:
    Context();

private:
    std::unique_ptr<ly_ctx, void(*)(ly_ctx*)> m_ctx;
};
}
