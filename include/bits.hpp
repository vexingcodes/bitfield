/// Low-level bit manipulation utilities.
#ifndef BITS_HPP
#define BITS_HPP

#include "config.hpp"

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
