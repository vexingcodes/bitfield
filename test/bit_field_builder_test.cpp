#include <cstdint>

#ifdef BIT_FIELD_TEST_SINGLE_HEADER
#  include "bit_field.hpp"
#else
#  include "bit_field_builder.hpp"
#endif

// Simple bitfield definition. See page 217, section A.1.2 of the IO-Link specification:
// https://io-link.com/share/Downloads/Package-2020/IOL-Interface-Spec_10002_V113_Jun19.pdf
struct m_sequence_control : BIT_FIELD_NAMESPACE::bit_field_builder<m_sequence_control, std::uint8_t> {
    enum class communication_channel {
        process   = 0,
        page      = 1,
        diagnosis = 2,
        isdu      = 3
    };

    enum class transmission_direction {
        write = 0,
        read  = 1
    };

    BIT_FIELD(address,   5, BIT_FIELD_NAMESPACE::bit_field_config<std::uint8_t>{});
    BIT_FIELD(channel,   2, BIT_FIELD_NAMESPACE::bit_field_config<communication_channel>{});
    BIT_FIELD(direction, 1, BIT_FIELD_NAMESPACE::bit_field_config<transmission_direction>{});
};

static_assert(m_sequence_control::is_complete());

// Static get.
static_assert(m_sequence_control::address::get(0b00000000) == 0);
static_assert(m_sequence_control::address::get(0b00011111) == 31);

static_assert(m_sequence_control::channel::get(0b00000000) == m_sequence_control::communication_channel::process);
static_assert(m_sequence_control::channel::get(0b00100000) == m_sequence_control::communication_channel::page);
static_assert(m_sequence_control::channel::get(0b01000000) == m_sequence_control::communication_channel::diagnosis);
static_assert(m_sequence_control::channel::get(0b01100000) == m_sequence_control::communication_channel::isdu);

static_assert(m_sequence_control::direction::get(0b00000000) == m_sequence_control::transmission_direction::write);
static_assert(m_sequence_control::direction::get(0b10000000) == m_sequence_control::transmission_direction::read);

// Member get.
static_assert(m_sequence_control{0b00000000}.get_address() == 0);
static_assert(m_sequence_control{0b00000001}.get_address() == 1);
static_assert(m_sequence_control{0b00011111}.get_address() == 31);

static_assert(m_sequence_control{0b00000000}.get_channel() == m_sequence_control::communication_channel::process);
static_assert(m_sequence_control{0b00100000}.get_channel() == m_sequence_control::communication_channel::page);
static_assert(m_sequence_control{0b01000000}.get_channel() == m_sequence_control::communication_channel::diagnosis);
static_assert(m_sequence_control{0b01100000}.get_channel() == m_sequence_control::communication_channel::isdu);

static_assert(m_sequence_control{0b00000000}.get_direction() == m_sequence_control::transmission_direction::write);
static_assert(m_sequence_control{0b10000000}.get_direction() == m_sequence_control::transmission_direction::read);

// Static set.
template <std::uint8_t NValue>
constexpr std::uint8_t set_address_static() {
    std::uint8_t value = 0;
    m_sequence_control::address::set(value, NValue);
    return value;
}

static_assert(set_address_static<0>() == 0);
static_assert(set_address_static<1>() == 1);
static_assert(set_address_static<31>() == 31);

// Member set.
template <std::uint8_t NValue>
constexpr m_sequence_control set_address() {
    m_sequence_control value{0b00000000};
    value.set_address(NValue);
    return value;
}

static_assert(set_address<0>().get_address() == 0);
static_assert(set_address<1>().get_address() == 1);
static_assert(set_address<31>().get_address() == 31);

template <m_sequence_control::communication_channel TValue>
constexpr m_sequence_control set_channel() {
    m_sequence_control value{0b00000000};
    value.set_channel(TValue);
    return value;
}

static_assert(set_channel<m_sequence_control::communication_channel::process>().get_channel() == 
                          m_sequence_control::communication_channel::process);
static_assert(set_channel<m_sequence_control::communication_channel::page>().get_channel() == 
                          m_sequence_control::communication_channel::page);
static_assert(set_channel<m_sequence_control::communication_channel::diagnosis>().get_channel() == 
                          m_sequence_control::communication_channel::diagnosis);
static_assert(set_channel<m_sequence_control::communication_channel::isdu>().get_channel() == 
                          m_sequence_control::communication_channel::isdu);

template <m_sequence_control::transmission_direction TValue>
constexpr m_sequence_control set_direction() {
    m_sequence_control value{0b00000000};
    value.set_direction(TValue);
    return value;
}

static_assert(set_direction<m_sequence_control::transmission_direction::write>().get_direction() ==
                            m_sequence_control::transmission_direction::write);
static_assert(set_direction<m_sequence_control::transmission_direction::read>().get_direction() ==
                            m_sequence_control::transmission_direction::read);
