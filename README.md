# C++20 Bit Field Library

This repository contains a C++20 header-only library for manipulating bit fields in a type-safe and flexible way. C++
has built-in bit-field support, but it is not as useful as one would hope. The standard does not define whether bit
fields should start at the least or most significant bit. Some types are not supported by built-in bit fields. The bits
for a single field in a bit field are always placed in the least significant position with built-in fields, i.e. you
cannot instruct the compiler to leave the bits in place rather than shifting them. Finally, with built-in bit fields, it
is the programmer's responsibility to catch some errors in their usage like assigning a field a value which has bits set
outside of the expected range, which by default are silently lost. This library aims to overcome all of these issues.

This is a header-only library. In this code base, the library is constructed as several header files to ease development
and make the code more readable, but a single header file can be constructed from the several header files to make it
easier to include the library in any project. This single header file is committed to the repository root and is named
simply `bit_field.hpp`. There are no external dependencies for this library, and it only depends on a few standard
library headers.

# Compiler Support

GCC supports this library from 10.1 and beyond. Clang does not yet have sufficient support (as of version 12.0) for
non-type template parameters being classes. It is believed that this implementation fully conforms to the C++20
specification, but that has not been proven. As such, Clang support should be forthcoming.

The library supports being compiled with or without exception handling support enabled. If exception handling is
disabled, all exception-related code will be ifdef'd out. Even if exception handling is enabled at the compiler level, a
compile-time flag can be added to disable the exception handling explicitly.

# Try it Out

It's easy to use [compiler explorer](https://godbolt.org/#g:!((g:!((g:!((h:codeEditor,i:(fontScale:12,fontUsePx:'0',j:1,lang:c%2B%2B,selection:(endColumn:1,endLineNumber:3,positionColumn:1,positionLineNumber:3,selectionStartColumn:1,selectionStartLineNumber:3,startColumn:1,startLineNumber:3),source:'%23include+%3Chttps://raw.githubusercontent.com/vexingcodes/bitfield/main/bit_field.hpp%3E%0A%0A'),l:'5',n:'0',o:'C%2B%2B+source+%231',t:'0')),k:53.162597421276644,l:'4',m:100,n:'0',o:'',s:0,t:'0'),(g:!((h:compiler,i:(compiler:gsnapshot,filters:(b:'0',binary:'1',commentOnly:'0',demangle:'0',directives:'0',execute:'1',intel:'0',libraryCode:'1',trim:'1'),fontScale:12,fontUsePx:'0',j:3,lang:c%2B%2B,libs:!(),options:'-std%3Dc%2B%2B20+-Wall+-Wextra+-Wpedantic+-Werror+-O2',selection:(endColumn:1,endLineNumber:1,positionColumn:1,positionLineNumber:1,selectionStartColumn:1,selectionStartLineNumber:1,startColumn:1,startLineNumber:1),source:1),l:'5',n:'0',o:'x86-64+gcc+(trunk)+(Editor+%231,+Compiler+%233)+C%2B%2B',t:'0')),header:(),k:21.520730819835435,l:'4',n:'0',o:'',s:0,t:'0'),(g:!((h:output,i:(compiler:3,editor:1,fontScale:14,fontUsePx:'0',wrap:'1'),l:'5',n:'0',o:'%233+with+x86-64+gcc+(trunk)',t:'0')),k:25.316671758887928,l:'4',n:'0',o:'',s:0,t:'0')),l:'2',m:100.00000000000003,n:'0',o:'',t:'0')),version:4) to play with this library and decide if it meets your needs. The code snippets below can be pasted into compiler explorer to ensure things work as expected.

If you wish to include the header file in your project, it can easily be downloaded on the command-line using `wget`,
for example:

```bash
wget https://raw.githubusercontent.com/vexingcodes/bitfield/v1.0.0/bit_field.hpp
```

Replace `v1.0.0` with the desired tag or branch.

# Compile-Time Configuration

There are a few compile-time configuration options the library provides. These can be configured using `#define`
directives before including the header file, or by using compiler directives like `-DOPTION=value`.

All symbols in the library are namespaced. By default this namespace is simply called `bf` for Bit Field. If this
namespace name clashes with something, you can define `BIT_FIELD_NAMESPACE` to be the desired namespace name before
including the header file.

Exception handling can be explicitly disabled by defining `BIT_FIELD_NO_EXCEPTIONS`.

The default assignment strategy for bit fields can be set using `BIT_FIELD_DEFAULT_STRATEGY`. The default strategy is
`mask`. The purpose and effects of different strategies are discussed later in the documentation.

# Usage

This library is designed to be as simple as possible for the most common use cases, but allow advanced usage at the
expense of some syntactical ugliness where needed. The library provides several separate interfaces allowing bit
manipulations, at different levels of abstraction. All interfaces can be used in `constexpr` context.

## bf::extract\_bits

At the lowest level, the `bf::extract_bits` is used to perform all actual bit movement in the library. It's not
recommended to directly use it, and its usage will not be described here, but it does exist, and you can use it if you
want -- I'm not your boss.

## bf::bit\_field

The next interface is `bf::bit_field`. A `bit_field` is a type which specifies a location from which bits should be
extracted, consisting of the number of consecutive bits to extract and the position of the first bit to extract relative
to the least significant bit. In the simplest case, its usage is quite straightforward, but advanced behavior can be
specified if needed.

### Extraction

For example, to extract three bits staring at bit 2 from any integer/byte/enum value the following can be used:

```cpp
static_assert( bf::bit_field<3, 2>::get(0b11100) == 0b111 );
static_assert( std::is_same_v<decltype(bf::bit_field<3, 2>::get(0b11100)), decltype(0b111)> );
```

As can be seen, the resulting type here is the same as the type provided to `get`, and the extracted bits are shifted to
the least significant position in the result. By default, the result value is always the same type as the input
argument. This can be seen by providing a `std::byte` instead of a plain integer value.

```cpp
static_assert( bf::bit_field<3, 2>::get(std::byte{0b11100}) == std::byte{0b111} );
```

Both the return type and the final offset of the bits within the result can be controlled with a third template
argument. To change the bit field settings, a `bf::bit_field_config` can be provided as a template argument to
`bit_field`. It has a single template argument which specifies the return type of the call to `get`. For example, to
extract a `std::byte` regardless of the input type, the following can be done: 

```cpp
static_assert( bf::bit_field<3, 2, bf::bit_field_config<std::byte>{}>::get(0b11100) == std::byte{0b111} );
```

The offset of the bits in the result type can also be adjusted, for example, instead of putting the output at the least
significant bit, we can place it at the third position.

```cpp
static_assert( bf::bit_field<3, 2, bf::bit_field_config{ .offset = 3 }>::get(0b11100) == 0b111000 );
```

Finally, both the type and the offset can be set in the same way, by specifying the `bf::bit_field_config` as desired.
Combining the two above examples:

```cpp
static_assert( bf::bit_field<3, 2, bf::bit_field_config<std::byte>{ .offset = 3 }>::get(0b11100) == std::byte{0b111000} );
```

This is quite a mouthful to type every time you want to extract bits. Fortunately, all of the information about how to
extract values is contained in the type, so it can be aliased and reused very easily.

```cpp
using my_field = bf::bit_field<3, 2, bf::bit_field_config<std::byte>{ .offset = 3 }>;
static_assert( my_field::get(0b11100) == std::byte{0b111000} );
```

It may be useful to define a default return type as above, but occasionally you may wish to extract the same bits, but
but get a different value type or offset. To accomodate this usage, the call to `get` itself can also accept a
`bf::bit_field_config` that will override the default values. Note, only values that you explicitly set are overridden.
So, for instance, to change the return type of the above but keep the specified offset, simply specify the new return
type, but do not set the offset at all. As an example, here we take the `bf::bit_field` type we defined earlier, and
make the single call to `get` return a `std::uint8_t` rather than a `std::byte` while keeping the offset of three.

```cpp
using my_field = bf::bit_field<3, 2, bf::bit_field_config<std::byte>{ .offset = 3 }>;
static_assert( my_field::get<bf::bit_field_config<std::uint8_t>{}>(0b11100) == 0b111000 );
```

Similarly, we can override the offset without changing the default type by omitting the type:

```cpp
using my_field = bf::bit_field<3, 2, bf::bit_field_config<std::byte>{ .offset = 3 }>;
static_assert( my_field::get<bf::bit_field_config{ .offset = 0 }>(0b11100) == std::byte{0b111} );
```

Finally, both the offset and the type can be overridden for a single call:

```cpp
using my_field = bf::bit_field<3, 2, bf::bit_field_config<std::byte>{ .offset = 3 }>;
static_assert( my_field::get<bf::bit_field_config<std::uint8_t>{ .offset = 0 }>(0b11100) == 0b111 );
```

The way this overriding works is that by default, if you don't override the type in the `get` call then the type
defaults to `void`, and if `void` is provided, then the value of the type specified in the `bf::bit_field` type is used
as a default. If that is also `void`, then we finally default to returning the same type that was given to the function.
Similarly for the offset, a sentinel value of `bf::no_override` is the default for the `bf::bit_field_config::offset`
field. If the value is `bf::no_override` in the `get` call, then we will default to the value provided to the
`bf::bit_field_config` specified in the `bf::bit_field` type. If that value is also `bf::no_override` then the global
default value of zero will be used for the offset. This mechanism means you don't have to specify redundant information
when overriding a single parameter, and instead you only need to specify the values you wish to explicitly override.

Finally, there is another special sentinel value for offset -- `bf::no_shift` -- which can be used to indicate that the
resulting value shouldn't be shifted at all, and the bits will be left in the same position as in the original value,
but all bits outside the specified span will be masked. In the below example, the result value is at offset 2, which is
the original offset provided to the `bf::bit_field` type in the second template parameter position.

```cpp
static_assert( bf::bit_field<3, 2, bf::bit_field_config{ .offset = bf::no_shift }>::get(0b11100) == 0b11100 );
```

The `bf::bit_field` type will fail to compile if you ask it to do something that cannot be done. For example, if you try
to extract the eighth (zero-indexed) bit from an eight-bit value, it will fail to compile:

```cpp
auto wont_compile = []{ bf::bit_field<1, 8>::get(std::byte{0}); };
```

And if you try to extract the eighth bit of a sixteen-bit value but place it in the eighth bit of an eight-bit value
that will also fail:

```cpp
using test_field = bf::bit_field<1, 8, bf::bit_field_config{ .offset = bf::no_shift }>;
static_assert( test_field::get(std::uint16_t{0b100000000}) == 0b100000000 );
auto wont_compile = []{ test_field::get<bf::bit_field_config<std::byte>{}>(std::uint16_t{0b100000000}); };
```

In summary, the library will not allow any action which causes a loss of information. Both the source and the
destination types must be capable of containing all of the bits in the bit field at the specified offset. Any violation
of this constraint will result in a failure to compile. Due to the use of C++ concepts, the error message in this case
is fairly easy to understand.

Extraction to integer/byte types has been covered, but enums and scoped enums can also be the source and/or target of
bit field extractions.

```cpp
enum test_enum {
    value_0 = 0b00,
    value_1 = 0b01,
    value_2 = 0b10,
    value_3 = 0b11
};

using enum_bit_field = bf::bit_field<2, 3, bf::bit_field_config<test_enum>{}>;

static_assert( enum_bit_field::get(0b00000) == test_enum::value_0);
static_assert( enum_bit_field::get(0b01000) == test_enum::value_1);
static_assert( enum_bit_field::get(0b10000) == test_enum::value_2);
static_assert( enum_bit_field::get(0b11000) == test_enum::value_3);

static_assert( bf::bit_field<2, 0, bf::bit_field_config<std::uint8_t>{}>::get(test_enum::value_3) == 0b11 );
```

```cpp
enum class test_scoped_enum : std::uint8_t {
    value_0 = 0b00,
    value_1 = 0b01,
    value_2 = 0b10,
    value_3 = 0b11
};

using scoped_enum_bit_field = bf::bit_field<2, 3, bf::bit_field_config<test_scoped_enum>{}>;

static_assert( scoped_enum_bit_field::get(0b00000) == test_scoped_enum::value_0);
static_assert( scoped_enum_bit_field::get(0b01000) == test_scoped_enum::value_1);
static_assert( scoped_enum_bit_field::get(0b10000) == test_scoped_enum::value_2);
static_assert( scoped_enum_bit_field::get(0b11000) == test_scoped_enum::value_3);

static_assert( bf::bit_field<2, 0, bf::bit_field_config<std::uint8_t>{}>::get(test_scoped_enum::value_3) == 0b11 );
```

### Insertion

In addition to extracting bits from values, there exists an interface to update values in bit fields. This interface is
similar to `get` but it also requires you to pass in the value to be updated. For instance, to set the two bits starting
at bit three to a specified value:

```cpp
static_assert(
    []() constexpr {
        std::uint8_t value{0};
        bf::bit_field<3, 2>::set(value, 0b101);
        return value;
    }() == 0b10100
);
```

Similarly to the `get` method, the `set` method optionally accepts a `bf::bit_field_config` template parameter that can
be used to override the expected offset of the bits in the new value. Since the type of value can be dictated by the
parameter passed to `set`, it is unnecessary to override the type parameter of the `bf::bit_field_config`, and in fact
overriding it will result in a compilation error.

There is an additional field on the `bf::bit_field_config` called `strategy` that dictates how `set` should behave
regarding additional bits set in the new bits outside of the specified bit range. The possible values for this
configuration variable are the following:

* `bf::bit_field_assignment_strategy::unchecked` -- This causes the code to assume that the value is correct without
  checking it or performing any masking. This is the most dangerous method, but also the most performant since it avoids
  needing to apply a mask to the value. If applied inappropriately, this strategy can set bits outside of the expected
  range and generally cause unexpected issues unless you're absolutely certain you've sanitized the values that will be
  used in the call to `set`.
* `bf::bit_field_assignment_strategy::mask` -- This is the default behavior. Silently ignores any bits set outside of
  the range in question.
* `bf::bit_field_assignment_strategy::return_bool` -- This strategy checks for bits set outside of the expected range
  before performing any modification. If there are bits set outside of the expected range then the call to `set` will
  return `false` and no modification will occur. If there are not bits set outside of the expected range then the value
  will be updated and the call to `set` will return `true`. When using this strategy the set function is marked as
  `[[nodiscard]]`, and the compiler will issue an error if the return value is ignored.
* `bf::bit_field_assignment_strategy::exception` -- This strategy checks for bits set outside of the expected range
  before performing any modification. If there are bits set outside of the expected range then the call to `set` will
  raise a `bf::bit_field_error` exception. If there are not bits set outside of the expected range then the value
  will be updated and the call will succeed without throwing an exception. This strategy is only available if exceptions
  are enabled by the compiler and exceptions are not explicitly disabled by defining `BIT_FIELD_NO_EXCEPTIONS`.

Here is an example of using the unchecked strategy with a good value:

```cpp
static_assert(
    []() constexpr {
        std::uint8_t value{0};
        bf::bit_field<3, 2, bf::bit_field_config{ .strategy = bf::bit_field_assignment_strategy::unchecked }>::set(value, 0b111);
        return value;
    }() == 0b11100
);
```

Since the value has no extraneous bits set, the above static assertion should work with _all_ strategies.

And now with a value that has an additional bit set that is not part of the range of interest:

```cpp
static_assert(
    []() constexpr {
        std::uint8_t value{0};
        bf::bit_field<3, 2, bf::bit_field_config{ .strategy = bf::bit_field_assignment_strategy::unchecked }>::set(value, 0b1111);
        return value;
    }() == 0b111100
);
```

Notice the return value has four bits set, but the bit field in question is only three bits. Clearly this can have
unintended consequences and should probably not be used unless you know for sure that the values used are correct.

Here is the same "bad" example, but using the `mask` strategy:

```cpp
static_assert(
    []() constexpr {
        std::uint8_t value{0};
        bf::bit_field<3, 2, bf::bit_field_config{ .strategy = bf::bit_field_assignment_strategy::mask }>::set(value, 0b1111);
        return value;
    }() == 0b11100
);
```

The additional bit is silently masked away. There is no indication that invalid bits were set in the source, but any
invalid bits set will be unable to affect the result.

The same assignment using the `return_bool` strategy will return `false`:

```cpp
static_assert(
    []() constexpr {
        std::uint8_t value{0};
        return bf::bit_field<3, 2, bf::bit_field_config{ .strategy = bf::bit_field_assignment_strategy::return_bool }>::set(value, 0b1111);
    }() == false
);
```

The same assignment using the `exception` strategy will throw an exception. This can't easily be checked at compile-time
since try/catch are not allowed in constexpr context. You can use the `exception` strategy in constexpr context, but if
an illegal access is performed you can't `catch` that exception, it will simply cause the compilation to fail.

```cpp
int main() {
    try {
        std::uint8_t value{0};
        bf::bit_field<3, 2, bf::bit_field_config{ .strategy = bf::bit_field_assignment_strategy::exception }>::set(value, 0b1111);
        return 1;
    } catch (bf::bit_field_error&) {
        return 0;
    }
}
```

If the above code is run, it should return zero.

Both the strategy and the offset provided to `bf::bit_field` can be overridden on individual calls to `set` in the same
way the overrides for the `get` call work.

### Additional Information

Each `bf::bit_field` type exposes some types and static constexpr values describing the bit field. Basically, all values
provided to the `bf::bit_field` template are exposed.

* `bits` -- The number of bits in the bit field. Static constexpr variable.
* `offset` -- Where the bit field starts, relative to the least significant bit. Static constexpr variable.
* `default_config` -- The default `bf::bit_field_config`. Static constexpr variable.

Here is an example of looking at an using those values:

```cpp
using test_field = bf::bit_field<3, 2, bf::bit_field_config<std::byte>{ .offset = 2, .strategy = bf::bit_field_assignment_strategy::exception }>;
static_assert(test_field::bits == 3);
static_assert(test_field::offset == 2);
static_assert(test_field::default_config.offset == 2);
static_assert(test_field::default_config.strategy == bf::bit_field_assignment_strategy::exception);
```

## bf::bit\_field\_builder

A `bf::bit_field_builder` base class is provided which can simplify the definition of collections of `bf::bit_field`
types, making the definitions as similar as possible to definitions for built-in C++ bit fields. This requires the use
of some code-generating macros, but macro use is kept as minimal as possible. (Note, the `bf::bit_field` does not use
any code generating macros. If you're trying to avoid their use, you can stick with that instead of using
`bf::bit_field_builder`).

First, to declare a new bit field collection, make a new class and derive from `bf::bit_field_builder`. The base class
has two required template arguments. The first is the name of the your class (see
[CRTP](https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern)). The second is the underlying storage type
for the bit field. The third optional template argument is a `bf::bit_field_config` representing the class-wide
defaults. Any configuration values not overridden will use the global default.

For example, the following creates a bit field builder on top of a `std::uint32_t`, using the default config:

```cpp
struct my_bit_field : bf::bit_field_builder<my_bit_field, std::uint32_t> {
};
```

This example specifies that by default each field should return a `std::byte` rather than a `std::uint32_t`. This
default can be overridden on a field-by-field basis.

```cpp
struct my_bit_field : bf::bit_field_builder<my_bit_field, std::uint32_t, bf::bit_field_config<std::byte>{}> {
};
```

The newly defined class or struct will have a `raw_value` member variable, whose type is the same as the second template
parameter's. In this case, the `raw_value` member will be a `std::uint32_t`. It is acceptable to specify the underlying
type as `volatile`. Using `volatile` can be useful when defining the type if the bitfield represents a hardware
register, and the bit field object is `reinterpret_cast`-ed over some arbitrary memory address.

Within the class definition the `BIT_FIELD` and `BIT_FIELD_PAD` code generating macros are used to define bit fields
within the `std::uint32_t` storage. The bit fields must be defined from least significant to most significant bit. As an
example, the following code declares three bit fields within the 32-bit integer. The first is the first five bits, the
second is the next two bits after that, and the third is the last three bits. The middle 22 bits are padding. Below is a
graphical representation of the bit field:

```
      LSB                          MSB
      00000000001111111111222222222233
index 01234567890123456789012345678901
      ================================
field 1111122pppppppppppppppppppppp333
```

```cpp
struct my_bit_field : bf::bit_field_builder<my_bit_field, std::uint32_t> {
    BIT_FIELD(field1, 5);
    BIT_FIELD(field2, 2, bf::bit_field_config<std::byte>{ .offset = bf::no_shift });
    BIT_FIELD_PAD(22);
    BIT_FIELD(field3, 3);
};
```

The first parameter to `BIT_FIELD` is the name of the field, and the second is the number of bits in the field. The third
parameter is optional, and if provided must be an instance of `bf::bit_field_config`. The third parameter is used to set
any field-level defaults which you wish to deviate from the class-level defaults. Note, you no longer have to specify
the starting offset. The offsets start at zero and continue up from there. No name is given for padding-only bits, so
only the number of bits is required for `BIT_FIELD_PAD`. Each `BIT_FIELD` macro invocation creates three new symbols in
the current scope. For example, the `BIT_FIELD` line for `field2` generates the following:

* `field2` -- This is a type, and is equivalent to `bf::bit_field<2, OFFSET, config>` where `OFFSET` is the current
              offset in the underlying storage type, and `config` is the optionally provided `bf::bit_field_config`
              merged with the class-level configuration which was also optionally provided. For `field2` the offset is
              `2` because `field1` consumes the first two bits.
* `get_field2` -- This is a member function which calls the underlying `field2::get` function with the `raw_value`
                  member variable. It takes the same template arguments as `field2::get` so that the field-level
                  defaults may be overridden on a call-by-call basis.
* `set_field2` -- This is a member function which calls the underlying `field2::set` function with the `raw_value`
                  member variable and the provided variable to set. It takes the same template arguments as
                  `field2::set` so that the field-level defaults may be overridden on a call-by-call basis.

The `bf::bit_field_builder` will not allow you to provision too many bits for the underlying storage type. For example,
adding a fourth single-bit field to the above definition fill fail to compile since it represents trying to allocate 33
bits from a 32 bit data type.

```cpp
// Should NOT compile.
struct my_bit_field : bf::bit_field_builder<my_bit_field, std::uint32_t> {
    BIT_FIELD(field1, 5);
    BIT_FIELD(field2, 2, bf::bit_field_config<std::byte>{ .offset = bf::no_shift });
    BIT_FIELD_PAD(22);
    BIT_FIELD(field3, 3);
    BIT_FIELD(field4, 1);
};
```

Given the compiling definition above, ignoring the member functions, the underlying `bf::bit_field` types can be used
like this:

```cpp
//                                         33222222222211111111110000000000
//                                         10987654321098765432109876543210
static_assert( my_bit_field::field1::get(0b00000000000000000000000000111111) == 0b11111 );
static_assert( my_bit_field::field2::get(0b00000000000000000000000011110000) == std::byte{0b01100000} );
static_assert( my_bit_field::field3::get(0b11110000000000000000000000000000) == 0b111 );
```

A function is provided to determine if all of the bits of the underlying storage have been allocated. This can be used
in a static assertion as follows:

```cpp
static_assert( my_bit_field::is_complete() );
```

Additionally, although you do not need to specify the offsets of each field, you could confirm the offset of each field
with a static assertion if desired:

```cpp
static_assert( my_bit_field::field1::bits   == 5);
static_assert( my_bit_field::field1::offset == 0);
static_assert( my_bit_field::field2::bits   == 2);
static_assert( my_bit_field::field2::offset == 5);
static_assert( my_bit_field::field3::bits   == 3);
static_assert( my_bit_field::field3::offset == 29);
```

The `::default_config` settings could also be static asserted if desired, as part of a compile-time test.

In addition to the bit field type definitions generated, the `bf::bit_field_builder` also allows you to declare
instances of your bit field type, and use the `get_field`/`set_field` functions on those instances. First, it can be
proven at compile-time that the `my_bit_field` type is exactly as large as the underlying type, which is `std::uint32_t`
in this case (the underlying type can be used at compile-time through the `value_type` member of your class):

```cpp
static_assert( std::is_same_v<my_bit_field::value_type, std::uint32_t> );
static_assert( sizeof(my_bit_field) == sizeof(my_bit_field::value_type) );
```

Getting field values from an instance is straightforward:

```cpp
constexpr my_bit_field instance{0b11100000000000000000000001111111};
static_assert( instance.get_field1() == 0b11111 );
static_assert( instance.get_field2() == std::byte{0b01100000} );
static_assert( instance.get_field3() == 0b111 );
```

Setting values is equally straightforward:

```cpp
static_assert(
    []() constexpr {
        my_bit_field instance{0};
        instance.set_field1(0b11111);
        return instance.raw_value;
    }() == 0b11111
);

static_assert(
    []() constexpr {
        my_bit_field instance{0};
        instance.set_field2(std::byte{0b1100000});
        return instance.raw_value;
    }() == 0b1100000
);

static_assert(
    []() constexpr {
        my_bit_field instance{0};
        instance.set_field3(0b111);
        return instance.raw_value;
    }() == 0b11100000000000000000000000000000
);

static_assert(
    []() constexpr {
        my_bit_field instance{0};
        instance.set_field1(0b11111);
        instance.set_field2(std::byte{0b1100000});
        instance.set_field3(0b111);
        return instance.raw_value;
    }() == 0b11100000000000000000000001111111
);
```

Note, as with `bf::bit_field`, fields created with `bf::bit_field_builder` can also have their assignment strategy
defaulted, and enum/scoped enum types can be used.

### Advanced Usage

If your bit field class is templated, then the `bit_field_builder` class will depend on those template parameters (since
it depends on the derived class because this library uses the curiously recurring template pattern). Because of this
dependency, dependent name lookups are required, and the normal code used to define the bit fields will not compile
because name lookups will fail. This case should be very rare, but two additional macros are provided to account for
this possibility -- `BIT_FIELD_PAD_DEP` and `BIT_FIELD_DEP`. They are the same as `BIT_FIELD_PAD` and `BIT_FIELD`
respectively, but each has an additional initial parameter which must be the "self" type name.

Take the following example, which does compile:

```cpp
struct my_bit_field : bf::bit_field_builder<my_bit_field, std::uint32_t> {
    BIT_FIELD(field1, 5);
    BIT_FIELD(field2, 2, bf::bit_field_config<std::byte>{ .offset = bf::no_shift });
    BIT_FIELD_PAD(22);
    BIT_FIELD(field3, 3);
};
```

Now, let's say we want to add a template parameter to `my_bit_field`, the naive implementation is below, but does not
compile:

```cpp
template <typename T>
struct my_bit_field : bf::bit_field_builder<my_bit_field<T>, std::uint32_t> {
    BIT_FIELD(field1, 5);
    BIT_FIELD(field2, 2, bf::bit_field_config<std::byte>{ .offset = bf::no_shift });
    BIT_FIELD_PAD(22);
    BIT_FIELD(field3, 3);
};
```

Changing to the following does compile and should work as expected:

```cpp
template <typename T>
struct my_bit_field : bf::bit_field_builder<my_bit_field<T>, std::uint32_t> {
    BIT_FIELD_DEP(my_bit_field::, field1, 5);
    BIT_FIELD_DEP(my_bit_field::, field2, 2, bf::bit_field_config<std::byte>{ .offset = bf::no_shift });
    BIT_FIELD_PAD_DEP(my_bit_field::, 22);
    BIT_FIELD_DEP(my_bit_field::, field3, 3);
};
```

### Comparison with Built-In Bit Fields

This works in GCC because GCC appears to lay out bit fields from least significant to most significant bit, but this
behavior is not dictated by the standard, so the following example may not work everywhere. Here we can see a comparison
between the syntax for built-in bit fields when compared to bit fields defined by this library. As can be seen, they are
quite similar, but there are some differences. Overall, declaring a bit field using this library is not appreciably more
code than defining a built-in bit field, but this library provides a standards-compliant way of laying out the bit field
in memory, and it allows for more advanced usage where desired.

```cpp
struct built_in_bit_field {
    std::uint8_t bit0 : 1;
    std::uint8_t bit1 : 1;
    std::uint8_t bit2 : 1;
    std::uint8_t bit3 : 1;
    std::uint8_t bit4 : 1;
    std::uint8_t bit5 : 1;
    std::uint8_t bit6 : 1;
    std::uint8_t bit7 : 1;
};

struct library_bit_field : bf::bit_field_builder<library_bit_field, std::uint8_t> {
    BIT_FIELD(bit0, 1);
    BIT_FIELD(bit1, 1);
    BIT_FIELD(bit2, 1);
    BIT_FIELD(bit3, 1);
    BIT_FIELD(bit4, 1);
    BIT_FIELD(bit5, 1);
    BIT_FIELD(bit6, 1);
    BIT_FIELD(bit7, 1);
};

constexpr built_in_bit_field built_in_bit_field_instance{1, 0, 1, 0, 1, 0, 1, 0};
constexpr library_bit_field library_bit_field_instance{0b01010101};

static_assert( built_in_bit_field_instance.bit0 == library_bit_field_instance.get_bit0() );
static_assert( built_in_bit_field_instance.bit1 == library_bit_field_instance.get_bit1() );
static_assert( built_in_bit_field_instance.bit2 == library_bit_field_instance.get_bit2() );
static_assert( built_in_bit_field_instance.bit3 == library_bit_field_instance.get_bit3() );
static_assert( built_in_bit_field_instance.bit4 == library_bit_field_instance.get_bit4() );
static_assert( built_in_bit_field_instance.bit5 == library_bit_field_instance.get_bit5() );
static_assert( built_in_bit_field_instance.bit6 == library_bit_field_instance.get_bit6() );
static_assert( built_in_bit_field_instance.bit7 == library_bit_field_instance.get_bit7() );
```

It would be very nice to have a memberwise assignment syntax like built-in bit fields, but dynamically generating the
constructor appears to be quite difficult.

# Build and Test

A `Makefile` is provided to build the single header file from the split header files, and to run the compile-time tests
that ensure the correctness of the library. See the comment header in `Makefile` for more information about the targets.

As an additional test, there exists `test/assembly.cpp` which has many manually crafted functions which are functionally
identical to the `bit_field` functions for several `bit_field_config` values. This file can be pasted into compiler
explorer to compare the generated assembly of the library to the generated assembly of the manually crafted functions to
ensure that the assembly generated by the library is as efficient as manually crafted code. The generated assembly for
some selected architectures and compilers is available in the `reports` directory.

Originally it was intended to put `/bit_field.hpp` in the `.gitignore` to not include the generated header file since it
can be built from the other headers, and the single header would be downloadable through the GitHub release page.
However, it appears that compiler explorer can't easily include a header file from the releases page, so we end up
committing the generated file to source control such that compiler explorer can easily include it. Care must be taken
not to allow the committed generated header file to fall out of sync with the other sources.
