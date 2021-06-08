#ifndef BIT_FIELD_HPP
#define BIT_FIELD_HPP

#include "config.hpp"

#include <concepts>
#include <cstddef>
#include <limits>
#if BIT_FIELD_EXCEPTIONS_ENABLED
#  include <stdexcept>
#endif

#include "bits.hpp"

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
