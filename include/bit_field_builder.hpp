#ifndef BIT_FIELD_BUILDER_HPP
#define BIT_FIELD_BUILDER_HPP

#include "config.hpp"

#include <tuple>

#include "bit_field.hpp"
#include "counter.hpp"

namespace BIT_FIELD_NAMESPACE {

namespace detail {

/// Helper function. If the argument pack is empty, returns the default bit_field_config, otherwise returns the first
/// template argument "merged" with the default bit_field_config. This is some trickery to allow the __VA_ARGS__ in the
/// BIT_FIELD macro to act as an optional argument.
template <auto TDefaultConfig, auto... TArgs>
static constexpr auto make_config = []() constexpr {
    if constexpr (sizeof...(TArgs) == 0) {
        return TDefaultConfig;
    } else {
        // A config was passed in. "Merge" it with the default config to get the effective field config.
        constexpr auto given_config = std::get<0>(std::make_tuple(TArgs...));
        constexpr std::size_t effective_offset =
            given_config.offset != no_override ? given_config.offset : TDefaultConfig.offset;
        constexpr bit_field_assignment_strategy effective_strategy =
            given_config.strategy != bit_field_assignment_strategy::no_override
            ? given_config.strategy : TDefaultConfig.strategy;
        using effective_type = std::conditional_t<std::is_void_v<typename decltype(given_config)::type>,
                                                  typename decltype(TDefaultConfig)::type,
                                                  typename decltype(given_config)::type>;
        return bit_field_config<effective_type>{ .offset = effective_offset, .strategy = effective_strategy };
    }
}();

} // End namespace detail.

/// A class that can be derived from to allow multiple type-safe bit fields to be defined with a DSL-like syntax.
///
/// Inside the derived class definition the BIT_FIELD and BIT_FIELD_PAD macros can be used to define the layout of the
/// bit field.
///
/// @tparam TDerived       Place the class deriving from bit_field here. This is basically only used as a tag to make
///                        the bit_field type unique so it can internally use multiple compile-time counters.
/// @tparam T              The underlying storage type of the bit field. Must be integral or std::byte.
/// @tparam TDefaultConfig The default configuration for any field that does not override settings.
template <typename TDerived, typename T, bit_field_config TDefaultConfig = bit_field_config{}>
    requires (std::integral<T> || std::is_same_v<T, std::byte>)
struct bit_field_builder {
    using value_type = T;

    static constexpr auto default_config = TDefaultConfig;

    /// The most fields we could possibly have is one for each bit. Use that as the maximum recursion depth for the
    /// counter.
    static constexpr unsigned max_field = bits<T>;

    /// Returns true if all of the bits in the value type have been allocated, false otherwise.
    static constexpr bool is_complete() {
        return COUNTER_VALUE(TDerived::count, max_field) == max_field;
    }

    // Compile-time counter to count the number of bits allocated so far.
    COUNTER_INITIALIZE(count, 0);

    T raw_value{0};
};

/// Increment the bit counter by num_bits without adding a new field. Used to represent padding bits.
///
/// @param self     The "self" type derived from bit_field_builder. This parameter is only needed when the derived class
///                 is templated, and the base class (bit_field_builder) depends on those template parameters. In that
///                 case, looking up any of the base's members is a dependent name, so needs to be specified
///                 differently. In those contexts, the "self" parameter should be the fully specified derived type with
///                 a trailing "::". Cases where this is required are very rare, and typically BIT_FIELD_PAD should be
///                 used instead, which passes nothing in for "self".
/// @param num_bits The number of bits of padding to add.
#define BIT_FIELD_PAD_DEP(self, num_bits) \
    COUNTER_ADD(self count, num_bits, self max_field)

/// Same as BIT_FIELD_PAD_DEP, but for use in contexts where dependent name lookups are not required (most cases.)
#define BIT_FIELD_PAD(num_bits) \
    BIT_FIELD_PAD_DEP(, num_bits)

/// Define a new field with a given number of bits.
///
/// @param self     The "self" type derived from bit_field_builder. This parameter is only needed when the derived class
///                 is templated, and the base class (bit_field_builder) depends on those template parameters. In that
///                 case, looking up any of the base's members is a dependent name, so needs to be specified
///                 differently. In those contexts, the "self" parameter should be the fully specified derived type with
///                 a trailing "::". Cases where this is required are very rare, and typically BIT_FIELD should be used
///                 instead, which passes nothing in for "self".
/// @param name     The name of the field. Used to generate the below symbol names.
/// @param num_bits The number of consecutive bits the field should consume.
/// @param ...      The field configuration for this config. Specifies type, offset, and default assignment strategy.
///                 This is optional. If one is not provided, the default configuration for the builder will be used.
///
/// This creates the following symbols at the current scope (replacing "name" with the given name of the field):
///     name     -- A bit_field type definition. Can be used to get information about the field, and to invoke the
///                 static get/set methods.
///     get_name -- Member function accessor for the field. Takes no parameters, and one template paramter which is the
///                 field configuration to use.
///     set_name -- Member function mutator for the field. Takes one template parameter which is the field configuration
///                 to use. Takes one parameter which is the value to set into the field. The type of this parameter is
///                 specified by the configuration template parameter. Two overloads of this function are provided so
///                 that if the return_bool strategy is employed, the function called will be marked with the nodiscard
///                 attribute.
#define BIT_FIELD_DEP(self, name, num_bits, ...)                                                                       \
    static_assert(COUNTER_VALUE(self count, self max_field) + num_bits <= self max_field);                             \
                                                                                                                       \
    using name = ::BIT_FIELD_NAMESPACE::bit_field<                                                                     \
        num_bits,                                                                                                      \
        COUNTER_VALUE(self count, self max_field),                                                                     \
        ::BIT_FIELD_NAMESPACE::detail::make_config<self default_config __VA_OPT__(,) __VA_ARGS__>>;                    \
                                                                                                                       \
    template <auto TConfig = ::BIT_FIELD_NAMESPACE::bit_field_config{}>                                                \
    constexpr auto get_##name() const noexcept {                                                                       \
        return name::get<TConfig>(self raw_value);                                                                     \
    }                                                                                                                  \
                                                                                                                       \
    template <auto TConfig = ::BIT_FIELD_NAMESPACE::bit_field_config{}>                                                \
    [[nodiscard]] constexpr bool set_##name(const auto value) noexcept                                                 \
            requires (name::template effective_strategy<TConfig> ==                                                    \
                      ::BIT_FIELD_NAMESPACE::bit_field_assignment_strategy::return_bool) {                             \
        return name::set<TConfig>(self raw_value, value);                                                              \
    }                                                                                                                  \
                                                                                                                       \
    template <auto TConfig = ::BIT_FIELD_NAMESPACE::bit_field_config{}>                                                \
    constexpr void set_##name(const auto value) noexcept(noexcept(name::set<TConfig>(self raw_value, value)))          \
            requires (name::template effective_strategy<TConfig> !=                                                    \
                      ::BIT_FIELD_NAMESPACE::bit_field_assignment_strategy::return_bool) {                             \
        name::set<TConfig>(self raw_value, value);                                                                     \
    }                                                                                                                  \
                                                                                                                       \
    BIT_FIELD_PAD_DEP(self, num_bits)

/// Same as BIT_FIELD_DEP, but for use in contexts where dependent name lookups are not required (most cases.)
#define BIT_FIELD(name, num_bits, ...) \
    BIT_FIELD_DEP(, name, num_bits, __VA_ARGS__)

} // End namespace BIT_FIELD_NAMESPACE.

#endif // BIT_FIELD_BUILDER_HPP
