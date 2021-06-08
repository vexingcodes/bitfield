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
