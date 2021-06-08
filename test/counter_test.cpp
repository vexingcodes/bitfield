#ifdef BIT_FIELD_TEST_SINGLE_HEADER
#  include "bit_field.hpp"
#else
#  include "counter.hpp"
#endif

struct counter_test {
    COUNTER_INITIALIZE(count, 0);
    static constexpr int value0 = COUNTER_VALUE(count, 256);
    COUNTER_ADD(count, 1, 256);
    static constexpr int value1 = COUNTER_VALUE(count, 256);
    COUNTER_ADD(count, 2, 256);
    static constexpr int value2 = COUNTER_VALUE(count, 256);
};

static_assert(counter_test::value0 == 0);
static_assert(counter_test::value1 == 1);
static_assert(counter_test::value2 == 3);
