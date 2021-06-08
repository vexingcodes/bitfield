#include <cstdint>

// This header happens to be named the same as the single header file, so no need to conditionally include.
#include "bit_field.hpp"

using namespace BIT_FIELD_NAMESPACE;

enum class test_enum {
    value_0 = 0,
    value_1 = 1,
    value_2 = 2
};

enum class test_enum_offset {
    value_0 = (0 << 2),
    value_1 = (1 << 2),
    value_2 = (2 << 2)
};

// Bit field get (offset 0).
using test_field_1 = bit_field<3, 0>;
static_assert(0b00000000 == test_field_1::get(0b00000000));
static_assert(0b00000001 == test_field_1::get(0b00000001));
static_assert(0b00000111 == test_field_1::get(0b00000111));
static_assert(0b00000111 == test_field_1::get(0b11111111));

// Bit field get (offset 2).
using test_field_2 = bit_field<3, 0, bit_field_config<std::uint8_t>{ .offset = 2 }>;
static_assert(0b00000000 == test_field_2::get(0b00000000));
static_assert(0b00000100 == test_field_2::get(0b00000001));
static_assert(0b00011100 == test_field_2::get(0b00000111));
static_assert(0b00011100 == test_field_2::get(0b11111111));

// Bit field get enum.
using test_field_3 = bit_field<3, 0, bit_field_config<test_enum>{}>;
static_assert(test_enum::value_0 == test_field_3::get(0b00000000));
static_assert(test_enum::value_1 == test_field_3::get(0b00000001));
static_assert(test_enum::value_2 == test_field_3::get(0b00000010));

// Bit field get enum (offset 2, no shifting should be needed).
using test_field_4 = bit_field<3, 2, bit_field_config<test_enum_offset>{ .offset = 2 }>;
static_assert(test_enum_offset::value_0 == test_field_4::get(0b00000000));
static_assert(test_enum_offset::value_1 == test_field_4::get(0b00000100));
static_assert(test_enum_offset::value_2 == test_field_4::get(0b00001000));

// Bit field get enum (offset 2, requires shifting left).
using test_field_5 = bit_field<3, 0, bit_field_config<test_enum_offset>{ .offset = 2 }>;
static_assert(test_enum_offset::value_0 == test_field_5::get(0b00000000));
static_assert(test_enum_offset::value_1 == test_field_5::get(0b00000001));
static_assert(test_enum_offset::value_2 == test_field_5::get(0b00000010));

// Bit field get enum (origin 2, requires shifting right).
using test_field_6 = bit_field<3, 2, bit_field_config<test_enum>{}>;
static_assert(test_enum::value_0 == test_field_6::get(0b00000000));
static_assert(test_enum::value_1 == test_field_6::get(0b00000100));
static_assert(test_enum::value_2 == test_field_6::get(0b00001000));

// Overriding the config on a call-by-call basis works.
// (test_field_5 has a default offset of 2, and a default type of test_enum_offset, but the get call here overrides the
// offset to 0 and the type to test_enum.)
static_assert(test_enum::value_1 == test_field_5::get<bit_field_config<test_enum>{ .offset = 0 }>(0b00000001));

// Simple setting of an enum using the mask strategy.
static_assert([]{
    std::uint8_t value{0};
    bit_field<3, 2, bit_field_config<test_enum>{
        .strategy = bit_field_assignment_strategy::mask }>::set(value, test_enum::value_1);
    return value == 0b00000100;
}());

// Test mask strategy.
static_assert([]{
    std::uint8_t value{0};
    test_enum enum_value = static_cast<test_enum>(0b11111111);
    bit_field<3, 2, bit_field_config<test_enum>{
        .strategy = bit_field_assignment_strategy::mask }>::set(value, enum_value);
    return value == 0b00011100;
}());

// Test unchecked strategy.
static_assert([]{
    std::uint8_t value{0};
    test_enum enum_value = static_cast<test_enum>(0b11111111);
    bit_field<3, 0, bit_field_config<test_enum>{
        .strategy = bit_field_assignment_strategy::unchecked }>::set(value, enum_value);
    return value == 0b11111111;
}());

// Test return_bool strategy (success).
static_assert([]{
    std::uint8_t value{0};
    test_enum enum_value = test_enum::value_2;
    return bit_field<3, 0, bit_field_config<test_enum>{
        .strategy = bit_field_assignment_strategy::return_bool }>::set(value, enum_value) == true;
}());

// Test return_bool strategy (failure).
static_assert([]{
    std::uint8_t value{0};
    test_enum enum_value = static_cast<test_enum>(0b11111111);
    return bit_field<3, 0, bit_field_config<test_enum>{
        .strategy = bit_field_assignment_strategy::return_bool }>::set(value, enum_value) == false;
}());

#if BIT_FIELD_EXCEPTIONS_ENABLED
// Test exception strategy (success).
static_assert([]{
    std::uint8_t value{0};
    test_enum enum_value = test_enum::value_2;
    bit_field<3, 0, bit_field_config<test_enum>{
        .strategy = bit_field_assignment_strategy::exception }>::set(value, enum_value);
    return true;
}());

// Test exception strategy (failure). A compile-time test to ensure this throws cannot be constructed.
#if 0
static_assert([]{
    std::uint8_t value{0};
    test_enum enum_value = static_cast<test_enum>(0b11111111);
    bit_field<3, 0, bit_field_config<test_enum>{
        .strategy = bit_field_assignment_strategy::exception }>::set(value, enum_value);
    return true;
}());
#endif // 0
#endif // BIT_FIELD_EXCEPTIONS_ENABLED
