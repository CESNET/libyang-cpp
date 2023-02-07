/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 * Written by Jan Kundrát <jan.kundrat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include <cstdint>
#include <libyang-cpp/Type.hpp>
#include <libyang-cpp/export.h>
#include <optional>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

struct lysc_ident;

namespace libyang {
class Identity;

/**
 * @brief Represents a YANG value of type `empty`.
 */
struct LIBYANG_CPP_EXPORT Empty {
    auto operator<=>(const Empty&) const = default;
};

/**
 * @brief Represents a YANG value of type `binary` as raw bytes and as a base64 string.
 */
struct LIBYANG_CPP_EXPORT Binary {
    auto operator<=>(const Binary&) const = default;
    std::vector<uint8_t> data;
    std::string base64;
};


/**
 * @brief Represents a single bit from a value of type `bits`.
 */
struct LIBYANG_CPP_EXPORT Bit {
    auto operator<=>(const Bit&) const = default;
    uint32_t position;
    std::string name;
};

/**
 * @brief Represents a value of type `enumeration`.
 */
struct LIBYANG_CPP_EXPORT Enum {
    auto operator<=>(const Enum&) const = default;
    std::string name;
    int32_t value;
};

/**
 * @brief Represents a value of type `identityref`.
 */
struct LIBYANG_CPP_EXPORT IdentityRef {
    auto operator==(const IdentityRef& other) const {
        return std::tie(this->module, this->name) == std::tie(other.module, other.name);
    }
    std::string module;
    std::string name;

    Identity schema;
};

struct Decimal64;
namespace impl {
constexpr int64_t abs(const int64_t number)
{
    return number < 0 ? -number : number;
}
constexpr int64_t pow10int(const uint8_t digits)
{
    int64_t exponent = 1;
    for (uint8_t i = 0; i < digits; ++i) {
        exponent *= 10;
    }
    return exponent;
}
static_assert(pow10int(0) == 1);
static_assert(pow10int(1) == 10);
static_assert(pow10int(2) == 100);

constexpr double pow10double(const int digits)
{
    auto exponent = pow10int(abs(digits));
    return digits < 0 ? 1.0 / exponent : 1.0 * exponent;
}
static_assert(pow10double(1) == 10);
static_assert(pow10double(2) == 100);
static_assert(pow10double(-1) == 0.1);
static_assert(pow10double(-2) == 0.01);

constexpr long long llround(double x)
{
    return (x >= 0.0) ? (long long)(x + 0.5) : (long long)(x - 0.5);
}
static_assert(llround(0.4999) == 0);
static_assert(llround(0.5001) == 1);
static_assert(llround(-0.4999) == 0);
static_assert(llround(-0.5001) == -1);

template <int64_t V, uint8_t IntegralDigits, uint8_t FractionDigitsPlusOne>
constexpr Decimal64 make_decimal64();
}

/**
 * @brief Represents a YANG value of type `decimal64`.
 */
struct LIBYANG_CPP_EXPORT Decimal64 {
    int64_t number;
    uint8_t digits;

    explicit constexpr operator double() const
    {
        return number * impl::pow10double(-digits);
    }

    constexpr Decimal64 operator-() const
    {
        return Decimal64{-number, digits};
    }

    template <uint8_t digits>
    constexpr static Decimal64 fromRawDecimal(const int64_t value)
    {
        static_assert(digits >= 1);
        static_assert(digits <= 18);
        return Decimal64{value, digits};
    }

    template <uint8_t digits>
    constexpr static Decimal64 fromDouble(const double value)
    {
        static_assert(digits >= 1);
        static_assert(digits <= 18);
        return Decimal64{impl::llround(value * impl::pow10int(digits)), digits};
    }
    explicit constexpr Decimal64(const int64_t number_, const uint8_t digits_)
        : number(number_)
        , digits(digits_)
    {
    }

    constexpr bool operator==(const Decimal64& a) const = default;

private:
    template <int64_t V, uint8_t IntegralDigits, uint8_t FractionDigitsPlusOne>
    friend constexpr Decimal64 impl::make_decimal64();
};

namespace impl {
template <int64_t V, uint8_t IntegralDigits, uint8_t FractionDigitsPlusOne>
constexpr Decimal64 make_decimal64()
{
    static_assert(IntegralDigits <= 18);
    static_assert(IntegralDigits + FractionDigitsPlusOne <= 20);
    if constexpr (FractionDigitsPlusOne < 2) {
        return Decimal64::fromRawDecimal<1>(V * 10);
    } else {
        return Decimal64::fromRawDecimal<FractionDigitsPlusOne - 1>(V);
    }
}
template <int64_t V, uint8_t IntegralDigits, uint8_t FractionDigitsPlusOne, char C, char... Cs>
constexpr Decimal64 make_decimal64()
{
    static_assert((C >= '0' && C <= '9') || C == '.', "Invalid numeric character for Decimal64");
    // more than one '.' is rejected by the lexer, apparently
    if constexpr (C == '.') {
        return make_decimal64<V, IntegralDigits, 1, Cs...>();
    } else if constexpr (FractionDigitsPlusOne > 0) {
        return make_decimal64<V * 10 + C - '0', IntegralDigits, FractionDigitsPlusOne + 1, Cs...>();
    } else {
        return make_decimal64<V * 10 + C - '0', IntegralDigits + 1, 0, Cs...>();
    }
}
}
inline namespace literals {
template <char... Cs>
constexpr Decimal64 operator"" _decimal64()
{
    return impl::make_decimal64<0, 0, 0, Cs...>();
}

static_assert(Decimal64::fromDouble<2>(12.34) == Decimal64::fromRawDecimal<2>(1234));
static_assert(Decimal64::fromDouble<2>(12.34) == 12.34_decimal64);
static_assert(double{Decimal64::fromDouble<2>(12.34)} == 12.34);
static_assert(Decimal64::fromDouble<1>(12.34) == Decimal64::fromRawDecimal<1>(123));
static_assert(Decimal64::fromDouble<1>(12.34) == 12.3_decimal64);
static_assert(double{Decimal64::fromDouble<1>(12.34)} == 12.3);

static_assert(123_decimal64 == Decimal64::fromRawDecimal<1>(1230));
static_assert(12_decimal64 == Decimal64::fromRawDecimal<1>(120));
static_assert(7_decimal64 == Decimal64::fromRawDecimal<1>(70));
static_assert(1._decimal64 == Decimal64::fromRawDecimal<1>(10));
static_assert(1.0_decimal64 == Decimal64::fromRawDecimal<1>(10));
static_assert(1.00_decimal64 == Decimal64::fromRawDecimal<2>(100));
static_assert(1.000_decimal64 == Decimal64::fromRawDecimal<3>(1000));
static_assert(1.000000000000000000_decimal64 == Decimal64::fromRawDecimal<18>(1000000000000000000));
static_assert(-1.000000000000000000_decimal64 == Decimal64::fromRawDecimal<18>(-1000000000000000000));
static_assert(1.2_decimal64 == Decimal64::fromRawDecimal<1>(12));
static_assert(12.3_decimal64 == Decimal64::fromRawDecimal<1>(123));
static_assert(456.7_decimal64 == Decimal64::fromRawDecimal<1>(4567));
static_assert(456.78_decimal64 == Decimal64::fromRawDecimal<2>(45678));
static_assert(456.789_decimal64 == Decimal64::fromRawDecimal<3>(456789));
static_assert(456.7890_decimal64 == Decimal64::fromRawDecimal<4>(4567890));
static_assert(-456.7890_decimal64 == Decimal64::fromRawDecimal<4>(-4567890));
}

/**
 * @brief A JSON value of an anydata node.
 */
struct LIBYANG_CPP_EXPORT JSON {
    std::string_view content;
};


/**
 * @brief A value of an anyxml node.
 */
struct LIBYANG_CPP_EXPORT XML {
    std::string_view content;
};

/**
 * Represents a value of DataNodeAny.
 * TODO: add support for all the types of values.
 */
using AnydataValue = std::optional<std::variant<DataNode, JSON, XML>>;
}
