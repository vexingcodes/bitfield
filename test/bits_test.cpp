#include <cstdint>

#ifdef BIT_FIELD_TEST_SINGLE_HEADER
#  include "bit_field.hpp"
#else
#  include "bits.hpp"
#endif

using namespace BIT_FIELD_NAMESPACE;

static_assert(bits<uint8_t>  ==  8);
static_assert(bits< int16_t> == 16);
static_assert(bits<uint32_t> == 32);
static_assert(bits< int64_t> == 64);

static_assert(bit_mask<std::uint8_t, 0, 1> == 0b00000001);
static_assert(bit_mask<std::uint8_t, 0, 2> == 0b00000011);
static_assert(bit_mask<std::uint8_t, 0, 3> == 0b00000111);
static_assert(bit_mask<std::uint8_t, 2, 3> == 0b00011100);
static_assert(bit_mask<std::uint8_t, 7, 1> == 0b10000000);

static_assert(extract_bits<1, 0, std::uint8_t>(0) == std::uint8_t{0});
static_assert(extract_bits<1, 0, std::uint8_t>(1) == std::uint8_t{1});

static_assert(extract_bits<2, 1, std::uint8_t>(0b11111000) == std::uint8_t{0b00});
static_assert(extract_bits<2, 1, std::uint8_t>(0b11111010) == std::uint8_t{0b01});
static_assert(extract_bits<2, 1, std::uint8_t>(0b11111100) == std::uint8_t{0b10});
static_assert(extract_bits<2, 1, std::uint8_t>(0b11111110) == std::uint8_t{0b11});

static_assert(extract_bits<2, 6, std::uint8_t>(0b00111111) == std::uint8_t{0b00});
static_assert(extract_bits<2, 6, std::uint8_t>(0b01111111) == std::uint8_t{0b01});
static_assert(extract_bits<2, 6, std::uint8_t>(0b10111111) == std::uint8_t{0b10});
static_assert(extract_bits<2, 6, std::uint8_t>(0b11111111) == std::uint8_t{0b11});

static_assert(extract_bits<2, 1, std::uint8_t, std::byte>(0b11111000) == std::byte{0b00});
static_assert(extract_bits<2, 1, std::uint8_t, std::byte>(0b11111010) == std::byte{0b01});
static_assert(extract_bits<2, 1, std::uint8_t, std::byte>(0b11111100) == std::byte{0b10});
static_assert(extract_bits<2, 1, std::uint8_t, std::byte>(0b11111110) == std::byte{0b11});

static_assert(extract_bits<2, 1, std::uint8_t, std::byte, 2>(0b11111000) == std::byte{0b0000});
static_assert(extract_bits<2, 1, std::uint8_t, std::byte, 2>(0b11111010) == std::byte{0b0100});
static_assert(extract_bits<2, 1, std::uint8_t, std::byte, 2>(0b11111100) == std::byte{0b1000});
static_assert(extract_bits<2, 1, std::uint8_t, std::byte, 2>(0b11111110) == std::byte{0b1100});

static_assert(extract_bits<2, 1, std::uint8_t, std::byte, 3>(0b11111000) == std::byte{0b00000});
static_assert(extract_bits<2, 1, std::uint8_t, std::byte, 3>(0b11111010) == std::byte{0b01000});
static_assert(extract_bits<2, 1, std::uint8_t, std::byte, 3>(0b11111100) == std::byte{0b10000});
static_assert(extract_bits<2, 1, std::uint8_t, std::byte, 3>(0b11111110) == std::byte{0b11000});

static_assert(extract_bits<2, 1, std::uint8_t, std::byte, 3, true>(0b11111000) == std::byte{0b11100000});
static_assert(extract_bits<2, 1, std::uint8_t, std::byte, 3, true>(0b11111010) == std::byte{0b11101000});
static_assert(extract_bits<2, 1, std::uint8_t, std::byte, 3, true>(0b11111100) == std::byte{0b11110000});
static_assert(extract_bits<2, 1, std::uint8_t, std::byte, 3, true>(0b11111110) == std::byte{0b11111000});
