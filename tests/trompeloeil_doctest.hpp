#pragma once
#include <doctest/doctest.h>
#include <doctest/trompeloeil.hpp>
#include <trompeloeil.hpp>

extern template struct trompeloeil::reporter<trompeloeil::specialized>;
