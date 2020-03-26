/**
 *  @file
 *  abi.h
 *
 *  @brief
 *  Contains all public type/function declarations for the ABI stable
 *  pure-C interface for the Dart library.
 *
 *  @details
 *  This file contains documentation for all publicly exported functions,
 *  but does not take time to explain general Dart library concepts.
 *  For better understanding, the general README.md file should be read
 *  before reading this documentation to familiarize yourself with library
 *  concepts.
 */

#ifndef DART_ABI_H
#define DART_ABI_H

/*----- System Includes -----*/

#include <stdlib.h>
#include <stdint.h>

/*----- Macros ------*/

// Get our declspec nonsense in place for windows.
#if defined(_MSC_VER) && defined(DART_EXPORTS)
#define DART_ABI_EXPORT __declspec(dllexport)
#elif defined(_MSC_VER)
#define DART_ABI_EXPORT __declspec(dllimport)
#else
#define DART_ABI_EXPORT
#endif

#define DART_BUFFER_MAX_SIZE      (1U << 5U)
#define DART_HEAP_MAX_SIZE        (1U << 6U)
#define DART_PACKET_MAX_SIZE      DART_HEAP_MAX_SIZE

// This is embarrassing.
// Dart iterators have big
// jobs, and I need two of them.
#define DART_ITERATOR_MAX_SIZE    (1U << 8U)

#define DART_FAILURE              (-1)

#define DART_CONCAT_IMPL(a, b) a##b
#define DART_CONCAT(a, b) DART_CONCAT_IMPL(a, b)

#ifdef __COUNTER__
#define DART_GEN_UNIQUE_NAME(base)                                                          \
  DART_CONCAT(base, __COUNTER__)

#define DART_FOR_EACH_IMPL(aggr, value, it_func, it_name, err_name)                         \
  dart_iterator_t it_name;                                                                  \
  dart_err_t err_name = it_func(&it_name, aggr);                                            \
  err_name = (err_name) ? err_name : dart_iterator_get_err(value, &it_name);                \
  for (; !dart_iterator_done_destroy(&it_name, value) && err_name == DART_NO_ERROR;         \
          err_name = (dart_iterator_next(&it_name),                                         \
                      dart_destroy(value),                                                  \
                      dart_iterator_get_err(value, &it_name)))

#define DART_FOR_EACH(aggr, value, it_func, it_name, err_name)                              \
  DART_FOR_EACH_IMPL(aggr, value, it_func, it_name, err_name)

/**
 *  @brief
 *  Macro allows for easy, automatically managed, iteration over the VALUES of a Dart
 *  aggregate (object or array). This is the preferred method of iteration if manual
 *  control of the iterator is not required.
 *
 *  @details
 *  Macro declares a for loop that automatically manages its resources and can be used in
 *  the following way:
 *  ```
 *  dart_packet_t val;
 *  dart_packet_t obj = dart_obj_init_va("sss", "hello", "world", "yes", "no", "stop", "go");
 *  dart_for_each(&obj, &val) {
 *    assert(dart_is_str(&val));
 *    printf("%s\n", dart_str_get(&val));
 *  }
 *  ```
 *  This will print:
 *  "no"
 *  "go"
 *  "world"
 */
#define dart_for_each(aggr, value)                                                          \
  DART_FOR_EACH(aggr, value, dart_iterator_init_from_err,                                   \
      DART_GEN_UNIQUE_NAME(__dart_iterator__), DART_GEN_UNIQUE_NAME(__dart_err__))

/**
 *  @brief
 *  Macro allows for easy, automatically managed, iteration over the KEYS of a Dart
 *  object. This is the preferred method of iteration if manual control of the iterator
 *  is not required.
 *
 *  @details
 *  Macro declares a for loop that automatically manages its resources and can be used in
 *  the following way:
 *  ```
 *  dart_packet_t key;
 *  dart_packet_t obj = dart_obj_init_va("sss", "hello", "world", "yes", "no", "stop", "go");
 *  dart_for_each_key(&obj, &key) {
 *    assert(dart_is_str(&key));
 *    printf("%s\n", dart_str_get(&key));
 *  }
 *  ```
 *  This will print:
 *  "yes"
 *  "stop"
 *  "hello"
 */
#define dart_for_each_key(aggr, value)                                                      \
  DART_FOR_EACH(aggr, value, dart_iterator_init_key_from_err,                               \
      DART_GEN_UNIQUE_NAME(__dart_iterator__), DART_GEN_UNIQUE_NAME(__dart_err__))
#endif

#ifdef __cplusplus
extern "C" {
#endif

  /*----- Public Type Declarations -----*/

  /**
   *  @brief
   *  Enum encodes the runtime type of a Dart type.
   */
  enum dart_type {
    DART_OBJECT = 1,
    DART_ARRAY,
    DART_STRING,
    DART_INTEGER,
    DART_DECIMAL,
    DART_BOOLEAN,
    DART_NULL,
    DART_INVALID
  };
  typedef enum dart_type dart_type_t;

  /**
   *  @brief
   *  Enum is used in the generic Dart API functions to
   *  encode which C++ API should be used internally.
   */
  enum dart_packet_type {
    DART_HEAP = 1,
    DART_BUFFER,
    DART_PACKET
  };
  typedef enum dart_packet_type dart_packet_type_t;

  /**
   *  @brief
   *  Enum is used in the generic Dart API functions to
   *  encode which underlying reference counter implementation
   *  is being used.
   *
   *  @details
   *  DART_RC_SAFE means that the Dart type should use a thread-safe
   *  reference counter implementation, DART_RC_UNSAFE means that the
   *  Dart type should use a thread-unsafe reference counter implementation.
   *
   *  @remarks
   *  A thread-unsafe reference counter makes copies cheaper, but also makes
   *  it much easier to break things accidentally.
   */
  enum dart_rc_type {
    DART_RC_SAFE = 1,
    DART_RC_UNSAFE
  };
  typedef enum dart_rc_type dart_rc_type_t;

  /**
   *  @brief
   *  Enum type is used to encode different classes of error
   *  conditions that can be returned by any of the Dart API
   *  functions. More detailed information can be acquired
   *  from errno or perror();
   */
  enum dart_err {
    DART_NO_ERROR = 0,
    DART_TYPE_ERROR,
    DART_LOGIC_ERROR,
    DART_STATE_ERROR,
    DART_PARSE_ERROR,
    DART_RUNTIME_ERROR,
    DART_CLIENT_ERROR,
    DART_UNKNOWN_ERROR
  };
  typedef enum dart_err dart_err_t;

  /**
   *  @brief
   *  Struct is used as a primitive form of RTTI for the Dart
   *  C API to encode what underlying C++ type must be manipulated.
   */
  struct dart_type_id {
    dart_packet_type_t p_id;
    dart_rc_type_t rc_id;
  };
  typedef struct dart_type_id dart_type_id_t;

  /**
   *  @brief
   *  Struct is used to encode iteration state while walking across
   *  a Dart aggregate type (object or array).
   *
   *  @remarks
   *  For the purposes of ABI stability, the definition of dart_iterator_t
   *  is entirely separate from all C++ Dart types, however, dart_iterator_t's
   *  internally hold a live C++ object, and therefore cannot simply be memcpy'd
   *  around. If an iterator must be copied, one of the dart_iterator_copy functions
   *  must be used.
   *  Treat these types as opaque handles to state that is managed FOR you by the
   *  public API functions.
   */
  struct dart_iterator {
    dart_type_id_t rtti;
    char bytes[DART_ITERATOR_MAX_SIZE];
  };
  typedef struct dart_iterator dart_iterator_t;

  /**
   *  @brief
   *  Struct is used to encode state for a mutable Dart type.
   *
   *  @details
   *  See documentation for dart::heap in the Doxygen docs, or the README.md file to get
   *  a better understanding of what this type is to be used for.
   *
   *  @remarks
   *  For the purposes of ABI stability, the definition of dart_heap_t
   *  is entirely separate from all C++ Dart types, however, dart_heap_t's
   *  internally hold a live C++ object, and therefore cannot simply be memcpy'd
   *  around. If a dart_heap_t must be copied, one of the dart_heap_copy functions
   *  must be used.
   *  Treat these types as opaque handles to state that is managed FOR you by the
   *  public API functions.
   */
  struct dart_heap {
    dart_type_id_t rtti;
    char bytes[DART_HEAP_MAX_SIZE];
  };
  typedef struct dart_heap dart_heap_t;

  /**
   *  @brief
   *  Struct is used to encode state for an immutable Dart type that is ready to be sent
   *  over the network.
   *
   *  @details
   *  See documentation for dart::buffer in the Doxygen docs, or the README.md file to get
   *  a better understanding of what this type is to be used for.
   *
   *  @remarks
   *  For the purposes of ABI stability, the definition of dart_buffer_t
   *  is entirely separate from all C++ Dart types, however, dart_buffer_t's
   *  internally hold a live C++ object, and therefore cannot simply be memcpy'd
   *  around. If a dart_buffer_t must be copied, one of the dart_buffer_copy functions
   *  must be used.
   *  Treat these types as opaque handles to state that is managed FOR you by the
   *  public API functions.
   */
  struct dart_buffer {
    dart_type_id_t rtti;
    char bytes[DART_BUFFER_MAX_SIZE];
  };
  typedef struct dart_buffer dart_buffer_t;

  /**
   *  @brief
   *  Struct is used to encode state for any type of Dart object, mutable or otherwise.
   *
   *  @details
   *  See documentation for dart::packet in the Doxygen docs, or the README.md file to get
   *  a better understanding of what this type is to be used for.
   *
   *  @remarks
   *  For the purposes of ABI stability, the definition of dart_packet_t
   *  is entirely separate from all C++ Dart types, however, dart_packet_t's
   *  internally hold a live C++ object, and therefore cannot simply be memcpy'd
   *  around. If a dart_packet_t must be copied, one of the dart_copy functions
   *  must be used.
   *  Treat these types as opaque handles to state that is managed FOR you by the
   *  public API functions.
   */
  struct dart_packet {
    dart_type_id_t rtti;
    char bytes[DART_PACKET_MAX_SIZE];
  };
  typedef struct dart_packet dart_packet_t;

  /**
   *  @brief
   *  Struct is used to export non-owning access to a string with explicit length
   *  information.
   */
  struct dart_string_view {
    char const* ptr;
    size_t len;
  };
  typedef struct dart_string_view dart_string_view_t;

  /*----- Public Function Declarations -----*/

  /*----- dart_heap Functions -----*/

  /*----- dart_heap Lifecycle Functions -----*/

  /**
   *  @brief
   *  Function is used to default-initialize a dart_heap_t instance to null.
   *
   *  @remarks
   *  Function cannot meaningfully fail, but has an error overload for API consistency.
   *
   *  @return
   *  The dart_heap instance initialized to null.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_init();

  /**
   *  @brief
   *  Function is used to default-initialize a dart_heap_instance to null.
   *
   *  @details
   *  Function expects to receive a pointer to uninitialized memory, and does not call
   *  dart_destroy before constructing the object. If you already have a live Dart object,
   *  you must pass it through one of the dart_destroy functions before calling this function.
   *
   *  @remarks
   *  Function cannot meaningfully fail, but has an error overload for API consistency.
   *
   *  @param[out] pkt
   *  The dart_heap_t instance to be initialized.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_init_err(dart_heap_t* pkt);

  /**
   *  @brief
   *  Function is used to default-initialize a dart_heap instance to null with an explicitly
   *  set reference counter type.
   *
   *  @remarks
   *  Function cannot meaningfully fail, but has an error overload for API consistency.
   *
   *  @param[in] rc
   *  The reference counter type the initialized Dart type should use.
   *
   *  @return
   *  The dart_heap instance initialized to null.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_init_rc(dart_rc_type_t rc);

  /**
   *  @brief
   *  Function is used to default-initialize a dart_heap instance to null with an explicitly
   *  set reference counter type.
   *
   *  @details
   *  Function expects to receive a pointer to uninitialized memory, and does not call
   *  dart_destroy before constructing the object. If you already have a live Dart object,
   *  you must pass it through one of the dart_destroy functions before calling this function.
   *
   *  @remarks
   *  Function cannot meaningfully fail, but has an error overload for API consistency.
   *
   *  @param[out] pkt
   *  The dart_heap_t instance to be initialized.
   *
   *  @param[in] rc
   *  The reference counter type the initialized Dart type should use.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_init_rc_err(dart_heap_t* pkt, dart_rc_type_t rc);

  /**
   *  @brief
   *  Function is used to copy-initialize a dart_heap instance to whatever value was provided.
   *
   *  @details
   *  Dart uses copy-on-write, so the copy is equivalent to a reference count increment.
   *
   *  @param[in] src
   *  Pointer to dart_heap_t instance to copy from.
   *
   *  @return
   *  The dart_heap instance initialized to null.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_copy(dart_heap_t const* src);

  /**
   *  @brief
   *  Function is used to copy-initialize a dart_heap instance to whatever value was provided.
   *
   *  @details
   *  Dart uses copy-on-write, so the copy is equivalent to a reference count increment.
   *
   *  @param[out] dst
   *  Pointer to uninitialized dart_heap_t instance to copy-initialize.
   *
   *  @param[in] src
   *  Pointer to dart_heap_t instance to copy from.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_copy_err(dart_heap_t* dst, dart_heap_t const* src);

  /**
   *  @brief
   *  Function is used to move-initialize a dart_heap instance to whatever value was provided.
   *  
   *  @details
   *  Operation relies on C++ move-semantics under the covers and "steals" the reference from
   *  the incoming object, resetting it to null.
   *
   *  @param[in,out] src
   *  Pointer to dart_heap_t instance to move from.
   *
   *  @return
   *  The dart_heap instance that was move-initialized.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_move(dart_heap_t* src);

  /**
   *  @brief
   *  Function is used to move-initialize a dart_heap instance to whatever value was provided.
   *
   *  @details
   *  Operation relies on C++ move-semantics under the covers and "steals" the reference from
   *  the incoming object, resetting it to null.
   *
   *  @param[out] dst
   *  Pointer to unitialized dart_heap_t instance to move-initialize.
   *
   *  @param[in,out] src
   *  Pointer to dart_heap_t instance to move from.
   *
   *  @return
   *  Whether anything went wrong during move-initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_move_err(dart_heap_t* dst, dart_heap_t* src);

  /**
   *  @brief
   *  Function is used to destroy a live dart_heap instance, releasing its reference count
   *  and any potentially exclusively owned resources.
   *
   *  @remarks
   *  Technically speaking, even a null dart_heap instance is a "live" object,
   *  and pedantically speaking, all live objects must be destroyed,
   *  so _all_ dart_heap instances must pass through this function.
   *  Practically speaking null dart_heap instances own _no_ resources,
   *  (imagine it as std::variant<std::monostate>), and so will not leak resources if not destroyed.
   *  What is done with this information is up to you.
   *
   *  @param[in,out] pkt
   *  The object to be destroyed.
   *
   *  @return
   *  Whether anything went wrong during destruction.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_destroy(dart_heap_t* pkt);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as an object.
   *
   *  @details
   *  Function can fail for any reason constructing std::map can fail.
   *  Will return a null packet if construction fails.
   *
   *  @return
   *  The dart_heap instance initialized as an object, or null on error.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_obj_init();

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as an object.
   *
   *  @details
   *  Function expects to receive a pointer to uninitialized memory, and does not call
   *  dart_destroy before constructing the object. If you already have a live Dart object,
   *  you must pass it through one of the dart_destroy functions before calling this function.
   *  Function can fail for any reason constructing std::map can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[out] pkt
   *  The dart_heap_t instance to be initialized.
   *
   *  @return
   *  Whether anything went wrong during destruction.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_init_err(dart_heap_t* pkt);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as an object with an explicitly
   *  set reference counter type.
   *
   *  @details
   *  Function can fail for any reason constructing std::map can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @return
   *  The dart_heap instance initialized as an object, or null on error.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_obj_init_rc(dart_rc_type_t rc);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as an object with an explicitly
   *  set reference counter type.
   *
   *  @details
   *  Function expects to receive a pointer to uninitialized memory, and does not call
   *  dart_destroy before constructing the object. If you already have a live Dart object,
   *  you must pass it through one of the dart_destroy functions before calling this function.
   *  Function can fail for any reason constructing std::map can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[out] pkt
   *  The packet to be initialized as an object.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @return
   *  Whether anything went wrong during destruction.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_init_rc_err(dart_heap_t* pkt, dart_rc_type_t rc);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as an object, according to the given
   *  format string.
   *
   *  @details
   *  Function uses an extremely simplistic DSL to encode the expected sequence of VALUES
   *  for the constructed object. Each character in the format string corresponds to a key-value
   *  PAIR in the incoming varargs argument list.
   *  Accepted characters:
   *    * o
   *      - Begin object. Adds a level of object nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * a
   *      - Begin array. Adds a level of array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * s
   *      - String argument. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a null-terminated string).
   *    * S
   *      - Sized string argument. Will consume three arguments, one for the key (always a string),
   *        one for the value (now assumed to be an unterminated string), and one for the length
   *        of the string.
   *    * ui
   *      - Unsigned integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned int).
   *    * ul
   *      - Unsigned long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned long).
   *    * i
   *      - Integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed integer).
   *    * l
   *      - Long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed long).
   *    * d
   *      - Decimal. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a double).
   *    * b
   *      - Boolean. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an int).
   *    * " "
   *    * n
   *      - A space means that no value, only a single key, has been provided, and the value
   *        should be initialized to null.
   *    * ,
   *      - End aggregate. Removes a level of object/array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *  Following code will build this object: `{"hello":"world","pi":3.14159}`
   *  ```
   *  dart_heap_obj_init_va("sd", "hello", "world", "pi", 3.14159);
   *  ```
   *  Function can fail for any reason constructing std::map can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[in] format
   *  The format string specifying the variadic arguments to expect.
   *
   *  @return
   *  The dart_heap instance initialized as an object, or null on error.
   *  Note that errors in the format string cannot be detected and will likely lead to crashes.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_obj_init_va(char const* format, ...);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as an object, according to the given
   *  format string.
   *
   *  @details
   *  Function uses an extremely simplistic DSL to encode the expected sequence of VALUES
   *  for the constructed object. Each character in the format string corresponds to a key-value
   *  PAIR in the incoming varargs argument list.
   *  Accepted characters:
   *    * o
   *      - Begin object. Adds a level of object nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * a
   *      - Begin array. Adds a level of array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * s
   *      - String argument. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a null-terminated string).
   *    * S
   *      - Sized string argument. Will consume three arguments, one for the key (always a string),
   *        one for the value (now assumed to be an unterminated string), and one for the length
   *        of the string.
   *    * ui
   *      - Unsigned integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned int).
   *    * ul
   *      - Unsigned long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned long).
   *    * i
   *      - Integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed integer).
   *    * l
   *      - Long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed long).
   *    * d
   *      - Decimal. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a double).
   *    * b
   *      - Boolean. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an int).
   *    * " "
   *    * n
   *      - A space means that no value, only a single key, has been provided, and the value
   *        should be initialized to null.
   *    * ,
   *      - End aggregate. Removes a level of object/array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *  Following code will build this object: `{"hello":"world","pi":3.14159}`
   *  ```
   *  dart_heap_obj_init_va("sd", "hello", "world", "pi", 3.14159);
   *  ```
   *  Function expects to receive a pointer to uninitialized memory, and does not call
   *  dart_destroy before constructing the object. If you already have a live Dart object,
   *  you must pass it through one of the dart_destroy functions before calling this function.
   *  Function can fail for any reason constructing std::map can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[out] pkt
   *  The dart_heap_t instance to be initialized.
   *
   *  @param[in] format
   *  The format string specifying the variadic arguments to expect.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   *  Note that errors in the format string cannot be detected and will likely lead to crashes.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_init_va_err(dart_heap_t* pkt, char const* format, ...);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as an object, according to the given
   *  format string, with an explicitly set reference counter type.
   *
   *  @details
   *  Function uses an extremely simplistic DSL to encode the expected sequence of VALUES
   *  for the constructed object. Each character in the format string corresponds to a key-value
   *  PAIR in the incoming varargs argument list.
   *  Accepted characters:
   *    * o
   *      - Begin object. Adds a level of object nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * a
   *      - Begin array. Adds a level of array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * s
   *      - String argument. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a null-terminated string).
   *    * S
   *      - Sized string argument. Will consume three arguments, one for the key (always a string),
   *        one for the value (now assumed to be an unterminated string), and one for the length
   *        of the string.
   *    * ui
   *      - Unsigned integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned int).
   *    * ul
   *      - Unsigned long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned long).
   *    * i
   *      - Integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed integer).
   *    * l
   *      - Long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed long).
   *    * d
   *      - Decimal. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a double).
   *    * b
   *      - Boolean. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an int).
   *    * " "
   *    * n
   *      - A space means that no value, only a single key, has been provided, and the value
   *        should be initialized to null.
   *    * ,
   *      - End aggregate. Removes a level of object/array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *  Following code will build this object: `{"hello":"world","pi":3.14159}`
   *  ```
   *  dart_heap_obj_init_va("sd", "hello", "world", "pi", 3.14159);
   *  ```
   *  Function can fail for any reason constructing std::map can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] format
   *  The format string specifying the variadic arguments to expect.
   *
   *  @return
   *  The dart_heap instance initialized as an object, or null on error.
   *  Note that errors in the format string cannot be detected and will likely lead to crashes.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_obj_init_va_rc(dart_rc_type_t rc, char const* format, ...);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as an object, according to the given
   *  format string, with an explicitly set reference counter type.
   *
   *  @details
   *  Function uses an extremely simplistic DSL to encode the expected sequence of VALUES
   *  for the constructed object. Each character in the format string corresponds to a key-value
   *  PAIR in the incoming varargs argument list.
   *  Accepted characters:
   *    * o
   *      - Begin object. Adds a level of object nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * a
   *      - Begin array. Adds a level of array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * s
   *      - String argument. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a null-terminated string).
   *    * S
   *      - Sized string argument. Will consume three arguments, one for the key (always a string),
   *        one for the value (now assumed to be an unterminated string), and one for the length
   *        of the string.
   *    * ui
   *      - Unsigned integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned int).
   *    * ul
   *      - Unsigned long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned long).
   *    * i
   *      - Integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed integer).
   *    * l
   *      - Long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed long).
   *    * d
   *      - Decimal. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a double).
   *    * b
   *      - Boolean. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an int).
   *    * " "
   *    * n
   *      - A space means that no value, only a single key, has been provided, and the value
   *        should be initialized to null.
   *    * ,
   *      - End aggregate. Removes a level of object/array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *  Following code will build this object: `{"hello":"world","pi":3.14159}`
   *  ```
   *  dart_heap_obj_init_va("sd", "hello", "world", "pi", 3.14159);
   *  ```
   *  Function expects to receive a pointer to uninitialized memory, and does not call
   *  dart_destroy before constructing the object. If you already have a live Dart object,
   *  you must pass it through one of the dart_destroy functions before calling this function.
   *  Function can fail for any reason constructing std::map can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[out] pkt
   *  The dart_heap_t instance to be initialized.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] format
   *  The format string specifying the variadic arguments to expect.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   *  Note that errors in the format string cannot be detected and will likely lead to crashes.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_init_va_rc_err(dart_heap_t* pkt, dart_rc_type_t rc, char const* format, ...);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as an array.
   *
   *  @details
   *  Function can fail for any reason constructing std::vector can fail.
   *  Will return a null packet if construction fails.
   *
   *  @return
   *  The dart_heap instance initialized as an object, or null on error.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_arr_init();

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as an array.
   *
   *  @details
   *  Function expects to receive a pointer to uninitialized memory, and does not call
   *  dart_destroy before constructing the object. If you already have a live Dart object,
   *  you must pass it through one of the dart_destroy functions before calling this function.
   *  Function can fail for any reason constructing std::vector can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[out] pkt
   *  The dart_heap_t instance to be initialized.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_arr_init_err(dart_heap_t* pkt);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as an array.
   *
   *  @details
   *  Function can fail for any reason constructing std::vector can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @return
   *  The dart_heap instance initialized as an object, or null on error.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_arr_init_rc(dart_rc_type_t rc);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as an array.
   *
   *  @details
   *  Function expects to receive a pointer to uninitialized memory, and does not call
   *  dart_destroy before constructing the object. If you already have a live Dart object,
   *  you must pass it through one of the dart_destroy functions before calling this function.
   *  Function can fail for any reason constructing std::vector can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[out] pkt
   *  The dart_heap_t instance to be initialized.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_arr_init_rc_err(dart_heap_t* pkt, dart_rc_type_t rc);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as an array, according to the given
   *  format string.
   *
   *  @details
   *  Function uses an extremely simplistic DSL to encode the expected sequence of VALUES
   *  for the constructed array. Each character in the format string corresponds to a value
   *  in the incoming varargs argument list.
   *  Accepted characters:
   *    * o
   *      - Begin object. Adds a level of object nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * a
   *      - Begin array. Adds a level of array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * s
   *      - String argument. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a null-terminated string).
   *    * S
   *      - Sized string argument. Will consume three arguments, one for the key (always a string),
   *        one for the value (now assumed to be an unterminated string), and one for the length
   *        of the string.
   *    * ui
   *      - Unsigned integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned int).
   *    * ul
   *      - Unsigned long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned long).
   *    * i
   *      - Integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed integer).
   *    * l
   *      - Long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed long).
   *    * d
   *      - Decimal. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a double).
   *    * b
   *      - Boolean. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an int).
   *    * " "
   *    * n
   *      - A space means that no value, only a single key, has been provided, and the value
   *        should be initialized to null.
   *    * ,
   *      - End aggregate. Removes a level of object/array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *  Following code will build this array: `[1, "two", 3.14159, null]`
   *  ```
   *  dart_heap_arr_init_va("isd ", 1, "two", 3.14159);
   *  ```
   *  Function can fail for any reason constructing std::vector can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[in] format
   *  The format string specifying the variadic arguments to expect.
   *
   *  @return
   *  The dart_heap instance initialized as an array, or null on error.
   *  Note that errors in the format string cannot be detected and will likely lead to crashes.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_arr_init_va(char const* format, ...);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as an array, according to the given
   *  format string.
   *
   *  @details
   *  Function uses an extremely simplistic DSL to encode the expected sequence of VALUES
   *  for the constructed array. Each character in the format string corresponds to a value
   *  in the incoming varargs argument list.
   *  Accepted characters:
   *    * o
   *      - Begin object. Adds a level of object nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * a
   *      - Begin array. Adds a level of array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * s
   *      - String argument. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a null-terminated string).
   *    * S
   *      - Sized string argument. Will consume three arguments, one for the key (always a string),
   *        one for the value (now assumed to be an unterminated string), and one for the length
   *        of the string.
   *    * ui
   *      - Unsigned integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned int).
   *    * ul
   *      - Unsigned long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned long).
   *    * i
   *      - Integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed integer).
   *    * l
   *      - Long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed long).
   *    * d
   *      - Decimal. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a double).
   *    * b
   *      - Boolean. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an int).
   *    * " "
   *    * n
   *      - A space means that no value, only a single key, has been provided, and the value
   *        should be initialized to null.
   *    * ,
   *      - End aggregate. Removes a level of object/array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *  Following code will build this array: `[1, "two", 3.14159, null]`
   *  ```
   *  dart_heap_arr_init_va("isd ", 1, "two", 3.14159);
   *  ```
   *  Function expects to receive a pointer to uninitialized memory, and does not call
   *  dart_destroy before constructing the object. If you already have a live Dart object,
   *  you must pass it through one of the dart_destroy functions before calling this function.
   *  Function can fail for any reason constructing std::vector can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[out] pkt
   *  The dart_heap_t instance to be initialized.
   *
   *  @param[in] format
   *  The format string specifying the variadic arguments to expect.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   *  Note that errors in the format string cannot be detected and will likely lead to crashes.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_arr_init_va_err(dart_heap_t* pkt, char const* format, ...);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as an array, according to the given
   *  format string, with an explicitly set reference counter type.
   *
   *  @details
   *  Function uses an extremely simplistic DSL to encode the expected sequence of VALUES
   *  for the constructed array. Each character in the format string corresponds to a value
   *  in the incoming varargs argument list.
   *  Accepted characters:
   *    * o
   *      - Begin object. Adds a level of object nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * a
   *      - Begin array. Adds a level of array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * s
   *      - String argument. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a null-terminated string).
   *    * S
   *      - Sized string argument. Will consume three arguments, one for the key (always a string),
   *        one for the value (now assumed to be an unterminated string), and one for the length
   *        of the string.
   *    * ui
   *      - Unsigned integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned int).
   *    * ul
   *      - Unsigned long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned long).
   *    * i
   *      - Integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed integer).
   *    * l
   *      - Long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed long).
   *    * d
   *      - Decimal. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a double).
   *    * b
   *      - Boolean. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an int).
   *    * " "
   *    * n
   *      - A space means that no value, only a single key, has been provided, and the value
   *        should be initialized to null.
   *    * ,
   *      - End aggregate. Removes a level of object/array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *  Following code will build this array: `[1, "two", 3.14159, null]`
   *  ```
   *  dart_heap_arr_init_va("isd ", 1, "two", 3.14159);
   *  ```
   *  Function can fail for any reason constructing std::vector can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] format
   *  The format string specifying the variadic arguments to expect.
   *
   *  @return
   *  The dart_heap instance initialized as an array, or null on error.
   *  Note that errors in the format string cannot be detected and will likely lead to crashes.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_arr_init_va_rc(dart_rc_type_t rc, char const* format, ...);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as an array, according to the given
   *  format string, with an explicitly set reference counter type.
   *
   *  @details
   *  Function uses an extremely simplistic DSL to encode the expected sequence of VALUES
   *  for the constructed array. Each character in the format string corresponds to a value
   *  in the incoming varargs argument list.
   *  Accepted characters:
   *    * o
   *      - Begin object. Adds a level of object nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * a
   *      - Begin array. Adds a level of array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * s
   *      - String argument. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a null-terminated string).
   *    * S
   *      - Sized string argument. Will consume three arguments, one for the key (always a string),
   *        one for the value (now assumed to be an unterminated string), and one for the length
   *        of the string.
   *    * ui
   *      - Unsigned integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned int).
   *    * ul
   *      - Unsigned long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned long).
   *    * i
   *      - Integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed integer).
   *    * l
   *      - Long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed long).
   *    * d
   *      - Decimal. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a double).
   *    * b
   *      - Boolean. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an int).
   *    * " "
   *    * n
   *      - A space means that no value, only a single key, has been provided, and the value
   *        should be initialized to null.
   *    * ,
   *      - End aggregate. Removes a level of object/array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *  Following code will build this array: `[1, "two", 3.14159, null]`
   *  ```
   *  dart_heap_arr_init_va("isd ", 1, "two", 3.14159);
   *  ```
   *  Function expects to receive a pointer to uninitialized memory, and does not call
   *  dart_destroy before constructing the object. If you already have a live Dart object,
   *  you must pass it through one of the dart_destroy functions before calling this function.
   *  Function can fail for any reason constructing std::vector can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[out] pkt
   *  The dart_heap_t instance to be initialized.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] format
   *  The format string specifying the variadic arguments to expect.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   *  Note that errors in the format string cannot be detected and will likely lead to crashes.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_arr_init_va_rc_err(dart_heap_t* pkt, dart_rc_type_t rc, char const* format, ...);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as a string.
   *
   *  @details
   *  If the provided string is less than or equal to the small string optimization size,
   *  function cannot fail, otherwise function can fail due to memory allocation failure.
   *
   *  @param[in] str
   *  The null-terminated string to copy into the dart_heap instance.
   *
   *  @return
   *  The dart_heap instance initialized as a string, or null on error.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_str_init(char const* str);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as a string.
   *
   *  @details
   *  If the provided string is less than or equal to the small string optimization size,
   *  function cannot fail, otherwise function can fail due to memory allocation failure.
   *
   *  @param[out] pkt
   *  The dart_heap_t instance to be initialized.
   *
   *  @param[in] str
   *  The null-terminated string to copy into the dart_heap instance.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_str_init_err(dart_heap_t* pkt, char const* str);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as a string with an explicit size.
   *
   *  @details
   *  If the provided string is less than or equal to the small string optimization size,
   *  function cannot fail, otherwise function can fail due to memory allocation failure.
   *  Function is useful when given string is not known to be terminated, or is otherwise
   *  untrusted.
   *
   *  @param[in] str
   *  The null-terminated string to copy into the dart_heap instance.
   *
   *  @param[in] len
   *  The length of the string in bytes.
   *
   *  @return
   *  Th dart_heap instance initialized as a string, or null on error.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_str_init_len(char const* str, size_t len);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as a string with an explicit size.
   *
   *  @details
   *  If the provided string is less than or equal to the small string optimization size,
   *  function cannot fail, otherwise function can fail due to memory allocation failure.
   *  Function is useful when given string is not known to be terminated, or is otherwise
   *  untrusted.
   *
   *  @param[out] pkt
   *  The dart_heap_t instance to be initialized.
   *
   *  @param[in] str
   *  The null-terminated string to copy into the dart_heap instance.
   *
   *  @param[in] len
   *  The length of the string in bytes.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_str_init_len_err(dart_heap_t* pkt, char const* str, size_t len);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as a string with an explicit
   *  reference counter implementation.
   *
   *  @details
   *  If the provided string is less than or equal to the small string optimization size,
   *  function cannot fail, otherwise function can fail due to memory allocation failure.
   *  Function is useful when given string is not known to be terminated, or is otherwise
   *  untrusted.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] str
   *  The null-terminated string to copy into the dart_heap instance.
   *
   *  @return
   *  Th dart_heap instance initialized as a string, or null on error.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_str_init_rc(dart_rc_type_t rc, char const* str);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as a string with an explicit
   *  reference counter implementation.
   *
   *  @details
   *  If the provided string is less than or equal to the small string optimization size,
   *  function cannot fail, otherwise function can fail due to memory allocation failure.
   *  Function is useful when given string is not known to be terminated, or is otherwise
   *  untrusted.
   *
   *  @param[out] pkt
   *  The dart_heap_t instance to be initialized.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] str
   *  The null-terminated string to copy into the dart_heap instance.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_str_init_rc_err(dart_heap_t* pkt, dart_rc_type_t rc, char const* str);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as a string with an explicit size
   *  and reference counter implementation.
   *
   *  @details
   *  If the provided string is less than or equal to the small string optimization size,
   *  function cannot fail, otherwise function can fail due to memory allocation failure.
   *  Function is useful when given string is not known to be terminated, or is otherwise
   *  untrusted.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] str
   *  The null-terminated string to copy into the dart_heap instance.
   *
   *  @param[in] len
   *  The length of the string in bytes.
   *
   *  @return
   *  Th dart_heap instance initialized as a string, or null on error.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_str_init_rc_len(dart_rc_type_t rc, char const* str, size_t len);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as a string with an explicit size
   *  and reference counter implementation.
   *
   *  @details
   *  If the provided string is less than or equal to the small string optimization size,
   *  function cannot fail, otherwise function can fail due to memory allocation failure.
   *  Function is useful when given string is not known to be terminated, or is otherwise
   *  untrusted.
   *
   *  @param[out] pkt
   *  The dart_heap_t instance to be initialized.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] str
   *  The null-terminated string to copy into the dart_heap instance.
   *
   *  @param[in] len
   *  The length of the string in bytes.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_str_init_rc_len_err(dart_heap_t* pkt, dart_rc_type_t rc, char const* str, size_t len);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as an integer.
   *
   *  @details
   *  Function cannot meaningfully fail.
   *
   *  @param[in] val
   *  The integer value to copy into the dart_heap instance.
   *
   *  @return
   *  The dart_heap instance initialized as an integer, or null on error.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_int_init(int64_t val);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as an integer.
   *
   *  @details
   *  Function cannot meaningfully fail. Function exists for API uniformity.
   *
   *  @param[out] pkt
   *  The dart_heap_t instance to be initialized.
   *
   *  @param[in] val
   *  The integer value to copy into the dart_heap instance.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_int_init_err(dart_heap_t* pkt, int64_t val);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as an integer with an
   *  explicitly set reference counter type.
   *
   *  @details
   *  Function cannot meaningfully fail.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] val
   *  The integer value to copy into the dart_heap instance.
   *
   *  @return
   *  The dart_heap instance initialized as an integer, or null on error.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_int_init_rc(dart_rc_type_t rc, int64_t val);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as an integer.
   *
   *  @details
   *  Function cannot meaningfully fail. Function exists for API uniformity.
   *
   *  @param[out] pkt
   *  The dart_heap_t instance to be initialized.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] val
   *  The integer value to copy into the dart_heap instance.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_int_init_rc_err(dart_heap_t* pkt, dart_rc_type_t rc, int64_t val);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as a decimal.
   *
   *  @details
   *  Function cannot meaningfully fail.
   *
   *  @param[in] val
   *  The double value to copy into the dart_heap instance.
   *
   *  @return
   *  The dart_heap instance initialized as a decimal, or null on error.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_dcm_init(double val);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as a decimal.
   *
   *  @details
   *  Function cannot meaningfully fail. Function exists for API uniformity.
   *
   *  @param[out] pkt
   *  The dart_heap_t instance to be initialized.
   *
   *  @param[in] val
   *  The double value to copy into the dart_heap instance.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_dcm_init_err(dart_heap_t* pkt, double val);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as a decimal with an
   *  explicitly set reference counter type.
   *
   *  @details
   *  Function cannot meaningfully fail.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] val
   *  The double value to copy into the dart_heap instance.
   *
   *  @return
   *  The dart_heap instance initialized as a decimal, or null on error.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_dcm_init_rc(dart_rc_type_t rc, double val);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as a decimal with an
   *  explicitly set reference counter type.
   *
   *  @details
   *  Function cannot meaningfully fail. Function exists for API uniformity.
   *
   *  @param[out] pkt
   *  The dart_heap_t instance to be initialized.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] val
   *  The double value to copy into the dart_heap instance.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_dcm_init_rc_err(dart_heap_t* pkt, dart_rc_type_t rc, double val);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as a boolean.
   *
   *  @details
   *  Function cannot meaningfully fail.
   *
   *  @param[in] val
   *  The boolean value to copy into the dart_heap instance.
   *
   *  @return
   *  The dart_heap instance initialized as a boolean, or null on error.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_bool_init(int val);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as a boolean.
   *
   *  @details
   *  Function cannot meaningfully fail. Function exists for API uniformity.
   *
   *  @param[out] pkt
   *  The dart_heap_t instance to be initialized.
   *
   *  @param[in] val
   *  The boolean value to copy into the dart_heap instance.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_bool_init_err(dart_heap_t* pkt, int val);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as a boolean with
   *  an explicitly set reference counter type.
   *
   *  @details
   *  Function cannot meaningfully fail.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] val
   *  The boolean value to copy into the dart_heap instance.
   *
   *  @return
   *  The dart_heap instance initialized as a boolean, or null on error.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_bool_init_rc(dart_rc_type_t rc, int val);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as a boolean with
   *  an explicitly set reference counter type.
   *
   *  @details
   *  Function cannot meaningfully fail. Function exists for API uniformity.
   *
   *  @param[out] pkt
   *  The dart_heap_t instance to be initialized.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] val
   *  The boolean value to copy into the dart_heap instance.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_bool_init_rc_err(dart_heap_t* pkt, dart_rc_type_t rc, int val);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as null.
   *
   *  @details
   *  Function cannot meaningfully fail.
   *
   *  @return
   *  The dart_heap instance initialized as null.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_null_init();

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as null.
   *
   *  @details
   *  Function cannot meaningfully fail. Function exists for API uniformity.
   *
   *  @return
   *  DART_NO_ERROR.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_null_init_err(dart_heap_t* pkt);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as null with
   *  an explicitly set reference counter type.
   *
   *  @details
   *  Function cannot meaningfully fail.
   *
   *  @return
   *  The dart_heap instance initialized as null.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_null_init_rc(dart_rc_type_t rc);

  /**
   *  @brief
   *  Function is used to construct a dart_heap_t instance as null with an
   *  explicitly set reference counter type.
   *
   *  @details
   *  Function cannot meaningfully fail. Function exists for API uniformity.
   *
   *  @return
   *  DART_NO_ERROR.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_null_init_rc_err(dart_heap_t* pkt, dart_rc_type_t rc);

  /*----- dart_heap Mutation Operations -----*/

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given null-terminated key and previously constructed dart_heap_t value.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in] val
   *  The previously initialized dart_heap_t instance (of any type) to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_insert_heap(dart_heap_t* pkt, char const* key, dart_heap_t const* val);

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given, possibly unterminated, key and previously constructed dart_heap_t value.
   *
   *  @details
   *  Function is behaviorally identical to dart_heap_obj_insert_heap, but can be used when the
   *  incoming key is either not known to be terminated or is otherwise untrusted.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The potentially unterminated string to use as a key for this key-value pair.
   *
   *  @param[in] len
   *  The length of the key to be inserted.
   *
   *  @param[in,out] val
   *  The previously initialized dart_heap_t instance (of any type) to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_insert_heap_len(dart_heap_t* pkt, char const* key, size_t len, dart_heap_t const* val);

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given null-terminated key and previously constructed dart_heap_t value.
   *
   *  @details
   *  Function takes ownership of ("steals") the resources referenced by val according
   *  to C++ move-semantics, potentially avoiding a reference increment at the cost of
   *  resetting val to null.
   *
   *  @remarks
   *  The post-conditions of this function are that the resources referenced by val will
   *  have been inserted into pkt, and the instance val will have been reset to null, as
   *  if it had been destroyed and then default constructed.
   *  Formally speaking, val is still a live object and must be destroyed, but it is
   *  guaranteed not to leak resources if not passed through dart_destroy.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in,out] val
   *  The previously initialized dart_heap_t instance to take as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_insert_take_heap(dart_heap_t* pkt, char const* key, dart_heap_t* val);

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given, possibly unterminated, key and previously constructed dart_heap_t value.
   *
   *  @details
   *  Function is behaviorally identical to dart_heap_obj_insert_take_heap, but can be used
   *  when the incoming key is either not known to be terminated or is otherwise untrusted.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in] len
   *  The length of the key to be inserted.
   *
   *  @param[in,out] val
   *  The previously initialized dart_heap_t instance to take as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_insert_take_heap_len(dart_heap_t* pkt, char const* key, size_t len, dart_heap_t* val);

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given pair of null-terminated strings.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in] val
   *  The null-terminated string to use as a value for this key-value pair.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_insert_str(dart_heap_t* pkt, char const* key, char const* val);

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given pair of, possibly unterminated, strings.
   *
   *  @details
   *  Function is behaviorally identical to dart_heap_obj_insert_str, but can be used
   *  when the incoming pair of strings is either not known to be terminated or is
   *  otherwise untrusted.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The potentially unterminated string to use as a key for this key-value pair.
   *
   *  @param[in] val
   *  The potentially unterminated string to use as a value for this key-value pair.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_insert_str_len(dart_heap_t* pkt, char const* key, size_t len, char const* val, size_t val_len);

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given null-terminated key and integer value.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in] val
   *  The integer to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_insert_int(dart_heap_t* pkt, char const* key, int64_t val);

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given, potentially unterminated, key and integer value.
   *
   *  @details
   *  Function is behaviorally identical to dart_heap_obj_insert_int, but can be used
   *  when the incoming key is either not known to be terminated or is otherwise untrusted.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The potentially unterminated string to use as a key for this key-value pair.
   *
   *  @param[in] len
   *  The length of the key to be inserted.
   *
   *  @param[in] val
   *  The integer to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_insert_int_len(dart_heap_t* pkt, char const* key, size_t len, int64_t val);

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given null-terminated key and decimal value.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in] val
   *  The decimal to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_insert_dcm(dart_heap_t* pkt, char const* key, double val);

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given, potentially unterminated, key and decimal value.
   *
   *  @details
   *  Function is behaviorally identical to dart_heap_obj_insert_dcm, but can be used when the
   *  incoming key is either not known to be terminated or is otherwise untrusted.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The, potentially unterminated, string to use as a key for this key-value pair.
   *
   *  @param[in] len
   *  The length of the key to be inserted.
   *
   *  @param[in] val
   *  The decimal to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_insert_dcm_len(dart_heap_t* pkt, char const* key, size_t len, double val);

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given null-terminated key and boolean value.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in] val
   *  The boolean to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_insert_bool(dart_heap_t* pkt, char const* key, int val);

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given, potentially unterminated, key and boolean value.
   *
   *  @details
   *  Function is behaviorally identical to dart_heap_obj_insert_bool, but can be used when the
   *  incoming key is either not known to be terminated or is otherwise untrusted.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The potentially unterminated string to use as a key for this key-value pair.
   *
   *  @param[in] len
   *  The length of the key to be inserted.
   *
   *  @param[in] val
   *  The boolean to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_insert_bool_len(dart_heap_t* pkt, char const* key, size_t len, int val);

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given null-terminated key and null value.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_insert_null(dart_heap_t* pkt, char const* key);

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given, potentially unterminated, key and null value.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The potentially unterminated string to use as a key for this key-value pair.
   *
   *  @param[in] len
   *  The length of the key to be inserted.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_insert_null_len(dart_heap_t* pkt, char const* key, size_t len);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given null-terminated key and previously constructed dart_heap_t value.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in] val
   *  The previously initialized dart_heap_t instance (of any type) to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_set_heap(dart_heap_t* pkt, char const* key, dart_heap_t const* val);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given, possibly unterminated, key and previously constructed dart_heap_t value.
   *
   *  @details
   *  Function is behaviorally identical to dart_heap_obj_set_heap, but can be used when the
   *  incoming key is either not known to be terminated or is otherwise untrusted.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The potentially unterminated string to use as a key for this key-value pair.
   *
   *  @param[in] len
   *  The length of the key to be inserted.
   *
   *  @param[in,out] val
   *  The previously initialized dart_heap_t instance (of any type) to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_set_heap_len(dart_heap_t* pkt, char const* key, size_t len, dart_heap_t const* val);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given null-terminated key and previously constructed dart_heap_t value.
   *
   *  @details
   *  Function takes ownership of ("steals") the resources referenced by val according
   *  to C++ move-semantics, potentially avoiding a reference increment at the cost of
   *  resetting val to null.
   *
   *  @remarks
   *  The post-conditions of this function are that the resources referenced by val will
   *  have been inserted into pkt, and the instance val will have been reset to null, as
   *  if it had been destroyed and then default constructed.
   *  Formally speaking, val is still a live object and must be destroyed, but it is
   *  guaranteed not to leak resources if not passed through dart_destroy.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in,out] val
   *  The previously initialized dart_heap_t instance to take as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_set_take_heap(dart_heap_t* pkt, char const* key, dart_heap_t* val);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given, possibly unterminated, key and previously constructed dart_heap_t value.
   *
   *  @details
   *  Function is behaviorally identical to dart_heap_obj_set_take_heap, but can be used
   *  when the incoming key is either not known to be terminated or is otherwise untrusted.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in] len
   *  The length of the key to be inserted.
   *
   *  @param[in,out] val
   *  The previously initialized dart_heap_t instance to take as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_set_take_heap_len(dart_heap_t* pkt, char const* key, size_t len, dart_heap_t* val);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given pair of null-terminated strings.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in] val
   *  The null-terminated string to use as a value for this key-value pair.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_set_str(dart_heap_t* pkt, char const* key, char const* val);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given pair of, possibly unterminated, strings.
   *
   *  @details
   *  Function is behaviorally identical to dart_heap_obj_set_str, but can be used
   *  when the incoming pair of strings is either not known to be terminated or is
   *  otherwise untrusted.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The potentially unterminated string to use as a key for this key-value pair.
   *
   *  @param[in] val
   *  The potentially unterminated string to use as a value for this key-value pair.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_set_str_len(dart_heap_t* pkt, char const* key, size_t len, char const* val, size_t val_len);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given null-terminated key and integer value.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in] val
   *  The integer to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_set_int(dart_heap_t* pkt, char const* key, int64_t val);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given, potentially unterminated, key and integer value.
   *
   *  @details
   *  Function is behaviorally identical to dart_heap_obj_set_int, but can be used
   *  when the incoming key is either not known to be terminated or is otherwise untrusted.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The potentially unterminated string to use as a key for this key-value pair.
   *
   *  @param[in] len
   *  The length of the key to be inserted.
   *
   *  @param[in] val
   *  The integer to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_set_int_len(dart_heap_t* pkt, char const* key, size_t len, int64_t val);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given null-terminated key and decimal value.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in] val
   *  The decimal to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_set_dcm(dart_heap_t* pkt, char const* key, double val);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given, potentially unterminated, key and decimal value.
   *
   *  @details
   *  Function is behaviorally identical to dart_heap_obj_set_dcm, but can be used when the
   *  incoming key is either not known to be terminated or is otherwise untrusted.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The, potentially unterminated, string to use as a key for this key-value pair.
   *
   *  @param[in] len
   *  The length of the key to be inserted.
   *
   *  @param[in] val
   *  The decimal to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_set_dcm_len(dart_heap_t* pkt, char const* key, size_t len, double val);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given null-terminated key and boolean value.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in] val
   *  The boolean to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_set_bool(dart_heap_t* pkt, char const* key, int val);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given, potentially unterminated, key and boolean value.
   *
   *  @details
   *  Function is behaviorally identical to dart_heap_obj_set_bool, but can be used when the
   *  incoming key is either not known to be terminated or is otherwise untrusted.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The potentially unterminated string to use as a key for this key-value pair.
   *
   *  @param[in] len
   *  The length of the key to be inserted.
   *
   *  @param[in] val
   *  The boolean to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_set_bool_len(dart_heap_t* pkt, char const* key, size_t len, int val);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given null-terminated key and null value.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_set_null(dart_heap_t* pkt, char const* key);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given, potentially unterminated, key and null value.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The potentially unterminated string to use as a key for this key-value pair.
   *
   *  @param[in] len
   *  The length of the key to be inserted.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_set_null_len(dart_heap_t* pkt, char const* key, size_t len);

  /**
   *  @brief
   *  Function is used to clear an existing object of any/all key-value pairs.
   *
   *  @param[in] pkt
   *  The object to clear.
   *
   *  @return
   *  Whether anything went wrong during the clear.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_clear(dart_heap_t* pkt);

  /**
   *  @brief
   *  Function is used to remove an individual key-value mapping from the given object.
   *
   *  @param[in] pkt
   *  The object to remove from.
   *
   *  @param[in] key
   *  The null-terminated string to use for removal.
   *
   *  @return
   *  Whether anything went wrong during the removal.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_erase(dart_heap_t* pkt, char const* key);

  /**
   *  @brief
   *  Function is used to remove an individual key-value mapping from the given object.
   *
   *  @param[in] pkt
   *  The object to remove from.
   *
   *  @param[in] key
   *  The potentially unterminated string to use for removal.
   *
   *  @param[in] len
   *  The length of the key to be removed.
   *
   *  @return
   *  Whether anything went wrong during the removal.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_erase_len(dart_heap_t* pkt, char const* key, size_t len);

  /**
   *  @brief
   *  Function is used to insert a new value within the given array at the specified index,
   *  using a previously initialized dart_heap_t instance.
   *
   *  @param[in] pkt
   *  The array to insert into
   *
   *  @param[in] idx
   *  The index to insert at.
   *
   *  @param[in] val
   *  The previously initialized dart_heap_t instance to insert.
   *
   *  @return
   *  Whether anything went wrong during the insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_arr_insert_heap(dart_heap_t* pkt, size_t idx, dart_heap_t const* val);

  /**
   *  @brief
   *  Function is used to insert a new value within the given array at the specified index,
   *  using a previously initialized dart_heap_t instance.
   *
   *  @details
   *  Function takes ownership of ("steals") the resources referenced by val according
   *  to C++ move-semantics, potentially avoiding a reference increment at the cost of
   *  resetting val to null.
   *
   *  @param[in] pkt
   *  The array to insert into
   *
   *  @param[in] idx
   *  The index to insert at.
   *
   *  @param[in,out] val
   *  The previously initialized dart_heap_t instance to take as a value.
   *
   *  @return
   *  Whether anything went wrong during the insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_arr_insert_take_heap(dart_heap_t* pkt, size_t idx, dart_heap_t* val);

  /**
   *  @brief
   *  Function is used to insert the given null-terminated string within the given array
   *  at the specified index.
   *
   *  @param[in] pkt
   *  The array to insert into
   *
   *  @param[in] idx
   *  The index to insert at
   *
   *  @param[in] val
   *  The string to insert into the array
   *
   *  @return
   *  Whether anything went wrong during the insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_arr_insert_str(dart_heap_t* pkt, size_t idx, char const* val);

  /**
   *  @brief
   *  Function is used to insert the given, potentially unterminated, string within the given array
   *  at the specified index.
   *
   *  @details
   *  Function is behaviorally identical to dart_heap_arr_insert_str, but can be used when the
   *  incoming string is either not known to be terminated or is otherwise untrusted.
   *
   *  @param[in] pkt
   *  The array to insert into
   *
   *  @param[in] idx
   *  The index to insert at
   *
   *  @param[in] val
   *  The string to insert into the array
   *
   *  @param[in] val_len
   *  The length of the string to insert into the array.
   *
   *  @return
   *  Whether anything went wrong during the insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_arr_insert_str_len(dart_heap_t* pkt, size_t idx, char const* val, size_t val_len);

  /**
   *  @brief
   *  Function is used to insert the given integer within the given array, at the specified index.
   *
   *  @param[in] pkt
   *  The array to insert into
   *
   *  @param[in] idx
   *  The index to insert at
   *
   *  @param[in] val
   *  The integer to insert.
   *
   *  @return
   *  Whether anything went wrong during the insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_arr_insert_int(dart_heap_t* pkt, size_t idx, int64_t val);

  /**
   *  @brief
   *  Function is used to insert the given decimal within the given array, at the specified index.
   *
   *  @param[in] pkt
   *  The array to insert into
   *
   *  @param[in] idx
   *  The index to insert at
   *
   *  @param[in] val
   *  The decimal to insert.
   *
   *  @return
   *  Whether anything went wrong during the insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_arr_insert_dcm(dart_heap_t* pkt, size_t idx, double val);

  /**
   *  @brief
   *  Function is used to insert the given boolean within the given array, at the specified index.
   *
   *  @param[in] pkt
   *  The array to insert into
   *
   *  @param[in] idx
   *  The index to insert at
   *
   *  @param[in] val
   *  The boolean to insert.
   *
   *  @return
   *  Whether anything went wrong during the insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_arr_insert_bool(dart_heap_t* pkt, size_t idx, int val);

  /**
   *  @brief
   *  Function is used to insert null within the given array, at the specified index.
   *
   *  @param[in] pkt
   *  The array to insert into
   *
   *  @param[in] idx
   *  The index to insert at
   *
   *  @return
   *  Whether anything went wrong during the insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_arr_insert_null(dart_heap_t* pkt, size_t idx);

  /**
   *  @brief
   *  Function is used to set an existing index within the given array to a
   *  previously initialized dart_heap_t instance.
   *
   *  @param[in] pkt
   *  The array to assign into
   *
   *  @param[in] idx
   *  The index to assign to.
   *
   *  @param[in] val
   *  The previously initialized dart_heap_t instance
   *
   *  @return
   *  Whether anything went wrong during the assignment.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_arr_set_heap(dart_heap_t* pkt, size_t idx, dart_heap_t const* val);

  /**
   *  @brief
   *  Function is used to set an existing index within the given array to a
   *  previously initialized dart_heap_t instance.
   *
   *  @details
   *  Function takes ownership of ("steals") the resources referenced by val according
   *  to C++ move-semantics, potentially avoiding a reference increment at the cost of
   *  resetting val to null.
   *
   *  @param[in] pkt
   *  The array to assign into
   *
   *  @param[in] idx
   *  The index to assign to.
   *
   *  @param[in,out] val
   *  The previously initialized dart_heap_t instance to take as a value.
   *
   *  @return
   *  Whether anything went wrong during the assignment.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_arr_set_take_heap(dart_heap_t* pkt, size_t idx, dart_heap_t* val);

  /**
   *  @brief
   *  Function is used to set an existing index within the given array to the given
   *  null-terminated string.
   *
   *  @param[in] pkt
   *  The array to assign into
   *
   *  @param[in] idx
   *  The index to assign to
   *
   *  @param[in] val
   *  The string to set in the array
   *
   *  @return
   *  Whether anything went wrong during the assignment.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_arr_set_str(dart_heap_t* pkt, size_t idx, char const* val);

  /**
   *  @brief
   *  Function is used to set an existing index within the given array to the given,
   *  potentially unterminated, string.
   *
   *  @details
   *  Function is behaviorally identical to dart_heap_arr_set_str, but can be used when the
   *  incoming string is either not known to be terminated or is otherwise untrusted.
   *
   *  @param[in] pkt
   *  The array to assign into
   *
   *  @param[in] idx
   *  The index to assign to
   *
   *  @param[in] val
   *  The string to set in the array
   *
   *  @param[in] val_len
   *  The length of the string to set in the array.
   *
   *  @return
   *  Whether anything went wrong during the assignment.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_arr_set_str_len(dart_heap_t* pkt, size_t idx, char const* val, size_t val_len);

  /**
   *  @brief
   *  Function is used to set an existing index with the given array to the given integer
   *
   *  @param[in] pkt
   *  The array to assign into
   *
   *  @param[in] idx
   *  The index to assign to
   *
   *  @param[in] val
   *  The integer to assign.
   *
   *  @return
   *  Whether anything went wrong during the assignment.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_arr_set_int(dart_heap_t* pkt, size_t idx, int64_t val);

  /**
   *  @brief
   *  Function is used to set an existing index within the given array to the given decimal
   *
   *  @param[in] pkt
   *  The array to assign into
   *
   *  @param[in] idx
   *  The index to assign to
   *
   *  @param[in] val
   *  The decimal to assign.
   *
   *  @return
   *  Whether anything went wrong during the assignment.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_arr_set_dcm(dart_heap_t* pkt, size_t idx, double val);

  /**
   *  @brief
   *  Function is used to set an existing index within the given array to the given boolean
   *
   *  @param[in] pkt
   *  The array to assign into
   *
   *  @param[in] idx
   *  The index to assign to
   *
   *  @param[in] val
   *  The boolean to assign.
   *
   *  @return
   *  Whether anything went wrong during the assignment.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_arr_set_bool(dart_heap_t* pkt, size_t idx, int val);

  /**
   *  @brief
   *  Function is used to assign null to an existing index within the given array
   *
   *  @param[in] pkt
   *  The array to assign into
   *
   *  @param[in] idx
   *  The index to assign to
   *
   *  @return
   *  Whether anything went wrong during the assignment.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_arr_set_null(dart_heap_t* pkt, size_t idx);

  /**
   *  @brief
   *  Function is used to clear an existing array of all values.
   *
   *  @param[in] pkt
   *  The array to clear.
   *
   *  @return
   *  Whether anything went wrong during the clear.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_arr_clear(dart_heap_t* pkt);

  /**
   *  @brief
   *  Function is used to "remove" an individual index from the given array,
   *  shifting all higher indices down.
   *
   *  @param[in] pkt
   *  The array to remove from.
   *
   *  @param[in] idx
   *  The index to "remove."
   *
   *  @return
   *  Whether anything went wrong during the removal.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_arr_erase(dart_heap_t* pkt, size_t idx);

  /**
   *  @brief
   *  Function is used to resize the array to the given length, dropping any indexes
   *  off the end if shrinking the array, and initializing any new indexes to null
   *  if growing the array.
   *
   *  @param[in] dst
   *  The array to resize
   *
   *  @param[in] len
   *  The new intended length of the array.
   *
   *  @return
   *  Whether anything went wrong during the resize.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_arr_resize(dart_heap_t* dst, size_t len);

  /**
   *  @brief
   *  Function is used to increase the size of the underlying storage medium
   *  of the given array, without changing the number of elements it logically
   *  contains.
   *
   *  @details
   *  Function can be useful to ensure a particular call to push_back or the like
   *  will be constant time.
   *
   *  @param[in] pkt
   *  The array to reserve space for.
   *
   *  @param[in] len
   *  The desired length of the underlying storage medium.
   *
   *  @return
   *  Whether anything went wrong during the reserve operation.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_arr_reserve(dart_heap_t* dst, size_t len);

  /*----- dart_heap Retrieval Operations -----*/

  /**
   *  @brief
   *  Function is used to check whether a key exists in a given object.
   *
   *  @remarks
   *  Can be useful to check whether a key is present without having to incur
   *  reference count overhead. Also can be useful to distinguish between an explicit
   *  null in an object and a key which is actually missing.
   *
   *  @param[in] src
   *  The object to query.
   *
   *  @param[in] key
   *  The key whose existence should be queried
   *
   *  @return
   *  Whether the given key is present.
   */
  DART_ABI_EXPORT int dart_heap_obj_has_key(dart_heap_t const* src, char const* key);

  /**
   *  @brief
   *  Function is used to check whether a key exists in a given object.
   *
   *  @details
   *  Function is behaviorally identical to dart_heap_obj_has_key, but can be used when the
   *  incoming string is either not known to be terminated or is otherwise untrusted.
   *
   *  @remarks
   *  Can be useful to check whether a key is present without having to incur
   *  reference count overhead. Also can be useful to distinguish between an explicit
   *  null in an object and a key which is actually missing.
   *
   *  @param[in] src
   *  The object to query.
   *
   *  @param[in] key
   *  The key whose existence should be queried
   *
   *  @param[in] len
   *  The length of the key in bytes.
   *
   *  @return
   *  Whether the given key is present.
   */
  DART_ABI_EXPORT int dart_heap_obj_has_key_len(dart_heap_t const* src, char const* key, size_t len);

  /**
   *  @brief
   *  Function is used to retrieve the value for a given null-terminated key from a given object.
   *
   *  @details
   *  Function returns null instances for non-existent keys without modifying the object.
   *  Lookup is based on std::map and should be log(N).
   *
   *  @param[in] src
   *  The source object to query from.
   *
   *  @param[in] key
   *  The null-terminated key to locate within the given object.
   *
   *  @return
   *  The dart_heap_t instance corresponding to the given key, or null in error.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_obj_get(dart_heap_t const* src, char const* key);

  /**
   *  @brief
   *  Function is used to retrieve the value for a given null-terminated key from a given object.
   *
   *  @details
   *  Function returns null instances for non-existent keys without modifying the object.
   *  Lookup is based on std::map and should be log(N).
   *  Function expects to received uninitialized memory. If using a pointer to a previous Dart
   *  instance, it must be passed through an appropriate dart_destroy function first.
   *
   *  @param[out] dst
   *  The dart_heap_t instance that should be initialized with the result of the lookup.
   *
   *  @param[in] src
   *  The object instance to query from.
   *
   *  @param[in] key
   *  The null-terminated key to locate within the given object.
   *
   *  @return
   *  Whether anything went wrong during the lookup.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_get_err(dart_heap_t* dst, dart_heap_t const* src, char const* key);

  /**
   *  @brief
   *  Function is used to retrieve the value for a given, possibly unterminated, key from a given object.
   *
   *  @details
   *  Function returns null instances for non-existent keys without modifying the object.
   *  Lookup is based on std::map and should be log(N).
   *
   *  @param[in] src
   *  The source object to query from.
   *
   *  @param[in] key
   *  The possibly unterminated key to locate within the given object.
   *
   *  @return
   *  The dart_heap_t instance corresponding to the given key, or null in error.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_obj_get_len(dart_heap_t const* src, char const* key, size_t len);

  /**
   *  @brief
   *  Function is used to retrieve the value for a given, possibly unterminated, key from a given object.
   *
   *  @details
   *  Function returns null instances for non-existent keys without modifying the object.
   *  Lookup is based on std::map and should be log(N).
   *  Function expects to received uninitialized memory. If using a pointer to a previous Dart
   *  instance, it must be passed through an appropriate dart_destroy function first.
   *
   *  @param[out] dst
   *  The dart_heap_t instance that should be initialized with the result of the lookup.
   *
   *  @param[in] src
   *  The object instance to query from.
   *
   *  @param[in] key
   *  The possibly unterminated key to locate within the given object.
   *
   *  @return
   *  Whether anything went wrong during the lookup.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_obj_get_len_err(dart_heap_t* dst, dart_heap_t const* src, char const* key, size_t len);

  /**
   *  @brief
   *  Function is used to retrieve the value for a given index within a given array.
   *
   *  @details
   *  Function returns null instances for non-existent indices without modifying the array.
   *
   *  @remarks
   *  Returning null from an out-of-bounds access is a potentially questionable move, but
   *  it was done to be in better behavioral conformance with object lookup, and also
   *  to behave more similarly to std::vector by not throwing exceptions (although also
   *  not causing ub).
   *
   *  @param[in] src
   *  The array to lookup into.
   *
   *  @param[in] idx
   *  The index to lookup.
   *
   *  @return
   *  The dart_heap_t instance corresponding to the given index, or null in error.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_arr_get(dart_heap_t const* src, size_t idx);

  /**
   *  @brief
   *  Function is used to retrieve the value for a given index within a given array.
   *
   *  @details
   *  Function returns null instances for non-existent indices without modifying the array.
   *
   *  @remarks
   *  Returning null from an out-of-bounds access is a potentially questionable move, but
   *  it was done to be in better behavioral conformance with object lookup, and also
   *  to behave more similarly to std::vector by not throwing exceptions (although also
   *  not causing ub).
   *
   *  @param[out] dst
   *  The dart_heap_t instance that should be initialized with the result of this lookup.
   *
   *  @param[in] src
   *  The array to lookup into.
   *
   *  @param[in] idx
   *  The index to lookup.
   *
   *  @return
   *  Whether anything went wrong during the lookup.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_arr_get_err(dart_heap_t* dst, dart_heap_t const* src, size_t idx);

  /**
   *  @brief
   *  Unwraps a given dart_heap string instance.
   *
   *  @details
   *  String is gauranteed to be terminated, but may contain additional nulls.
   *  Use the sized overload of this function to simultaneously get the actual length.
   *
   *  @param[in] src
   *  The string instance to unwrap.
   *
   *  @return
   *  A pointer to the character data for the given string, or NULL on error.
   */
  DART_ABI_EXPORT char const* dart_heap_str_get(dart_heap_t const* src);

  /**
   *  @brief
   *  Unwraps a given dart_heap string instance.
   *
   *  @details
   *  String is gauranteed to be terminated, but may contain additional nulls.
   *  For gauranteed correctness in the face of non-ASCII data, use the provided
   *  length out parameter.
   *
   *  @param[in] src
   *  The string instance to unwrap.
   *
   *  @param[out] len
   *  The length of the unwrapped string.
   *
   *  @return
   *  A pointer to the character data for the given string, or NULL on error.
   */
  DART_ABI_EXPORT char const* dart_heap_str_get_len(dart_heap_t const* src, size_t* len);

  /**
   *  @brief
   *  Unwraps a given dart_heap integer instance.
   *
   *  @details
   *  Function returns zero on error, but could also return zero on success.
   *  If the type of the passed instance is not known to be integer, use the error overload
   *  to retrieve an error code.
   *
   *  @param[in] src
   *  The integer instance that should be unwrapped.
   *
   *  @return
   *  The integer value of the given instance.
   */
  DART_ABI_EXPORT int64_t dart_heap_int_get(dart_heap_t const* src);

  /**
   *  @brief
   *  Unwraps a given dart_heap integer instance.
   *
   *  @param[in] src
   *  The integer instance that should be unwrapped.
   *
   *  @param[out] val
   *  A pointer to the integer that should be initialized to the unwrapped value.
   *
   *  @return
   *  Whether anything went wrong during the unwrap.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_int_get_err(dart_heap_t const* src, int64_t* val);

  /**
   *  @brief
   *  Unwraps a given dart_heap decimal instance.
   *
   *  @details
   *  Function returns zero on error, but could also return zero on success.
   *  If the type of the passed instance is not known to be decimal, use the error overload
   *  to retrieve an error code.
   *
   *  @param[in] src
   *  The decimal instance that should be unwrapped.
   *
   *  @return
   *  The decimal value of the given instance.
   */
  DART_ABI_EXPORT double dart_heap_dcm_get(dart_heap_t const* src);

  /**
   *  @brief
   *  Unwraps a given dart_heap decimal instance.
   *
   *  @param[in] src
   *  The decimal instance that should be unwrapped.
   *
   *  @param[out] val
   *  A pointer to the decimal that should be initialized to the unwrapped value.
   *
   *  @return
   *  Whether anything went wrong during the unwrap.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_dcm_get_err(dart_heap_t const* src, double* val);

  /**
   *  @brief
   *  Unwraps a given dart_heap boolean instance.
   *
   *  @details
   *  Function returns zero on error, but could also return zero on success.
   *  If the type of the passed instance is not known to be boolean, use the error overload
   *  to retrieve an error code.
   *
   *  @param[in] src
   *  The boolean instance that should be unwrapped.
   *
   *  @return
   *  The boolean value of the given instance.
   */
  DART_ABI_EXPORT int dart_heap_bool_get(dart_heap_t const* src);

  /**
   *  @brief
   *  Unwraps a given dart_heap boolean instance.
   *
   *  @param[in] src
   *  The boolean instance that should be unwrapped.
   *
   *  @param[out] val
   *  A pointer to the boolean that should be initialized to the unwrapped value.
   *
   *  @return
   *  Whether anything went wrong during the unwrap.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_bool_get_err(dart_heap_t const* src, int* val);

  /**
   *  @brief
   *  Function returns the size of a Dart aggregate (object or array) or string
   *  instance.
   *
   *  @param[in] src
   *  The instance whose length should be queried.
   *
   *  @return
   *  The size of the given aggregate/string.
   */
  DART_ABI_EXPORT size_t dart_heap_size(dart_heap_t const* src);

  /**
   *  @brief
   *  Function recursively calculates equality for the given instances.
   *
   *  @details
   *  Disparate types always compare unequal, disparate reference counters always
   *  compare unequal, same types are recursively compared.
   *
   *  @param[in] lhs
   *  The "left hand side" of the expression.
   *
   *  @param[in] rhs
   *  The "right hand side" of the expression.
   *
   *  @return
   *  Whether the two instances were equivalent.
   */
  DART_ABI_EXPORT int dart_heap_equal(dart_heap_t const* lhs, dart_heap_t const* rhs);

  /**
   *  @brief
   *  Function checks whether the given instance is of object type.
   */
  DART_ABI_EXPORT int dart_heap_is_obj(dart_heap_t const* src);

  /**
   *  @brief
   *  Function checks whether the given instance is of array type.
   */
  DART_ABI_EXPORT int dart_heap_is_arr(dart_heap_t const* src);

  /**
   *  @brief
   *  Function checks whether the given instance is of string type.
   */
  DART_ABI_EXPORT int dart_heap_is_str(dart_heap_t const* src);

  /**
   *  @brief
   *  Function checks whether the given instance is of integer type.
   */
  DART_ABI_EXPORT int dart_heap_is_int(dart_heap_t const* src);

  /**
   *  @brief
   *  Function checks whether the given instance is of decimal type.
   */
  DART_ABI_EXPORT int dart_heap_is_dcm(dart_heap_t const* src);

  /**
   *  @brief
   *  Function checks whether the given instance is of boolean type.
   */
  DART_ABI_EXPORT int dart_heap_is_bool(dart_heap_t const* src);

  /**
   *  @brief
   *  Function checks whether the given instance is null.
   */
  DART_ABI_EXPORT int dart_heap_is_null(dart_heap_t const* src);

  /**
   *  @brief
   *  Returns the type of the given instance.
   */
  DART_ABI_EXPORT dart_type_t dart_heap_get_type(dart_heap_t const* src);

  /*----- dart_heap JSON Manipulation Functions -----*/

  /**
   *  @brief
   *  Function parses a given null-terminated JSON string and returns
   *  a handle to a Dart object hierarchy representing the given string.
   *
   *  @param[in] str
   *  A null-terminated JSON string.
   *
   *  @return
   *  The dart_heap_t instance representing the string.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_from_json(char const* str);

  /**
   *  @brief
   *  Function parses a given null-terminated JSON string and initializes
   *  a handle to a Dart object hierarchy representing the given string.
   *
   *  @param[out] pkt
   *  The packet to initialize as a handle to the parsed representation.
   *
   *  @param[in] str
   *  A null-terminated JSON string.
   *
   *  @return
   *  Whether anything went wrong during parsing.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_from_json_err(dart_heap_t* pkt, char const* str);

  /**
   *  @brief
   *  Function parses a given null-terminated JSON string and returns
   *  a handle to a Dart object hierarchy representing the given string, and using
   *  a specific reference counter type.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] str
   *  A null-terminated JSON string.
   *
   *  @return
   *  The dart_heap_t instance representing the string.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_from_json_rc(dart_rc_type_t rc, char const* str);

  /**
   *  @brief
   *  Function parses a given null-terminated JSON string and initializes
   *  a handle to a Dart object hierarchy representing the given string, and using
   *  a specific reference counter type.
   *
   *  @param[out] pkt
   *  The packet to initialize as a handle to the parsed representation.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] str
   *  A null-terminated JSON string.
   *
   *  @return
   *  Whether anything went wrong during parsing.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_from_json_rc_err(dart_heap_t* pkt, dart_rc_type_t rc, char const* str);

  /**
   *  @brief
   *  Function parses a given, possibly unterminated, JSON string and returns
   *  a handle to a Dart object hierarchy representing the given string.
   *
   *  @param[in] str
   *  A possibly unterminated JSON string.
   *
   *  @param[in] len
   *  The length of the given JSON string.
   *
   *  @return
   *  The dart_heap_t instance representing the string.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_from_json_len(char const* str, size_t len);

  /**
   *  @brief
   *  Function parses a given, possibly unterminated, JSON string and initializes
   *  a handle to a Dart object hierarchy representing the given string.
   *
   *  @param[out] pkt
   *  The packet to initialize as a handle to the parsed representation.
   *
   *  @param[in] str
   *  A possibly unterminated JSON string.
   *
   *  @param[in] len
   *  The length of the given JSON string.
   *
   *  @return
   *  Whether anything went wrong during parsing.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_from_json_len_err(dart_heap_t* pkt, char const* str, size_t len);

  /**
   *  @brief
   *  Function parses a given, possibly unterminated, JSON string and returns
   *  a handle to a Dart object hierarchy representing the given string, and using
   *  a specific reference counter type.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] str
   *  A possibly unterminated JSON string.
   *
   *  @param[in] len
   *  The length of the given JSON string.
   *
   *  @return
   *  The dart_heap_t instance representing the string.
   */
  DART_ABI_EXPORT dart_heap_t dart_heap_from_json_len_rc(dart_rc_type_t rc, char const* str, size_t len);

  /**
   *  @brief
   *  Function parses a given, possibly unterminated, JSON string and initializes
   *  a handle to a Dart object hierarchy representing the given string, and using
   *  a specific reference counter type.
   *
   *  @param[out] pkt
   *  The packet to initialize as a handle to the parsed representation.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] str
   *  A possibly unterminated JSON string.
   *
   *  @param[in] len
   *  The length of the given JSON string.
   *
   *  @return
   *  Whether anything went wrong during parsing.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_from_json_len_rc_err(dart_heap_t* pkt, dart_rc_type_t rc, char const* str, size_t len);

  /**
   *  @brief
   *  Function stringifies a given dart_heap_t instance into a valid JSON string.
   *
   *  @param[in] pkt
   *  The packet to stringify
   *
   *  @param[out] len
   *  The length of the resulting string.
   *
   *  @return
   *  The stringified result of the given dart_heap_t instance. Allocated with malloc, must be freed.
   */
  DART_ABI_EXPORT char* dart_heap_to_json(dart_heap_t const* pkt, size_t* len);

  /*----- dart_heap API Transition Functions -----*/

  /**
   *  @brief
   *  Function will create a flat, serialized, network-ready, representation of the given dynamic
   *  object hierarchy.
   *
   *  @details
   *  Function serves as a go-between for the mutable and immutable, dynamic and network-ready,
   *  representations Dart uses.
   *
   *  @param[in] pkt
   *  The dynamic instance to be serialized.
   *
   *  @return
   *  The serialized, immutable, representation of the given instance, or null on error.
   */
  DART_ABI_EXPORT dart_buffer_t dart_heap_lower(dart_heap_t const* pkt);

  /**
   *  @brief
   *  Function will create a flat, serialized, network-ready, representation of the given dynamic
   *  object hierarchy.
   *
   *  @details
   *  Function serves as a go-between for the mutable and immutable, dynamic and network-ready,
   *  representations Dart uses.
   *
   *  @param[out] dst
   *  The dart_buffer_t instance to be initialized.
   *
   *  @param[in] pkt
   *  The dynamic instance to be serialized.
   *
   *  @return
   *  The serialized, immutable, representation of the given instance, or null on error.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_lower_err(dart_buffer_t* dst, dart_heap_t const* pkt);

  /**
   *  @brief
   *  Function will create a flat, serialized, network-ready, representation of the given dynamic
   *  object hierarchy.
   *
   *  @details
   *  Function serves as a go-between for the mutable and immutable, dynamic and network-ready,
   *  representations Dart uses.
   *
   *  @param[in] pkt
   *  The dynamic instance to be serialized.
   *
   *  @return
   *  The serialized, immutable, representation of the given instance, or null on error.
   */
  DART_ABI_EXPORT dart_buffer_t dart_heap_finalize(dart_heap_t const* pkt);

  /**
   *  @brief
   *  Function will create a flat, serialized, network-ready, representation of the given dynamic
   *  object hierarchy.
   *
   *  @details
   *  Function serves as a go-between for the mutable and immutable, dynamic and network-ready,
   *  representations Dart uses.
   *
   *  @param[out] dst
   *  The dart_buffer_t instance to be initialized.
   *
   *  @param[in] pkt
   *  The dynamic instance to be serialized.
   *
   *  @return
   *  The serialized, immutable, representation of the given instance, or null on error.
   */
  DART_ABI_EXPORT dart_err_t dart_heap_finalize_err(dart_buffer_t* dst, dart_heap_t const* pkt);

  /*----- dart_buffer Lifecycle Functions -----*/

  /**
   *  @brief
   *  Function is used to default-initialize a dart_buffer_t instance to null.
   *
   *  @remarks
   *  Function cannot meaningfully fail, but has an error overload for API consistency.
   *
   *  @return
   *  The dart_buffer instance initialized to null.
   */
  DART_ABI_EXPORT dart_buffer_t dart_buffer_init();

  /**
   *  @brief
   *  Function is used to default-initialize a dart_buffer_instance to null.
   *
   *  @details
   *  Function expects to receive a pointer to uninitialized memory, and does not call
   *  dart_destroy before constructing the object. If you already have a live Dart object,
   *  you must pass it through one of the dart_destroy functions before calling this function.
   *
   *  @remarks
   *  Function cannot meaningfully fail, but has an error overload for API consistency.
   *
   *  @param[out] pkt
   *  The dart_buffer_t instance to be initialized.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_buffer_init_err(dart_buffer_t* pkt);

  /**
   *  @brief
   *  Function is used to default-initialize a dart_buffer instance to null with an explicitly
   *  set reference counter type.
   *
   *  @remarks
   *  Function cannot meaningfully fail, but has an error overload for API consistency.
   *
   *  @param[in] rc
   *  The reference counter type the initialized Dart type should use.
   *
   *  @return
   *  The dart_buffer instance initialized to null.
   */
  DART_ABI_EXPORT dart_buffer_t dart_buffer_init_rc(dart_rc_type_t rc);

  /**
   *  @brief
   *  Function is used to default-initialize a dart_buffer instance to null with an explicitly
   *  set reference counter type.
   *
   *  @details
   *  Function expects to receive a pointer to uninitialized memory, and does not call
   *  dart_destroy before constructing the object. If you already have a live Dart object,
   *  you must pass it through one of the dart_destroy functions before calling this function.
   *
   *  @remarks
   *  Function cannot meaningfully fail, but has an error overload for API consistency.
   *
   *  @param[out] pkt
   *  The dart_buffer_t instance to be initialized.
   *
   *  @param[in] rc
   *  The reference counter type the initialized Dart type should use.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_buffer_init_rc_err(dart_buffer_t* pkt, dart_rc_type_t rc);

  /**
   *  @brief
   *  Function is used to copy-initialize a dart_buffer instance to whatever value was provided.
   *
   *  @details
   *  Dart uses copy-on-write, so the copy is equivalent to a reference count increment.
   *
   *  @param[in] src
   *  Pointer to dart_buffer_t instance to copy from.
   *
   *  @return
   *  The dart_buffer instance initialized to null.
   */
  DART_ABI_EXPORT dart_buffer_t dart_buffer_copy(dart_buffer_t const* src);

  /**
   *  @brief
   *  Function is used to copy-initialize a dart_buffer instance to whatever value was provided.
   *
   *  @details
   *  Dart uses copy-on-write, so the copy is equivalent to a reference count increment.
   *
   *  @param[out] dst
   *  Pointer to uninitialized dart_buffer_t instance to copy-initialize.
   *
   *  @param[in] src
   *  Pointer to dart_buffer_t instance to copy from.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_buffer_copy_err(dart_buffer_t* dst, dart_buffer_t const* src);

  /**
   *  @brief
   *  Function is used to move-initialize a dart_buffer instance to whatever value was provided.
   *  
   *  @details
   *  Operation relies on C++ move-semantics under the covers and "steals" the reference from
   *  the incoming object, resetting it to null.
   *
   *  @param[in,out] src
   *  Pointer to dart_buffer_t instance to move from.
   *
   *  @return
   *  The dart_buffer instance that was move-initialized.
   */
  DART_ABI_EXPORT dart_buffer_t dart_buffer_move(dart_buffer_t* src);

  /**
   *  @brief
   *  Function is used to move-initialize a dart_buffer instance to whatever value was provided.
   *
   *  @details
   *  Operation relies on C++ move-semantics under the covers and "steals" the reference from
   *  the incoming object, resetting it to null.
   *
   *  @param[out] dst
   *  Pointer to unitialized dart_buffer_t instance to move-initialize.
   *
   *  @param[in,out] src
   *  Pointer to dart_buffer_t instance to move from.
   *
   *  @return
   *  Whether anything went wrong during move-initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_buffer_move_err(dart_buffer_t* dst, dart_buffer_t* src);

  /**
   *  @brief
   *  Function is used to destroy a live dart_buffer instance, releasing its reference count
   *  and any potentially exclusively owned resources.
   *
   *  @remarks
   *  Technically speaking, even a null dart_buffer instance is a "live" object,
   *  and pedantically speaking, all live objects must be destroyed,
   *  so _all_ dart_buffer instances must pass through this function.
   *  Practically speaking null dart_buffer instances own _no_ resources,
   *  (imagine it as std::variant<std::monostate>), and so will not leak resources if not destroyed.
   *  What is done with this information is up to you.
   *
   *  @param[in,out] pkt
   *  The object to be destroyed.
   *
   *  @return
   *  Whether anything went wrong during destruction.
   */
  DART_ABI_EXPORT dart_err_t dart_buffer_destroy(dart_buffer_t* pkt);

  /*----- dart_buffer Retrieval Operations -----*/

  /**
   *  @brief
   *  Function is used to check whether a key exists in a given object.
   *
   *  @remarks
   *  Can be useful to check whether a key is present without having to incur
   *  reference count overhead. Also can be useful to distinguish between an explicit
   *  null in an object and a key which is actually missing.
   *
   *  @param[in] src
   *  The object to query.
   *
   *  @param[in] key
   *  The key whose existence should be queried
   *
   *  @return
   *  Whether the given key is present.
   */
  DART_ABI_EXPORT int dart_buffer_obj_has_key(dart_buffer_t const* src, char const* key);

  /**
   *  @brief
   *  Function is used to check whether a key exists in a given object.
   *
   *  @details
   *  Function is behaviorally identical to dart_buffer_obj_has_key, but can be used when the
   *  incoming string is either not known to be terminated or is otherwise untrusted.
   *
   *  @remarks
   *  Can be useful to check whether a key is present without having to incur
   *  reference count overhead. Also can be useful to distinguish between an explicit
   *  null in an object and a key which is actually missing.
   *
   *  @param[in] src
   *  The object to query.
   *
   *  @param[in] key
   *  The key whose existence should be queried
   *
   *  @param[in] len
   *  The length of the key in bytes.
   *
   *  @return
   *  Whether the given key is present.
   */
  DART_ABI_EXPORT int dart_buffer_obj_has_key_len(dart_buffer_t const* src, char const* key, size_t len);

  /**
   *  @brief
   *  Function is used to retrieve the value for a given null-terminated key from a given object.
   *
   *  @details
   *  Function returns null instances for non-existent keys without modifying the object.
   *  Lookup is based on std::map and should be log(N).
   *
   *  @param[in] src
   *  The source object to query from.
   *
   *  @param[in] key
   *  The null-terminated key to locate within the given object.
   *
   *  @return
   *  The dart_buffer_t instance corresponding to the given key, or null in error.
   */
  DART_ABI_EXPORT dart_buffer_t dart_buffer_obj_get(dart_buffer_t const* src, char const* key);

  /**
   *  @brief
   *  Function is used to retrieve the value for a given null-terminated key from a given object.
   *
   *  @details
   *  Function returns null instances for non-existent keys without modifying the object.
   *  Lookup is based on std::map and should be log(N).
   *  Function expects to received uninitialized memory. If using a pointer to a previous Dart
   *  instance, it must be passed through an appropriate dart_destroy function first.
   *
   *  @param[out] dst
   *  The dart_buffer_t instance that should be initialized with the result of the lookup.
   *
   *  @param[in] src
   *  The object instance to query from.
   *
   *  @param[in] key
   *  The null-terminated key to locate within the given object.
   *
   *  @return
   *  Whether anything went wrong during the lookup.
   */
  DART_ABI_EXPORT dart_err_t dart_buffer_obj_get_err(dart_buffer_t* dst, dart_buffer_t const* src, char const* key);

  /**
   *  @brief
   *  Function is used to retrieve the value for a given, possibly unterminated, key from a given object.
   *
   *  @details
   *  Function returns null instances for non-existent keys without modifying the object.
   *  Lookup is based on std::map and should be log(N).
   *
   *  @param[in] src
   *  The source object to query from.
   *
   *  @param[in] key
   *  The possibly unterminated key to locate within the given object.
   *
   *  @return
   *  The dart_buffer_t instance corresponding to the given key, or null in error.
   */
  DART_ABI_EXPORT dart_buffer_t dart_buffer_obj_get_len(dart_buffer_t const* src, char const* key, size_t len);

  /**
   *  @brief
   *  Function is used to retrieve the value for a given, possibly unterminated, key from a given object.
   *
   *  @details
   *  Function returns null instances for non-existent keys without modifying the object.
   *  Lookup is based on std::map and should be log(N).
   *  Function expects to received uninitialized memory. If using a pointer to a previous Dart
   *  instance, it must be passed through an appropriate dart_destroy function first.
   *
   *  @param[out] dst
   *  The dart_buffer_t instance that should be initialized with the result of the lookup.
   *
   *  @param[in] src
   *  The object instance to query from.
   *
   *  @param[in] key
   *  The possibly unterminated key to locate within the given object.
   *
   *  @return
   *  Whether anything went wrong during the lookup.
   */
  DART_ABI_EXPORT dart_err_t dart_buffer_obj_get_len_err(dart_buffer_t* dst, dart_buffer_t const* src, char const* key, size_t len);

  /**
   *  @brief
   *  Function is used to retrieve the value for a given index within a given array.
   *
   *  @details
   *  Function returns null instances for non-existent indices without modifying the array.
   *
   *  @remarks
   *  Returning null from an out-of-bounds access is a potentially questionable move, but
   *  it was done to be in better behavioral conformance with object lookup, and also
   *  to behave more similarly to std::vector by not throwing exceptions (although also
   *  not causing ub).
   *
   *  @param[in] src
   *  The array to lookup into.
   *
   *  @param[in] idx
   *  The index to lookup.
   *
   *  @return
   *  The dart_buffer_t instance corresponding to the given index, or null in error.
   */
  DART_ABI_EXPORT dart_buffer_t dart_buffer_arr_get(dart_buffer_t const* src, size_t idx);

  /**
   *  @brief
   *  Function is used to retrieve the value for a given index within a given array.
   *
   *  @details
   *  Function returns null instances for non-existent indices without modifying the array.
   *
   *  @remarks
   *  Returning null from an out-of-bounds access is a potentially questionable move, but
   *  it was done to be in better behavioral conformance with object lookup, and also
   *  to behave more similarly to std::vector by not throwing exceptions (although also
   *  not causing ub).
   *
   *  @param[out] dst
   *  The dart_buffer_t instance that should be initialized with the result of this lookup.
   *
   *  @param[in] src
   *  The array to lookup into.
   *
   *  @param[in] idx
   *  The index to lookup.
   *
   *  @return
   *  Whether anything went wrong during the lookup.
   */
  DART_ABI_EXPORT dart_err_t dart_buffer_arr_get_err(dart_buffer_t* dst, dart_buffer_t const* src, size_t idx);

  /**
   *  @brief
   *  Unwraps a given dart_buffer string instance.
   *
   *  @details
   *  String is gauranteed to be terminated, but may contain additional nulls.
   *  Use the sized overload of this function to simultaneously get the actual length.
   *
   *  @param[in] src
   *  The string instance to unwrap.
   *
   *  @return
   *  A pointer to the character data for the given string, or NULL on error.
   */
  DART_ABI_EXPORT char const* dart_buffer_str_get(dart_buffer_t const* src);

  /**
   *  @brief
   *  Unwraps a given dart_buffer string instance.
   *
   *  @details
   *  String is gauranteed to be terminated, but may contain additional nulls.
   *  For gauranteed correctness in the face of non-ASCII data, use the provided
   *  length out parameter.
   *
   *  @param[in] src
   *  The string instance to unwrap.
   *
   *  @param[out] len
   *  The length of the unwrapped string.
   *
   *  @return
   *  A pointer to the character data for the given string, or NULL on error.
   */
  DART_ABI_EXPORT char const* dart_buffer_str_get_len(dart_buffer_t const* src, size_t* len);

  /**
   *  @brief
   *  Unwraps a given dart_buffer integer instance.
   *
   *  @details
   *  Function returns zero on error, but could also return zero on success.
   *  If the type of the passed instance is not known to be integer, use the error overload
   *  to retrieve an error code.
   *
   *  @param[in] src
   *  The integer instance that should be unwrapped.
   *
   *  @return
   *  The integer value of the given instance.
   */
  DART_ABI_EXPORT int64_t dart_buffer_int_get(dart_buffer_t const* src);

  /**
   *  @brief
   *  Unwraps a given dart_buffer integer instance.
   *
   *  @param[in] src
   *  The integer instance that should be unwrapped.
   *
   *  @param[out] val
   *  A pointer to the integer that should be initialized to the unwrapped value.
   *
   *  @return
   *  Whether anything went wrong during the unwrap.
   */
  DART_ABI_EXPORT dart_err_t dart_buffer_int_get_err(dart_buffer_t const* src, int64_t* val);

  /**
   *  @brief
   *  Unwraps a given dart_buffer decimal instance.
   *
   *  @details
   *  Function returns zero on error, but could also return zero on success.
   *  If the type of the passed instance is not known to be decimal, use the error overload
   *  to retrieve an error code.
   *
   *  @param[in] src
   *  The decimal instance that should be unwrapped.
   *
   *  @return
   *  The decimal value of the given instance.
   */
  DART_ABI_EXPORT double dart_buffer_dcm_get(dart_buffer_t const* src);

  /**
   *  @brief
   *  Unwraps a given dart_buffer decimal instance.
   *
   *  @param[in] src
   *  The decimal instance that should be unwrapped.
   *
   *  @param[out] val
   *  A pointer to the decimal that should be initialized to the unwrapped value.
   *
   *  @return
   *  Whether anything went wrong during the unwrap.
   */
  DART_ABI_EXPORT dart_err_t dart_buffer_dcm_get_err(dart_buffer_t const* src, double* val);

  /**
   *  @brief
   *  Unwraps a given dart_buffer boolean instance.
   *
   *  @details
   *  Function returns zero on error, but could also return zero on success.
   *  If the type of the passed instance is not known to be boolean, use the error overload
   *  to retrieve an error code.
   *
   *  @param[in] src
   *  The boolean instance that should be unwrapped.
   *
   *  @return
   *  The boolean value of the given instance.
   */
  DART_ABI_EXPORT int dart_buffer_bool_get(dart_buffer_t const* src);

  /**
   *  @brief
   *  Unwraps a given dart_buffer boolean instance.
   *
   *  @param[in] src
   *  The boolean instance that should be unwrapped.
   *
   *  @param[out] val
   *  A pointer to the boolean that should be initialized to the unwrapped value.
   *
   *  @return
   *  Whether anything went wrong during the unwrap.
   */
  DART_ABI_EXPORT dart_err_t dart_buffer_bool_get_err(dart_buffer_t const* src, int* val);

  /**
   *  @brief
   *  Function returns the size of a Dart aggregate (object or array) or string
   *  instance.
   *
   *  @param[in] src
   *  The instance whose length should be queried.
   *
   *  @return
   *  The size of the given aggregate/string.
   */
  DART_ABI_EXPORT size_t dart_buffer_size(dart_buffer_t const* src);

  /**
   *  @brief
   *  Function recursively calculates equality for the given instances.
   *
   *  @details
   *  Disparate types always compare unequal, disparate reference counters always
   *  compare unequal, same types are recursively compared.
   *
   *  @param[in] lhs
   *  The "left hand side" of the expression.
   *
   *  @param[in] rhs
   *  The "right hand side" of the expression.
   *
   *  @return
   *  Whether the two instances were equivalent.
   */
  DART_ABI_EXPORT int dart_buffer_equal(dart_buffer_t const* lhs, dart_buffer_t const* rhs);

  /**
   *  @brief
   *  Function checks whether the given instance is of object type.
   */
  DART_ABI_EXPORT int dart_buffer_is_obj(dart_buffer_t const* src);

  /**
   *  @brief
   *  Function checks whether the given instance is of array type.
   */
  DART_ABI_EXPORT int dart_buffer_is_arr(dart_buffer_t const* src);

  /**
   *  @brief
   *  Function checks whether the given instance is of string type.
   */
  DART_ABI_EXPORT int dart_buffer_is_str(dart_buffer_t const* src);

  /**
   *  @brief
   *  Function checks whether the given instance is of integer type.
   */
  DART_ABI_EXPORT int dart_buffer_is_int(dart_buffer_t const* src);

  /**
   *  @brief
   *  Function checks whether the given instance is of decimal type.
   */
  DART_ABI_EXPORT int dart_buffer_is_dcm(dart_buffer_t const* src);

  /**
   *  @brief
   *  Function checks whether the given instance is of boolean type.
   */
  DART_ABI_EXPORT int dart_buffer_is_bool(dart_buffer_t const* src);

  /**
   *  @brief
   *  Function checks whether the given instance is null.
   */
  DART_ABI_EXPORT int dart_buffer_is_null(dart_buffer_t const* src);

  /**
   *  @brief
   *  Returns the type of the given instance.
   */
  DART_ABI_EXPORT dart_type_t dart_buffer_get_type(dart_buffer_t const* src);

  /*----- dart_buffer JSON Manipulation Functions -----*/

  /**
   *  @brief
   *  Function parses a given null-terminated JSON string and returns
   *  a handle to a Dart object hierarchy representing the given string.
   *
   *  @param[in] str
   *  A null-terminated JSON string.
   *
   *  @return
   *  The dart_buffer_t instance representing the string.
   */
  DART_ABI_EXPORT dart_buffer_t dart_buffer_from_json(char const* str);

  /**
   *  @brief
   *  Function parses a given null-terminated JSON string and initializes
   *  a handle to a Dart object hierarchy representing the given string.
   *
   *  @param[out] pkt
   *  The packet to initialize as a handle to the parsed representation.
   *
   *  @param[in] str
   *  A null-terminated JSON string.
   *
   *  @return
   *  Whether anything went wrong during parsing.
   */
  DART_ABI_EXPORT dart_err_t dart_buffer_from_json_err(dart_buffer_t* pkt, char const* str);

  /**
   *  @brief
   *  Function parses a given null-terminated JSON string and returns
   *  a handle to a Dart object hierarchy representing the given string, and using
   *  a specific reference counter type.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] str
   *  A null-terminated JSON string.
   *
   *  @return
   *  The dart_buffer_t instance representing the string.
   */
  DART_ABI_EXPORT dart_buffer_t dart_buffer_from_json_rc(dart_rc_type_t rc, char const* str);

  /**
   *  @brief
   *  Function parses a given null-terminated JSON string and initializes
   *  a handle to a Dart object hierarchy representing the given string, and using
   *  a specific reference counter type.
   *
   *  @param[out] pkt
   *  The packet to initialize as a handle to the parsed representation.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] str
   *  A null-terminated JSON string.
   *
   *  @return
   *  Whether anything went wrong during parsing.
   */
  DART_ABI_EXPORT dart_err_t dart_buffer_from_json_rc_err(dart_buffer_t* pkt, dart_rc_type_t rc, char const* str);

  /**
   *  @brief
   *  Function parses a given, possibly unterminated, JSON string and returns
   *  a handle to a Dart object hierarchy representing the given string.
   *
   *  @param[in] str
   *  A possibly unterminated JSON string.
   *
   *  @param[in] len
   *  The length of the given JSON string.
   *
   *  @return
   *  The dart_buffer_t instance representing the string.
   */
  DART_ABI_EXPORT dart_buffer_t dart_buffer_from_json_len(char const* str, size_t len);

  /**
   *  @brief
   *  Function parses a given, possibly unterminated, JSON string and initializes
   *  a handle to a Dart object hierarchy representing the given string.
   *
   *  @param[out] pkt
   *  The packet to initialize as a handle to the parsed representation.
   *
   *  @param[in] str
   *  A possibly unterminated JSON string.
   *
   *  @param[in] len
   *  The length of the given JSON string.
   *
   *  @return
   *  Whether anything went wrong during parsing.
   */
  DART_ABI_EXPORT dart_err_t dart_buffer_from_json_len_err(dart_buffer_t* pkt, char const* str, size_t len);

  /**
   *  @brief
   *  Function parses a given, possibly unterminated, JSON string and returns
   *  a handle to a Dart object hierarchy representing the given string, and using
   *  a specific reference counter type.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] str
   *  A possibly unterminated JSON string.
   *
   *  @param[in] len
   *  The length of the given JSON string.
   *
   *  @return
   *  The dart_buffer_t instance representing the string.
   */
  DART_ABI_EXPORT dart_buffer_t dart_buffer_from_json_len_rc(dart_rc_type_t rc, char const* str, size_t len);

  /**
   *  @brief
   *  Function parses a given, possibly unterminated, JSON string and initializes
   *  a handle to a Dart object hierarchy representing the given string, and using
   *  a specific reference counter type.
   *
   *  @param[out] pkt
   *  The packet to initialize as a handle to the parsed representation.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] str
   *  A possibly unterminated JSON string.
   *
   *  @param[in] len
   *  The length of the given JSON string.
   *
   *  @return
   *  Whether anything went wrong during parsing.
   */
  DART_ABI_EXPORT dart_err_t dart_buffer_from_json_len_rc_err(dart_buffer_t* pkt, dart_rc_type_t rc, char const* str, size_t len);

  /**
   *  @brief
   *  Function stringifies a given dart_buffer_t instance into a valid JSON string.
   *
   *  @param[in] pkt
   *  The packet to stringify
   *
   *  @param[out] len
   *  The length of the resulting string.
   *
   *  @return
   *  The stringified result of the given dart_buffer_t instance. Allocated with malloc, must be freed.
   */
  DART_ABI_EXPORT char* dart_buffer_to_json(dart_buffer_t const* pkt, size_t* len);

  /**
   *  @brief
   *  Function will create a dynamic, mutable, representation of the given serialized object hierarchy.
   *
   *  @details
   *  Function serves as a go-between for the mutable and immutable, dynamic and network-ready,
   *  representations Dart uses.
   *
   *  @param[in] src
   *  The serialized instance to be serialized.
   *
   *  @return
   *  The dynamic, mutable, representation of the given instance, or null on error.
   */
  DART_ABI_EXPORT dart_heap_t dart_buffer_lift(dart_buffer_t const* src);

  /**
   *  @brief
   *  Function will create a dynamic, mutable, representation of the given serialized object hierarchy.
   *
   *  @details
   *  Function serves as a go-between for the mutable and immutable, dynamic and network-ready,
   *  representations Dart uses.
   *
   *  @param[out] dst
   *  The dart_heap_t instance to be initialized.
   *
   *  @param[in] src
   *  The serialized instance to be serialized.
   *
   *  @return
   *  Whether anything went wrong during de-finalizing.
   */
  DART_ABI_EXPORT dart_err_t dart_buffer_lift_err(dart_heap_t* dst, dart_buffer_t const* src);

  /**
   *  @brief
   *  Function will create a dynamic, mutable, representation of the given serialized object hierarchy.
   *
   *  @details
   *  Function serves as a go-between for the mutable and immutable, dynamic and network-ready,
   *  representations Dart uses.
   *
   *  @param[in] src
   *  The serialized instance to be serialized.
   *
   *  @return
   *  The dynamic, mutable, representation of the given instance, or null on error.
   */
  DART_ABI_EXPORT dart_heap_t dart_buffer_definalize(dart_buffer_t const* src);

  /**
   *  @brief
   *  Function will create a dynamic, mutable, representation of the given serialized object hierarchy.
   *
   *  @details
   *  Function serves as a go-between for the mutable and immutable, dynamic and network-ready,
   *  representations Dart uses.
   *
   *  @param[out] dst
   *  The dart_heap_t instance to be initialized.
   *
   *  @param[in] src
   *  The serialized instance to be serialized.
   *
   *  @return
   *  Whether anything went wrong during de-finalizing.
   */
  DART_ABI_EXPORT dart_err_t dart_buffer_definalize_err(dart_heap_t* dst, dart_buffer_t const* src);

  /**
   *  @brief
   *  Function returns a non-owning pointer to the underlying network buffer for a dart_buffer_t
   *  instance.
   *
   *  @param[in] src
   *  The instance to pull the buffer from.
   *
   *  @param[out] len
   *  An integer where the size of the buffer can be written.
   *
   *  @return
   *  The underlying network buffer.
   */
  DART_ABI_EXPORT void const* dart_buffer_get_bytes(dart_buffer_t const* src, size_t* len);

  /**
   *  @brief
   *  Function returns an owning pointer to a copy of the underlying network buffer for a dart_buffer_t
   *  instance.
   *
   *  @param[in] src
   *  The instance to pull the buffer from.
   *
   *  @param[out] len
   *  An integer where the size of the buffer can be written.
   *
   *  @return
   *  The underlying network buffer. Was created via malloc, must be freed.
   */
  DART_ABI_EXPORT void* dart_buffer_dup_bytes(dart_buffer_t const* src, size_t* len);

  /**
   *  @brief
   *  Function reconstructs a dart_buffer object from the network buffer of another.
   *
   *  @details
   *  The length parameter passed to the function may be larger than the underlying
   *  Dart buffer itself, but it must be valid for memcpy to read from all passed bytes,
   *  and len must be at least as large as the original buffer.
   *
   *  @param[in] bytes
   *  The underlying network buffer from a previous dart_buffer instance.
   *
   *  @param[in] len
   *  The length of the buffer being passed.
   *
   *  @return
   *  The reconstructed dart_buffer instance.
   */
  DART_ABI_EXPORT dart_buffer_t dart_buffer_from_bytes(void const* bytes, size_t len);

  /**
   *  @brief
   *  Function reconstructs a dart_buffer object from the network buffer of another.
   *
   *  @details
   *  The length parameter passed to the function may be larger than the underlying
   *  Dart buffer itself, but it must be valid for memcpy to read from all passed bytes,
   *  and len must be at least as large as the original buffer.
   *
   *  @param[out] dst
   *  The dart_buffer_t instance to initialize.
   *
   *  @param[in] bytes
   *  The underlying network buffer from a previous dart_buffer instance.
   *
   *  @param[in] len
   *  The length of the buffer being passed.
   *
   *  @return
   *  Whether anything went wrong during reconstruction.
   */
  DART_ABI_EXPORT dart_err_t dart_buffer_from_bytes_err(dart_buffer_t* dst, void const* bytes, size_t len);

  /**
   *  @brief
   *  Function reconstructs a dart_buffer object from the network buffer of another,
   *  with a specific reference counter.
   *
   *  @details
   *  The length parameter passed to the function may be larger than the underlying
   *  Dart buffer itself, but it must be valid for memcpy to read from all passed bytes,
   *  and len must be at least as large as the original buffer.
   *
   *  @param[in] bytes
   *  The underlying network buffer from a previous dart_buffer instance.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] len
   *  The length of the buffer being passed.
   *
   *  @return
   *  The reconstructed dart_buffer instance.
   */
  DART_ABI_EXPORT dart_buffer_t dart_buffer_from_bytes_rc(void const* bytes, dart_rc_type_t rc, size_t len);

  /**
   *  @brief
   *  Function reconstructs a dart_buffer object from the network buffer of another,
   *  with a specific reference counter.
   *
   *  @details
   *  The length parameter passed to the function may be larger than the underlying
   *  Dart buffer itself, but it must be valid for memcpy to read from all passed bytes,
   *  and len must be at least as large as the original buffer.
   *
   *  @param[in] bytes
   *  The underlying network buffer from a previous dart_buffer instance.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] len
   *  The length of the buffer being passed.
   *
   *  @return
   *  Whether anything went wrong during reconstruction.
   */
  DART_ABI_EXPORT dart_err_t dart_buffer_from_bytes_rc_err(dart_buffer_t* dst, dart_rc_type_t rc, void const* bytes, size_t len);

  /**
   *  @brief
   *  Function takes ownership of, and reconstructs a dart_buffer object from,
   *  the network buffer of another.
   *
   *  @param[in] bytes
   *  The buffer to be moved in.
   *
   *  @return
   *  The reconstructed dart_buffer instance.
   */
  DART_ABI_EXPORT dart_buffer_t dart_buffer_take_bytes(void* bytes);

  /**
   *  @brief
   *  Function takes ownership of, and reconstructs a dart_buffer object from,
   *  the network buffer of another.
   *
   *  @param[out] dst
   *  The dart_buffer_t instance to be initialized.
   *
   *  @param[in,out] bytes
   *  The buffer to be moved in.
   *
   *  @return
   *  Whether anything went wrong during reconstruction.
   */
  DART_ABI_EXPORT dart_err_t dart_buffer_take_bytes_err(dart_buffer_t* dst, void* bytes);

  /**
   *  @brief
   *  Function takes ownership of, and reconstructs a dart_buffer object from,
   *  the network buffer of another, with an explicitly set reference counter type.
   *
   *  @param[in,out] bytes
   *  The buffer to be moved in.
   *
   *  @param[in] rc
   *  The reference counter type to use.
   *
   *  @return
   *  The reconstructed dart_buffer instance.
   */
  DART_ABI_EXPORT dart_buffer_t dart_buffer_take_bytes_rc(void* bytes, dart_rc_type_t rc);

  /**
   *  @brief
   *  Function takes ownership of, and reconstructs a dart_buffer object from,
   *  the network buffer of another.
   *
   *  @param[out] dst
   *  The dart_buffer_t instance to be initialized.
   *
   *  @param[in,out] bytes
   *  The buffer to be moved in.
   *
   *  @return
   *  Whether anything went wrong during reconstruction.
   */
  DART_ABI_EXPORT dart_err_t dart_buffer_take_bytes_rc_err(dart_buffer_t* dst, dart_rc_type_t rc, void* bytes);

  /*----- Generic Lifecycle Functions -----*/

  /**
   *  @brief
   *  Function is used to default-initialize a dart_packet_t instance to null.
   *
   *  @remarks
   *  Function cannot meaningfully fail, but has an error overload for API consistency.
   *
   *  @return
   *  The dart_packet instance initialized to null.
   */
  DART_ABI_EXPORT dart_packet_t dart_init();

  /**
   *  @brief
   *  Function is used to default-initialize a dart_packet_instance to null.
   *
   *  @details
   *  Function expects to receive a pointer to uninitialized memory, and does not call
   *  dart_destroy before constructing the object. If you already have a live Dart object,
   *  you must pass it through one of the dart_destroy functions before calling this function.
   *
   *  @remarks
   *  Function cannot meaningfully fail, but has an error overload for API consistency.
   *
   *  @param[out] pkt
   *  The dart_packet_t instance to be initialized.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_init_err(dart_packet_t* dst);

  /**
   *  @brief
   *  Function is used to default-initialize a dart_packet instance to null with an explicitly
   *  set reference counter type.
   *
   *  @remarks
   *  Function cannot meaningfully fail, but has an error overload for API consistency.
   *
   *  @param[in] rc
   *  The reference counter type the initialized Dart type should use.
   *
   *  @return
   *  The dart_packet instance initialized to null.
   */
  DART_ABI_EXPORT dart_packet_t dart_init_rc(dart_rc_type_t rc);

  /**
   *  @brief
   *  Function is used to default-initialize a dart_packet instance to null with an explicitly
   *  set reference counter type.
   *
   *  @details
   *  Function expects to receive a pointer to uninitialized memory, and does not call
   *  dart_destroy before constructing the object. If you already have a live Dart object,
   *  you must pass it through one of the dart_destroy functions before calling this function.
   *
   *  @remarks
   *  Function cannot meaningfully fail, but has an error overload for API consistency.
   *
   *  @param[out] pkt
   *  The dart_packet_t instance to be initialized.
   *
   *  @param[in] rc
   *  The reference counter type the initialized Dart type should use.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_init_rc_err(dart_packet_t* dst, dart_rc_type_t rc);

  /**
   *  @brief
   *  Function is used to copy-initialize a dart_packet instance to whatever value was provided.
   *
   *  @details
   *  Dart uses copy-on-write, so the copy is equivalent to a reference count increment.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] src
   *  Pointer to dart_packet_t instance to copy from.
   *
   *  @return
   *  The dart_packet instance initialized to null.
   */
  DART_ABI_EXPORT dart_packet_t dart_copy(void const* src);

  /**
   *  @brief
   *  Function is used to copy-initialize a dart_packet instance to whatever value was provided.
   *
   *  @details
   *  Dart uses copy-on-write, so the copy is equivalent to a reference count increment.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[out] dst
   *  Pointer to uninitialized dart_packet_t instance to copy-initialize.
   *
   *  @param[in] src
   *  Pointer to dart_packet_t instance to copy from.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_copy_err(void* dst, void const* src);

  /**
   *  @brief
   *  Function is used to move-initialize a dart_packet instance to whatever value was provided.
   *  
   *  @details
   *  Operation relies on C++ move-semantics under the covers and "steals" the reference from
   *  the incoming object, resetting it to null.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in,out] src
   *  Pointer to dart_packet_t instance to move from.
   *
   *  @return
   *  The dart_packet instance that was move-initialized.
   */
  DART_ABI_EXPORT dart_packet_t dart_move(void* src);

  /**
   *  @brief
   *  Function is used to move-initialize a dart_packet instance to whatever value was provided.
   *
   *  @details
   *  Operation relies on C++ move-semantics under the covers and "steals" the reference from
   *  the incoming object, resetting it to null.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[out] dst
   *  Pointer to unitialized dart_packet_t instance to move-initialize.
   *
   *  @param[in,out] src
   *  Pointer to dart_packet_t instance to move from.
   *
   *  @return
   *  Whether anything went wrong during move-initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_move_err(void* dst, void* src);

  /**
   *  @brief
   *  Function is used to destroy a live dart_packet instance, releasing its reference count
   *  and any potentially exclusively owned resources.
   *
   *  @remarks
   *  Technically speaking, even a null dart_packet instance is a "live" object,
   *  and pedantically speaking, all live objects must be destroyed,
   *  so _all_ dart_packet instances must pass through this function.
   *  Practically speaking null dart_packet instances own _no_ resources,
   *  (imagine it as std::variant<std::monostate>), and so will not leak resources if not destroyed.
   *  What is done with this information is up to you.
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in,out] pkt
   *  The object to be destroyed.
   *
   *  @return
   *  Whether anything went wrong during destruction.
   */
  DART_ABI_EXPORT dart_err_t dart_destroy(void* pkt);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as an object.
   *
   *  @details
   *  Function can fail for any reason constructing std::map can fail.
   *  Will return a null packet if construction fails.
   *
   *  @return
   *  The dart_packet instance initialized as an object, or null on error.
   */
  DART_ABI_EXPORT dart_packet_t dart_obj_init();

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as an object.
   *
   *  @details
   *  Function expects to receive a pointer to uninitialized memory, and does not call
   *  dart_destroy before constructing the object. If you already have a live Dart object,
   *  you must pass it through one of the dart_destroy functions before calling this function.
   *  Function can fail for any reason constructing std::map can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[out] pkt
   *  The dart_packet_t instance to be initialized.
   *
   *  @return
   *  Whether anything went wrong during destruction.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_init_err(dart_packet_t* dst);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as an object with an explicitly
   *  set reference counter type.
   *
   *  @details
   *  Function can fail for any reason constructing std::map can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @return
   *  The dart_packet instance initialized as an object, or null on error.
   */
  DART_ABI_EXPORT dart_packet_t dart_obj_init_rc(dart_rc_type_t rc);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as an object with an explicitly
   *  set reference counter type.
   *
   *  @details
   *  Function expects to receive a pointer to uninitialized memory, and does not call
   *  dart_destroy before constructing the object. If you already have a live Dart object,
   *  you must pass it through one of the dart_destroy functions before calling this function.
   *  Function can fail for any reason constructing std::map can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[out] pkt
   *  The packet to be initialized as an object.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @return
   *  Whether anything went wrong during destruction.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_init_rc_err(dart_packet_t* dst, dart_rc_type_t rc);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as an object, according to the given
   *  format string.
   *
   *  @details
   *  Function uses an extremely simplistic DSL to encode the expected sequence of VALUES
   *  for the constructed object. Each character in the format string corresponds to a key-value
   *  PAIR in the incoming varargs argument list.
   *  Accepted characters:
   *    * o
   *      - Begin object. Adds a level of object nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * a
   *      - Begin array. Adds a level of array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * s
   *      - String argument. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a null-terminated string).
   *    * S
   *      - Sized string argument. Will consume three arguments, one for the key (always a string),
   *        one for the value (now assumed to be an unterminated string), and one for the length
   *        of the string.
   *    * ui
   *      - Unsigned integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned int).
   *    * ul
   *      - Unsigned long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned long).
   *    * i
   *      - Integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed integer).
   *    * l
   *      - Long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed long).
   *    * d
   *      - Decimal. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a double).
   *    * b
   *      - Boolean. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an int).
   *    * " "
   *    * n
   *      - A space means that no value, only a single key, has been provided, and the value
   *        should be initialized to null.
   *    * ,
   *      - End aggregate. Removes a level of object/array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *  Following code will build this object: `{"hello":"world","pi":3.14159}`
   *  ```
   *  dart_packet_obj_init_va("sd", "hello", "world", "pi", 3.14159);
   *  ```
   *  Function can fail for any reason constructing std::map can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[in] format
   *  The format string specifying the variadic arguments to expect.
   *
   *  @return
   *  The dart_packet instance initialized as an object, or null on error.
   *  Note that errors in the format string cannot be detected and will likely lead to crashes.
   */
  DART_ABI_EXPORT dart_packet_t dart_obj_init_va(char const* format, ...);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as an object, according to the given
   *  format string.
   *
   *  @details
   *  Function uses an extremely simplistic DSL to encode the expected sequence of VALUES
   *  for the constructed object. Each character in the format string corresponds to a key-value
   *  PAIR in the incoming varargs argument list.
   *  Accepted characters:
   *    * o
   *      - Begin object. Adds a level of object nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * a
   *      - Begin array. Adds a level of array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * s
   *      - String argument. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a null-terminated string).
   *    * S
   *      - Sized string argument. Will consume three arguments, one for the key (always a string),
   *        one for the value (now assumed to be an unterminated string), and one for the length
   *        of the string.
   *    * ui
   *      - Unsigned integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned int).
   *    * ul
   *      - Unsigned long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned long).
   *    * i
   *      - Integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed integer).
   *    * l
   *      - Long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed long).
   *    * d
   *      - Decimal. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a double).
   *    * b
   *      - Boolean. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an int).
   *    * " "
   *    * n
   *      - A space means that no value, only a single key, has been provided, and the value
   *        should be initialized to null.
   *    * ,
   *      - End aggregate. Removes a level of object/array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *  Following code will build this object: `{"hello":"world","pi":3.14159}`
   *  ```
   *  dart_packet_obj_init_va("sd", "hello", "world", "pi", 3.14159);
   *  ```
   *  Function expects to receive a pointer to uninitialized memory, and does not call
   *  dart_destroy before constructing the object. If you already have a live Dart object,
   *  you must pass it through one of the dart_destroy functions before calling this function.
   *  Function can fail for any reason constructing std::map can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[out] pkt
   *  The dart_packet_t instance to be initialized.
   *
   *  @param[in] format
   *  The format string specifying the variadic arguments to expect.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   *  Note that errors in the format string cannot be detected and will likely lead to crashes.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_init_va_err(dart_packet_t* dst, char const* format, ...);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as an object, according to the given
   *  format string, with an explicitly set reference counter type.
   *
   *  @details
   *  Function uses an extremely simplistic DSL to encode the expected sequence of VALUES
   *  for the constructed object. Each character in the format string corresponds to a key-value
   *  PAIR in the incoming varargs argument list.
   *  Accepted characters:
   *    * o
   *      - Begin object. Adds a level of object nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * a
   *      - Begin array. Adds a level of array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * s
   *      - String argument. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a null-terminated string).
   *    * S
   *      - Sized string argument. Will consume three arguments, one for the key (always a string),
   *        one for the value (now assumed to be an unterminated string), and one for the length
   *        of the string.
   *    * ui
   *      - Unsigned integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned int).
   *    * ul
   *      - Unsigned long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned long).
   *    * i
   *      - Integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed integer).
   *    * l
   *      - Long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed long).
   *    * d
   *      - Decimal. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a double).
   *    * b
   *      - Boolean. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an int).
   *    * " "
   *    * n
   *      - A space means that no value, only a single key, has been provided, and the value
   *        should be initialized to null.
   *    * ,
   *      - End aggregate. Removes a level of object/array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *  Following code will build this object: `{"hello":"world","pi":3.14159}`
   *  ```
   *  dart_packet_obj_init_va("sd", "hello", "world", "pi", 3.14159);
   *  ```
   *  Function can fail for any reason constructing std::map can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] format
   *  The format string specifying the variadic arguments to expect.
   *
   *  @return
   *  The dart_packet instance initialized as an object, or null on error.
   *  Note that errors in the format string cannot be detected and will likely lead to crashes.
   */
  DART_ABI_EXPORT dart_packet_t dart_obj_init_va_rc(dart_rc_type_t rc, char const* format, ...);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as an object, according to the given
   *  format string, with an explicitly set reference counter type.
   *
   *  @details
   *  Function uses an extremely simplistic DSL to encode the expected sequence of VALUES
   *  for the constructed object. Each character in the format string corresponds to a key-value
   *  PAIR in the incoming varargs argument list.
   *  Accepted characters:
   *    * o
   *      - Begin object. Adds a level of object nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * a
   *      - Begin array. Adds a level of array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * s
   *      - String argument. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a null-terminated string).
   *    * S
   *      - Sized string argument. Will consume three arguments, one for the key (always a string),
   *        one for the value (now assumed to be an unterminated string), and one for the length
   *        of the string.
   *    * ui
   *      - Unsigned integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned int).
   *    * ul
   *      - Unsigned long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned long).
   *    * i
   *      - Integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed integer).
   *    * l
   *      - Long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed long).
   *    * d
   *      - Decimal. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a double).
   *    * b
   *      - Boolean. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an int).
   *    * " "
   *    * n
   *      - A space means that no value, only a single key, has been provided, and the value
   *        should be initialized to null.
   *    * ,
   *      - End aggregate. Removes a level of object/array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *  Following code will build this object: `{"hello":"world","pi":3.14159}`
   *  ```
   *  dart_packet_obj_init_va("sd", "hello", "world", "pi", 3.14159);
   *  ```
   *  Function expects to receive a pointer to uninitialized memory, and does not call
   *  dart_destroy before constructing the object. If you already have a live Dart object,
   *  you must pass it through one of the dart_destroy functions before calling this function.
   *  Function can fail for any reason constructing std::map can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[out] pkt
   *  The dart_packet_t instance to be initialized.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] format
   *  The format string specifying the variadic arguments to expect.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   *  Note that errors in the format string cannot be detected and will likely lead to crashes.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_init_va_rc_err(dart_packet_t* dst, dart_rc_type_t rc, char const* format, ...);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as an array.
   *
   *  @details
   *  Function can fail for any reason constructing std::vector can fail.
   *  Will return a null packet if construction fails.
   *
   *  @return
   *  The dart_packet instance initialized as an object, or null on error.
   */
  DART_ABI_EXPORT dart_packet_t dart_arr_init();

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as an array.
   *
   *  @details
   *  Function expects to receive a pointer to uninitialized memory, and does not call
   *  dart_destroy before constructing the object. If you already have a live Dart object,
   *  you must pass it through one of the dart_destroy functions before calling this function.
   *  Function can fail for any reason constructing std::vector can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[out] pkt
   *  The dart_packet_t instance to be initialized.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_arr_init_err(dart_packet_t* pkt);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as an array.
   *
   *  @details
   *  Function can fail for any reason constructing std::vector can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @return
   *  The dart_packet instance initialized as an object, or null on error.
   */
  DART_ABI_EXPORT dart_packet_t dart_arr_init_rc(dart_rc_type_t rc);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as an array.
   *
   *  @details
   *  Function expects to receive a pointer to uninitialized memory, and does not call
   *  dart_destroy before constructing the object. If you already have a live Dart object,
   *  you must pass it through one of the dart_destroy functions before calling this function.
   *  Function can fail for any reason constructing std::vector can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[out] pkt
   *  The dart_packet_t instance to be initialized.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_arr_init_rc_err(dart_packet_t* pkt, dart_rc_type_t rc);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as an array, according to the given
   *  format string.
   *
   *  @details
   *  Function uses an extremely simplistic DSL to encode the expected sequence of VALUES
   *  for the constructed array. Each character in the format string corresponds to a value
   *  in the incoming varargs argument list.
   *  Accepted characters:
   *    * o
   *      - Begin object. Adds a level of object nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * a
   *      - Begin array. Adds a level of array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * s
   *      - String argument. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a null-terminated string).
   *    * S
   *      - Sized string argument. Will consume three arguments, one for the key (always a string),
   *        one for the value (now assumed to be an unterminated string), and one for the length
   *        of the string.
   *    * ui
   *      - Unsigned integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned int).
   *    * ul
   *      - Unsigned long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned long).
   *    * i
   *      - Integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed integer).
   *    * l
   *      - Long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed long).
   *    * d
   *      - Decimal. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a double).
   *    * b
   *      - Boolean. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an int).
   *    * " "
   *    * n
   *      - A space means that no value, only a single key, has been provided, and the value
   *        should be initialized to null.
   *    * ,
   *      - End aggregate. Removes a level of object/array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *  Following code will build this array: `[1, "two", 3.14159, null]`
   *  ```
   *  dart_packet_arr_init_va("isd ", 1, "two", 3.14159);
   *  ```
   *  Function can fail for any reason constructing std::vector can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[in] format
   *  The format string specifying the variadic arguments to expect.
   *
   *  @return
   *  The dart_packet instance initialized as an array, or null on error.
   *  Note that errors in the format string cannot be detected and will likely lead to crashes.
   */
  DART_ABI_EXPORT dart_packet_t dart_arr_init_va(char const* format, ...);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as an array, according to the given
   *  format string.
   *
   *  @details
   *  Function uses an extremely simplistic DSL to encode the expected sequence of VALUES
   *  for the constructed array. Each character in the format string corresponds to a value
   *  in the incoming varargs argument list.
   *  Accepted characters:
   *    * o
   *      - Begin object. Adds a level of object nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * a
   *      - Begin array. Adds a level of array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * s
   *      - String argument. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a null-terminated string).
   *    * S
   *      - Sized string argument. Will consume three arguments, one for the key (always a string),
   *        one for the value (now assumed to be an unterminated string), and one for the length
   *        of the string.
   *    * ui
   *      - Unsigned integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned int).
   *    * ul
   *      - Unsigned long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned long).
   *    * i
   *      - Integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed integer).
   *    * l
   *      - Long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed long).
   *    * d
   *      - Decimal. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a double).
   *    * b
   *      - Boolean. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an int).
   *    * " "
   *    * n
   *      - A space means that no value, only a single key, has been provided, and the value
   *        should be initialized to null.
   *    * ,
   *      - End aggregate. Removes a level of object/array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *  Following code will build this array: `[1, "two", 3.14159, null]`
   *  ```
   *  dart_packet_arr_init_va("isd ", 1, "two", 3.14159);
   *  ```
   *  Function expects to receive a pointer to uninitialized memory, and does not call
   *  dart_destroy before constructing the object. If you already have a live Dart object,
   *  you must pass it through one of the dart_destroy functions before calling this function.
   *  Function can fail for any reason constructing std::vector can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[out] pkt
   *  The dart_packet_t instance to be initialized.
   *
   *  @param[in] format
   *  The format string specifying the variadic arguments to expect.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   *  Note that errors in the format string cannot be detected and will likely lead to crashes.
   */
  DART_ABI_EXPORT dart_err_t dart_arr_init_va_err(dart_packet_t* pkt, char const* format, ...);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as an array, according to the given
   *  format string, with an explicitly set reference counter type.
   *
   *  @details
   *  Function uses an extremely simplistic DSL to encode the expected sequence of VALUES
   *  for the constructed array. Each character in the format string corresponds to a value
   *  in the incoming varargs argument list.
   *  Accepted characters:
   *    * o
   *      - Begin object. Adds a level of object nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * a
   *      - Begin array. Adds a level of array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * s
   *      - String argument. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a null-terminated string).
   *    * S
   *      - Sized string argument. Will consume three arguments, one for the key (always a string),
   *        one for the value (now assumed to be an unterminated string), and one for the length
   *        of the string.
   *    * ui
   *      - Unsigned integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned int).
   *    * ul
   *      - Unsigned long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned long).
   *    * i
   *      - Integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed integer).
   *    * l
   *      - Long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed long).
   *    * d
   *      - Decimal. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a double).
   *    * b
   *      - Boolean. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an int).
   *    * " "
   *    * n
   *      - A space means that no value, only a single key, has been provided, and the value
   *        should be initialized to null.
   *    * ,
   *      - End aggregate. Removes a level of object/array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *  Following code will build this array: `[1, "two", 3.14159, null]`
   *  ```
   *  dart_packet_arr_init_va("isd ", 1, "two", 3.14159);
   *  ```
   *  Function can fail for any reason constructing std::vector can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] format
   *  The format string specifying the variadic arguments to expect.
   *
   *  @return
   *  The dart_packet instance initialized as an array, or null on error.
   *  Note that errors in the format string cannot be detected and will likely lead to crashes.
   */
  DART_ABI_EXPORT dart_packet_t dart_arr_init_va_rc(dart_rc_type_t rc, char const* format, ...);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as an array, according to the given
   *  format string, with an explicitly set reference counter type.
   *
   *  @details
   *  Function uses an extremely simplistic DSL to encode the expected sequence of VALUES
   *  for the constructed array. Each character in the format string corresponds to a value
   *  in the incoming varargs argument list.
   *  Accepted characters:
   *    * o
   *      - Begin object. Adds a level of object nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * a
   *      - Begin array. Adds a level of array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *    * s
   *      - String argument. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a null-terminated string).
   *    * S
   *      - Sized string argument. Will consume three arguments, one for the key (always a string),
   *        one for the value (now assumed to be an unterminated string), and one for the length
   *        of the string.
   *    * ui
   *      - Unsigned integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned int).
   *    * ul
   *      - Unsigned long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an unsigned long).
   *    * i
   *      - Integer. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed integer).
   *    * l
   *      - Long. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a signed long).
   *    * d
   *      - Decimal. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be a double).
   *    * b
   *      - Boolean. Will consume two arguments, one for the key (always a string)
   *        and one for the value (now assumed to be an int).
   *    * " "
   *    * n
   *      - A space means that no value, only a single key, has been provided, and the value
   *        should be initialized to null.
   *    * ,
   *      - End aggregate. Removes a level of object/array nesting before continuing to consume
   *        more arguments. Does NOT consume an argument.
   *  Following code will build this array: `[1, "two", 3.14159, null]`
   *  ```
   *  dart_packet_arr_init_va("isd ", 1, "two", 3.14159);
   *  ```
   *  Function expects to receive a pointer to uninitialized memory, and does not call
   *  dart_destroy before constructing the object. If you already have a live Dart object,
   *  you must pass it through one of the dart_destroy functions before calling this function.
   *  Function can fail for any reason constructing std::vector can fail.
   *  Will return a null packet if construction fails.
   *
   *  @param[out] pkt
   *  The dart_packet_t instance to be initialized.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] format
   *  The format string specifying the variadic arguments to expect.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   *  Note that errors in the format string cannot be detected and will likely lead to crashes.
   */
  DART_ABI_EXPORT dart_err_t dart_arr_init_va_rc_err(dart_packet_t* pkt, dart_rc_type_t rc, char const* format, ...);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as a string.
   *
   *  @details
   *  If the provided string is less than or equal to the small string optimization size,
   *  function cannot fail, otherwise function can fail due to memory allocation failure.
   *
   *  @param[in] str
   *  The null-terminated string to copy into the dart_packet instance.
   *
   *  @return
   *  The dart_packet instance initialized as a string, or null on error.
   */
  DART_ABI_EXPORT dart_packet_t dart_str_init(char const* str);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as a string.
   *
   *  @details
   *  If the provided string is less than or equal to the small string optimization size,
   *  function cannot fail, otherwise function can fail due to memory allocation failure.
   *
   *  @param[out] pkt
   *  The dart_packet_t instance to be initialized.
   *
   *  @param[in] str
   *  The null-terminated string to copy into the dart_packet instance.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_str_init_err(dart_packet_t* pkt, char const* str);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as a string with an explicit size.
   *
   *  @details
   *  If the provided string is less than or equal to the small string optimization size,
   *  function cannot fail, otherwise function can fail due to memory allocation failure.
   *  Function is useful when given string is not known to be terminated, or is otherwise
   *  untrusted.
   *
   *  @param[in] str
   *  The null-terminated string to copy into the dart_packet instance.
   *
   *  @param[in] len
   *  The length of the string in bytes.
   *
   *  @return
   *  Th dart_packet instance initialized as a string, or null on error.
   */
  DART_ABI_EXPORT dart_packet_t dart_str_init_len(char const* str, size_t len);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as a string with an explicit size.
   *
   *  @details
   *  If the provided string is less than or equal to the small string optimization size,
   *  function cannot fail, otherwise function can fail due to memory allocation failure.
   *  Function is useful when given string is not known to be terminated, or is otherwise
   *  untrusted.
   *
   *  @param[out] pkt
   *  The dart_packet_t instance to be initialized.
   *
   *  @param[in] str
   *  The null-terminated string to copy into the dart_packet instance.
   *
   *  @param[in] len
   *  The length of the string in bytes.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_str_init_len_err(dart_packet_t* pkt, char const* str, size_t len);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as a string with an explicit
   *  reference counter implementation.
   *
   *  @details
   *  If the provided string is less than or equal to the small string optimization size,
   *  function cannot fail, otherwise function can fail due to memory allocation failure.
   *  Function is useful when given string is not known to be terminated, or is otherwise
   *  untrusted.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] str
   *  The null-terminated string to copy into the dart_packet instance.
   *
   *  @return
   *  Th dart_packet instance initialized as a string, or null on error.
   */
  DART_ABI_EXPORT dart_packet_t dart_str_init_rc(dart_rc_type_t rc, char const* str);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as a string with an explicit
   *  reference counter implementation.
   *
   *  @details
   *  If the provided string is less than or equal to the small string optimization size,
   *  function cannot fail, otherwise function can fail due to memory allocation failure.
   *  Function is useful when given string is not known to be terminated, or is otherwise
   *  untrusted.
   *
   *  @param[out] pkt
   *  The dart_packet_t instance to be initialized.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] str
   *  The null-terminated string to copy into the dart_packet instance.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_str_init_rc_err(dart_packet_t* pkt, dart_rc_type_t rc, char const* str);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as a string with an explicit size
   *  and reference counter implementation.
   *
   *  @details
   *  If the provided string is less than or equal to the small string optimization size,
   *  function cannot fail, otherwise function can fail due to memory allocation failure.
   *  Function is useful when given string is not known to be terminated, or is otherwise
   *  untrusted.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] str
   *  The null-terminated string to copy into the dart_packet instance.
   *
   *  @param[in] len
   *  The length of the string in bytes.
   *
   *  @return
   *  Th dart_packet instance initialized as a string, or null on error.
   */
  DART_ABI_EXPORT dart_packet_t dart_str_init_rc_len(dart_rc_type_t rc, char const* str, size_t len);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as a string with an explicit size
   *  and reference counter implementation.
   *
   *  @details
   *  If the provided string is less than or equal to the small string optimization size,
   *  function cannot fail, otherwise function can fail due to memory allocation failure.
   *  Function is useful when given string is not known to be terminated, or is otherwise
   *  untrusted.
   *
   *  @param[out] pkt
   *  The dart_packet_t instance to be initialized.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] str
   *  The null-terminated string to copy into the dart_packet instance.
   *
   *  @param[in] len
   *  The length of the string in bytes.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_str_init_rc_len_err(dart_packet_t* pkt, dart_rc_type_t rc, char const* str, size_t len);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as an integer.
   *
   *  @details
   *  Function cannot meaningfully fail.
   *
   *  @param[in] val
   *  The integer value to copy into the dart_packet instance.
   *
   *  @return
   *  The dart_packet instance initialized as an integer, or null on error.
   */
  DART_ABI_EXPORT dart_packet_t dart_int_init(int64_t val);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as an integer.
   *
   *  @details
   *  Function cannot meaningfully fail. Function exists for API uniformity.
   *
   *  @param[out] pkt
   *  The dart_packet_t instance to be initialized.
   *
   *  @param[in] val
   *  The integer value to copy into the dart_packet instance.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_int_init_err(dart_packet_t* pkt, int64_t val);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as an integer with an
   *  explicitly set reference counter type.
   *
   *  @details
   *  Function cannot meaningfully fail.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] val
   *  The integer value to copy into the dart_packet instance.
   *
   *  @return
   *  The dart_packet instance initialized as an integer, or null on error.
   */
  DART_ABI_EXPORT dart_packet_t dart_int_init_rc(dart_rc_type_t rc, int64_t val);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as an integer.
   *
   *  @details
   *  Function cannot meaningfully fail. Function exists for API uniformity.
   *
   *  @param[out] pkt
   *  The dart_packet_t instance to be initialized.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] val
   *  The integer value to copy into the dart_packet instance.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_int_init_rc_err(dart_packet_t* pkt, dart_rc_type_t rc, int64_t val);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as a decimal.
   *
   *  @details
   *  Function cannot meaningfully fail.
   *
   *  @param[in] val
   *  The double value to copy into the dart_packet instance.
   *
   *  @return
   *  The dart_packet instance initialized as a decimal, or null on error.
   */
  DART_ABI_EXPORT dart_packet_t dart_dcm_init(double val);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as a decimal.
   *
   *  @details
   *  Function cannot meaningfully fail. Function exists for API uniformity.
   *
   *  @param[out] pkt
   *  The dart_packet_t instance to be initialized.
   *
   *  @param[in] val
   *  The double value to copy into the dart_packet instance.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_dcm_init_err(dart_packet_t* pkt, double val);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as a decimal with an
   *  explicitly set reference counter type.
   *
   *  @details
   *  Function cannot meaningfully fail.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] val
   *  The double value to copy into the dart_packet instance.
   *
   *  @return
   *  The dart_packet instance initialized as a decimal, or null on error.
   */
  DART_ABI_EXPORT dart_packet_t dart_dcm_init_rc(dart_rc_type_t rc, double val);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as a decimal with an
   *  explicitly set reference counter type.
   *
   *  @details
   *  Function cannot meaningfully fail. Function exists for API uniformity.
   *
   *  @param[out] pkt
   *  The dart_packet_t instance to be initialized.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] val
   *  The double value to copy into the dart_packet instance.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_dcm_init_rc_err(dart_packet_t* pkt, dart_rc_type_t rc, double val);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as a boolean.
   *
   *  @details
   *  Function cannot meaningfully fail.
   *
   *  @param[in] val
   *  The boolean value to copy into the dart_packet instance.
   *
   *  @return
   *  The dart_packet instance initialized as a boolean, or null on error.
   */
  DART_ABI_EXPORT dart_packet_t dart_bool_init(int val);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as a boolean.
   *
   *  @details
   *  Function cannot meaningfully fail. Function exists for API uniformity.
   *
   *  @param[out] pkt
   *  The dart_packet_t instance to be initialized.
   *
   *  @param[in] val
   *  The boolean value to copy into the dart_packet instance.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_bool_init_err(dart_packet_t* pkt, int val);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as a boolean with
   *  an explicitly set reference counter type.
   *
   *  @details
   *  Function cannot meaningfully fail.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] val
   *  The boolean value to copy into the dart_packet instance.
   *
   *  @return
   *  The dart_packet instance initialized as a boolean, or null on error.
   */
  DART_ABI_EXPORT dart_packet_t dart_bool_init_rc(dart_rc_type_t rc, int val);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as a boolean with
   *  an explicitly set reference counter type.
   *
   *  @details
   *  Function cannot meaningfully fail. Function exists for API uniformity.
   *
   *  @param[out] pkt
   *  The dart_packet_t instance to be initialized.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] val
   *  The boolean value to copy into the dart_packet instance.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_bool_init_rc_err(dart_packet_t* pkt, dart_rc_type_t rc, int val);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as null.
   *
   *  @details
   *  Function cannot meaningfully fail.
   *
   *  @return
   *  The dart_packet instance initialized as null.
   */
  DART_ABI_EXPORT dart_packet_t dart_null_init();

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as null.
   *
   *  @details
   *  Function cannot meaningfully fail. Function exists for API uniformity.
   *
   *  @return
   *  DART_NO_ERROR.
   */
  DART_ABI_EXPORT dart_err_t dart_null_init_err(dart_packet_t* pkt);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as null with
   *  an explicitly set reference counter type.
   *
   *  @details
   *  Function cannot meaningfully fail.
   *
   *  @return
   *  The dart_packet instance initialized as null.
   */
  DART_ABI_EXPORT dart_packet_t dart_null_init_rc(dart_rc_type_t rc);

  /**
   *  @brief
   *  Function is used to construct a dart_packet_t instance as null with an
   *  explicitly set reference counter type.
   *
   *  @details
   *  Function cannot meaningfully fail. Function exists for API uniformity.
   *
   *  @return
   *  DART_NO_ERROR.
   */
  DART_ABI_EXPORT dart_err_t dart_null_init_rc_err(dart_packet_t* pkt, dart_rc_type_t rc);
  
  /*----- Generic Mutation Operations -----*/

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given null-terminated key and previously constructed dart_packet_t value.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in] val
   *  The previously initialized dart_packet_t instance (of any type) to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_insert_dart(void* dst, char const* key, void const* val);

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given, possibly unterminated, key and previously constructed dart_packet_t value.
   *
   *  @details
   *  Function is behaviorally identical to dart_packet_obj_insert_heap, but can be used when the
   *  incoming key is either not known to be terminated or is otherwise untrusted.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The potentially unterminated string to use as a key for this key-value pair.
   *
   *  @param[in] len
   *  The length of the key to be inserted.
   *
   *  @param[in,out] val
   *  The previously initialized dart_packet_t instance (of any type) to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_insert_dart_len(void* dst, char const* key, size_t len, void const* val);

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given null-terminated key and previously constructed dart_packet_t value.
   *
   *  @details
   *  Function takes ownership of ("steals") the resources referenced by val according
   *  to C++ move-semantics, potentially avoiding a reference increment at the cost of
   *  resetting val to null.
   *
   *  @remarks
   *  The post-conditions of this function are that the resources referenced by val will
   *  have been inserted into pkt, and the instance val will have been reset to null, as
   *  if it had been destroyed and then default constructed.
   *  Formally speaking, val is still a live object and must be destroyed, but it is
   *  guaranteed not to leak resources if not passed through dart_destroy.
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in,out] val
   *  The previously initialized dart_packet_t instance to take as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_insert_take_dart(void* dst, char const* key, void* val);

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given, possibly unterminated, key and previously constructed dart_packet_t value.
   *
   *  @details
   *  Function is behaviorally identical to dart_packet_obj_insert_take_heap, but can be used
   *  when the incoming key is either not known to be terminated or is otherwise untrusted.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in] len
   *  The length of the key to be inserted.
   *
   *  @param[in,out] val
   *  The previously initialized dart_packet_t instance to take as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_insert_take_dart_len(void* dst, char const* key, size_t len, void* val);

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given pair of null-terminated strings.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in] val
   *  The null-terminated string to use as a value for this key-value pair.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_insert_str(void* dst, char const* key, char const* val);

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given pair of, possibly unterminated, strings.
   *
   *  @details
   *  Function is behaviorally identical to dart_packet_obj_insert_str, but can be used
   *  when the incoming pair of strings is either not known to be terminated or is
   *  otherwise untrusted.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The potentially unterminated string to use as a key for this key-value pair.
   *
   *  @param[in] val
   *  The potentially unterminated string to use as a value for this key-value pair.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_insert_str_len(void* dst, char const* key, size_t len, char const* val, size_t val_len);

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given null-terminated key and integer value.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in] val
   *  The integer to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_insert_int(void* dst, char const* key, int64_t val);

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given, potentially unterminated, key and integer value.
   *
   *  @details
   *  Function is behaviorally identical to dart_packet_obj_insert_int, but can be used
   *  when the incoming key is either not known to be terminated or is otherwise untrusted.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The potentially unterminated string to use as a key for this key-value pair.
   *
   *  @param[in] len
   *  The length of the key to be inserted.
   *
   *  @param[in] val
   *  The integer to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_insert_int_len(void* dst, char const* key, size_t len, int64_t val);

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given null-terminated key and decimal value.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in] val
   *  The decimal to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_insert_dcm(void* dst, char const* key, double val);

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given, potentially unterminated, key and decimal value.
   *
   *  @details
   *  Function is behaviorally identical to dart_packet_obj_insert_dcm, but can be used when the
   *  incoming key is either not known to be terminated or is otherwise untrusted.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The, potentially unterminated, string to use as a key for this key-value pair.
   *
   *  @param[in] len
   *  The length of the key to be inserted.
   *
   *  @param[in] val
   *  The decimal to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_insert_dcm_len(void* dst, char const* key, size_t len, double val);

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given null-terminated key and boolean value.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in] val
   *  The boolean to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_insert_bool(void* dst, char const* key, int val);

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given, potentially unterminated, key and boolean value.
   *
   *  @details
   *  Function is behaviorally identical to dart_packet_obj_insert_bool, but can be used when the
   *  incoming key is either not known to be terminated or is otherwise untrusted.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The potentially unterminated string to use as a key for this key-value pair.
   *
   *  @param[in] len
   *  The length of the key to be inserted.
   *
   *  @param[in] val
   *  The boolean to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_insert_bool_len(void* dst, char const* key, size_t len, int val);

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given null-terminated key and null value.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_insert_null(void* dst, char const* key);

  /**
   *  @brief
   *  Function is used to create a new key-value mapping within the given object
   *  for the given, potentially unterminated, key and null value.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The potentially unterminated string to use as a key for this key-value pair.
   *
   *  @param[in] len
   *  The length of the key to be inserted.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_insert_null_len(void* dst, char const* key, size_t len);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given null-terminated key and previously constructed dart_packet_t value.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in] val
   *  The previously initialized dart_packet_t instance (of any type) to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_set_dart(void* dst, char const* key, void const* val);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given, possibly unterminated, key and previously constructed dart_packet_t value.
   *
   *  @details
   *  Function is behaviorally identical to dart_packet_obj_set_heap, but can be used when the
   *  incoming key is either not known to be terminated or is otherwise untrusted.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The potentially unterminated string to use as a key for this key-value pair.
   *
   *  @param[in] len
   *  The length of the key to be inserted.
   *
   *  @param[in,out] val
   *  The previously initialized dart_packet_t instance (of any type) to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_set_dart_len(void* dst, char const* key, size_t len, void const* val);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given null-terminated key and previously constructed dart_packet_t value.
   *
   *  @details
   *  Function takes ownership of ("steals") the resources referenced by val according
   *  to C++ move-semantics, potentially avoiding a reference increment at the cost of
   *  resetting val to null.
   *
   *  @remarks
   *  The post-conditions of this function are that the resources referenced by val will
   *  have been inserted into pkt, and the instance val will have been reset to null, as
   *  if it had been destroyed and then default constructed.
   *  Formally speaking, val is still a live object and must be destroyed, but it is
   *  guaranteed not to leak resources if not passed through dart_destroy.
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in,out] val
   *  The previously initialized dart_packet_t instance to take as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_set_take_dart(void* dst, char const* key, void* val);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given, possibly unterminated, key and previously constructed dart_packet_t value.
   *
   *  @details
   *  Function is behaviorally identical to dart_packet_obj_set_take_heap, but can be used
   *  when the incoming key is either not known to be terminated or is otherwise untrusted.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in] len
   *  The length of the key to be inserted.
   *
   *  @param[in,out] val
   *  The previously initialized dart_packet_t instance to take as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_set_take_dart_len(void* dst, char const* key, size_t len, void* val);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given pair of null-terminated strings.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in] val
   *  The null-terminated string to use as a value for this key-value pair.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_set_str(void* dst, char const* key, char const* val);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given pair of, possibly unterminated, strings.
   *
   *  @details
   *  Function is behaviorally identical to dart_packet_obj_set_str, but can be used
   *  when the incoming pair of strings is either not known to be terminated or is
   *  otherwise untrusted.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The potentially unterminated string to use as a key for this key-value pair.
   *
   *  @param[in] val
   *  The potentially unterminated string to use as a value for this key-value pair.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_set_str_len(void* dst, char const* key, size_t len, char const* val, size_t val_len);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given null-terminated key and integer value.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in] val
   *  The integer to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_set_int(void* dst, char const* key, int64_t val);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given, potentially unterminated, key and integer value.
   *
   *  @details
   *  Function is behaviorally identical to dart_packet_obj_set_int, but can be used
   *  when the incoming key is either not known to be terminated or is otherwise untrusted.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The potentially unterminated string to use as a key for this key-value pair.
   *
   *  @param[in] len
   *  The length of the key to be inserted.
   *
   *  @param[in] val
   *  The integer to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_set_int_len(void* dst, char const* key, size_t len, int64_t val);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given null-terminated key and decimal value.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in] val
   *  The decimal to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_set_dcm(void* dst, char const* key, double val);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given, potentially unterminated, key and decimal value.
   *
   *  @details
   *  Function is behaviorally identical to dart_packet_obj_set_dcm, but can be used when the
   *  incoming key is either not known to be terminated or is otherwise untrusted.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The, potentially unterminated, string to use as a key for this key-value pair.
   *
   *  @param[in] len
   *  The length of the key to be inserted.
   *
   *  @param[in] val
   *  The decimal to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_set_dcm_len(void* dst, char const* key, size_t len, double val);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given null-terminated key and boolean value.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @param[in] val
   *  The boolean to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_set_bool(void* dst, char const* key, int val);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given, potentially unterminated, key and boolean value.
   *
   *  @details
   *  Function is behaviorally identical to dart_packet_obj_set_bool, but can be used when the
   *  incoming key is either not known to be terminated or is otherwise untrusted.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The potentially unterminated string to use as a key for this key-value pair.
   *
   *  @param[in] len
   *  The length of the key to be inserted.
   *
   *  @param[in] val
   *  The boolean to use as a value.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_set_bool_len(void* dst, char const* key, size_t len, int val);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given null-terminated key and null value.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The null-terminated string to use as a key for this key-value pair.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_set_null(void* dst, char const* key);

  /**
   *  @brief
   *  Function is used to update an existing key-value mapping within the given object
   *  for the given, potentially unterminated, key and null value.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to insert into.
   *
   *  @param[in] key
   *  The potentially unterminated string to use as a key for this key-value pair.
   *
   *  @param[in] len
   *  The length of the key to be inserted.
   *
   *  @return
   *  Whether anything went wrong during insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_set_null_len(void* dst, char const* key, size_t len);

  /**
   *  @brief
   *  Function is used to clear an existing object of any/all key-value pairs.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to clear.
   *
   *  @return
   *  Whether anything went wrong during the clear.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_clear(void* dst);

  /**
   *  @brief
   *  Function is used to remove an individual key-value mapping from the given object.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to remove from.
   *
   *  @param[in] key
   *  The null-terminated string to use for removal.
   *
   *  @return
   *  Whether anything went wrong during the removal.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_erase(void* dst, char const* key);

  /**
   *  @brief
   *  Function is used to remove an individual key-value mapping from the given object.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The object to remove from.
   *
   *  @param[in] key
   *  The potentially unterminated string to use for removal.
   *
   *  @param[in] len
   *  The length of the key to be removed.
   *
   *  @return
   *  Whether anything went wrong during the removal.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_erase_len(void* dst, char const* key, size_t len);

  /**
   *  @brief
   *  Function is used to insert a new value within the given array at the specified index,
   *  using a previously initialized dart_packet_t instance.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The array to insert into
   *
   *  @param[in] idx
   *  The index to insert at.
   *
   *  @param[in] val
   *  The previously initialized dart_packet_t instance to insert.
   *
   *  @return
   *  Whether anything went wrong during the insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_arr_insert_dart(void* dst, size_t idx, void const* val);

  /**
   *  @brief
   *  Function is used to insert a new value within the given array at the specified index,
   *  using a previously initialized dart_packet_t instance.
   *
   *  @details
   *  Function takes ownership of ("steals") the resources referenced by val according
   *  to C++ move-semantics, potentially avoiding a reference increment at the cost of
   *  resetting val to null.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The array to insert into
   *
   *  @param[in] idx
   *  The index to insert at.
   *
   *  @param[in,out] val
   *  The previously initialized dart_packet_t instance to take as a value.
   *
   *  @return
   *  Whether anything went wrong during the insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_arr_insert_take_dart(void* dst, size_t idx, void* val);

  /**
   *  @brief
   *  Function is used to insert the given null-terminated string within the given array
   *  at the specified index.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The array to insert into
   *
   *  @param[in] idx
   *  The index to insert at
   *
   *  @param[in] val
   *  The string to insert into the array
   *
   *  @return
   *  Whether anything went wrong during the insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_arr_insert_str(void* dst, size_t idx, char const* val);

  /**
   *  @brief
   *  Function is used to insert the given, potentially unterminated, string within the given array
   *  at the specified index.
   *
   *  @details
   *  Function is behaviorally identical to dart_packet_arr_insert_str, but can be used when the
   *  incoming string is either not known to be terminated or is otherwise untrusted.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The array to insert into
   *
   *  @param[in] idx
   *  The index to insert at
   *
   *  @param[in] val
   *  The string to insert into the array
   *
   *  @param[in] val_len
   *  The length of the string to insert into the array.
   *
   *  @return
   *  Whether anything went wrong during the insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_arr_insert_str_len(void* dst, size_t idx, char const* val, size_t val_len);

  /**
   *  @brief
   *  Function is used to insert the given integer within the given array, at the specified index.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The array to insert into
   *
   *  @param[in] idx
   *  The index to insert at
   *
   *  @param[in] val
   *  The integer to insert.
   *
   *  @return
   *  Whether anything went wrong during the insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_arr_insert_int(void* dst, size_t idx, int64_t val);

  /**
   *  @brief
   *  Function is used to insert the given decimal within the given array, at the specified index.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The array to insert into
   *
   *  @param[in] idx
   *  The index to insert at
   *
   *  @param[in] val
   *  The decimal to insert.
   *
   *  @return
   *  Whether anything went wrong during the insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_arr_insert_dcm(void* dst, size_t idx, double val);

  /**
   *  @brief
   *  Function is used to insert the given boolean within the given array, at the specified index.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The array to insert into
   *
   *  @param[in] idx
   *  The index to insert at
   *
   *  @param[in] val
   *  The boolean to insert.
   *
   *  @return
   *  Whether anything went wrong during the insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_arr_insert_bool(void* dst, size_t idx, int val);

  /**
   *  @brief
   *  Function is used to insert null within the given array, at the specified index.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The array to insert into
   *
   *  @param[in] idx
   *  The index to insert at
   *
   *  @return
   *  Whether anything went wrong during the insertion.
   */
  DART_ABI_EXPORT dart_err_t dart_arr_insert_null(void* dst, size_t idx);

  /**
   *  @brief
   *  Function is used to set an existing index within the given array to a
   *  previously initialized dart_packet_t instance.
   *
   *  @param[in] pkt
   *  The array to assign into
   *
   *  @param[in] idx
   *  The index to assign to.
   *
   *  @param[in] val
   *  The previously initialized dart_packet_t instance
   *
   *  @return
   *  Whether anything went wrong during the assignment.
   */
  DART_ABI_EXPORT dart_err_t dart_arr_set_dart(void* dst, size_t idx, void const* val);

  /**
   *  @brief
   *  Function is used to set an existing index within the given array to a
   *  previously initialized dart_packet_t instance.
   *
   *  @details
   *  Function takes ownership of ("steals") the resources referenced by val according
   *  to C++ move-semantics, potentially avoiding a reference increment at the cost of
   *  resetting val to null.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The array to assign into
   *
   *  @param[in] idx
   *  The index to assign to.
   *
   *  @param[in,out] val
   *  The previously initialized dart_packet_t instance to take as a value.
   *
   *  @return
   *  Whether anything went wrong during the assignment.
   */
  DART_ABI_EXPORT dart_err_t dart_arr_set_take_dart(void* dst, size_t idx, void* val);

  /**
   *  @brief
   *  Function is used to set an existing index within the given array to the given
   *  null-terminated string.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The array to assign into
   *
   *  @param[in] idx
   *  The index to assign to
   *
   *  @param[in] val
   *  The string to set in the array
   *
   *  @return
   *  Whether anything went wrong during the assignment.
   */
  DART_ABI_EXPORT dart_err_t dart_arr_set_str(void* dst, size_t idx, char const* val);

  /**
   *  @brief
   *  Function is used to set an existing index within the given array to the given,
   *  potentially unterminated, string.
   *
   *  @details
   *  Function is behaviorally identical to dart_packet_arr_set_str, but can be used when the
   *  incoming string is either not known to be terminated or is otherwise untrusted.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The array to assign into
   *
   *  @param[in] idx
   *  The index to assign to
   *
   *  @param[in] val
   *  The string to set in the array
   *
   *  @param[in] val_len
   *  The length of the string to set in the array.
   *
   *  @return
   *  Whether anything went wrong during the assignment.
   */
  DART_ABI_EXPORT dart_err_t dart_arr_set_str_len(void* dst, size_t idx, char const* val, size_t val_len);

  /**
   *  @brief
   *  Function is used to set an existing index with the given array to the given integer
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The array to assign into
   *
   *  @param[in] idx
   *  The index to assign to
   *
   *  @param[in] val
   *  The integer to assign.
   *
   *  @return
   *  Whether anything went wrong during the assignment.
   */
  DART_ABI_EXPORT dart_err_t dart_arr_set_int(void* dst, size_t idx, int64_t val);

  /**
   *  @brief
   *  Function is used to set an existing index within the given array to the given decimal
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The array to assign into
   *
   *  @param[in] idx
   *  The index to assign to
   *
   *  @param[in] val
   *  The decimal to assign.
   *
   *  @return
   *  Whether anything went wrong during the assignment.
   */
  DART_ABI_EXPORT dart_err_t dart_arr_set_dcm(void* dst, size_t idx, double val);

  /**
   *  @brief
   *  Function is used to set an existing index within the given array to the given boolean
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The array to assign into
   *
   *  @param[in] idx
   *  The index to assign to
   *
   *  @param[in] val
   *  The boolean to assign.
   *
   *  @return
   *  Whether anything went wrong during the assignment.
   */
  DART_ABI_EXPORT dart_err_t dart_arr_set_bool(void* dst, size_t idx, int val);

  /**
   *  @brief
   *  Function is used to assign null to an existing index within the given array
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The array to assign into
   *
   *  @param[in] idx
   *  The index to assign to
   *
   *  @return
   *  Whether anything went wrong during the assignment.
   */
  DART_ABI_EXPORT dart_err_t dart_arr_set_null(void* dst, size_t idx);

  /**
   *  @brief
   *  Function is used to clear an existing array of all values.
   *
   *  @param[in] pkt
   *  The array to clear.
   *
   *  @return
   *  Whether anything went wrong during the clear.
   */
  DART_ABI_EXPORT dart_err_t dart_arr_clear(void* pkt);

  /**
   *  @brief
   *  Function is used to "remove" an individual index from the given array,
   *  shifting all higher indices down.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The array to remove from.
   *
   *  @param[in] idx
   *  The index to "remove."
   *
   *  @return
   *  Whether anything went wrong during the removal.
   */
  DART_ABI_EXPORT dart_err_t dart_arr_erase(void* pkt, size_t idx);

  /**
   *  @brief
   *  Function is used to resize the array to the given length, dropping any indexes
   *  off the end if shrinking the array, and initializing any new indexes to null
   *  if growing the array.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] dst
   *  The array to resize
   *
   *  @param[in] len
   *  The new intended length of the array.
   *
   *  @return
   *  Whether anything went wrong during the resize.
   */
  DART_ABI_EXPORT dart_err_t dart_arr_resize(void* dst, size_t len);

  /**
   *  @brief
   *  Function is used to increase the size of the underlying storage medium
   *  of the given array, without changing the number of elements it logically
   *  contains.
   *
   *  @details
   *  Function can be useful to ensure a particular call to push_back or the like
   *  will be constant time.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The array to reserve space for.
   *
   *  @param[in] len
   *  The desired length of the underlying storage medium.
   *
   *  @return
   *  Whether anything went wrong during the reserve operation.
   */
  DART_ABI_EXPORT dart_err_t dart_arr_reserve(void* dst, size_t len);

  /*----- Generic Retrieval Operations -----*/

  /**
   *  @brief
   *  Function is used to check whether a key exists in a given object.
   *
   *  @remarks
   *  Can be useful to check whether a key is present without having to incur
   *  reference count overhead. Also can be useful to distinguish between an explicit
   *  null in an object and a key which is actually missing.
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] src
   *  The object to query.
   *
   *  @param[in] key
   *  The key whose existence should be queried
   *
   *  @return
   *  Whether the given key is present.
   */
  DART_ABI_EXPORT int dart_obj_has_key(void const* src, char const* key);

  /**
   *  @brief
   *  Function is used to check whether a key exists in a given object.
   *
   *  @details
   *  Function is behaviorally identical to dart_packet_obj_has_key, but can be used when the
   *  incoming string is either not known to be terminated or is otherwise untrusted.
   *
   *  @remarks
   *  Can be useful to check whether a key is present without having to incur
   *  reference count overhead. Also can be useful to distinguish between an explicit
   *  null in an object and a key which is actually missing.
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] src
   *  The object to query.
   *
   *  @param[in] key
   *  The key whose existence should be queried
   *
   *  @param[in] len
   *  The length of the key in bytes.
   *
   *  @return
   *  Whether the given key is present.
   */
  DART_ABI_EXPORT int dart_obj_has_key_len(void const* src, char const* key, size_t len);

  /**
   *  @brief
   *  Function is used to retrieve the value for a given null-terminated key from a given object.
   *
   *  @details
   *  Function returns null instances for non-existent keys without modifying the object.
   *  Lookup is based on std::map and should be log(N).
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] src
   *  The source object to query from.
   *
   *  @param[in] key
   *  The null-terminated key to locate within the given object.
   *
   *  @return
   *  The dart_packet_t instance corresponding to the given key, or null in error.
   */
  DART_ABI_EXPORT dart_packet_t dart_obj_get(void const* src, char const* key);

  /**
   *  @brief
   *  Function is used to retrieve the value for a given null-terminated key from a given object.
   *
   *  @details
   *  Function returns null instances for non-existent keys without modifying the object.
   *  Lookup is based on std::map and should be log(N).
   *  Function expects to received uninitialized memory. If using a pointer to a previous Dart
   *  instance, it must be passed through an appropriate dart_destroy function first.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[out] dst
   *  The dart_packet_t instance that should be initialized with the result of the lookup.
   *
   *  @param[in] src
   *  The object instance to query from.
   *
   *  @param[in] key
   *  The null-terminated key to locate within the given object.
   *
   *  @return
   *  Whether anything went wrong during the lookup.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_get_err(dart_packet_t* dst, void const* src, char const* key);

  /**
   *  @brief
   *  Function is used to retrieve the value for a given, possibly unterminated, key from a given object.
   *
   *  @details
   *  Function returns null instances for non-existent keys without modifying the object.
   *  Lookup is based on std::map and should be log(N).
   *
   *  @param[in] src
   *  The source object to query from.
   *
   *  @param[in] key
   *  The possibly unterminated key to locate within the given object.
   *
   *  @return
   *  The dart_packet_t instance corresponding to the given key, or null in error.
   */
  DART_ABI_EXPORT dart_packet_t dart_obj_get_len(void const* src, char const* key, size_t len);

  /**
   *  @brief
   *  Function is used to retrieve the value for a given, possibly unterminated, key from a given object.
   *
   *  @details
   *  Function returns null instances for non-existent keys without modifying the object.
   *  Lookup is based on std::map and should be log(N).
   *  Function expects to received uninitialized memory. If using a pointer to a previous Dart
   *  instance, it must be passed through an appropriate dart_destroy function first.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[out] dst
   *  The dart_packet_t instance that should be initialized with the result of the lookup.
   *
   *  @param[in] src
   *  The object instance to query from.
   *
   *  @param[in] key
   *  The possibly unterminated key to locate within the given object.
   *
   *  @return
   *  Whether anything went wrong during the lookup.
   */
  DART_ABI_EXPORT dart_err_t dart_obj_get_len_err(dart_packet_t* dst, void const* src, char const* key, size_t len);

  /**
   *  @brief
   *  Function is used to retrieve the value for a given index within a given array.
   *
   *  @details
   *  Function returns null instances for non-existent indices without modifying the array.
   *
   *  @remarks
   *  Returning null from an out-of-bounds access is a potentially questionable move, but
   *  it was done to be in better behavioral conformance with object lookup, and also
   *  to behave more similarly to std::vector by not throwing exceptions (although also
   *  not causing ub).
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] src
   *  The array to lookup into.
   *
   *  @param[in] idx
   *  The index to lookup.
   *
   *  @return
   *  The dart_packet_t instance corresponding to the given index, or null in error.
   */
  DART_ABI_EXPORT dart_packet_t dart_arr_get(void const* src, size_t idx);

  /**
   *  @brief
   *  Function is used to retrieve the value for a given index within a given array.
   *
   *  @details
   *  Function returns null instances for non-existent indices without modifying the array.
   *
   *  @remarks
   *  Returning null from an out-of-bounds access is a potentially questionable move, but
   *  it was done to be in better behavioral conformance with object lookup, and also
   *  to behave more similarly to std::vector by not throwing exceptions (although also
   *  not causing ub).
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[out] dst
   *  The dart_packet_t instance that should be initialized with the result of this lookup.
   *
   *  @param[in] src
   *  The array to lookup into.
   *
   *  @param[in] idx
   *  The index to lookup.
   *
   *  @return
   *  Whether anything went wrong during the lookup.
   */
  DART_ABI_EXPORT dart_err_t dart_arr_get_err(dart_packet_t* dst, void const* src, size_t idx);

  /**
   *  @brief
   *  Unwraps a given dart_packet string instance.
   *
   *  @details
   *  String is gauranteed to be terminated, but may contain additional nulls.
   *  Use the sized overload of this function to simultaneously get the actual length.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] src
   *  The string instance to unwrap.
   *
   *  @return
   *  A pointer to the character data for the given string, or NULL on error.
   */
  DART_ABI_EXPORT char const* dart_str_get(void const* src);

  /**
   *  @brief
   *  Unwraps a given dart_packet string instance.
   *
   *  @details
   *  String is gauranteed to be terminated, but may contain additional nulls.
   *  For gauranteed correctness in the face of non-ASCII data, use the provided
   *  length out parameter.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] src
   *  The string instance to unwrap.
   *
   *  @param[out] len
   *  The length of the unwrapped string.
   *
   *  @return
   *  A pointer to the character data for the given string, or NULL on error.
   */
  DART_ABI_EXPORT char const* dart_str_get_len(void const* src, size_t* len);

  /**
   *  @brief
   *  Unwraps a given dart_packet integer instance.
   *
   *  @details
   *  Function returns zero on error, but could also return zero on success.
   *  If the type of the passed instance is not known to be integer, use the error overload
   *  to retrieve an error code.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] src
   *  The integer instance that should be unwrapped.
   *
   *  @return
   *  The integer value of the given instance.
   */
  DART_ABI_EXPORT int64_t dart_int_get(void const* src);

  /**
   *  @brief
   *  Unwraps a given dart_packet integer instance.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] src
   *  The integer instance that should be unwrapped.
   *
   *  @param[out] val
   *  A pointer to the integer that should be initialized to the unwrapped value.
   *
   *  @return
   *  Whether anything went wrong during the unwrap.
   */
  DART_ABI_EXPORT dart_err_t dart_int_get_err(void const* src, int64_t* val);

  /**
   *  @brief
   *  Unwraps a given dart_packet decimal instance.
   *
   *  @details
   *  Function returns zero on error, but could also return zero on success.
   *  If the type of the passed instance is not known to be decimal, use the error overload
   *  to retrieve an error code.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] src
   *  The decimal instance that should be unwrapped.
   *
   *  @return
   *  The decimal value of the given instance.
   */
  DART_ABI_EXPORT double dart_dcm_get(void const* src);

  /**
   *  @brief
   *  Unwraps a given dart_packet decimal instance.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] src
   *  The decimal instance that should be unwrapped.
   *
   *  @param[out] val
   *  A pointer to the decimal that should be initialized to the unwrapped value.
   *
   *  @return
   *  Whether anything went wrong during the unwrap.
   */
  DART_ABI_EXPORT dart_err_t dart_dcm_get_err(void const* src, double* val);

  /**
   *  @brief
   *  Unwraps a given dart_packet boolean instance.
   *
   *  @details
   *  Function returns zero on error, but could also return zero on success.
   *  If the type of the passed instance is not known to be boolean, use the error overload
   *  to retrieve an error code.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] src
   *  The boolean instance that should be unwrapped.
   *
   *  @return
   *  The boolean value of the given instance.
   */
  DART_ABI_EXPORT int dart_bool_get(void const* src);

  /**
   *  @brief
   *  Unwraps a given dart_packet boolean instance.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] src
   *  The boolean instance that should be unwrapped.
   *
   *  @param[out] val
   *  A pointer to the boolean that should be initialized to the unwrapped value.
   *
   *  @return
   *  Whether anything went wrong during the unwrap.
   */
  DART_ABI_EXPORT dart_err_t dart_bool_get_err(void const* src, int* val);

  /**
   *  @brief
   *  Function returns the size of a Dart aggregate (object or array) or string
   *  instance.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] src
   *  The instance whose length should be queried.
   *
   *  @return
   *  The size of the given aggregate/string.
   */
  DART_ABI_EXPORT size_t dart_size(void const* src);

  /**
   *  @brief
   *  Function recursively calculates equality for the given instances.
   *
   *  @details
   *  Disparate types always compare unequal, disparate reference counters always
   *  compare unequal, same types are recursively compared.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *  Function will even calculate equality across separate Dart implementation types.
   *  A call to this function with lhs as a dart_heap_t* and rhs as a dart_buffer_t*
   *  is well formed.
   *
   *  @param[in] lhs
   *  The "left hand side" of the expression.
   *
   *  @param[in] rhs
   *  The "right hand side" of the expression.
   *
   *  @return
   *  Whether the two instances were equivalent.
   */
  DART_ABI_EXPORT int dart_equal(void const* lhs, void const* rhs);

  /**
   *  @brief
   *  Function checks whether the given instance is of object type.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   */
  DART_ABI_EXPORT int dart_is_obj(void const* src);

  /**
   *  @brief
   *  Function checks whether the given instance is of array type.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   */
  DART_ABI_EXPORT int dart_is_arr(void const* src);

  /**
   *  @brief
   *  Function checks whether the given instance is of string type.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   */
  DART_ABI_EXPORT int dart_is_str(void const* src);

  /**
   *  @brief
   *  Function checks whether the given instance is of integer type.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   */
  DART_ABI_EXPORT int dart_is_int(void const* src);

  /**
   *  @brief
   *  Function checks whether the given instance is of decimal type.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   */
  DART_ABI_EXPORT int dart_is_dcm(void const* src);

  /**
   *  @brief
   *  Function checks whether the given instance is of boolean type.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   */
  DART_ABI_EXPORT int dart_is_bool(void const* src);

  /**
   *  @brief
   *  Function checks whether the given instance is null.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   */
  DART_ABI_EXPORT int dart_is_null(void const* src);

  /**
   *  @brief
   *  Function checks whether the underlying implementation type for this
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *  Dart instance is immutable or not.
   */
  DART_ABI_EXPORT int dart_is_finalized(void const* src);

  /**
   *  @brief
   *  Returns the type of the given instance.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   */
  DART_ABI_EXPORT dart_type_t dart_get_type(void const* src);

  /**
   *  @brief
   *  Return the current refcount for the given instance.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   */
  DART_ABI_EXPORT size_t dart_refcount(void const* src);
  
  /*----- Generic JSON Manipulation Functions -----*/

  /**
   *  @brief
   *  Function parses a given null-terminated JSON string and returns
   *  a handle to a Dart object hierarchy representing the given string.
   *
   *  @param[in] str
   *  A null-terminated JSON string.
   *
   *  @return
   *  The dart_packet_t instance representing the string.
   */
  DART_ABI_EXPORT dart_packet_t dart_from_json(char const* str);

  /**
   *  @brief
   *  Function parses a given null-terminated JSON string and initializes
   *  a handle to a Dart object hierarchy representing the given string.
   *
   *  @param[out] pkt
   *  The packet to initialize as a handle to the parsed representation.
   *
   *  @param[in] str
   *  A null-terminated JSON string.
   *
   *  @return
   *  Whether anything went wrong during parsing.
   */
  DART_ABI_EXPORT dart_err_t dart_from_json_err(dart_packet_t* dst, char const* str);

  /**
   *  @brief
   *  Function parses a given null-terminated JSON string and returns
   *  a handle to a Dart object hierarchy representing the given string, and using
   *  a specific reference counter type.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] str
   *  A null-terminated JSON string.
   *
   *  @return
   *  The dart_packet_t instance representing the string.
   */
  DART_ABI_EXPORT dart_packet_t dart_from_json_rc(dart_rc_type_t rc, char const* str);

  /**
   *  @brief
   *  Function parses a given null-terminated JSON string and initializes
   *  a handle to a Dart object hierarchy representing the given string, and using
   *  a specific reference counter type.
   *
   *  @param[out] pkt
   *  The packet to initialize as a handle to the parsed representation.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] str
   *  A null-terminated JSON string.
   *
   *  @return
   *  Whether anything went wrong during parsing.
   */
  DART_ABI_EXPORT dart_err_t dart_from_json_rc_err(dart_packet_t* dst, dart_rc_type_t rc, char const* str);

  /**
   *  @brief
   *  Function parses a given, possibly unterminated, JSON string and returns
   *  a handle to a Dart object hierarchy representing the given string.
   *
   *  @param[in] str
   *  A possibly unterminated JSON string.
   *
   *  @param[in] len
   *  The length of the given JSON string.
   *
   *  @return
   *  The dart_packet_t instance representing the string.
   */
  DART_ABI_EXPORT dart_packet_t dart_from_json_len(char const* str, size_t len);

  /**
   *  @brief
   *  Function parses a given, possibly unterminated, JSON string and initializes
   *  a handle to a Dart object hierarchy representing the given string.
   *
   *  @param[out] pkt
   *  The packet to initialize as a handle to the parsed representation.
   *
   *  @param[in] str
   *  A possibly unterminated JSON string.
   *
   *  @param[in] len
   *  The length of the given JSON string.
   *
   *  @return
   *  Whether anything went wrong during parsing.
   */
  DART_ABI_EXPORT dart_err_t dart_from_json_len_err(dart_packet_t* dst, char const* str, size_t len);

  /**
   *  @brief
   *  Function parses a given, possibly unterminated, JSON string and returns
   *  a handle to a Dart object hierarchy representing the given string, and using
   *  a specific reference counter type.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] str
   *  A possibly unterminated JSON string.
   *
   *  @param[in] len
   *  The length of the given JSON string.
   *
   *  @return
   *  The dart_packet_t instance representing the string.
   */
  DART_ABI_EXPORT dart_packet_t dart_from_json_len_rc(dart_rc_type_t rc, char const* str, size_t len);

  /**
   *  @brief
   *  Function parses a given, possibly unterminated, JSON string and initializes
   *  a handle to a Dart object hierarchy representing the given string, and using
   *  a specific reference counter type.
   *
   *  @param[out] pkt
   *  The packet to initialize as a handle to the parsed representation.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] str
   *  A possibly unterminated JSON string.
   *
   *  @param[in] len
   *  The length of the given JSON string.
   *
   *  @return
   *  Whether anything went wrong during parsing.
   */
  DART_ABI_EXPORT dart_err_t dart_from_json_len_rc_err(dart_packet_t* dst, dart_rc_type_t rc, char const* str, size_t len);

  /**
   *  @brief
   *  Function stringifies a given dart_packet_t instance into a valid JSON string.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The packet to stringify
   *
   *  @param[out] len
   *  The length of the resulting string.
   *
   *  @return
   *  The stringified result of the given dart_packet_t instance. Allocated with malloc, must be freed.
   */
  DART_ABI_EXPORT char* dart_to_json(void const* src, size_t* len);

  /*----- Generic API Transition Functions -----*/

  /**
   *  @brief
   *  Function generically takes any kind of Dart instance and returns a mutable,
   *  dynamic, representation.
   *
   *  @details
   *  If the incoming argument is a dart_heap_t instance, function is equivalent to dart_heap_copy.
   *  If the incoming argument is a dart_packet_t instance that IS NOT finalized,
   *  function is equivalent to dart_heap_copy.
   *  In other cases function is equivalent to dart_lift.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] src
   *  The source instance to lift.
   *
   *  @return
   *  The initialized dart_heap_t instance.
   */
  DART_ABI_EXPORT dart_heap_t dart_to_heap(void const* src);

  /**
   *  @brief
   *  Function generically takes any kind of Dart instance and returns a mutable,
   *  dynamic, representation.
   *
   *  @details
   *  If the incoming argument is a dart_heap_t instance, function is equivalent to dart_heap_copy.
   *  If the incoming argument is a dart_packet_t instance that IS NOT finalized,
   *  function is equivalent to dart_heap_copy.
   *  In other cases function is equivalent to dart_lift.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[out] dst
   *  The dart_heap_t instance to initialize.
   *
   *  @param[in] src
   *  The source instance to lift.
   *
   *  @return
   *  Whether anything went wrong during the conversion.
   */
  DART_ABI_EXPORT dart_err_t dart_to_heap_err(dart_heap_t* dst, void const* src);

  /**
   *  @brief
   *  Function generically takes any kind of Dart instance and returns an immutable,
   *  fixed, representation.
   *
   *  @details
   *  If the incoming argument is a dart_buffer_t instance, function is equivalent to dart_buffer_copy.
   *  If the incoming argument is a dart_packet_t instance that IS finalized,
   *  function is equivalent to dart_buffer_copy.
   *  In other cases function is equivalent to dart_lower.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] src
   *  The source instance to lower.
   *
   *  @return
   *  The initialized dart_buffer_t instance.
   */
  DART_ABI_EXPORT dart_buffer_t dart_to_buffer(void const* src);

  /**
   *  @brief
   *  Function generically takes any kind of Dart instance and returns an immutable,
   *  fixed, representation.
   *
   *  @details
   *  If the incoming argument is a dart_buffer_t instance, function is equivalent to dart_buffer_copy.
   *  If the incoming argument is a dart_packet_t instance that IS finalized,
   *  function is equivalent to dart_buffer_copy.
   *  In other cases function is equivalent to dart_lower.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[out] dst
   *  The dart_buffer_t instance to initialize.
   *
   *  @param[in] src
   *  The source instance to lower.
   *
   *  @return
   *  Whether anything went wrong during the conversion.
   */
  DART_ABI_EXPORT dart_err_t dart_to_buffer_err(dart_buffer_t* dst, void const* src);

  /**
   *  @brief
   *  Function will create a flat, serialized, network-ready, representation of the given dynamic
   *  object hierarchy.
   *
   *  @details
   *  Function serves as a go-between for the mutable and immutable, dynamic and network-ready,
   *  representations Dart uses.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The dynamic instance to be serialized.
   *
   *  @return
   *  The serialized, immutable, representation of the given instance, or null on error.
   */
  DART_ABI_EXPORT dart_packet_t dart_lower(void const* src);

  /**
   *  @brief
   *  Function will create a flat, serialized, network-ready, representation of the given dynamic
   *  object hierarchy.
   *
   *  @details
   *  Function serves as a go-between for the mutable and immutable, dynamic and network-ready,
   *  representations Dart uses.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[out] dst
   *  The dart_buffer_t instance to be initialized.
   *
   *  @param[in] pkt
   *  The dynamic instance to be serialized.
   *
   *  @return
   *  The serialized, immutable, representation of the given instance, or null on error.
   */
  DART_ABI_EXPORT dart_err_t dart_lower_err(dart_packet_t* dst, void const* src);

  /**
   *  @brief
   *  Function will create a flat, serialized, network-ready, representation of the given dynamic
   *  object hierarchy.
   *
   *  @details
   *  Function serves as a go-between for the mutable and immutable, dynamic and network-ready,
   *  representations Dart uses.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] pkt
   *  The dynamic instance to be serialized.
   *
   *  @return
   *  The serialized, immutable, representation of the given instance, or null on error.
   */
  DART_ABI_EXPORT dart_packet_t dart_finalize(void const* src);

  /**
   *  @brief
   *  Function will create a flat, serialized, network-ready, representation of the given dynamic
   *  object hierarchy.
   *
   *  @details
   *  Function serves as a go-between for the mutable and immutable, dynamic and network-ready,
   *  representations Dart uses.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[out] dst
   *  The dart_buffer_t instance to be initialized.
   *
   *  @param[in] pkt
   *  The dynamic instance to be serialized.
   *
   *  @return
   *  The serialized, immutable, representation of the given instance, or null on error.
   */
  DART_ABI_EXPORT dart_err_t dart_finalize_err(dart_packet_t* dst, void const* src);

  /**
   *  @brief
   *  Function will create a dynamic, mutable, representation of the given serialized object hierarchy.
   *
   *  @details
   *  Function serves as a go-between for the mutable and immutable, dynamic and network-ready,
   *  representations Dart uses.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] src
   *  The serialized instance to be serialized.
   *
   *  @return
   *  The dynamic, mutable, representation of the given instance, or null on error.
   */
  DART_ABI_EXPORT dart_packet_t dart_lift(void const* src);

  /**
   *  @brief
   *  Function will create a dynamic, mutable, representation of the given serialized object hierarchy.
   *
   *  @details
   *  Function serves as a go-between for the mutable and immutable, dynamic and network-ready,
   *  representations Dart uses.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[out] dst
   *  The dart_packet_t instance to be initialized.
   *
   *  @param[in] src
   *  The serialized instance to be serialized.
   *
   *  @return
   *  Whether anything went wrong during de-finalizing.
   */
  DART_ABI_EXPORT dart_err_t dart_lift_err(dart_packet_t* dst, void const* src);

  /**
   *  @brief
   *  Function will create a dynamic, mutable, representation of the given serialized object hierarchy.
   *
   *  @details
   *  Function serves as a go-between for the mutable and immutable, dynamic and network-ready,
   *  representations Dart uses.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] src
   *  The serialized instance to be serialized.
   *
   *  @return
   *  The dynamic, mutable, representation of the given instance, or null on error.
   */
  DART_ABI_EXPORT dart_packet_t dart_definalize(void const* src);

  /**
   *  @brief
   *  Function will create a dynamic, mutable, representation of the given serialized object hierarchy.
   *
   *  @details
   *  Function serves as a go-between for the mutable and immutable, dynamic and network-ready,
   *  representations Dart uses.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[out] dst
   *  The dart_packet_t instance to be initialized.
   *
   *  @param[in] src
   *  The serialized instance to be serialized.
   *
   *  @return
   *  Whether anything went wrong during de-finalizing.
   */
  DART_ABI_EXPORT dart_err_t dart_definalize_err(dart_packet_t* dst, void const* src);

  /**
   *  @brief
   *  Function returns a non-owning pointer to the underlying network buffer for a dart_buffer_t
   *  instance.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] src
   *  The instance to pull the buffer from.
   *
   *  @param[out] len
   *  An integer where the size of the buffer can be written.
   *
   *  @return
   *  The underlying network buffer.
   */
  DART_ABI_EXPORT void const* dart_get_bytes(void const* src, size_t* len);

  /**
   *  @brief
   *  Function returns an owning pointer to a copy of the underlying network buffer for a dart_buffer_t
   *  instance.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] src
   *  The instance to pull the buffer from.
   *
   *  @param[out] len
   *  An integer where the size of the buffer can be written.
   *
   *  @return
   *  The underlying network buffer. Was created via malloc, must be freed.
   */
  DART_ABI_EXPORT void* dart_dup_bytes(void const* src, size_t* len);

  /**
   *  @brief
   *  Function reconstructs a dart_buffer object from the network buffer of another.
   *
   *  @details
   *  The length parameter passed to the function may be larger than the underlying
   *  Dart buffer itself, but it must be valid for memcpy to read from all passed bytes,
   *  and len must be at least as large as the original buffer.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] bytes
   *  The underlying network buffer from a previous dart_buffer instance.
   *
   *  @param[in] len
   *  The length of the buffer being passed.
   *
   *  @return
   *  The reconstructed dart_buffer instance.
   */
  DART_ABI_EXPORT dart_packet_t dart_from_bytes(void const* bytes, size_t len);

  /**
   *  @brief
   *  Function reconstructs a dart_buffer object from the network buffer of another.
   *
   *  @details
   *  The length parameter passed to the function may be larger than the underlying
   *  Dart buffer itself, but it must be valid for memcpy to read from all passed bytes,
   *  and len must be at least as large as the original buffer.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[out] dst
   *  The dart_buffer_t instance to initialize.
   *
   *  @param[in] bytes
   *  The underlying network buffer from a previous dart_buffer instance.
   *
   *  @param[in] len
   *  The length of the buffer being passed.
   *
   *  @return
   *  Whether anything went wrong during reconstruction.
   */
  DART_ABI_EXPORT dart_err_t dart_from_bytes_err(dart_packet_t* dst, void const* bytes, size_t len);

  /**
   *  @brief
   *  Function reconstructs a dart_buffer object from the network buffer of another,
   *  with a specific reference counter.
   *
   *  @details
   *  The length parameter passed to the function may be larger than the underlying
   *  Dart buffer itself, but it must be valid for memcpy to read from all passed bytes,
   *  and len must be at least as large as the original buffer.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] bytes
   *  The underlying network buffer from a previous dart_buffer instance.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] len
   *  The length of the buffer being passed.
   *
   *  @return
   *  The reconstructed dart_buffer instance.
   */
  DART_ABI_EXPORT dart_packet_t dart_from_bytes_rc(void const* bytes, dart_rc_type_t rc, size_t len);

  /**
   *  @brief
   *  Function reconstructs a dart_buffer object from the network buffer of another,
   *  with a specific reference counter.
   *
   *  @details
   *  The length parameter passed to the function may be larger than the underlying
   *  Dart buffer itself, but it must be valid for memcpy to read from all passed bytes,
   *  and len must be at least as large as the original buffer.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[in] bytes
   *  The underlying network buffer from a previous dart_buffer instance.
   *
   *  @param[in] rc
   *  The reference counter implementation to use.
   *
   *  @param[in] len
   *  The length of the buffer being passed.
   *
   *  @return
   *  Whether anything went wrong during reconstruction.
   */
  DART_ABI_EXPORT dart_err_t dart_from_bytes_rc_err(dart_packet_t* dst, dart_rc_type_t rc, void const* bytes, size_t len);

  /**
   *  @brief
   *  Function takes ownership of, and reconstructs a dart_buffer object from,
   *  the network buffer of another.
   *
   *  @param[in] bytes
   *  The buffer to be moved in.
   *
   *  @return
   *  The reconstructed dart_buffer instance.
   */
  DART_ABI_EXPORT dart_packet_t dart_take_bytes(void* bytes);

  /**
   *  @brief
   *  Function takes ownership of, and reconstructs a dart_buffer object from,
   *  the network buffer of another.
   *
   *  @param[out] dst
   *  The dart_buffer_t instance to be initialized.
   *
   *  @param[in,out] bytes
   *  The buffer to be moved in.
   *
   *  @return
   *  Whether anything went wrong during reconstruction.
   */
  DART_ABI_EXPORT dart_err_t dart_take_bytes_err(dart_packet_t* dst, void* bytes);

  /**
   *  @brief
   *  Function takes ownership of, and reconstructs a dart_buffer object from,
   *  the network buffer of another, with an explicitly set reference counter type.
   *
   *  @param[in,out] bytes
   *  The buffer to be moved in.
   *
   *  @param[in] rc
   *  The reference counter type to use.
   *
   *  @return
   *  The reconstructed dart_buffer instance.
   */
  DART_ABI_EXPORT dart_packet_t dart_take_bytes_rc(void* bytes, dart_rc_type_t rc);

  /**
   *  @brief
   *  Function takes ownership of, and reconstructs a dart_buffer object from,
   *  the network buffer of another.
   *
   *  @param[out] dst
   *  The dart_buffer_t instance to be initialized.
   *
   *  @param[in,out] bytes
   *  The buffer to be moved in.
   *
   *  @return
   *  Whether anything went wrong during reconstruction.
   */
  DART_ABI_EXPORT dart_err_t dart_take_bytes_rc_err(dart_packet_t* dst, dart_rc_type_t rc, void* bytes);

  /**
   *  @brief
   *  Function provides a way to check if an arbitrary buffer of bytes
   *  can be successfully interpreted as a Dart buffer.
   *
   *  @details
   *  Function validates whether the given network buffer is well formed.
   *  If the function returns true it does NOT mean that the given buffer
   *  definitely wasn't corrupted in any way, just that the whole buffer
   *  is safely traversable, all necessary invariants hold, and it can be
   *  used without worry of undefined behavior.
   *
   *  @remarks
   *  Function is largely intended to be used when the buffer in question
   *  came from an untrusted source.
   */
  DART_ABI_EXPORT int dart_buffer_is_valid(void const* bytes, size_t len);

  /**
   *  @brief
   *  Function default-initializes a Dart iterator.
   *
   *  @remarks
   *  Function is not very useful as it does not associate the initialized iterator with an
   *  instance to iterate over, and largely exists for completeness.
   *
   *  @param[out] dst
   *  The dart_iterator_t instance to initialize.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_iterator_init_err(dart_iterator_t* dst);

  /**
   *  @brief
   *  Function initializes a Dart iterator and associates it with a specific Dart aggregate
   *  (object or array) instance, allowing for iteration over the value-space of said aggregate.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[out] dst
   *  The iterator instance to initialize.
   *
   *  @param[in] src
   *  Some Dart aggregate, of dart_heap_t, dart_buffer_t, or dart_packet_t type.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_iterator_init_from_err(dart_iterator_t* dst, void const* src);

  /**
   *  @brief
   *  Function initializes a Dart iterator and associates it with a specific Dart object
   *  instance, allowing for iteration over the key-space of said object.
   *
   *  @remarks
   *  Function is generic and will exhibit sensible semantics for any input Dart type.
   *
   *  @param[out] dst
   *  The iterator instance to initialize.
   *
   *  @param[in] src
   *  Some Dart object, of dart_heap_t, dart_buffer_t, or dart_packet_t type.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_iterator_init_key_from_err(dart_iterator_t* dst, void const* src);

  /**
   *  @brief
   *  Function copies a Dart iterator into a new instance, allowing for caching of iterators
   *  at different positions.
   *
   *  @param[out] dst
   *  The iterator instance to initialize
   *
   *  @param[in] src
   *  The iterator instance to initialize from.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_iterator_copy_err(dart_iterator_t* dst, dart_iterator_t const* src);

  /**
   *  @brief
   *  Function moves a Dart iterator into a new instance, allowing for iterator state to
   *  be relocated when necessary.
   *
   *  @details
   *  Operation relies on C++ move-semantics under the covers and "steals" the contents of
   *  the incoming iterator, resetting it to a "default constructed" state.
   *
   *  @param[out] dst
   *  The iterator instance to move-initialize into.
   *
   *  @param[in,out] src
   *  The iterator instance to move-initialize from.
   *
   *  @return
   *  Whether anything went wrong during initialization.
   */
  DART_ABI_EXPORT dart_err_t dart_iterator_move_err(dart_iterator_t* dst, dart_iterator_t* src);

  /**
   *  @brief
   *  Function is used to destroy a live dart_iterator_t instance, releasing any held
   *  resources.
   *
   *  @param[in,out] dst
   *  The iterator instance to be destroyed.
   *
   *  @return
   *  Whether anything went wrong during destruction.
   */
  DART_ABI_EXPORT dart_err_t dart_iterator_destroy(dart_iterator_t* dst);

  /**
   *  @brief
   *  Function is used to "unwrap" an iterator, returning a dart_packet_t instance to represent
   *  the current iterator "value".
   *
   *  @details
   *  Returned type is of type dart_packet_t, regardless of whether the iterator was constructed
   *  from a dart_packet_t, dart_heap_t, or dart_buffer_t.
   *  If constructed from a dart_heap_t, returned dart_packet_t will not be finalized.
   *  If constructed from a dart_buffer_t, returned dart_packet_t will be finalized.
   *  If constructed from a dart_packet_t, returned dart_packet_t will have the same finalized state.
   *
   *  @param[in] src
   *  The iterator to "unwrap"
   *
   *  @return
   *  The current value of the iterator.
   */
  DART_ABI_EXPORT dart_packet_t dart_iterator_get(dart_iterator_t const* src);

  /**
   *  @brief
   *  Function is used to "unwrap" an iterator, initializing a dart_packet_t instance to represent
   *  the current iterator "value".
   *
   *  @details
   *  Returned type is of type dart_packet_t, regardless of whether the iterator was constructed
   *  from a dart_packet_t, dart_heap_t, or dart_buffer_t.
   *  If constructed from a dart_heap_t, returned dart_packet_t will not be finalized.
   *  If constructed from a dart_buffer_t, returned dart_packet_t will be finalized.
   *  If constructed from a dart_packet_t, returned dart_packet_t will have the same finalized state.
   *
   *  @param[out] dst
   *  The dart_packet_t instance to initialize.
   *
   *  @param[in] src
   *  The iterator to "unwrap"
   *
   *  @return
   *  The current value of the iterator.
   */
  DART_ABI_EXPORT dart_err_t dart_iterator_get_err(dart_packet_t* dst, dart_iterator_t const* src);

  /**
   *  @brief
   *  Function is used to "increment" a Dart iterator, moving to the next element in the sequence,
   *  or potentially finishing its iteration.
   *
   *  @param[in] dst
   *  The iterator to "increment"
   *
   *  @return
   *  Whether anything went wrong during the "increment"
   */
  DART_ABI_EXPORT dart_err_t dart_iterator_next(dart_iterator_t* dst);

  /**
   *  @brief
   *  Function is used to check whether a particular Dart iterator has been exhausted
   *  (reached the end of its iteration).
   *
   *  @param[in] src
   *  The iterator to check
   *
   *  @return
   *  Whether the iterator has finished its iteration.
   */
  DART_ABI_EXPORT int dart_iterator_done(dart_iterator_t const* src);

  /**
   *  @brief
   *  Function is used to check whether a particular Dart iterator has been exhausted
   *  (reached the end of its iteration), and destroy it if so.
   *
   *  @details
   *  Function is semantically identical to dart_iterator_done, with the exception that
   *  the passed iterator instance is destroyed if it's done.
   *
   *  @param[in,out] dst
   *  The iterator to check and potentially destroy
   *
   *  @param[in,out] pkt
   *  An associated packet instance to conditionally destroy if iteration is done.
   *
   *  @return
   *  Whether the iterator has finished its iteration. If true, the iterator has now been destroyed.
   */
  DART_ABI_EXPORT int dart_iterator_done_destroy(dart_iterator_t* dst, dart_packet_t* pkt);

  /**
   *  @brief
   *  Function returns an associated human-readable error message for the last encountered error.
   *
   *  @details
   *  Function returns a pointer into a thread-local string variable that will be re-assigned to
   *  on the next error to occur, meaning that the returned pointer should only be assumed
   *  to be valid until the next call to any Dart API function.
   *
   *  @return
   *  The human readable error string.
   */
  DART_ABI_EXPORT char const* dart_get_error();

  /**
   *  @brief
   *  Free a buffer returned from one of the dart_*_dup_bytes functions
   *
   *  @details
   *  Dart requires its buffer representations to be aligned to a 64-bit
   *  boundary for internal design reasons (simplifies alignment logic significantly).
   *  Very easy to do on *nix machines with posix_memalign, which allocates aligned
   *  memory that can be passed directly to free.
   *  Windows, on the other hand, has _aligned_malloc and _aligned_free, which MUST
   *  be paired, so to write portable code this function must exist.
   */
  DART_ABI_EXPORT void dart_aligned_free(void* ptr);

#ifdef __cplusplus
}
#endif

#endif
