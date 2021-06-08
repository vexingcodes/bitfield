/// Utilities for implementing a stateful compile-time counter.
#ifndef COUNTER_HPP
#define COUNTER_HPP

#include "config.hpp"

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
