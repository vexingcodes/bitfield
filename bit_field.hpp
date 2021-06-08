/*
File: bit_field.hpp (generated header file)
Version: 1.0.0

Copyright 2021 WinterWinds Robotics, Inc.

Licensed under the Apache License, Version 2.0 (the License);
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an AS IS BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

// Compile-time configuration options for the bit field library.
#ifndef BIT_FIELD_CONFIG_HPP
#define BIT_FIELD_CONFIG_HPP

// Allow the user to define BIT_FIELD_NO_EXCEPTIONS before including this header to explicitly disable exceptions.
// Otherwise follow whether or not the compiler has exceptions enabled.
#ifdef BIT_FIELD_NO_EXCEPTIONS
#  define BIT_FIELD_EXCEPTIONS_ENABLED 0
#else
#  if __cpp_exceptions >= 199711
#    define BIT_FIELD_EXCEPTIONS_ENABLED 1
#  else
#    define BIT_FIELD_EXCEPTIONS_ENABLED 0
#  endif
#endif

// Allow the user to define the default strategy for handling invalid bits before they include the header file.
#ifndef BIT_FIELD_DEFAULT_STRATEGY
#  define BIT_FIELD_DEFAULT_STRATEGY mask
#endif

// Allow the user to define what namespace everything goes into.
#ifndef BIT_FIELD_NAMESPACE
#  define BIT_FIELD_NAMESPACE bf
#endif

#endif // BIT_FIELD_CONFIG_HPP
/// Low-level bit manipulation utilities.
#ifndef BITS_HPP
#define BITS_HPP


#include <climits>
#include <concepts>
#include <cstddef>

namespace BIT_FIELD_NAMESPACE {

/// Get the number of bits required to store a value of the given type.
///
/// @tparam T The type whose size in bits is to be calculated.
template <typename T>
constexpr std::size_t bits = sizeof(T) * CHAR_BIT;

/// Construct a bit mask of type T with NCount consecutive set bits starting at NStart (from LSB).
///
/// @tparam T      The result type of the bit mask. Must be integral or std::byte.
/// @tparam NStart The bit offset from the least significant bit representing the first bit set in the bit mask.
/// @tparam NCount The number of consecutive bits to set in the bit mask.
///
/// @returns A value of the requested type T representing the desired bit mask.
template <typename T, unsigned NStart, unsigned NCount>
    requires ((std::integral<T> || std::is_same_v<T, std::byte>) && NCount > 0 && NStart + NCount <= bits<T>)
constexpr T bit_mask = []() constexpr {
    T value{0};
    for (unsigned i = NStart; i < NStart + NCount; ++i) {
        value |= static_cast<T>(1 << i);
    }
    return value;
}();

/// Takes a conseuctive run of a specified number of bits starting at some lsb-relative offset from some source and
/// places those bits at some other lsb-relative offset in a destination of user-configurable type. Basically it moves
/// a conseuctive chunk of bits around.
///
/// @tparam NBits              The number of bits to extract.
/// @tparam NSourceOffset      The bit offset of the first bit to extract from the source, relative to the least
///                            significant bit.
/// @tparam TSource            The source type, i.e. the type from which bits will be extracted.
/// @tparam NDestinationOffset The bit offset of the first bit in the destination that should contain the extracted
///                            bits, relative to the least significant bit.
/// @tparam TDestination       The type into which the extracted bits should be placed.
/// @tparam BSkipMask          Avoid masking the source value. If this is true, it is the caller's responsibility
///                            to ensure that the source value is already masked, or that not masking the value does
///                            not result in any issues.
///
/// @param source The value from which the bits should be extracted.
///
/// @returns A TDestination containing the NBits extracted from source at offset NSourceOffset starting at
///          NDestinationOffset.
template <std::size_t NBits,
          std::size_t NSourceOffset,
          typename    TSource,
          typename    TDestination = TSource,
          std::size_t NDestinationOffset = 0,
          bool        BSkipMask = false>
    requires (
        (std::integral<TSource> || std::is_enum_v<TSource> || std::is_same_v<TSource, std::byte>) &&
        (std::integral<TDestination> || std::is_enum_v<TDestination> || std::is_same_v<TDestination, std::byte>) &&
        (NBits > 0) &&
        (bits<TSource> >= NBits + NSourceOffset) &&
        (bits<TDestination> >= NBits + NDestinationOffset)
    )
constexpr TDestination extract_bits(const TSource source) noexcept {
    using TSourceUnderlying = typename std::conditional_t<std::is_enum_v<TSource>,
                                                          std::underlying_type<TSource>,
                                                          std::type_identity<TSource>>::type;
    using TDestinationUnderlying = typename std::conditional_t<std::is_enum_v<TDestination>,
                                                               std::underlying_type<TDestination>,
                                                               std::type_identity<TDestination>>::type;

    constexpr TSourceUnderlying mask = bit_mask<TSourceUnderlying, NSourceOffset, NBits>;
    constexpr int shift = static_cast<int>(NSourceOffset) - static_cast<int>(NDestinationOffset);

    TSourceUnderlying source_bits = [&source]() constexpr -> TSourceUnderlying {
        if constexpr (!BSkipMask) {
            return static_cast<TSourceUnderlying>(source) & mask;
        } else {
            return static_cast<TSourceUnderlying>(source);
        }
    }();

    if constexpr (shift == 0) {
        // No shifting required. The bits stay in the same place.
        return static_cast<TDestination>(static_cast<TDestinationUnderlying>(source_bits));
    } else if constexpr (shift > 0) {
        // Need to right-shift the bits into place. Perform the shift on the source type first since the initial
        // position of the bits could be out of range for the destination type.
        return static_cast<TDestination>(static_cast<TDestinationUnderlying>(source_bits >> shift));
    } else /* shift < 0 */ {
        // Need to left-shift the bits into place. First, cast the source bits to the underlying destination type since
        // the final position of the bits could be out of range for the source type.
        return static_cast<TDestination>(static_cast<TDestinationUnderlying>(source_bits) << (-shift));
    }
}

} // End namespace BIT_FIELD_NAMESPACE.

#endif // BITS_HPP
#ifndef BIT_FIELD_HPP
#define BIT_FIELD_HPP


#include <concepts>
#include <cstddef>
#include <limits>
#if BIT_FIELD_EXCEPTIONS_ENABLED
#  include <stdexcept>
#endif


namespace BIT_FIELD_NAMESPACE {

/// Enum allowing a behavioral strategy to be selected at compile-time, dictating how to respond if the field being set
/// has bits set outside of the expected range. The strategy is set first as a class-wide default. The class-wide
/// default can be overridden on a field-by-field basis. The field setting can be overridden on individual "set" calls.
enum class bit_field_assignment_strategy {
    /// Do nothing. Assume the field is valid. If invalid bits are set, they can corrupt other fields.
    unchecked,

    /// Silently mask away invalid bits, keeping only valid bits. This is the default behavior.
    mask,

    /// Return false from "set" functions if invalid bits are set, and do not change anything. Otherwise, return true
    /// and actually set the value.
    return_bool,

#if BIT_FIELD_EXCEPTIONS_ENABLED
    /// Throw a bit_field_error from "set" functions if invalid bits are set, and do not change anything. Otherwise,
    /// actually set the value.
    exception,
#endif

    /// This is not an actual strategy that can be employed. It is a sentinel value indicating that the default value
    /// for the current context should be used.
    no_override
};

/// A sentinel value that can be used in a bit_field_config type to indicate that the default offset should be used.
/// The default chosen is context-sensitive. There's a global default and a field-level default. If using the
/// bit_field_builder class there's also a class-level default.
constexpr std::size_t no_override = std::numeric_limits<std::size_t>::max();

/// A sentinel value that can be used in a bit_field_config type to indicate that the offset in the result type
/// should be the same as the offset in the storage type, resulting in no bit shifting for field extraction.
constexpr std::size_t no_shift = no_override - 1;

#if BIT_FIELD_EXCEPTIONS_ENABLED
/// Exception type thrown when invalid bits are set in a value which is being written to a bit field. This is only
/// thrown if the bit_field_assignment_strategy::exception strategy is used.
struct bit_field_error : public std::runtime_error {
    using std::runtime_error::runtime_error;
};
#endif

/// The configuration information for a bit field. Represents things about a single bit field type that are allowed to
/// vary on a call-by-call basis. This includes the return type, the bit offset of the result, and a strategy to employ
/// when trying to set values in the bit field that contain bits set outside of the expected span.
///
/// @tparam TFieldType The desired result type of the field. The type of void means use the default.
template <typename TBitFieldType = void>
struct bit_field_config {
    using type = TBitFieldType;

    /// LSB-relative offset within the TBitFieldType to place the bits.
    std::size_t offset{no_override};

    /// Strategy to use when setting fields.
    bit_field_assignment_strategy strategy{bit_field_assignment_strategy::no_override};
};

/// A type representing a single field within the bit field.
///
/// @tparam NBits          The number of bits in the field.
/// @tparam NOffset        The lsb-relative offset (within the bit field's value type) where the field's bits begin.
/// @tparam TDefaultConfig The default field configuration to use when calling get/set. Can be overridden on
///                        individual calls to get/set.
///
/// @note This class does not actually store information. See bit_field_builder if you want a type with bit fields that
///       contains its own data.
template <std::size_t NBits, std::size_t NOffset, auto TDefaultConfig = bit_field_config{}>
class bit_field {
public:
    static constexpr std::size_t offset = NOffset;
    static constexpr std::size_t bits = NBits;
    static constexpr auto default_config = TDefaultConfig;

    /// Determines the actual offset to use based on the passed in bit_field_config.
    template <auto TConfig>
    static constexpr std::size_t effective_offset = []() constexpr {
        if constexpr (TConfig.offset == no_override) {
            if constexpr (default_config.offset == no_override) {
                return 0;
            } else {
                if constexpr (default_config.offset == no_shift) {
                    return offset;
                } else {
                    return default_config.offset;
                }
            }
        } else {
            if constexpr (TConfig.offset == no_shift) {
                return offset;
            } else {
                return TConfig.offset;
            }
        }
    }();

    /// Determines the actual assignment strategy to use based on the passed in bit_field_config.
    template <auto TConfig>
    static constexpr bit_field_assignment_strategy effective_strategy = []() constexpr {
        if constexpr (TConfig.strategy == bit_field_assignment_strategy::no_override) {
            if constexpr (default_config.strategy == bit_field_assignment_strategy::no_override) {
                return bit_field_assignment_strategy::BIT_FIELD_DEFAULT_STRATEGY;
            } else {
                return default_config.strategy;
            }
        } else {
            return TConfig.strategy;
        }
    }();

    /// Determines the actual result type to use based on the passed in bit_field_config.
    template <auto TConfig, typename TStorage>
    using effective_storage =
        std::conditional_t<std::is_void_v<typename decltype(TConfig)::type>,
                           std::conditional_t<std::is_void_v<typename decltype(default_config)::type>,
                                              TStorage,
                                              typename decltype(default_config)::type>,
                           typename decltype(TConfig)::type>;

    /// Extract the desired run of bits from a value and place them in to some other value, possibly at an offset.
    ///
    /// @tparam TConfig A bit field configuration dictating what type to return and what offset to use. This is optional
    ///                 and if not provided then the field default will be used. If no field default was specified then
    ///                 the global default will be used.
    ///
    /// @param value The value from which the bits will be extracted.
    ///
    /// @returns The extracted bits at the desired offset in the desired data type.
    template <auto TConfig = bit_field_config{}>
    static constexpr auto get(const auto value) noexcept {
        static_assert(TConfig.strategy == bit_field_assignment_strategy::no_override,
                      "Overriding the strategy in TConfig does nothing.");
        using TStorage = std::remove_const_t<decltype(value)>;
        return extract_bits<bits, offset, TStorage, effective_storage<TConfig, TStorage>, effective_offset<TConfig>>(
            value);
    }

#if BIT_FIELD_EXCEPTIONS_ENABLED
#  define BIT_FIELD_SET_NOEXCEPT noexcept(effective_strategy<TConfig> != bit_field_assignment_strategy::exception)
#else
#  define BIT_FIELD_SET_NOEXCEPT noexcept
#endif

    /// Set implmenetation specialized for the return_bool strategy, exists only to set the nodisdcard attribute so the
    /// bool return value cannot be ignored. See set_impl for where the set logic is actually implented.
    template <auto TConfig = bit_field_config{}>
    [[nodiscard]] static constexpr bool set(auto& into, const auto value) noexcept
            requires (effective_strategy<TConfig> == bit_field_assignment_strategy::return_bool) {
        return set_impl<TConfig>(into, value);
    }

    /// Set implmenetation specialized for any non-return_bool strategy, which has a void return type and does not
    /// require the nodiscard attribute. See set_impl for where the set logic is actually implented.
    template <auto TConfig = bit_field_config{}>
    static constexpr void set(auto& into, const auto value) BIT_FIELD_SET_NOEXCEPT
            requires (effective_strategy<TConfig> != bit_field_assignment_strategy::return_bool) {
        set_impl<TConfig>(into, value);
    }

private:
    /// Extract the desired run of bits from a value and place them in some other value, possibly at an offset.
    ///
    /// @tparam TConfig A bit field configuration dictating at what offset the bits are located, and what assignment
    ///                 strategy to use. This is optional and if not provided then the field default will be used. If no
    ///                 field default was specified then the global default will be used.
    ///
    /// @param into  The value to be updated into the bit field storage variable.
    /// @param value The value containing the bits which will be inserted into into.
    ///
    /// @returns All strategies except for return_bool return nothing. The return_bool strategy returns true if the
    ///          operation was successful and false otherwise. The operation will fail if bits are set on value that
    ///          are outside the range of the bits being copied.
    ///
    /// @throws bit_field_error If the strategy is the exception strategy and there are invalid bets set on the value.
    template <auto TConfig = bit_field_config{}>
    static constexpr auto set_impl(auto& into, const auto value) BIT_FIELD_SET_NOEXCEPT {
        static_assert(std::is_void_v<typename decltype(TConfig)::type>, "Overriding the type in TConfig does nothing.");

        using TValue = std::remove_const_t<decltype(value)>;
        using TStorage = std::remove_cvref_t<decltype(into)>;
        using TUnderlying [[maybe_unused]] = typename std::conditional_t<std::is_enum_v<TValue>,
                                                                         std::underlying_type<TValue>,
                                                                         std::type_identity<TValue>>::type;

        // The code that actually performs the setting in memory. Pull out into a lambda for multiple uses. Hopefully
        // the compiler is smart enough to inline it since it's a single statement, and it's only used locally. In the
        // default case we skip masking the value inside extract_bits because we've either already done that in the case
        // of the return_bool and exception strategies, or we're not doing it at all in the case of the unchecked
        // strategy. The only strategy that does do masking is the mask strategy itself.
        auto set_helper = [&]<bool skip_mask = true>() {
            into = static_cast<TStorage>(into & ~bit_mask<TStorage, offset, bits>) |
                   extract_bits<bits, effective_offset<TConfig>, TValue, TStorage, offset, skip_mask>(value);
        };

        if constexpr (effective_strategy<TConfig> == bit_field_assignment_strategy::unchecked) {
            set_helper();
        } else if constexpr (effective_strategy<TConfig> == bit_field_assignment_strategy::mask) {
            set_helper.template operator()<false>();
        } else if constexpr (effective_strategy<TConfig> == bit_field_assignment_strategy::return_bool) {
            constexpr TUnderlying inverse_mask =
                static_cast<TUnderlying>(~bit_mask<TUnderlying, effective_offset<TConfig>, bits>);
            if (static_cast<TUnderlying>(value) & inverse_mask) {
                return false;
            } else {
                set_helper();
                return true;
            }
#if BIT_FIELD_EXCEPTIONS_ENABLED
        } else if constexpr (effective_strategy<TConfig> == bit_field_assignment_strategy::exception) {
            constexpr TUnderlying inverse_mask =
                static_cast<TUnderlying>(~bit_mask<TUnderlying, effective_offset<TConfig>, bits>);
            if (static_cast<TUnderlying>(value) & inverse_mask) {
                throw bit_field_error("invalid bits set");
            } else {
                set_helper();
            }
#endif
        } else {
            // Can't just directly static_assert here. See https://stackoverflow.com/questions/38304847.
            []<bool flag = false>(){ static_assert(flag, "unknown bit field assignment strategy"); }();
        }
    }

#undef BIT_FIELD_SET_NOEXCEPT
};

} // End namespace BIT_FIELD_NAMESPACE.

#endif // BIT_FIELD_HPP
/// Utilities for implementing a stateful compile-time counter.
#ifndef COUNTER_HPP
#define COUNTER_HPP


namespace BIT_FIELD_NAMESPACE {

/// A curious type which takes a number as its template parameter, and derives from all previous numbers down to zero in
/// a derivation chain. When combined with some clever function definitions, this allows stateful compile-time addition
/// to be implemented.
///
/// First, the counter can be seeded with an initial value like this:
///
///   static constexpr counter<0> count(counter<0>);
///
/// The initial value is zero here, but other values can be used. It simply declares a function called "count" that
/// takes a "counter<0>" and returns a "counter<0>". This function is never defined, it is simply declared, so it will
/// never consume space in the final binary. To get the current value of the counter the following expression can be
/// used:
///
///   decltype(count(counter<256>{}))::value
///
/// Here, 256 is some pre-determined maximum value for the counter. It is saying "give me the return type" when the
/// "count" function is called with an instance of "count<256>". Since there is no overload of "count" which directly
/// takes a "count<256>" the function overloading rules kick in.  There does exist the "count" function which takes a
/// "count<0>" as a parameter, as we explicitly declared. Since "count<256>" indirectly derives from "count<0>" that
/// function is selected because it is valid to call any function which takes a "count<0>" with a "count<256>" since
/// "count<256>" derives from "count<0>". The selected function returns a "count<0>" so the end result of the
/// "decltype" expression is "count<0>", and the result of "count<0>::value" is the integer value zero, the value we
/// used to initialize the counter.
///
/// To set a higher value for the counter, a new overload of the "count" function can be declared like this:
///
///   static constexpr counter<1> count(counter<1>);
///
/// After this function is declared, invoking the above expression to get the current counter value will return one
/// instead of zero, because the "count" overload for "counter<1>" is more specific than the overload of "count" that
/// accepts a "counter<0>". Note, these numbers may only ascend.
///
/// The expression to get the current value of the counter and the function declaration can be combined to increment the
/// counter value by one like this:
///
///   static constexpr counter<decltype(count(counter<256>{}))::value + 1> count(
///        counter<decltype(count(counter<256>{}))::value + 1>);
///
/// To ease the syntax here, convenience macros are provided. The following is a simple example using the macros:
///
///   struct counter_test {
///       COUNTER_INITIALIZE(count, 0);
///       static constexpr unsigned value0 = COUNTER_VALUE(count, 256);
///       COUNTER_ADD(count, 1, 256);
///       static constexpr unsigned value1 = COUNTER_VALUE(count, 256);
///       COUNTER_ADD(count, 1, 256);
///       static constexpr unsigned value2 = COUNTER_VALUE(count, 256);
///   };
///
///   static_assert(counter_test::value0 == 0);
///   static_assert(counter_test::value1 == 1);
///   static_assert(counter_test::value2 == 2);
template<unsigned N>
struct counter : public counter<N - 1> {
    static constexpr unsigned value = N;
};

/// Template specialization of "counter" that terminates the derivation chain for counter.
template<>
struct counter<0> {
    static constexpr unsigned value = 0;
};

/// Convenience macro for initializing a new compile-time counter.
///
/// @param name          The name of the function used to represent the counter. Allows multiple
///                      counters to be used in the same scope.
/// @param initial_value The initial value to assign to the counter.
#define COUNTER_INITIALIZE(name, initial_value) \
    static constexpr ::BIT_FIELD_NAMESPACE::counter<initial_value> name(::BIT_FIELD_NAMESPACE::counter<initial_value>)

/// Convenience macro for getting the current value of a counter.
///
/// @param name      The name of the counter whose value is being queried.
/// @param max_value The maximum possible value of the counter. Must be a constant expression.
#define COUNTER_VALUE(name, max_value) \
    decltype(name(::BIT_FIELD_NAMESPACE::counter<max_value>{}))::value

/// Convenience macro for adding some amount to an existing compile-time counter.
///
/// @param name      The name of the counter whose value is being updated.
/// @param add_value The amount to be added to the counter.
/// @param max_value The maximum possible value of the counter. Must be a constant expression.
#define COUNTER_ADD(name, add_value, max_value)                                                        \
    static constexpr ::BIT_FIELD_NAMESPACE::counter<COUNTER_VALUE(name, max_value) + add_value> count( \
        ::BIT_FIELD_NAMESPACE::counter<COUNTER_VALUE(name, max_value) + add_value>)

} // End namespace BIT_FIELD_NAMESPACE.

#endif // COUNTER_HPP
#ifndef BIT_FIELD_BUILDER_HPP
#define BIT_FIELD_BUILDER_HPP


#include <tuple>


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
