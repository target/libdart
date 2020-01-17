/**
 *  @file
 *  dart.h
 *
 *  @brief
 *  Contains all public declarations for the Dart JSON serialization library.
 *
 *  @details
 *  Dart is both a wire-level binary JSON protocol, along with an extremely
 *  high performance, and surprisingly high level, C++ API to interact with that JSON.
 *  It is primarily optimized for on-the-wire representation size along with
 *  efficiency of receiver-side interaction, however, it also allows for reasonably
 *  performant dynamic modification when necessary.
 *  Dart can be used in any application as a dead-simple and lightweight JSON parser,
 *  but first and foremost it targets real-time stream processing engines in a
 *  schema-less environment. It retains logarithmic complexity of object key-lookup,
 *  requires zero receiver-side memory allocations for read-only interactions,
 *  and requires an average of 20% memory overhead compared to the input JSON.
 *
 *  @author
 *  Chris Fretz
 *
 *  @date
 *  5/1/19
 */

#ifndef DART_H
#define DART_H

// Check to make sure we have at least c++14.
#if __cplusplus < 201402L
static_assert(false, "libdart requires a c++14 enabled compiler.");
#endif

/*----- System Includes -----*/

#include <map>
#include <array>
#include <string>
#include <vector>
#include <memory>
#include <cstddef>
#include <sstream>
#include <gsl/gsl>
#include <cstring>
#include <algorithm>
#include <type_traits>

/*----- Local Includes -----*/

#include "dart/common.h"
#include "dart/refcount_traits.h"
#include "dart/conversion_traits.h"

/*----- Macro Definitions -----*/

// Version macros for conditional compilation/feature checks.
#define DART_MAJOR_VERSION          0
#define DART_MINOR_VERSION          9
#define DART_PATCH_VERSION          0

/*----- Type Declarations -----*/

namespace dart {

#ifdef DART_USE_SAJSON
  static constexpr auto default_parse_stack_size = (1U << 12U);
#endif
#if DART_HAS_RAPIDJSON
  static constexpr auto parse_default = rapidjson::kParseDefaultFlags;
  static constexpr auto parse_comments = rapidjson::kParseCommentsFlag;
  static constexpr auto parse_nan = rapidjson::kParseNanAndInfFlag;
  static constexpr auto parse_trailing_commas = rapidjson::kParseTrailingCommasFlag;
  static constexpr auto parse_permissive = parse_comments | parse_nan | parse_trailing_commas;
  static constexpr auto write_default = rapidjson::kWriteDefaultFlags;
  static constexpr auto write_nan = rapidjson::kWriteNanAndInfFlag;
  static constexpr auto write_permissive = write_nan;
#endif

  template <template <class> class RefCount>
  class basic_packet;

  /**
   *  @brief
   *  Class operates as a strongly-typed wrapper around any of the
   *  dart::packet, dart::heap, dart::buffer classes, enforcing that
   *  any wrapped packet must be an object.
   *
   *  @details
   *  Dart is primarly a dynamically typed library for interacting with
   *  a dynamically typed notation language, but as C++ is a statically
   *  typed language, it can often be useful to statically know the type
   *  of a packet at compile-time.
   *  Type is implemented as a wrapper around the existing Dart types,
   *  and therefore does not come with any performance benefit. It is
   *  intended to be used to increase readability/enforce
   *  constraints/preconditions where applicable.
   */
  template <class Object>
  class basic_object final {

    // Type aliases used to SFINAE away member functions we shouldn't have.
    template <class Obj, class... As>
    using make_object_t = decltype(Obj::make_object(std::declval<As>()...));
    template <class Obj, class K>
    using subscript_operator_t = decltype(std::declval<Obj>()[std::declval<K>()]);
    template <class Obj, class K, class V>
    using add_field_t = decltype(std::declval<Obj>().add_field(std::declval<K>(), std::declval<V>()));
    template <class Obj, class A>
    using remove_field_t = decltype(std::declval<Obj>().remove_field(std::declval<A>()));
    template <class Obj, class K, class V>
    using insert_t = decltype(std::declval<Obj>().insert(std::declval<K>(), std::declval<V>()));
    template <class Obj, class K, class V>
    using set_t = decltype(std::declval<Obj>().set(std::declval<K>(), std::declval<V>()));
    template <class Obj, class A>
    using erase_t = decltype(std::declval<Obj>().erase(std::declval<A>()));
    template <class Obj>
    using clear_t = decltype(std::declval<Obj>().clear());
    template <class Obj, class... As>
    using inject_t = decltype(std::declval<Obj>().inject(std::declval<As>()...));
    template <class Obj, class S>
    using project_t = decltype(std::declval<Obj>().project(std::declval<S>()));
    template <class Obj, class A>
    using get_t = decltype(std::declval<Obj>().get(std::declval<A>()));
    template <class Obj, class A, class T>
    using get_or_t = decltype(std::declval<Obj>().get_or(std::declval<A>(), std::declval<T>()));
    template <class Obj, class A>
    using at_t = decltype(std::declval<Obj>().at(std::declval<A>()));
    template <class Obj, class A>
    using find_t = decltype(std::declval<Obj>().find(std::declval<A>()));
    template <class Obj, class A>
    using find_key_t = decltype(std::declval<Obj>().find_key(std::declval<A>()));
    template <class Obj, class A>
    using has_key_t = decltype(std::declval<Obj>().has_key(std::declval<A>()));
    template <class Obj>
    using get_bytes_t = decltype(std::declval<Obj>().get_bytes());
    template <class Obj, class A>
    using dup_bytes_t = decltype(std::declval<Obj>().dup_bytes(std::declval<A>()));
    template <class Obj, class A>
    using share_bytes_t = decltype(std::declval<Obj>().share_bytes(std::declval<A>()));

    // Exists to check some basic cases for non type-converting
    // member functions.
    template <class KeyType>
    struct probable_string :
      meta::conjunction<
        meta::negation<
          std::is_convertible<
            std::decay_t<KeyType>,
            typename Object::size_type
          >
        >,
        meta::negation<
          meta::is_specialization_of<
            std::decay_t<KeyType>,
            dart::basic_number
          >
        >
      >
    {};

    public:

      /*----- Public Types -----*/

      using value_type = Object;
      using type = typename Object::type;
      using iterator = typename Object::iterator;
      using size_type = typename Object::size_type;
      using generic_type = typename value_type::generic_type;
      using reverse_iterator = typename Object::reverse_iterator;

      /*----- Lifecycle Functions -----*/

      /**
       *  @brief
       *  Constructor can be used to directly initialize a strongly
       *  typed object with a set of initial contents.
       *
       *  @details
       *  Internally calls make_object on the underlying packet type
       *  and forwards through the provided arguments.
       */
      template <class... Args, class EnableIf =
        std::enable_if_t<
          meta::is_detected<make_object_t, value_type, Args...>::value
        >
      >
      explicit basic_object(Args&&... the_args) :
        val(value_type::make_object(std::forward<Args>(the_args)...))
      {}

      /**
       *  @brief
       *  Function is responsible for forwarding any constructors
       *  from the wrapped type up to the wrapper, including the
       *  explicit/implicit declaration.
       *
       *  @details
       *  In practice, function allows the wrapper to be constructed
       *  from the wrapped type itself, along with any network buffer
       *  constructors (in the case of dart::buffer and dart::packet).
       *
       *  @remarks
       *  I may regret making the constructor forwarding this wide open,
       *  but as a fallback to ensure nothing slips through the tests,
       *  the constructor will throw if the user somehow manages to
       *  slip something through the API that results in a non-object.
       */
      template <class Arg,
        std::enable_if_t<
          !meta::is_specialization_of<
            Arg,
            dart::basic_object
          >::value
          &&
          !std::is_convertible<
            Arg,
            Object
          >::value
          &&
          std::is_constructible<
            Object,
            Arg
          >::value
        >* EnableIf = nullptr
      >
      explicit basic_object(Arg&& arg);

      /**
       *  @brief
       *  Function is responsible for forwarding any constructors
       *  from the wrapped type up to the wrapper, including the
       *  explicit/implicit declaration.
       *
       *  @details
       *  In practice, function allows the wrapper to be constructed
       *  from the wrapped type itself, along with any network buffer
       *  constructors (in the case of dart::buffer and dart::packet).
       *
       *  @remarks
       *  I may regret making the constructor forwarding this wide open,
       *  but as a fallback to ensure nothing slips through the tests,
       *  the constructor will throw if the user somehow manages to
       *  slip something through the API that results in a non-object.
       */
      template <class Arg,
        std::enable_if_t<
          !meta::is_specialization_of<
            Arg,
            dart::basic_object
          >::value
          &&
          std::is_convertible<
            Arg,
            Object
          >::value
        >* EnableIf = nullptr
      >
      basic_object(Arg&& arg);

      /**
       *  @brief
       *  Converting constructor to allow interoperability between underlying
       *  packet types for strongly typed objects.
       *
       *  @details
       *  Constructor is declared to be conditionally explicit based on whether
       *  the underlying implementation types are implicitly convertible.
       *  As a result, this code:
       *  ```
       *  dart::heap::object obj {"hello", "world"};
       *  dart::packet::object dup = obj;
       *  ```
       *  Will compile, whereas this:
       *  ```
       *  dart::packet::object obj {"hello", "world"};
       *  dart::heap::object dup = obj;
       *  ```
       *  Will not. It would need to use an explicit cast as such:
       *  ```
       *  dart::packet::object obj {"hello", "world"};
       *  dart::heap::object dup {obj};
       *  ```
       */
      template <class Obj,
        std::enable_if_t<
          !std::is_convertible<
            Obj,
            Object
          >::value
          &&
          std::is_constructible<
            Object,
            Obj
          >::value
        >* = nullptr
      >
      explicit basic_object(basic_object<Obj> const& obj) : val(value_type {obj.val}) {}

      /**
       *  @brief
       *  Converting constructor to allow interoperability between underlying
       *  packet types for strongly typed objects.
       *
       *  @details
       *  Constructor is declared to be conditionally explicit based on whether
       *  the underlying implementation types are implicitly convertible.
       *  As a result, this code:
       *  ```
       *  dart::heap::object obj {"hello", "world"};
       *  dart::packet::object dup = obj;
       *  ```
       *  Will compile, whereas this:
       *  ```
       *  dart::packet::object obj {"hello", "world"};
       *  dart::heap::object dup = obj;
       *  ```
       *  Will not. It would need to use an explicit cast as such:
       *  ```
       *  dart::packet::object obj {"hello", "world"};
       *  dart::heap::object dup {obj};
       *  ```
       */
      template <class Obj,
        std::enable_if_t<
          std::is_convertible<
            Obj,
            Object
          >::value
        >* = nullptr
      >
      basic_object(basic_object<Obj> const& obj) : val(obj.val) {}

      /**
       *  @brief
       *  Copy constructor.
       *
       *  @details
       *  Increments refcount of underlying implementation type.
       */
      basic_object(basic_object const&) = default;

      /**
       *  @brief
       *  Move constructor.
       *
       *  @details
       *  Steals the refcount of an expiring strongly typed object.
       */
      basic_object(basic_object&&) noexcept = default;

      /**
       *  @brief
       *  Destructor. Does what you think it does.
       */
      ~basic_object() = default;

      /*----- Operators -----*/

      /**
       *  @brief
       *  Copy assignment operator.
       *
       *  @details
       *  Increments the refcount of the underlying implementation type.
       */
      basic_object& operator =(basic_object const&) & = default;

      // An MSVC bug prevents us from deleting based on ref qualifiers.
#if !DART_USING_MSVC
      basic_object& operator =(basic_object const&) && = delete;
#endif


      /**
       *  @brief
       *  Move assignment operator.
       *
       *  @details
       *  Steals the refcount of an expiring strongly typed object.
       */
      basic_object& operator =(basic_object&&) & = default;

      // An MSVC bug prevents us from deleting based on ref qualifiers.
#if !DART_USING_MSVC
      basic_object& operator =(basic_object&&) && = delete;
#endif

      /**
       *  @brief
       *  Object subscript operator.
       *
       *  @details
       *  Assuming this is an object, returns the value associated with the given
       *  key, or a null packet if not such mapping exists.
       *
       *  @remarks
       *  Function returns null if the requested mapping isn't present, but cannot
       *  be marked noexcept due to the possibility that the user might pass a
       *  dart::packet which is not a string.
       *  If you pass this function reasonable things, it'll never throw an exception.
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          probable_string<KeyType>::value
          &&
          meta::is_detected<subscript_operator_t, value_type const&, KeyType const&>::value
        >
      >
      auto operator [](KeyType const& key) const& -> value_type;

      /**
       *  @brief
       *  Object subscript operator.
       *
       *  @details
       *  Assuming this is an object, returns the value associated with the given
       *  key, or a null packet if not such mapping exists.
       *
       *  @remarks
       *  Function returns null if the requested mapping isn't present, but cannot
       *  be marked noexcept due to the possibility that the user might pass a
       *  dart::packet which is not a string.
       *  If you pass this function reasonable things, it'll never throw an exception.
       *
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto obj = some_object();
       *  auto deep = obj["first"]["second"]["third"];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          probable_string<KeyType>::value
          &&
          meta::is_detected<subscript_operator_t, value_type&&, KeyType const&>::value
        >
      >
      decltype(auto) operator [](KeyType const& key) &&;

      /**
       *  @brief
       *  Equality operator.
       *  
       *  @details
       *  All packet types always return deep equality (aggregates included).
       *  While idiomatic for C++, this means that non-finalized object comparisons
       *  can be reasonably expensive.
       *  Finalized comparisons, however, use memcmp to compare the underlying
       *  byte buffers, and is _stupendously_ fast.
       *  
       *  @remarks
       *  "Do as vector does"
       */
      template <class OtherObject>
      bool operator ==(basic_object<OtherObject> const& other) const noexcept;

      /**
       *  @brief
       *  Inequality operator.
       *  
       *  @details
       *  All packet types always return deep equality (aggregates included).
       *  While idiomatic C++, this means that non-finalized object/array comparisons
       *  can be arbitrarily expensive.
       *  Finalized comparisons, however, use memcmp to compare the underlying
       *  byte buffers, and are _stupendously_ fast.
       *  
       *  @remarks
       *  "Do as vector does"
       */
      template <class OtherObject>
      bool operator !=(basic_object<OtherObject> const& other) const noexcept;

      /**
       *  @brief
       *  Implicit conversion operator to underlying implementation type.
       *
       *  @details
       *  Dart is primarily a dynamically typed library, and so, while the type
       *  wrappers exist to enable additional expressiveness and readability,
       *  they shouldn't make the library more cumbersome to work with, and so
       *  type wrappers are allowed to implicitly discard their type information
       *  if the user requests it.
       */
      operator value_type() const& noexcept;

      /**
       *  @brief
       *  Implicit conversion operator to underlying implementation type.
       *
       *  @details
       *  Dart is primarily a dynamically typed library, and so, while the type
       *  wrappers exist to enable additional expressiveness and readability,
       *  they shouldn't make the library more cumbersome to work with, and so
       *  type wrappers are allowed to implicitly discard their type information
       *  if the user requests it.
       */
      operator value_type() && noexcept;

      /**
       *  @brief
       *  Existential operator (bool conversion operator).
       *
       *  @details
       *  Always returns true for strongly typed objects.
       */
      explicit operator bool() const noexcept;

      /*----- Public API -----*/

      /*----- Aggregate Builder Functions -----*/

      /**
       *  @brief
       *  Function adds, or replaces, a key-value pair mapping to/in a non-finalized packet.
       *
       *  @details
       *  Given key and value can be any types for which type conversions are defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *  The result of converting the key must yield a string.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto obj = dart::packet::make_object("hello", "goodbye");
       *  auto nested = obj["hello"].add_field("yes", "no").add_field("stop", "go, go, go");
       *  ```
       */
      template <class KeyType, class ValueType, class EnableIf =
        std::enable_if_t<
          meta::is_detected<add_field_t, value_type&, KeyType, ValueType>::value
        >
      >
      basic_object& add_field(KeyType&& key, ValueType&& value) &;

      /**
       *  @brief
       *  Function adds, or replaces, a key-value pair mapping to/in a non-finalized object.
       *
       *  @details
       *  Given key and value can be any types for which type conversions are defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *  The result of converting the key must yield a string.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto obj = dart::packet::make_object("hello", "goodbye");
       *  auto nested = obj["hello"].add_field("yes", "no").add_field("stop", "go, go, go");
       *  ```
       */
      template <class KeyType, class ValueType, class EnableIf =
        std::enable_if_t<
          meta::is_detected<add_field_t, value_type&&, KeyType, ValueType>::value
        >
      >
      DART_NODISCARD basic_object&& add_field(KeyType&& key, ValueType&& value) &&;

      /**
       *  @brief
       *  Function removes a key-value pair mapping from a non-finalized object.
       *
       *  @details
       *  dart::packet::add_field does not throw if the key-value pair is already present,
       *  allowing for key replacement, and so for API uniformity, this function also does
       *  not throw if the key-value pair is NOT present.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto obj = get_object();
       *  auto nested = obj["nested"].remove_field("hello");
       *  ```
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          meta::is_detected<remove_field_t, value_type&, KeyType const&>::value
        >
      >
      basic_object& remove_field(KeyType const& key) &;

      /**
       *  @brief
       *  Function removes a key-value pair mapping from a non-finalized object.
       *
       *  @details
       *  dart::packet::add_field does not throw if the key-value pair is already present,
       *  allowing for key replacement, and so for API uniformity, this function also does
       *  not throw if the key-value pair is NOT present.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto obj = get_object();
       *  auto nested = obj["nested"].remove_field("hello");
       *  ```
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          meta::is_detected<remove_field_t, value_type&&, KeyType const&>::value
        >
      >
      DART_NODISCARD basic_object&& remove_field(KeyType const& key) &&;

      /**
       *  @brief
       *  Function adds, replaces, or inserts, a (new) key-value pair to a
       *  non-finalized object.
       *
       *  @details
       *  Given key and value can be any types for which type conversions are defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *  The result of converting the key must yield a string.
       *
       *  @remarks
       *  add_field/insert allow the user to pass non-string values, falling back on
       *  runtime checks, and this function does not.
       *  An unfortunate irregularity, hopefully it won't cause any problems.
       *  add_field/insert **has** to convert its key argument into a dart::packet,
       *  which uses the same conversion pathway as everything else, which means it could
       *  call user conversions, which can't be statically checked at this time.
       *  erase doesn't convert its argument, and so allows a much smaller set of types
       */
      template <class KeyType, class ValueType, class EnableIf =
        std::enable_if_t<
          meta::is_detected<insert_t, value_type&, KeyType, ValueType>::value
        >
      >
      auto insert(KeyType&& key, ValueType&& value) -> iterator;

      /**
       *  @brief
       *  Function replaces a key-value pair in a non-finalized object.
       *
       *  @details
       *  Given key and value can be any types for which type conversions are defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *  The result of converting the key must yield a string.
       */
      template <class KeyType, class ValueType, class EnableIf =
        std::enable_if_t<
          meta::is_detected<set_t, value_type&, KeyType, ValueType>::value
        >
      >
      auto set(KeyType&& key, ValueType&& value) -> iterator;

      /**
       *  @brief
       *  Function removes a key-value pair from a non-finalized object.
       *
       *  @details
       *  If the key-value pair is not present in the current object, function
       *  returns the end iterator, otherwise it returns an iterator to one past the element
       *  removed.
       *
       *  @remarks
       *  add_field/insert allow the user to pass non-string values, falling back on
       *  runtime checks, and this function does not.
       *  An unfortunate irregularity, hopefully it won't cause any problems.
       *  add_field/insert **has** to convert its key argument into a dart::packet,
       *  which uses the same conversion pathway as everything else, which means it could
       *  call user conversions, which can't be statically checked at this time.
       *  erase doesn't convert its argument, and so allows a much smaller set of types
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          probable_string<KeyType>::value
          &&
          meta::is_detected<erase_t, value_type&, KeyType const&>::value
        >
      >
      auto erase(KeyType const& key) -> iterator;

      /**
       *  @brief
       *  Function clears an aggregate of all values.
       *
       *  @details
       *  If this is non-finalized, will remove all key-value pairs.
       */
      template <class Obj = Object, class EnableIf =
        std::enable_if_t<
          meta::is_detected<clear_t, Obj&>::value
        >
      >
      void clear();

      /**
       *  @brief
       *  Function "injects" a set of key-value pairs into an object without
       *  modifying it by returning a new object with the union of the keyset
       *  of the original object and the provided key-value pairs.
       *
       *  @details
       *  Function provides a uniform interface for _efficient_ injection of
       *  keys across the entire Dart API (also works for dart::buffer).
       */
      template <class... Args, class EnableIf =
        std::enable_if_t<
          meta::is_detected<inject_t, Object const&, Args...>::value
        >
      >
      basic_object inject(Args&&... the_args) const;

      basic_object project(std::initializer_list<shim::string_view> keys) const;

      template <class StringSpan, class EnableIf =
        std::enable_if_t<
          meta::is_detected<project_t, Object const&, StringSpan>::value
        >
      >
      basic_object project(StringSpan const& keys) const;

      /**
       *  @brief
       *  Function transitions the current packet from being finalized to non-finalized in place.
       *
       *  @details
       *  dart::packet has two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart::packet maintains a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart::packet maintains a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::packet::finalize, the reverse can be accomplished using dart::packet::definalize.
       */
      decltype(auto) definalize() &;

      /**
       *  @brief
       *  Function transitions the current packet from being finalized to non-finalized in place.
       *
       *  @details
       *  dart::packet has two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart::packet maintains a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart::packet maintains a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::packet::finalize, the reverse can be accomplished using dart::packet::definalize.
       */
      decltype(auto) definalize() &&;

      /**
       *  @brief
       *  Function transitions the current packet from being finalized to non-finalized in place.
       *
       *  @details
       *  dart::packet has two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart::packet maintains a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart::packet maintains a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::packet::finalize, the reverse can be accomplished using dart::packet::definalize.
       */
      decltype(auto) lift() &;

      /**
       *  @brief
       *  Function transitions the current packet from being finalized to non-finalized in place.
       *
       *  @details
       *  dart::packet has two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart::packet maintains a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart::packet maintains a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::packet::finalize, the reverse can be accomplished using dart::packet::definalize.
       */
      decltype(auto) lift() &&;

      /**
       *  @brief
       *  Function transitions the current packet from being non-finalized to finalized in place.
       *
       *  @details
       *  dart::packet has two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart::packet maintains a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart::packet maintains a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::packet::finalize, the reverse can be accomplished using dart::packet::definalize.
       */
      decltype(auto) finalize() &;

      /**
       *  @brief
       *  Function transitions the current packet from being non-finalized to finalized in place.
       *
       *  @details
       *  dart::packet has two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart::packet maintains a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart::packet maintains a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::packet::finalize, the reverse can be accomplished using dart::packet::definalize.
       */
      decltype(auto) finalize() &&;

      /**
       *  @brief
       *  Function transitions the current packet from being non-finalized to finalized in place.
       *
       *  @details
       *  dart::packet has two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart::packet maintains a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart::packet maintains a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::packet::finalize, the reverse can be accomplished using dart::packet::definalize.
       */
      decltype(auto) lower() &;

      /**
       *  @brief
       *  Function transitions the current packet from being non-finalized to finalized in place.
       *
       *  @details
       *  dart::packet has two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart::packet maintains a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart::packet maintains a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::packet::finalize, the reverse can be accomplished using dart::packet::definalize.
       */
      decltype(auto) lower() &&;

      /*----- JSON Helper Functions -----*/

#if DART_HAS_RAPIDJSON
      /**
       *  @brief
       *  Function serializes any packet into a JSON string.
       *
       *  @details
       *  Serialization is based on RapidJSON, and so exposes the same customization points as
       *  RapidJSON.
       *  If your packet has NaN or +/-Infinity values, you can serialize it in the following way:
       *  ```
       *  auto obj = some_object();
       *  auto json = obj.to_json<dart::write_nan>();
       *  do_something(std::move(json));
       *  ```
       */
      template <unsigned flags = write_default>
      std::string to_json() const;
#endif

      /*----- Value Accessor Functions -----*/

      /**
       *  @brief
       *  Object access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Function will return the requested key-value pair as a new packet.
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          probable_string<KeyType>::value
          &&
          meta::is_detected<get_t, value_type const&, KeyType const&>::value
        >
      >
      auto get(KeyType const& key) const& -> value_type;

      /**
       *  @brief
       *  Object access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Function will return the requested key-value pair as a new packet.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto obj = some_object();
       *  auto deep = obj["first"]["second"]["third"];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          probable_string<KeyType>::value
          &&
          meta::is_detected<get_t, value_type&&, KeyType const&>::value
        >
      >
      decltype(auto) get(KeyType const& key) &&;

      /**
       *  @brief
       *  Optional object access method, similar to std::optional::value_or.
       *
       *  @details
       *  If this is an object, and contains the key-value pair, function returns the
       *  value associated with the specified key.
       *  If this is not an object, or the specified key-value pair is not present,
       *  function returns the optional value cast to a dart::packet according to
       *  the usual conversion API rules.
       *
       *  @remarks
       *  Function is "logically" noexcept, but cannot be marked so as allocations necessary
       *  for converting the optional value might throw, and user defined conversions aren't
       *  required to be noexcept.
       */
      template <class KeyType, class T, class EnableIf =
        std::enable_if_t<
          probable_string<KeyType>::value
          &&
          meta::is_detected<get_or_t, value_type const&, KeyType const&, T>::value
        >
      >
      auto get_or(KeyType const& key, T&& opt) const -> value_type;

      /**
       *  @brief
       *  Function returns the value associated with the separator delimited object path.
       */
      auto get_nested(shim::string_view path, char separator = '.') const -> value_type;

      /**
       *  @brief
       *  Object access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Function will return the requested key-value pair as a new packet.
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          probable_string<KeyType>::value
          &&
          meta::is_detected<at_t, value_type const&, KeyType const&>::value
        >
      >
      auto at(KeyType const& key) const& -> value_type;

      /**
       *  @brief
       *  Object access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Function will return the requested key-value pair as a new packet.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto obj = some_object();
       *  auto deep = obj["first"]["second"]["third"];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          probable_string<KeyType>::value
          &&
          meta::is_detected<at_t, value_type&&, KeyType const&>::value
        >
      >
      decltype(auto) at(KeyType const& key) &&;

      /**
       *  @brief
       *  Finds the requested key-value mapping.
       *
       *  @details
       *  Function will return an iterator to the requested VALUE mapping,
       *  or the end iterator.
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          meta::is_detected<find_t, value_type const&, KeyType const&>::value
        >
      >
      auto find(KeyType const& key) const -> iterator;

      /**
       *  @brief
       *  Finds the requested key-value mapping.
       *
       *  @details
       *  Function will return an iterator to the requested KEY mapping,
       *  or the end iterator.
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          meta::is_detected<find_key_t, value_type const&, KeyType const&>::value
        >
      >
      auto find_key(KeyType const& key) const -> iterator;

      /**
       *  @brief
       *  Returns all keys associated with the current object.
       */
      auto keys() const -> std::vector<value_type>;

      /**
       *  @brief
       *  Returns all subvalues of the current object.
       */
      auto values() const -> std::vector<value_type>;

      /**
       *  @brief
       *  Returns whether a particular key-value pair exists within an object.
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          meta::is_detected<has_key_t, value_type const&, KeyType const&>::value
        >
      >
      bool has_key(KeyType const& key) const noexcept;

      /**
       *  @brief
       *  Function returns the number of key-value pairs in the current object.
       */
      auto size() const noexcept -> size_type;

      /**
       *  @brief
       *  Function returns whether the current packet is empty, according to the semantics
       *  of its type.
       *
       *  @details
       *  In actuality, returns whether size() == 0;
       */
      bool empty() const noexcept;

      /**
       *  @brief
       *  Helper function returns a const& to the underlying dynamic type.
       *
       *  @details
       *  Can be useful for efficiently implementing wrapper API behavior
       *  in some spots.
       */
      auto dynamic() const noexcept -> value_type const&;

      /*----- Introspection Functions -----*/

      /**
       *  @brief
       *  Returns whether the current packet is an object or not.
       */
      bool is_object() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an array or not.
       */
      constexpr bool is_array() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an object or array or not.
       */
      bool is_aggregate() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a string or not.
       */
      constexpr bool is_str() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an integer or not.
       */
      constexpr bool is_integer() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a decimal or not.
       */
      constexpr bool is_decimal() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an integer or decimal or not.
       */
      constexpr bool is_numeric() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a boolean or not.
       */
      constexpr bool is_boolean() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is null or not.
       */
      bool is_null() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a string, integer, decimal,
       *  boolean, or null.
       */
      constexpr bool is_primitive() const noexcept;

      /**
       *  @brief
       *  Returns the type of the current packet as an enum value.
       */
      type get_type() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is finalized.
       */
      bool is_finalized() const noexcept;

      /**
       *  @brief
       *  Returns the current reference count of the underlying reference counter.
       */
      auto refcount() const noexcept -> size_type;

      /*----- Network Buffer Accessors -----*/

      /**
       *  @brief
       *  Function allows the network buffer of the current packet to be exported
       *  directly, without copies.
       *
       *  @details
       *  Function has a fairly awkward API, requiring the user to pass an existing
       *  instance (potentially default-constructed) of the reference counter type,
       *  which it then destroys and reconstructs such that it is a copy of the
       *  current reference counter.
       *  Function is written this way as the reference counter type cannot be assumed
       *  to be either copyable or moveable, and at this time I don't want to expose
       *  dart::shareable_ptr externally.
       */
      template <class RC, class EnableIf =
        std::enable_if_t<
          meta::is_detected<share_bytes_t, Object&, RC&>::value
        >
      >
      auto share_bytes(RC& bytes) const;

      /**
       *  @brief
       *  Function returns the network-level buffer for a finalized packet.
       *
       *  @details
       *  gsl::span is a non-owning view type, like std::string_view, and so the
       *  lifetype of the returned view is equal to the lifetime of the current
       *  packet.
       */
      template <class Obj = Object, class EnableIf =
        std::enable_if_t<
          meta::is_detected<get_bytes_t, Obj&>::value
        >
      >
      auto get_bytes() const;

      /**
       *  @brief
       *  Function returns an owning copy of the network-level buffer for a
       *  finalized packet.
       *
       *  @details
       *  The primary difference between this and dart::packet::get_bytes is that
       *  get_bytes returns a non-owning view into the packet's buffer, whereas
       *  dup_bytes copies the buffer out into a newly allocated region, and returns
       *  that.
       */
      template <class Obj = Object, class EnableIf =
        std::enable_if_t<
          meta::is_detected<dup_bytes_t, Obj&, size_type&>::value
        >
      >
      auto dup_bytes() const;

      /**
       *  @brief
       *  Function returns an owning copy of the network-level buffer for a
       *  finalized packet.
       *
       *  @details
       *  The primary difference between this and dart::packet::get_bytes is that
       *  get_bytes returns a non-owning view into the packet's buffer, whereas
       *  dup_bytes copies the buffer out into a newly allocated region, and returns
       *  that.
       *
       *  @remarks
       *  Out-parameters aren't great, but it seemed like the easiest way to do this.
       */
      template <class Obj = Object, class EnableIf =
        std::enable_if_t<
          meta::is_detected<dup_bytes_t, Obj&, size_type&>::value
        >
      >
      auto dup_bytes(size_type& len) const;

      /*----- Iterator Functions -----*/

      /**
       *  @brief
       *  Function returns a dart::packet::iterator that can be used to iterate over the
       *  VALUES of the current object.
       */
      auto begin() const -> iterator;

      /**
       *  @brief
       *  Function returns a dart::packet::iterator that can be used to iterate over the
       *  VALUES of the current object.
       *
       *  @remarks
       *  Function exists for API uniformity with the STL, but is precisely equal to
       *  dart::packet::begin.
       */
      auto cbegin() const -> iterator;

      /**
       *  @brief
       *  Function returns a dart::packet::iterator one-past-the-end for iterating over
       *  the VALUES of the current object.
       *
       *  @details
       *  Dart iterators are bidirectional, so the iterator could be decremented to iterate
       *  over the valuespace backwards, or it can be used as a sentinel value for
       *  traditional iteration.
       */
      auto end() const -> iterator;

      /**
       *  @brief
       *  Function returns a dart::packet::iterator one-past-the-end for iterating over
       *  the VALUES of the current object.
       *
       *  @details
       *  Dart iterators are bidirectional, so the iterator could be decremented to iterate
       *  over the valuespace backwards, or it can be used as a sentinel value for
       *  traditional iteration.
       *
       *  @remarks
       *  Function exists for API uniformity with the STL, but is precisely equal to
       *  dart::packet::end.
       */
      auto cend() const -> iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator that iterates in reverse order from
       *  dart::packet::end to dart::packet::begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::begin.
       */
      auto rbegin() const -> reverse_iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator that iterates in reverse order from
       *  dart::packet::end to dart::packet::begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::begin.
       *
       *  @remarks
       *  Function exists for API uniformity with the STL, but is precisely equal to
       *  dart::packet::rbegin.
       */
      auto crbegin() const -> reverse_iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator one-past-the-end, that iterates in
       *  reverse order from dart::packet::end to dart::packet::begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::end.
       */
      auto rend() const -> reverse_iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator one-past-the-end, that iterates in
       *  reverse order from dart::packet::end to dart::packet::begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::end.
       *
       *  @remarks
       *  Function exists for API uniformity with the STL, but is precisely equal to
       *  dart::packet::rbegin.
       */
      auto crend() const -> reverse_iterator;

      /**
       *  @brief
       *  Function returns a dart::packet::iterator that can be used to iterate over the
       *  KEYS of the current packet.
       *
       *  @details
       *  Throws if this is not an object
       */
      auto key_begin() const -> iterator;

      /**
       *  @brief
       *  Function returns a dart::packet::iterator one-past-the-end that can be used to
       *  iterate over the KEYS of the current packet.
       *
       *  @details
       *  Dart iterators are bidirectional, so the iterator could be decremented to iterate
       *  over the keyspace backwards, or it can be used as a sentinel value for traditional
       *  iteration.
       */
      auto key_end() const -> iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator that iterates over the KEYS of the current
       *  packet in reverse order from dart::packet::key_end to dart::packet::key_begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::key_begin.
       */
      auto rkey_begin() const -> reverse_iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator one-past-the-end that iterates over the
       *  KEYS of the current packet in reverse order from dart::packet::key_end to
       *  dart::packet::key_begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::key_end.
       */
      auto rkey_end() const -> reverse_iterator;

      /**
       *  @brief
       *  Function returns a tuple of key-value iterators to allow for easy iteration
       *  over keys and values with C++17 structured bindings.
       */
      auto kvbegin() const -> std::tuple<iterator, iterator>;

      /**
       *  @brief
       *  Function returns a tuple of key-value iterators to allow for easy iteration
       *  over keys and values with C++17 structured bindings.
       */
      auto kvend() const -> std::tuple<iterator, iterator>;

      /**
       *  @brief
       *  Function returns a tuple of key-value reverse iterators to allow for easy
       *  iteration over keys and values with C++17 structured bindings.
       */
      auto rkvbegin() const -> std::tuple<reverse_iterator, reverse_iterator>;

      /**
       *  @brief
       *  Function returns a tuple of key-value reverse iterators to allow for easy
       *  iteration over keys and values with C++17 structured bindings.
       */
      auto rkvend() const -> std::tuple<reverse_iterator, reverse_iterator>;

    private:

      /*----- Private Helpers -----*/

      void ensure_object(shim::string_view msg) const;

      /*----- Private Members -----*/

      value_type val;

      /*----- Friends -----*/

      // Allow type conversion logic to work.
      friend struct convert::detail::caster_impl<convert::detail::wrapper_tag>;

      // We're friends of all other basic_object specializations.
      template <class>
      friend class basic_object;

  };

  /**
   *  @brief
   *  Class operates as a strongly-typed wrapped around any of the
   *  dart::packet, dart::heap, dart::buffer classes, enforcing that
   *  any wrapped packet must be an array.
   *
   *  @details
   *  Dart is primarly a dynamically typed library for interacting with
   *  a dynamically typed notation language, but as C++ is a statically
   *  typed language, it can often be useful to statically know the type
   *  of a packet at compile-time.
   *  Type is implemented as a wrapper around the existing Dart types,
   *  and therefore does not come with any performance benefit. It is
   *  intended to be used to increase readability/enforce
   *  constraints/preconditions where applicable.
   */
  template <class Array>
  class basic_array final {

    // Type aliases used to SFINAE away member functions we shouldn't have.
    template <class Arr, class... Args>
    using make_array_t = decltype(Arr::make_array(std::declval<Args>()...));
    template <class Arr, class I>
    using subscript_operator_t = decltype(std::declval<Arr>()[std::declval<I>()]);
    template <class Arr, class V>
    using push_front_t = decltype(std::declval<Arr>().push_front(std::declval<V>()));
    template <class Arr>
    using pop_front_t = decltype(std::declval<Arr>().pop_front());
    template <class Arr, class V>
    using push_back_t = decltype(std::declval<Arr>().push_front(std::declval<V>()));
    template <class Arr>
    using pop_back_t = decltype(std::declval<Arr>().pop_back());
    template <class Arr, class I, class V>
    using insert_t = decltype(std::declval<Arr>().insert(std::declval<I>(), std::declval<V>()));
    template <class Arr, class I, class V>
    using set_t = decltype(std::declval<Arr>().set(std::declval<I>(), std::declval<V>()));
    template <class Arr, class I>
    using erase_t = decltype(std::declval<Arr>().erase(std::declval<I>()));
    template <class Arr>
    using clear_t = decltype(std::declval<Arr>().clear());
    template <class Arr, class A>
    using reserve_t = decltype(std::declval<Arr>().reserve(std::declval<A>()));
    template <class Arr, class A, class T>
    using resize_t = decltype(std::declval<Arr>().resize(std::declval<A>(), std::declval<T>()));
    template <class Arr, class A>
    using get_t = decltype(std::declval<Arr>().get(std::declval<A>()));
    template <class Arr, class A, class T>
    using get_or_t = decltype(std::declval<Arr>().get_or(std::declval<A>(), std::declval<T>()));
    template <class Arr, class A>
    using at_t = decltype(std::declval<Arr>().at(std::declval<A>()));
    template <class Arr, class T>
    using front_or_t = decltype(std::declval<Arr>().front_or(std::declval<T>()));
    template <class Arr, class T>
    using back_or_t = decltype(std::declval<Arr>().back_or(std::declval<T>()));

    template <class Index>
    struct probable_integer :
      meta::conjunction<
        meta::negation<
          std::is_convertible<
            Index,
            shim::string_view
          >
        >,
        meta::negation<
          meta::is_specialization_of<
            std::decay_t<Index>,
            dart::basic_string
          >
        >
      >
    {};

    public:

      /*----- Public Types -----*/

      using value_type = Array;
      using type = typename Array::type;
      using iterator = typename Array::iterator;
      using size_type = typename Array::size_type;
      using generic_type = typename value_type::generic_type;
      using reverse_iterator = typename Array::reverse_iterator;

      /*----- Lifecycle Functions -----*/

      /**
       *  @brief
       *  Constructor can be used to directly initialize a strongly
       *  typed array with a set of initial contents.
       *
       *  @details
       *  Internally calls make_array on the underlying packet type
       *  (if it's implemented) and forwards through the provided
       *  arguments.
       */
      template <class... Args, class EnableIf =
        std::enable_if_t<
          meta::is_detected<make_array_t, Array, Args...>::value
          &&
          (
            sizeof...(Args) > 1
            ||
            !meta::is_specialization_of<
              meta::first_type_t<std::decay_t<Args>...>,
              dart::basic_array
            >::value
          )
        >
      >
      explicit basic_array(Args&&... the_args) :
        val(value_type::make_array(std::forward<Args>(the_args)...))
      {}

      /**
       *  @brief
       *  Constructor allows an existing packet instance to be wrapped
       *  under the strongly typed API.
       *
       *  @details
       *  If the passed packet is not an array, will throw an exception.
       */
      template <class Arr, class EnableIf =
        std::enable_if_t<
          std::is_same<
            std::decay_t<Arr>,
            Array
          >::value
        >
      >
      explicit basic_array(Arr&& arr);

      /**
       *  @brief
       *  Converting constructor to allow interoperability between underlying
       *  packet types for strongly typed arrays.
       *
       *  @details
       *  Constructor is declared to be conditionally explicit based on whether
       *  the underlying implementation types are implicitly convertible.
       *  As a result, this code:
       *  ```
       *  dart::heap::array arr {"hello", "world"};
       *  dart::packet::array dup = arr;
       *  ```
       *  Will compile, whereas this:
       *  ```
       *  dart::packet::array arr {"hello", "world"};
       *  dart::heap::array dup = arr;
       *  ```
       *  Will not. It would need to use an explicit cast as such:
       *  ```
       *  dart::packet::array arr {"hello", "world"};
       *  dart::heap::array dup {arr};
       *  ```
       */
      template <class Arr,
        std::enable_if_t<
          !std::is_convertible<
            Arr,
            Array
          >::value
          &&
          std::is_constructible<
            Array,
            Arr
          >::value
        >* = nullptr
      >
      explicit basic_array(basic_array<Arr> const& arr) : val(value_type {arr.val}) {}

      /**
       *  @brief
       *  Converting constructor to allow interoperability between underlying
       *  packet types for strongly typed arrays.
       *
       *  @details
       *  Constructor is declared to be conditionally explicit based on whether
       *  the underlying implementation types are implicitly convertible.
       *  As a result, this code:
       *  ```
       *  dart::heap::array arr {"hello", "world"};
       *  dart::packet::array dup = arr;
       *  ```
       *  Will compile, whereas this:
       *  ```
       *  dart::packet::array arr {"hello", "world"};
       *  dart::heap::array dup = arr;
       *  ```
       *  Will not. It would need to use an explicit cast as such:
       *  ```
       *  dart::packet::array arr {"hello", "world"};
       *  dart::heap::array dup {arr};
       *  ```
       */
      template <class Arr,
        std::enable_if_t<
          std::is_convertible<
            Arr,
            Array
          >::value
        >* = nullptr
      >
      basic_array(basic_array<Arr> const& arr) : val(arr.val) {}

      /**
       *  @brief
       *  Copy constructor.
       *
       *  @details
       *  Increments refcount of underlying implementation type.
       */
      basic_array(basic_array const&) = default;

      /**
       *  @brief
       *  Move constructor.
       *
       *  @details
       *  Steals the refcount of an expiring strongly typed object.
       */
      basic_array(basic_array&&) noexcept = default;

      /**
       *  @brief
       *  Destructor. Does what you think it does.
       */
      ~basic_array() = default;

      /*----- Operators -----*/

      /**
       *  @brief
       *  Copy assignment operator.
       *
       *  @details
       *  Increments the refcount of the underlying implementation type.
       */
      basic_array& operator =(basic_array const&) & = default;

      // An MSVC bug prevents us from deleting based on ref qualifiers.
#if !DART_USING_MSVC
      basic_array& operator =(basic_array const&) && = delete;
#endif

      /**
       *  @brief
       *  Move assignment operator.
       *
       *  @details
       *  Steals the refcount of an expiring strongly typed object.
       */
      basic_array& operator =(basic_array&&) & = default;

      // An MSVC bug prevents us from deleting based on ref qualifiers.
#if !DART_USING_MSVC
      basic_array& operator =(basic_array&&) && = delete;
#endif

      /**
       *  @brief
       *  Array subscript operator.
       *
       *  @details
       *  Will return the requested array index as a new packet, or null if the
       *  requested index was out of bounds.
       *
       *  @remarks
       *  Function returns null if the requested mapping isn't present, but cannot
       *  be marked noexcept due to the possibility that the user might pass a
       *  dart::packet which is not an integer.
       *  If you pass this function reasonable things, it'll never throw an exception.
       *
       *  Not throwing an exception, and simply returning null, in the case of an out
       *  of bounds access is potentially a contentious design move.
       *  It was done in order to keep the "logical" exception behavior of element access
       *  in line with std::vector, but with different preconditions.
       */
      template <class Index, class EnableIf =
        std::enable_if_t<
          probable_integer<Index>::value
          &&
          meta::is_detected<subscript_operator_t, value_type const&, Index const&>::value
        >
      >
      auto operator [](Index const& idx) const& -> value_type;

      /**
       *  @brief
       *  Array subscript operator.
       *
       *  @details
       *  Will return the requested array index as a new packet.
       *
       *  @remarks
       *  Function returns null if the requested mapping isn't present, but cannot
       *  be marked noexcept due to the possibility that the user might pass a
       *  dart::packet which is not an integer.
       *  If you pass this function reasonable things, it'll never throw an exception.
       *
       *  Not throwing an exception, and simply returning null, in the case of an out
       *  of bounds access is potentially a contentious design move.
       *  It was done in order to keep the "logical" exception behavior of element access
       *  in line with std::vector, but with different preconditions.
       *
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto arr = some_array();
       *  auto deep = arr[0][1][2];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <class Index, class EnableIf =
        std::enable_if_t<
          probable_integer<Index>::value
          &&
          meta::is_detected<subscript_operator_t, value_type&&, Index const&>::value
        >
      >
      decltype(auto) operator [](Index const& idx) &&;

      /**
       *  @brief
       *  Equality operator.
       *  
       *  @details
       *  All packet types always return deep equality (aggregates included).
       *  While idiomatic for C++, this means that non-finalized object comparisons
       *  can be reasonably expensive.
       *  Finalized comparisons, however, use memcmp to compare the underlying
       *  byte buffers, and is _stupendously_ fast.
       *  
       *  @remarks
       *  "Do as vector does"
       */
      template <class OtherArray>
      bool operator ==(basic_array<OtherArray> const& other) const noexcept;

      /**
       *  @brief
       *  Inequality operator.
       *  
       *  @details
       *  All packet types always return deep equality (aggregates included).
       *  While idiomatic C++, this means that non-finalized object/array comparisons
       *  can be arbitrarily expensive.
       *  Finalized comparisons, however, use memcmp to compare the underlying
       *  byte buffers, and are _stupendously_ fast.
       *  
       *  @remarks
       *  "Do as vector does"
       */
      template <class OtherArray>
      bool operator !=(basic_array<OtherArray> const& other) const noexcept;

      /**
       *  @brief
       *  Implicit conversion operator to underlying implementation type.
       *
       *  @details
       *  Dart is primarily a dynamically typed library, and so, while the type
       *  wrappers exist to enable additional expressiveness and readability,
       *  they shouldn't make the library more cumbersome to work with, and so
       *  type wrappers are allowed to implicitly discard their type information
       *  if the user requests it.
       */
      operator value_type() const& noexcept;

      /**
       *  @brief
       *  Implicit conversion operator to underlying implementation type.
       *
       *  @details
       *  Dart is primarily a dynamically typed library, and so, while the type
       *  wrappers exist to enable additional expressiveness and readability,
       *  they shouldn't make the library more cumbersome to work with, and so
       *  type wrappers are allowed to implicitly discard their type information
       *  if the user requests it.
       */
      operator value_type() && noexcept;

      /**
       *  @brief
       *  Existential operator (bool conversion operator).
       *
       *  @details
       *  Always returns true for strongly typed arrays.
       */
      explicit operator bool() const noexcept;

      /*----- Public API -----*/

      /*----- Aggregate Builder Functions -----*/

      /**
       *  @brief
       *  Function inserts the given element at the front of a non-finalized array.
       *
       *  @details
       *  Given element can be any type for which a type conversion is defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto arr = get_array();
       *  auto nested = arr[0].push_front("testing 1, 2, 3").push_front("a, b, c");
       *  ```
       */
      template <class ValueType, class EnableIf =
        std::enable_if_t<
          meta::is_detected<push_front_t, value_type&, ValueType>::value
        >
      >
      basic_array& push_front(ValueType&& value) &;

      /**
       *  @brief
       *  Function inserts the given element at the front of a non-finalized array.
       *
       *  @details
       *  Given element can be any type for which a type conversion is defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto arr = get_array();
       *  auto nested = arr[0].push_front("testing 1, 2, 3").push_front("a, b, c");
       *  ```
       */
      template <class ValueType, class EnableIf =
        std::enable_if_t<
          meta::is_detected<push_front_t, value_type&&, ValueType>::value
        >
      >
      DART_NODISCARD basic_array&& push_front(ValueType&& value) &&;
      
      /**
       *  @brief
       *  Function removes the first element of a non-finalized array.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto arr = get_array();
       *  auto nested = arr[0].pop_front().pop_front();
       *  ```
       */
      template <class Arr = Array, class EnableIf =
        std::enable_if_t<
          meta::is_detected<pop_front_t, Arr&>::value
        >
      >
      basic_array& pop_front() &;

      /**
       *  @brief
       *  Function removes the first element of a non-finalized array.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto arr = get_array();
       *  auto nested = arr[0].pop_front().pop_front();
       *  ```
       */
      template <class Arr = Array, class EnableIf =
        std::enable_if_t<
          meta::is_detected<pop_front_t, Arr&&>::value
        >
      >
      DART_NODISCARD basic_array&& pop_front() &&;

      /**
       *  @brief
       *  Function inserts the given element at the end of a non-finalized array.
       *
       *  @details
       *  Given element can be any type for which a type conversion is defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto arr = get_array();
       *  auto nested = arr[0].push_back("testing 1, 2, 3").push_back("a, b, c");
       *  ```
       */
      template <class ValueType, class EnableIf =
        std::enable_if_t<
          meta::is_detected<push_back_t, value_type&, ValueType>::value
        >
      >
      basic_array& push_back(ValueType&& value) &;

      /**
       *  @brief
       *  Function inserts the given element at the end of a non-finalized array.
       *
       *  @details
       *  Given element can be any type for which a type conversion is defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto arr = get_array();
       *  auto nested = arr[0].push_back("testing 1, 2, 3").push_back("a, b, c");
       *  ```
       */
      template <class ValueType, class EnableIf =
        std::enable_if_t<
          meta::is_detected<push_back_t, value_type&&, ValueType>::value
        >
      >
      DART_NODISCARD basic_array&& push_back(ValueType&& value) &&;

      /**
       *  @brief
       *  Function removes the last element of a non-finalized array.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto arr = get_array();
       *  auto nested = arr[0].pop_back().pop_back();
       *  ```
       */
      template <class Arr = Array, class EnableIf =
        std::enable_if_t<
          meta::is_detected<pop_back_t, Arr&>::value
        >
      >
      basic_array& pop_back() &;

      /**
       *  @brief
       *  Function removes the last element of a non-finalized array.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto arr = get_array();
       *  auto nested = arr[0].pop_back().pop_back();
       *  ```
       */
      template <class Arr = Array, class EnableIf =
        std::enable_if_t<
          meta::is_detected<pop_back_t, Arr&&>::value
        >
      >
      DART_NODISCARD basic_array&& pop_back() &&;

      /**
       *  @brief
       *  Function adds, replaces, or inserts, a (new) index-value pair to a non-finalized
       *  array
       *
       *  @details
       *  Given key and value can be any types for which type conversions are defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *  The result of converting the key must yield an integer.
       */
      template <class Index, class ValueType, class EnableIf =
        std::enable_if_t<
          meta::is_detected<insert_t, value_type&, Index, ValueType>::value
        >
      >
      auto insert(Index&& idx, ValueType&& value) -> iterator;

      /**
       *  @brief
       *  Function replaces an index-value pair in a non-finalized array.
       *
       *  @details
       *  Given key and value can be any types for which type conversions are defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *  The reuslt of converting the key must yield an integer.
       */
      template <class Index, class ValueType, class EnableIf =
        std::enable_if_t<
          meta::is_detected<set_t, value_type&, Index, ValueType>::value
        >
      >
      auto set(Index&& idx, ValueType&& value) -> iterator;

      /**
       *  @brief
       *  Function removes an index-value pair from a non-finalized array.
       *
       *  @details
       *  If the index-value pair is not present in the current aggregate, function
       *  returns the end iterator, otherwise it returns an iterator to one past the element
       *  removed.
       */
      template <class Index, class EnableIf =
        std::enable_if_t<
          probable_integer<Index>::value
          &&
          meta::is_detected<erase_t, value_type&, Index const&>::value
        >
      >
      auto erase(Index const& idx) -> iterator;

      /**
       *  @brief
       *  Function clears an aggregate of all values.
       *
       *  @details
       *  If this is non-finalized, will remove all elements.
       */
      template <class Arr = Array, class EnableIf =
        std::enable_if_t<
          meta::is_detected<clear_t, Arr&>::value
        >
      >
      void clear();

      /*----- State Manipulation Functions -----*/

      /**
       *  @brief
       *  Function ensures enough underlying storage for at least
       *  as many elements as specified.
       *
       *  @details
       *  If used judiciously, can increase insertion performance.
       *  If used poorly, will definitely decrease insertion performance.
       */
      template <class Arr = Array, class EnableIf =
        std::enable_if_t<
          meta::is_detected<reserve_t, Arr&, size_type>::value
        >
      >
      void reserve(size_type count);

      /**
       *  @brief
       *  Function sets the current capacity of the underlying array storage.
       *
       *  @details
       *  If the underlying array grows, values are initialized by casting
       *  the supplied default argument according to the usual conversion
       *  API rules.
       */
      template <class T = std::nullptr_t, class EnableIf =
        std::enable_if_t<
          meta::is_detected<resize_t, value_type&, size_type, T const&>::value
        >
      >
      void resize(size_type count, T const& def = T {});

      /*----- JSON Helper Functions -----*/

#if DART_HAS_RAPIDJSON
      /**
       *  @brief
       *  Function serializes any packet into a JSON string.
       *
       *  @details
       *  Serialization is based on RapidJSON, and so exposes the same customization points as
       *  RapidJSON.
       *  If your packet has NaN or +/-Infinity values, you can serialize it in the following way:
       *  ```
       *  auto obj = some_object();
       *  auto json = obj.to_json<dart::write_nan>();
       *  do_something(std::move(json));
       *  ```
       */
      template <unsigned flags = write_default>
      std::string to_json() const;
#endif

      /*----- Value Accessor Functions -----*/

      /**
       *  @brief
       *  Array access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming the requested index is within bounds, will return the requested array index
       *  as a new packet.
       */
      template <class Index, class EnableIf =
        std::enable_if_t<
          probable_integer<Index>::value
          &&
          meta::is_detected<get_t, value_type const&, Index const&>::value
        >
      >
      auto get(Index const& idx) const& -> value_type;

      /**
       *  @brief
       *  Array access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming the requested index is within bounds, will return the requested array index
       *  as a new packet.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto arr = some_array();
       *  auto deep = arr[0][1][2];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <class Index, class EnableIf =
        std::enable_if_t<
          probable_integer<Index>::value
          &&
          meta::is_detected<get_t, value_type&&, Index const&>::value
        >
      >
      decltype(auto) get(Index const& idx) &&;

      /**
       *  @brief
       *  Optional array access method, similar to std::optional::value_or.
       *
       *  @details
       *  If this contains the specified index, function returns the value
       *  associated with that index.
       *  If the specified index is out of bounds, function returns the optional
       *  value cast to a dart::packet according to the usual conversion API rules.
       *
       *  @remarks
       *  Function is "logically" noexcept, but cannot be marked so as allocations necessary
       *  for converting the optional value might throw, and user defined conversions aren't
       *  required to be noexcept.
       */
      template <class Index, class T, class EnableIf =
        std::enable_if_t<
          probable_integer<Index>::value
          &&
          meta::is_detected<get_or_t, value_type const&, Index const&, T>::value
        >
      >
      auto get_or(Index const& idx, T&& opt) const -> value_type;

      /**
       *  @brief
       *  Array access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming the requested index is within bounds, will return the requested array index
       *  as a new packet.
       */
      template <class Index, class EnableIf =
        std::enable_if_t<
          probable_integer<Index>::value
          &&
          meta::is_detected<at_t, value_type const&, Index const&>::value
        >
      >
      auto at(Index const& idx) const& -> value_type;

      /**
       *  @brief
       *  Array access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming the requested index is within bounds, will return the requested array index
       *  as a new packet.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto arr = some_array();
       *  auto deep = arr[0][1][2];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <class Index, class EnableIf =
        std::enable_if_t<
          probable_integer<Index>::value
          &&
          meta::is_detected<at_t, value_type&&, Index const&>::value
        >
      >
      decltype(auto) at(Index const& idx) &&;

      /**
       *  @brief
       *  Function returns the first element in an array.
       *
       *  @details
       *  Return the first element or throws if the array is empty.
       */
      auto at_front() const& -> value_type;

      /**
       *  @brief
       *  Function returns the first element in an array.
       *
       *  @details
       *  Return the first element or throws if the array is empty.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto arr = some_array();
       *  auto deep = arr[0][1][2];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      decltype(auto) at_front() &&;

      /**
       *  @brief
       *  Function returns the last element in an array.
       *
       *  @details
       *  Returns the last element or throws if the array is empty.
       */
      auto at_back() const& -> value_type;

      /**
       *  @brief
       *  Function returns the last element in an array.
       *
       *  @details
       *  Returns the last element or throws if the array is empty.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto arr = some_array();
       *  auto deep = arr[0][1][2];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      decltype(auto) at_back() &&;

      /**
       *  @brief
       *  Function returns the first element in an array.
       *
       *  @details
       *  Returns the first element or null.
       */
      auto front() const& -> value_type;

      /**
       *  @brief
       *  Function returns the first element in an array.
       *
       *  @details
       *  Returns the first element or null.
       */
      decltype(auto) front() &&;

      /**
       *  @brief
       *  Optionally returns the first element in an array.
       *
       *  @details
       *  If this has at least one thing in it, returns the first element.
       *  Otherwise, returns the optional value cast to a dart::packet
       *  according to the usual conversion API rules.
       */
      template <class T, class EnableIf =
        std::enable_if_t<
          meta::is_detected<front_or_t, value_type const&, T>::value
        >
      >
      auto front_or(T&& opt) const -> value_type;

      /**
       *  @brief
       *  Function returns the last element in an array.
       *
       *  @details
       *  If this has at least one thing in it, returns the last element.
       *  Throws otherwise.
       */
      auto back() const& -> value_type;

      /**
       *  @brief
       *  Function returns the last element in an array.
       *
       *  @details
       *  If this has at least one thing in it, returns the last element.
       *  Throws otherwise.
       */
      decltype(auto) back() &&;

      /**
       *  @brief
       *  Optionally returns the last element in an array.
       *
       *  @details
       *  If this has at least one thing in it, returns the last element.
       *  Otherwise, returns the optional value cast to a dart::packet
       *  according to the usual conversion API rules.
       */
      template <class T, class EnableIf =
        std::enable_if_t<
          meta::is_detected<back_or_t, value_type const&, T>::value
        >
      >
      auto back_or(T&& opt) const -> value_type;

      /**
       *  @brief
       *  Returns all subvalues of the current aggregate.
       *
       *  @details
       *  If this is an object or array, returns all of the values contained
       *  within
       *  Throws otherwise.
       */
      auto values() const -> std::vector<value_type>;

      /**
       *  @brief
       *  Function returns the size of the current packet, according to the semantics
       *  of its type.
       *
       *  @details
       *  If this is an object, returns the number of key-value pairs within the object.
       *  If this is an array, returns the number of elements within the array.
       *  If this is a string, returns the length of the string NOT INCLUDING the null
       *  terminator.
       */
      auto size() const noexcept -> size_type;

      /**
       *  @brief
       *  Function returns the current capacity of the underlying storage array.
       *
       *  @details
       *  If this is finalized, capacity is fixed.
       */
      auto capacity() const -> size_type;

      /**
       *  @brief
       *  Function returns whether the current packet is empty, according to the semantics
       *  of its type.
       *
       *  @details
       *  In actuality, returns whether size() == 0;
       */
      bool empty() const noexcept;

      /**
       *  @brief
       *  Helper function returns a const& to the underlying dynamic type.
       *
       *  @details
       *  Can be useful for efficiently implementing wrapper API behavior
       *  in some spots.
       */
      auto dynamic() const noexcept -> value_type const&;

      /*----- Introspection Functions -----*/

      /**
       *  @brief
       *  Returns whether the current packet is an object or not.
       */
      constexpr bool is_object() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an array or not.
       */
      bool is_array() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an object or array or not.
       */
      bool is_aggregate() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a string or not.
       */
      constexpr bool is_str() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an integer or not.
       */
      constexpr bool is_integer() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a decimal or not.
       */
      constexpr bool is_decimal() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an integer or decimal or not.
       */
      constexpr bool is_numeric() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a boolean or not.
       */
      constexpr bool is_boolean() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is null or not.
       */
      bool is_null() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a string, integer, decimal,
       *  boolean, or null.
       */
      constexpr bool is_primitive() const noexcept;

      /**
       *  @brief
       *  Returns the type of the current packet as an enum value.
       */
      type get_type() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is finalized.
       */
      bool is_finalized() const noexcept;

      /**
       *  @brief
       *  Returns the current reference count of the underlying reference counter.
       */
      auto refcount() const noexcept -> size_type;

      /*----- Iterator Functions -----*/

      /**
       *  @brief
       *  Function returns a dart::packet::iterator that can be used to iterate over the
       *  VALUES of the current packet.
       *
       *  @details
       *  Throws if this is not an object or array.
       */
      auto begin() const -> iterator;

      /**
       *  @brief
       *  Function returns a dart::packet::iterator that can be used to iterate over the
       *  VALUES of the current packet.
       *
       *  @details
       *  Throws if this is not an object or array.
       *
       *  @remarks
       *  Function exists for API uniformity with the STL, but is precisely equal to
       *  dart::packet::begin.
       */
      auto cbegin() const -> iterator;

      /**
       *  @brief
       *  Function returns a dart::packet::iterator one-past-the-end for iterating over
       *  the VALUES of the current packet.
       *
       *  @details
       *  Dart iterators are bidirectional, so the iterator could be decremented to iterate
       *  over the valuespace backwards, or it can be used as a sentinel value for
       *  traditional iteration.
       */
      auto end() const -> iterator;

      /**
       *  @brief
       *  Function returns a dart::packet::iterator one-past-the-end for iterating over
       *  the VALUES of the current packet.
       *
       *  @details
       *  Dart iterators are bidirectional, so the iterator could be decremented to iterate
       *  over the valuespace backwards, or it can be used as a sentinel value for
       *  traditional iteration.
       *
       *  @remarks
       *  Function exists for API uniformity with the STL, but is precisely equal to
       *  dart::packet::end.
       */
      auto cend() const -> iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator that iterates in reverse order from
       *  dart::packet::end to dart::packet::begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::begin.
       */
      auto rbegin() const -> reverse_iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator that iterates in reverse order from
       *  dart::packet::end to dart::packet::begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::begin.
       *
       *  @remarks
       *  Function exists for API uniformity with the STL, but is precisely equal to
       *  dart::packet::rbegin.
       */
      auto crbegin() const -> iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator one-past-the-end, that iterates in
       *  reverse order from dart::packet::end to dart::packet::begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::end.
       */
      auto rend() const -> reverse_iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator one-past-the-end, that iterates in
       *  reverse order from dart::packet::end to dart::packet::begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::end.
       *
       *  @remarks
       *  Function exists for API uniformity with the STL, but is precisely equal to
       *  dart::packet::rbegin.
       */
      auto crend() const -> iterator;

    private:

      /*----- Private Members -----*/

      value_type val;

      /*----- Friends -----*/

      // Allow type conversion logic to work.
      friend struct convert::detail::caster_impl<convert::detail::wrapper_tag>;

      // We're friends of all other basic_array specializations.
      template <class>
      friend class basic_array;

  };

  /**
   *  brief
   *  Class operates as a strongly-typed wrapper around any of the
   *  dart::packet, dart::heap, dart::buffer classes, enforcing that
   *  any wrapped packet must be a string.
   *
   *  @details
   *  Dart is primarly a dynamically typed library for interacting with
   *  a dynamically typed notation language, but as C++ is a statically
   *  typed language, it can often be useful to statically know the type
   *  of a packet at compile-time.
   *  Type is implemented as a wrapper around the existing Dart types,
   *  and therefore does not come with any performance benefit. It is
   *  intended to be used to increase readability/enforce
   *  constraints/preconditions where applicable.
   */
  template <class String>
  class basic_string {

    template <class Str, class... Args>
    using make_string_t = decltype(Str::make_string(std::declval<Args>()...));

    public:

      /*----- Public Types -----*/

      using value_type = String;
      using type = typename String::type;
      using size_type = typename String::size_type;
      using generic_type = typename value_type::generic_type;

      /*----- Lifecycle Functions -----*/

      /**
       *  @brief
       *  Constructor can be used to initialize a strongly typed
       *  string as the empty string.
       *
       *  @details
       *  Internally calls make_string on the underlying packet type
       *  (if implemented).
       */
      template <class Str = String, class EnableIf =
        std::enable_if_t<
          meta::is_detected<make_string_t, Str, shim::string_view>::value
        >
      >
      basic_string() : basic_string("") {}

      /**
       *  @brief
       *  Constructor can be used to directly initialize a strongly
       *  typed string with an initial value.
       *
       *  @details
       *  Internally calls make_string on the underlying packet type
       *  (if implemented).
       */
      template <class Str = String, class EnableIf =
        std::enable_if_t<
          meta::is_detected<make_string_t, Str, shim::string_view>::value
        >
      >
      explicit basic_string(shim::string_view str) :
        val(value_type::make_string(str))
      {}

      /**
       *  @brief
       *  Constructor allows an existing packet instance to be wrapped
       *  under the storngly typed API.
       *
       *  @details
       *  If the passed packet is not a string, will throw an exception.
       */
      template <class Str, class EnableIf =
        std::enable_if_t<
          std::is_same<
            std::decay_t<Str>,
            String
          >::value
        >
      >
      explicit basic_string(Str&& val);

      /**
       *  @brief
       *  Converting constructor to allow interoperability between underlying
       *  packet types for strongly typed strings.
       *
       *  @details
       *  Constructor is declared to be conditionally explicit based on whether
       *  the underlying implementation types are implicitly convertible.
       *  As a result, this code:
       *  ```
       *  dart::heap::string str {"hello world"};
       *  dart::packet::string dup = str;
       *  ```
       *  Will compile, whereas this:
       *  ```
       *  dart::packet::string str {"hello world"};
       *  dart::heap::string dup = str;
       *  ```
       *  Will not. It would need to use an explicit cast as such:
       *  ```
       *  dart::packet::string str {"hello world"};
       *  dart::heap::string dup {str};
       *  ```
       */
      template <class Str,
        std::enable_if_t<
          !std::is_convertible<
            Str,
            String
          >::value
          &&
          std::is_constructible<
            String,
            Str
          >::value
        >* = nullptr
      >
      explicit basic_string(basic_string<Str> const& str) : val(value_type {str.val}) {}

      /**
       *  @brief
       *  Converting constructor to allow interoperability between underlying
       *  packet types for strongly typed strings.
       *
       *  @details
       *  Constructor is declared to be conditionally explicit based on whether
       *  the underlying implementation types are implicitly convertible.
       *  As a result, this code:
       *  ```
       *  dart::heap::string str {"hello world"};
       *  dart::packet::string dup = str;
       *  ```
       *  Will compile, whereas this:
       *  ```
       *  dart::packet::string str {"hello world"};
       *  dart::heap::string dup = str;
       *  ```
       *  Will not. It would need to use an explicit cast as such:
       *  ```
       *  dart::packet::string str {"hello world"};
       *  dart::heap::string dup {str};
       *  ```
       */
      template <class Str,
        std::enable_if_t<
          std::is_convertible<
            Str,
            String
          >::value
        >* = nullptr
      >
      basic_string(basic_string<Str> const& str) : val(str.val) {}

      /**
       *  @brief
       *  Copy constructor.
       *
       *  @details
       *  Increments refcount of underlying implementation type.
       */
      basic_string(basic_string const&) = default;

      /**
       *  @brief
       *  Move constructor.
       *
       *  @details
       *  Steals the refcount of an expiring strongly typed object.
       */
      basic_string(basic_string&&) noexcept = default;

      /**
       *  @brief
       *  Destructor. Does what you think it does.
       */
      ~basic_string() = default;

      /*----- Operators -----*/

      /**
       *  @brief
       *  Copy assignment operator.
       *
       *  @details
       *  Increments the refcount of the underlying implementation type.
       */
      basic_string& operator =(basic_string const&) & = default;

      // An MSVC bug prevents us from deleting based on ref qualifiers.
#if !DART_USING_MSVC
      basic_string& operator =(basic_string const&) && = delete;
#endif

      /**
       *  @brief
       *  Move assignment operator.
       *
       *  @details
       *  Steals the refcount of an expiring strongly typed object.
       */
      basic_string& operator =(basic_string&&) & = default;

      // An MSVC bug prevents us from deleting based on ref qualifiers.
#if !DART_USING_MSVC
      basic_string& operator =(basic_string&&) && = delete;
#endif

      /**
       *  @brief
       *  Equality operator.
       *  
       *  @details
       *  All packet types always return deep equality (aggregates included).
       *  While idiomatic for C++, this means that non-finalized object comparisons
       *  can be reasonably expensive.
       *  Finalized comparisons, however, use memcmp to compare the underlying
       *  byte buffers, and is _stupendously_ fast.
       *  
       *  @remarks
       *  "Do as vector does"
       */
      template <class OtherString>
      bool operator ==(basic_string<OtherString> const& other) const noexcept;

      /**
       *  @brief
       *  Inequality operator.
       *  
       *  @details
       *  All packet types always return deep equality (aggregates included).
       *  While idiomatic C++, this means that non-finalized object/array comparisons
       *  can be arbitrarily expensive.
       *  Finalized comparisons, however, use memcmp to compare the underlying
       *  byte buffers, and are _stupendously_ fast.
       *  
       *  @remarks
       *  "Do as vector does"
       */
      template <class OtherString>
      bool operator !=(basic_string<OtherString> const& other) const noexcept;

      /**
       *  @brief
       *  Dereference operator.
       *
       *  @details
       *  Returns the string value as a string_view.
       */
      shim::string_view operator *() const noexcept;

      /**
       *  @brief
       *  Implicit conversion operator to underlying implementation type.
       *
       *  @details
       *  Dart is primarily a dynamically typed library, and so, while the type
       *  wrappers exist to enable additional expressiveness and readability,
       *  they shouldn't make the library more cumbersome to work with, and so
       *  type wrappers are allowed to implicitly discard their type information
       *  if the user requests it.
       */
      operator value_type() const& noexcept;

      /**
       *  @brief
       *  Implicit conversion operator to underlying implementation type.
       *
       *  @details
       *  Dart is primarily a dynamically typed library, and so, while the type
       *  wrappers exist to enable additional expressiveness and readability,
       *  they shouldn't make the library more cumbersome to work with, and so
       *  type wrappers are allowed to implicitly discard their type information
       *  if the user requests it.
       */
      operator value_type() && noexcept;

      /**
       *  @brief
       *  String conversion operator.
       */
      explicit operator std::string() const;

      /**
       *  @brief
       *  String conversion operator.
       */
      explicit operator shim::string_view() const noexcept;

      /**
       *  @brief
       *  Existential operator (bool conversion operator).
       *
       *  @details
       *  Operator always returns true for strongly typed strings.
       */
      explicit operator bool() const noexcept;

      /*----- Public API -----*/

      /*----- JSON Helper Functions -----*/

#if DART_HAS_RAPIDJSON
      /**
       *  @brief
       *  Function serializes any packet into a JSON string.
       *
       *  @details
       *  Serialization is based on RapidJSON, and so exposes the same customization points as
       *  RapidJSON.
       *  If your packet has NaN or +/-Infinity values, you can serialize it in the following way:
       *  ```
       *  auto obj = some_object();
       *  auto json = obj.to_json<dart::write_nan>();
       *  do_something(std::move(json));
       *  ```
       */
      template <unsigned flags = write_default>
      std::string to_json() const;
#endif

      /*----- Value Accessor Functions -----*/

      /**
       *  @brief
       *  Function returns a null-terminated c-string representing the current packet.
       *
       *  @remarks
       *  Lifetime of the returned pointer is tied to the current instance.
       */
      char const* str() const noexcept;

      /**
       *  @brief
       *  Function returns a string view representing the current packet.
       *
       *  @remarks
       *  Lifetime of the string view is tied to the current instance.
       */
      shim::string_view strv() const noexcept;

      /**
       *  @brief
       *  Function returns the length of the current string value.
       */
      auto size() const noexcept -> size_type;

      /**
       *  @brief
       *  Function returns whether the current packet is empty, according to the semantics
       *  of its type.
       *
       *  @details
       *  In actuality, returns whether size() == 0;
       */
      bool empty() const noexcept;

      /**
       *  @brief
       *  Helper function returns a const& to the underlying dynamic type.
       *
       *  @details
       *  Can be useful for efficiently implementing wrapper API behavior
       *  in some spots.
       */
      auto dynamic() const noexcept -> value_type const&;

      /*----- Introspection Functions -----*/

      /**
       *  @brief
       *  Returns whether the current packet is an object or not.
       */
      constexpr bool is_object() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an array or not.
       */
      constexpr bool is_array() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an object or array or not.
       */
      constexpr bool is_aggregate() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a string or not.
       */
      bool is_str() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an integer or not.
       */
      constexpr bool is_integer() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a decimal or not.
       */
      constexpr bool is_decimal() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an integer or decimal or not.
       */
      constexpr bool is_numeric() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a boolean or not.
       */
      constexpr bool is_boolean() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is null or not.
       */
      bool is_null() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a string, integer, decimal,
       *  boolean, or null.
       */
      constexpr bool is_primitive() const noexcept;

      /**
       *  @brief
       *  Returns the type of the current packet as an enum value.
       */
      type get_type() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is finalized.
       */
      bool is_finalized() const noexcept;

      /**
       *  @brief
       *  Returns the current reference count of the underlying reference counter.
       */
      auto refcount() const noexcept -> size_type;

    private:

      /*----- Private Members -----*/

      value_type val;

      /*----- Friends -----*/

      // Allow type conversion logic to work.
      friend struct convert::detail::caster_impl<convert::detail::wrapper_tag>;

      // We're friends of all other basic_string specializations.
      template <class>
      friend class basic_string;

  };

  /**
   *  @brief
   *  Class operates as a strongly-typed wrapper around any of the
   *  dart::packet, dart::heap, dart::buffer classes, enforcing that
   *  any wrapped packet must be a number.
   *
   *  @details
   *  Dart is primarly a dynamically typed library for interacting with
   *  a dynamically typed notation language, but as C++ is a statically
   *  typed language, it can often be useful to statically know the type
   *  of a packet at compile-time.
   *  Type is implemented as a wrapper around the existing Dart types,
   *  and therefore does not come with any performance benefit. It is
   *  intended to be used to increase readability/enforce
   *  constraints/preconditions where applicable.
   */
  template <class Number>
  class basic_number {
    
    template <class Num, class... Args>
    using make_integer_t = decltype(Num::make_integer(std::declval<Args>()...));
    template <class Num, class... Args>
    using make_decimal_t = decltype(Num::make_decimal(std::declval<Args>()...));

    public:

      /*----- Public Types -----*/

      using value_type = Number;
      using type = typename Number::type;
      using size_type = typename Number::size_type;
      using generic_type = typename value_type::generic_type;

      /*----- Lifecycle Functions -----*/

      /**
       *  @brief
       *  Constructor can be used to initialize a strongly typed
       *  number as zero.
       *
       *  @details
       *  Internally calls make_integer on the underlying packet type
       *  (if implemented).
       */
      template <class Num = Number, class EnableIf =
        std::enable_if_t<
          meta::is_detected<make_integer_t, Num, int64_t>::value
        >
      >
      basic_number() : basic_number(0) {}

      /**
       *  @brief
       *  Constructor can be used to directly initialize a strongly
       *  typed number with an initial value.
       */
      template <class Arg,
        std::enable_if_t<
          meta::is_detected<make_integer_t, value_type, int64_t>::value
          &&
          std::is_integral<Arg>::value
        >* = nullptr
      >
      explicit basic_number(Arg val) :
        val(value_type::make_integer(val))
      {}

      /**
       *  @brief
       *  Constructor can be used to directly initialize a strongly
       *  typed number with an initial value.
       */
      template <class Arg, 
        std::enable_if_t<
          meta::is_detected<make_decimal_t, value_type, double>::value
          &&
          std::is_floating_point<Arg>::value
        >* = nullptr
      >
      explicit basic_number(Arg val) :
        val(value_type::make_decimal(val))
      {}

      /**
       *  @brief
       *  Constructor allows an existing packet instance to be wrapped
       *  under the strongly typed API.
       *
       *  @details
       *  If the passed packet is not a number, will throw an exception.
       */
      template <class Num, class EnableIf =
        std::enable_if_t<
          std::is_same<
            std::decay_t<Num>,
            Number 
          >::value
        >
      >
      explicit basic_number(Num&& val);

      /**
       *  @brief
       *  Converting constructor to allow interoperability between underlying
       *  packet types for strongly typed numbers.
       *
       *  @details
       *  Constructor is declared to be conditionally explicit based on whether
       *  the underlying implementation types are implicitly convertible.
       *  As a result, this code:
       *  ```
       *  dart::heap::number num {5};
       *  dart::packet::number dup = num;
       *  ```
       *  Will compile, whereas this:
       *  ```
       *  dart::packet::number num {5};
       *  dart::heap::number dup = num;
       *  ```
       *  Will not. It would need to use an explicit cast as such:
       *  ```
       *  dart::packet::number num {5};
       *  dart::heap::number dup {num};
       *  ```
       */
      template <class Num,
        std::enable_if_t<
          !std::is_convertible<
            Num,
            Number
          >::value
          &&
          std::is_constructible<
            Number,
            Num
          >::value
        >* = nullptr
      >
      explicit basic_number(basic_number<Num> const& num) : val(value_type {num.val}) {}

      /**
       *  @brief
       *  Converting constructor to allow interoperability between underlying
       *  packet types for strongly typed numbers.
       *
       *  @details
       *  Constructor is declared to be conditionally explicit based on whether
       *  the underlying implementation types are implicitly convertible.
       *  As a result, this code:
       *  ```
       *  dart::heap::number num {5};
       *  dart::packet::number dup = num;
       *  ```
       *  Will compile, whereas this:
       *  ```
       *  dart::packet::number num {5};
       *  dart::heap::number dup = num;
       *  ```
       *  Will not. It would need to use an explicit cast as such:
       *  ```
       *  dart::packet::number num {5};
       *  dart::heap::number dup {num};
       *  ```
       */
      template <class Num,
        std::enable_if_t<
          std::is_convertible<
            Num,
            Number
          >::value
        >* = nullptr
      >
      basic_number(basic_number<Num> const& num) : val(num.val) {}

      /**
       *  @brief
       *  Copy constructor.
       *
       *  @details
       *  Increments refcount of underlying implementation type.
       */
      basic_number(basic_number const&) = default;

      /**
       *  @brief
       *  Move constructor.
       *
       *  @details
       *  Steals the refcount of an expiring strongly typed object.
       */
      basic_number(basic_number&&) noexcept = default;

      /**
       *  @brief
       *  Destructor. Does what you think it does.
       */
      ~basic_number() = default;

      /*----- Operators -----*/

      /**
       *  @brief
       *  Copy assignment operator.
       *
       *  @details
       *  Increments the refcount of the underlying implementation type.
       */
      basic_number& operator =(basic_number const&) & = default;

      // An MSVC bug prevents us from deleting based on ref qualifiers.
#if !DART_USING_MSVC
      basic_number& operator =(basic_number const&) && = delete;
#endif

      /**
       *  @brief
       *  Move assignment operator.
       *
       *  @details
       *  Steals the refcount of an expiring strongly typed object.
       */
      basic_number& operator =(basic_number&&) & = default;

      // An MSVC bug prevents us from deleting based on ref qualifiers.
#if !DART_USING_MSVC
      basic_number& operator =(basic_number&&) && = delete;
#endif

      /**
       *  @brief
       *  Equality operator.
       *  
       *  @details
       *  All packet types always return deep equality (aggregates included).
       *  While idiomatic for C++, this means that non-finalized object comparisons
       *  can be reasonably expensive.
       *  Finalized comparisons, however, use memcmp to compare the underlying
       *  byte buffers, and is _stupendously_ fast.
       *  
       *  @remarks
       *  "Do as vector does"
       */
      template <class OtherNumber>
      bool operator ==(basic_number<OtherNumber> const& other) const noexcept;

      /**
       *  @brief
       *  Inequality operator.
       *  
       *  @details
       *  All packet types always return deep equality (aggregates included).
       *  While idiomatic C++, this means that non-finalized object/array comparisons
       *  can be arbitrarily expensive.
       *  Finalized comparisons, however, use memcmp to compare the underlying
       *  byte buffers, and are _stupendously_ fast.
       *  
       *  @remarks
       *  "Do as vector does"
       */
      template <class OtherNumber>
      bool operator !=(basic_number<OtherNumber> const& other) const noexcept;

      /**
       *  @brief
       *  Dereference operator.
       *
       *  @details
       *  Returns the number value as a double
       */
      double operator *() const noexcept;

      /**
       *  @brief
       *  Implicit conversion operator to underlying implementation type.
       *
       *  @details
       *  Dart is primarily a dynamically typed library, and so, while the type
       *  wrappers exist to enable additional expressiveness and readability,
       *  they shouldn't make the library more cumbersome to work with, and so
       *  type wrappers are allowed to implicitly discard their type information
       *  if the user requests it.
       */
      operator value_type() const& noexcept;

      /**
       *  @brief
       *  Implicit conversion operator to underlying implementation type.
       *
       *  @details
       *  Dart is primarily a dynamically typed library, and so, while the type
       *  wrappers exist to enable additional expressiveness and readability,
       *  they shouldn't make the library more cumbersome to work with, and so
       *  type wrappers are allowed to implicitly discard their type information
       *  if the user requests it.
       */
      operator value_type() && noexcept;

      /**
       *  @brief
       *  Machine type conversion operator.
       */
      explicit operator int64_t() const noexcept;

      /**
       *  @brief
       *  Machine type conversion operator.
       */
      explicit operator double() const noexcept;

      /**
       *  @brief
       *  Existential operator (bool conversion operator).
       *
       *  @details
       *  Operator always returns true for strongly typed numbers.
       */
      explicit operator bool() const noexcept;

      /*----- Public API -----*/

      /*----- JSON Helper Functions -----*/

#if DART_HAS_RAPIDJSON
      /**
       *  @brief
       *  Function serializes any packet into a JSON string.
       *
       *  @details
       *  Serialization is based on RapidJSON, and so exposes the same customization points as
       *  RapidJSON.
       *  If your packet has NaN or +/-Infinity values, you can serialize it in the following way:
       *  ```
       *  auto obj = some_object();
       *  auto json = obj.to_json<dart::write_nan>();
       *  do_something(std::move(json));
       *  ```
       */
      template <unsigned flags = write_default>
      std::string to_json() const;
#endif

      /*----- Value Accessor Functions -----*/

      /**
       *  @brief
       *  Function returns an integer representing the current packet.
       */
      int64_t integer() const;

      /**
       *  @brief
       *  Function returns a decimal representing the current packet.
       *
       *  @details
       *  If the current packet is a decimal, returns a it, otherwise throws.
       */
      double decimal() const;

      /**
       *  @brief
       *  Function returns a decimal representing the current packet.
       *
       *  @details
       *  If the current packet is an integer or decimal, returns a it as a decimal,
       *  otherwise throws.
       */
      double numeric() const noexcept;

      /**
       *  @brief
       *  Helper function returns a const& to the underlying dynamic type.
       *
       *  @details
       *  Can be useful for efficiently implementing wrapper API behavior
       *  in some spots.
       */
      auto dynamic() const noexcept -> value_type const&;

      /*----- Introspection Functions -----*/

      /**
       *  @brief
       *  Returns whether the current packet is an object or not.
       */
      constexpr bool is_object() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an array or not.
       */
      constexpr bool is_array() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an object or array or not.
       */
      constexpr bool is_aggregate() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a string or not.
       */
      constexpr bool is_str() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an integer or not.
       */
      bool is_integer() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a decimal or not.
       */
      bool is_decimal() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an integer or decimal or not.
       */
      bool is_numeric() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a boolean or not.
       */
      constexpr bool is_boolean() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is null or not.
       */
      bool is_null() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a string, integer, decimal,
       *  boolean, or null.
       */
      constexpr bool is_primitive() const noexcept;

      /**
       *  @brief
       *  Returns the type of the current packet as an enum value.
       */
      type get_type() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is finalized.
       */
      bool is_finalized() const noexcept;

      /**
       *  @brief
       *  Returns the current reference count of the underlying reference counter.
       */
      auto refcount() const noexcept -> size_type;

    private:

      /*----- Private Members -----*/

      value_type val;

      /*----- Friends -----*/

      // Allow type conversion logic to work.
      friend struct convert::detail::caster_impl<convert::detail::wrapper_tag>;

      // We're friends of all other basic_number specializations.
      template <class>
      friend class basic_number;

  };

  /**
   *  @brief
   *  Class operates as a strongly-typed wrapper around any of the
   *  dart::packet, dart::heap, dart::buffer classes, enforcing that
   *  any wrapped packet must be a boolean.
   *
   *  @details
   *  Dart is primarly a dynamically typed library for interacting with
   *  a dynamically typed notation language, but as C++ is a statically
   *  typed language, it can often be useful to statically know the type
   *  of a packet at compile-time.
   *  Type is implemented as a wrapper around the existing Dart types,
   *  and therefore does not come with any performance benefit. It is
   *  intended to be used to increase readability/enforce
   *  constraints/preconditions where applicable.
   */
  template <class Boolean>
  class basic_flag {

    template <class Bool, class... Args>
    using make_boolean_t = decltype(Bool::make_boolean(std::declval<Args>()...));

    public:

      /*----- Public Types -----*/

      using value_type = Boolean;
      using type = typename Boolean::type;
      using size_type = typename Boolean::size_type;
      using generic_type = typename value_type::generic_type;

      /*----- Lifecycle Functions -----*/

      /**
       *  @brief
       *  Constructor can be used to initialize a strongly typed
       *  flag as false.
       *
       *  @details
       *  Internally calls make_boolean on the underlying packet type
       *  (if implemented).
       */
      template <class Bool = Boolean, class EnableIf =
        std::enable_if_t<
          meta::is_detected<make_boolean_t, Bool, bool>::value
        >
      >
      basic_flag() : basic_flag(false) {}

      /**
       *  @brief
       *  Constructor can be used to directly initialize a strongly
       *  typed boolean with an initial value.
       *
       *  @details
       *  Internally calls make_boolean on the underlying packet type
       *  (if implemented).
       */
      template <class Bool = Boolean, class EnableIf =
        std::enable_if_t<
          meta::is_detected<make_boolean_t, Bool, bool>::value
        >
      >
      explicit basic_flag(bool val) :
        val(value_type::make_boolean(val))
      {}

      /**
       *  @brief
       *  Constructor allows an existing packet instance to be wrapped
       *  under the strongly typed API.
       *
       *  @details
       *  If the passed packet is not a boolean, will throw an exception.
       */
      template <class Bool, class EnableIf =
        std::enable_if_t<
          std::is_same<
            std::decay_t<Bool>,
            Boolean
          >::value
        >
      >
      explicit basic_flag(Bool&& val);

      /**
       *  @brief
       *  Converting constructor to allow interoperability between underlying
       *  packet types for strongly typed booleans.
       *
       *  @details
       *  Constructor is declared to be conditionally explicit based on whether
       *  the underlying implementation types are implicitly convertible.
       *  As a result, this code:
       *  ```
       *  dart::heap::flag flag {true};
       *  dart::packet::flag dup = flag;
       *  ```
       *  Will compile, whereas this:
       *  ```
       *  dart::packet::flag flag {true};
       *  dart::heap::flag dup = flag;
       *  ```
       *  Will not. It would need to use an explicit cast as such:
       *  ```
       *  dart::packet::flag flag {true};
       *  dart::heap::flag dup {flag};
       *  ```
       */
      template <class Bool,
        std::enable_if_t<
          !std::is_convertible<
            Bool,
            Boolean
          >::value
          &&
          std::is_constructible<
            Boolean,
            Bool
          >::value
        >* = nullptr
      >
      explicit basic_flag(basic_flag<Bool> const& flag) : val(value_type {flag.val}) {}

      /**
       *  @brief
       *  Converting constructor to allow interoperability between underlying
       *  packet types for strongly typed booleans.
       *
       *  @details
       *  Constructor is declared to be conditionally explicit based on whether
       *  the underlying implementation types are implicitly convertible.
       *  As a result, this code:
       *  ```
       *  dart::heap::flag flag {true};
       *  dart::packet::flag dup = flag;
       *  ```
       *  Will compile, whereas this:
       *  ```
       *  dart::packet::flag flag {true};
       *  dart::heap::flag dup = flag;
       *  ```
       *  Will not. It would need to use an explicit cast as such:
       *  ```
       *  dart::packet::flag flag {true};
       *  dart::heap::flag dup {flag};
       *  ```
       */
      template <class Bool,
        std::enable_if_t<
          std::is_convertible<
            Bool,
            Boolean
          >::value
        >* = nullptr
      >
      basic_flag(basic_flag<Bool> const& flag) : val(flag.val) {}

      /**
       *  @brief
       *  Copy constructor.
       *
       *  @details
       *  Increments refcount of underlying implementation type.
       */
      basic_flag(basic_flag const&) = default;

      /**
       *  @brief
       *  Move constructor.
       *
       *  @details
       *  Steals the refcount of an expiring strongly typed object.
       */
      basic_flag(basic_flag&&) noexcept = default;

      /**
       *  @brief
       *  Destructor. Does what you think it does.
       */
      ~basic_flag() = default;

      /*----- Operators -----*/

      /**
       *  @brief
       *  Copy assignment operator.
       *
       *  @details
       *  Increments the refcount of the underlying implementation type.
       */
      basic_flag& operator =(basic_flag const&) & = default;

      // An MSVC bug prevents us from deleting based on ref qualifiers.
#if !DART_USING_MSVC
      basic_flag& operator =(basic_flag const&) && = delete;
#endif

      /**
       *  @brief
       *  Move assignment operator.
       *
       *  @details
       *  Steals the refcount of an expiring strongly typed object.
       */
      basic_flag& operator =(basic_flag&&) & = default;

      // An MSVC bug prevents us from deleting based on ref qualifiers.
#if !DART_USING_MSVC
      basic_flag& operator =(basic_flag&&) && = delete;
#endif

      /**
       *  @brief
       *  Equality operator.
       *  
       *  @details
       *  All packet types always return deep equality (aggregates included).
       *  While idiomatic for C++, this means that non-finalized object comparisons
       *  can be reasonably expensive.
       *  Finalized comparisons, however, use memcmp to compare the underlying
       *  byte buffers, and is _stupendously_ fast.
       *  
       *  @remarks
       *  "Do as vector does"
       */
      template <class OtherBoolean>
      bool operator ==(basic_flag<OtherBoolean> const& other) const noexcept;

      /**
       *  @brief
       *  Inequality operator.
       *  
       *  @details
       *  All packet types always return deep equality (aggregates included).
       *  While idiomatic C++, this means that non-finalized object/array comparisons
       *  can be arbitrarily expensive.
       *  Finalized comparisons, however, use memcmp to compare the underlying
       *  byte buffers, and are _stupendously_ fast.
       *  
       *  @remarks
       *  "Do as vector does"
       */
      template <class OtherBoolean>
      bool operator !=(basic_flag<OtherBoolean> const& other) const noexcept;

      /**
       *  @brief
       *  Dereference operator.
       *
       *  @details
       *  Returns current boolean value.
       */
      bool operator *() const noexcept;

      /**
       *  @brief
       *  Implicit conversion operator to underlying implementation type.
       *
       *  @details
       *  Dart is primarily a dynamically typed library, and so, while the type
       *  wrappers exist to enable additional expressiveness and readability,
       *  they shouldn't make the library more cumbersome to work with, and so
       *  type wrappers are allowed to implicitly discard their type information
       *  if the user requests it.
       */
      operator value_type() const& noexcept;

      /**
       *  @brief
       *  Implicit conversion operator to underlying implementation type.
       *
       *  @details
       *  Dart is primarily a dynamically typed library, and so, while the type
       *  wrappers exist to enable additional expressiveness and readability,
       *  they shouldn't make the library more cumbersome to work with, and so
       *  type wrappers are allowed to implicitly discard their type information
       *  if the user requests it.
       */
      operator value_type() && noexcept;

      /**
       *  @brief
       *  Existential operator (bool conversion operator).
       *
       *  @details
       *  Returns the current boolean value for strongly typed booleans.
       */
      explicit operator bool() const noexcept;

      /*----- Public API -----*/

      /*----- JSON Helper Functions -----*/

#if DART_HAS_RAPIDJSON
      /**
       *  @brief
       *  Function serializes any packet into a JSON string.
       *
       *  @details
       *  Serialization is based on RapidJSON, and so exposes the same customization points as
       *  RapidJSON.
       *  If your packet has NaN or +/-Infinity values, you can serialize it in the following way:
       *  ```
       *  auto obj = some_object();
       *  auto json = obj.to_json<dart::write_nan>();
       *  do_something(std::move(json));
       *  ```
       */
      template <unsigned flags = write_default>
      std::string to_json() const;
#endif

      /*----- Value Accessor Functions -----*/

      /**
       *  @brief
       *  Function returns a boolean representing the current packet.
       *
       *  @details
       *  If the current packet is a boolean, returns a it, otherwise throws.
       */
      bool boolean() const noexcept;

      /**
       *  @brief
       *  Helper function returns a const& to the underlying dynamic type.
       *
       *  @details
       *  Can be useful for efficiently implementing wrapper API behavior
       *  in some spots.
       */
      auto dynamic() const noexcept -> value_type const&;

      /*----- Introspection Functions -----*/

      /**
       *  @brief
       *  Returns whether the current packet is an object or not.
       */
      constexpr bool is_object() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an array or not.
       */
      constexpr bool is_array() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an object or array or not.
       */
      constexpr bool is_aggregate() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a string or not.
       */
      constexpr bool is_str() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an integer or not.
       */
      constexpr bool is_integer() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a decimal or not.
       */
      constexpr bool is_decimal() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an integer or decimal or not.
       */
      constexpr bool is_numeric() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a boolean or not.
       */
      bool is_boolean() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is null or not.
       */
      bool is_null() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a string, integer, decimal,
       *  boolean, or null.
       */
      constexpr bool is_primitive() const noexcept;

      /**
       *  @brief
       *  Returns the type of the current packet as an enum value.
       */
      type get_type() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is finalized.
       */
      bool is_finalized() const noexcept;

      /**
       *  @brief
       *  Returns the current reference count of the underlying reference counter.
       */
      auto refcount() const noexcept -> size_type;

    private:

      /*----- Private Members -----*/

      value_type val;

      /*----- Friends -----*/

      // Allow type conversion logic to work.
      friend struct convert::detail::caster_impl<convert::detail::wrapper_tag>;

      // We're friends of all other basic_flag specializations.
      template <class>
      friend class basic_flag;

  };

  /**
   *  @brief
   *  Class operates as a strongly-typed wrapper around any of the
   *  dart::packet, dart::heap, dart::buffer classes, enforcing that
   *  any wrapped packet must be an object.
   *
   *  @details
   *  Dart is primarly a dynamically typed library for interacting with
   *  a dynamically typed notation language, but as C++ is a statically
   *  typed language, it can often be useful to statically know the type
   *  of a packet at compile-time.
   *  Type is implemented as a wrapper around the existing Dart types,
   *  and therefore does not come with any performance benefit. It is
   *  intended to be used to increase readability/enforce
   *  constraints/preconditions where applicable.
   *
   *  @remarks
   *  Class is largely included for completeness, frankly speaking I
   *  doubt it will ever be of significant use.
   */
  template <class Null>
  class basic_null {

    public:

      /*----- Public Types -----*/

      using value_type = Null;
      using type = typename Null::type;
      using size_type = typename Null::size_type;
      using generic_type = typename value_type::generic_type;

      /*----- Lifecycle Functions -----*/

      /**
       *  @brief
       *  Converting constructor to allow interoperability between underlying
       *  packet types for strongly typed nulls.
       *
       *  @details
       *  Constructor is declared to be conditionally explicit based on whether
       *  the underlying implementation types are implicitly convertible.
       *  As a result, this code:
       *  ```
       *  dart::heap::null none;
       *  dart::packet::null dup = none;
       *  ```
       *  Will compile, whereas this:
       *  ```
       *  dart::packet::null none;
       *  dart::heap::null dup = none;
       *  ```
       *  Will not. It would need to use an explicit cast as such:
       *  ```
       *  dart::packet::null none;
       *  dart::heap::null dup {none};
       *  ```
       */
      template <class N,
        std::enable_if_t<
          !std::is_convertible<
            N,
            Null
          >::value
          &&
          std::is_constructible<
            Null,
            N
          >::value
        >* = nullptr
      >
      explicit basic_null(basic_null<N> const&) {}

      /**
       *  @brief
       *  Converting constructor to allow interoperability between underlying
       *  packet types for strongly typed nulls.
       *
       *  @details
       *  Constructor is declared to be conditionally explicit based on whether
       *  the underlying implementation types are implicitly convertible.
       *  As a result, this code:
       *  ```
       *  dart::heap::null none;
       *  dart::packet::null dup = none;
       *  ```
       *  Will compile, whereas this:
       *  ```
       *  dart::packet::null none;
       *  dart::heap::null dup = none;
       *  ```
       *  Will not. It would need to use an explicit cast as such:
       *  ```
       *  dart::packet::null none;
       *  dart::heap::null dup {none};
       *  ```
       */
      template <class N,
        std::enable_if_t<
          std::is_convertible<
            N,
            Null
          >::value
        >* = nullptr
      >
      basic_null(basic_null<N> const&) {}

      /**
       *  @brief
       *  Default constructor, initializes contents to null.
       */
      basic_null() noexcept = default;

      /**
       *  @brief
       *  Converting constructor, allows construction from nullptr.
       */
      basic_null(std::nullptr_t) noexcept {}

      /**
       *  @brief
       *  Copy constructor.
       *
       *  @details
       *  Increments refcount of underlying implementation type.
       */
      basic_null(basic_null const&) = default;

      /**
       *  @brief
       *  Move constructor.
       *
       *  @details
       *  Steals the refcount of an expiring strongly typed object.
       */
      basic_null(basic_null&&) noexcept = default;

      /**
       *  @brief
       *  Destructor. Does what you think it does.
       */
      ~basic_null() = default;

      /*----- Operators -----*/

      /**
       *  @brief
       *  Copy assignment operator.
       *
       *  @details
       *  Increments the refcount of the underlying implementation type.
       */
      basic_null& operator =(basic_null const&) & = default;

      // An MSVC bug prevents us from deleting based on ref qualifiers.
#if !DART_USING_MSVC
      basic_null& operator =(basic_null const&) && = delete;
#endif

      /**
       *  @brief
       *  Move assignment operator.
       *
       *  @details
       *  Steals the refcount of an expiring strongly typed object.
       */
      basic_null& operator =(basic_null&&) & = default;

      // An MSVC bug prevents us from deleting based on ref qualifiers.
#if !DART_USING_MSVC
      basic_null& operator =(basic_null&&) && = delete;
#endif

      /**
       *  @brief
       *  Equality operator.
       *  
       *  @details
       *  For strongly typed null packets, always returns true as all
       *  null instances are considered equal to each other.
       */
      template <class OtherNull>
      constexpr bool operator ==(basic_null<OtherNull> const& other) const noexcept;

      /**
       *  @brief
       *  Equality operator.
       *  
       *  @details
       *  For strongly typed null packets, always returns true as all
       *  null instances are considered equal to each other.
       */
      template <class OtherNull>
      constexpr bool operator !=(basic_null<OtherNull> const& other) const noexcept;

      /**
       *  @brief
       *  Implicit conversion operator to underlying implementation type.
       *
       *  @details
       *  Dart is primarily a dynamically typed library, and so, while the type
       *  wrappers exist to enable additional expressiveness and readability,
       *  they shouldn't make the library more cumbersome to work with, and so
       *  type wrappers are allowed to implicitly discard their type information
       *  if the user requests it.
       */
      operator value_type() const noexcept;

      /**
       *  @brief
       *  Existential operator (bool conversion operator).
       *
       *  @details
       *  Always returns false for strongly typed nulls.
       */
      explicit constexpr operator bool() const noexcept;

      /*----- Public API -----*/

      /*----- JSON Helper Functions -----*/

#if DART_HAS_RAPIDJSON
      /**
       *  @brief
       *  Function serializes any packet into a JSON string.
       *
       *  @details
       *  Serialization is based on RapidJSON, and so exposes the same customization points as
       *  RapidJSON.
       *  If your packet has NaN or +/-Infinity values, you can serialize it in the following way:
       *  ```
       *  auto obj = some_object();
       *  auto json = obj.to_json<dart::write_nan>();
       *  do_something(std::move(json));
       *  ```
       */
      template <unsigned flags = write_default>
      std::string to_json() const;
#endif

      /**
       *  @brief
       *  Helper function returns a const& to the underlying dynamic type.
       *
       *  @details
       *  Can be useful for efficiently implementing wrapper API behavior
       *  in some spots.
       */
      auto dynamic() const noexcept -> value_type const&;

      /*----- Introspection Functions -----*/

      /**
       *  @brief
       *  Returns whether the current packet is an object or not.
       */
      constexpr bool is_object() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an array or not.
       */
      constexpr bool is_array() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an object or array or not.
       */
      constexpr bool is_aggregate() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a string or not.
       */
      constexpr bool is_str() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an integer or not.
       */
      constexpr bool is_integer() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a decimal or not.
       */
      constexpr bool is_decimal() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an integer or decimal or not.
       */
      constexpr bool is_numeric() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a boolean or not.
       */
      constexpr bool is_boolean() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is null or not.
       */
      constexpr bool is_null() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a string, integer, decimal,
       *  boolean, or null.
       */
      constexpr bool is_primitive() const noexcept;

      /**
       *  @brief
       *  Returns the type of the current packet as an enum value.
       */
      constexpr type get_type() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is finalized.
       */
      bool is_finalized() const noexcept;
      
      /**
       *  @brief
       *  Returns the current reference count of the underlying reference counter.
       */
      auto refcount() const noexcept -> size_type;

    private:

      /*----- Private Members -----*/

      value_type val;

      /*----- Friends -----*/

      // Allow type conversion logic to work.
      friend struct convert::detail::caster_impl<convert::detail::wrapper_tag>;

  };

  template <template <class> class RefCount>
  class basic_buffer;

  /**
   *  @brief
   *  dart::heap implements the non-finalized subset of behavior exported by dart::packet,
   *  and can be used when wishing to explicitly document in code that you have a mutable
   *  packet.
   *
   *  @details
   *  dart packet objects have two logically distinct modes: finalized and dynamic.
   *
   *  While in dynamic mode, dart packets maintain a heap-based object tree which can
   *  be used to traverse, or mutate, arbitrary data representations in a reasonably
   *  efficient manner.
   *
   *  While in finalized mode, dart packets maintain a contiguously allocated, flattened
   *  object tree designed specifically for efficient/cache-friendly immutable interaction,
   *  and readiness to be distributed via network/shared-memory/filesystem/etc.
   *
   *  Switching from dynamic to finalized mode is accomplished via a call to
   *  dart::heap::finalize. The current mode can be queried using dart::packet::is_finalized
   *
   *  @remarks
   *  Dart is distinct from other JSON libraries in that only aggregates (objects and arrays)
   *  can be mutated directly, and whenever accessing data within an aggregate, logically
   *  independent subtrees are returned. To give a concrete example of this,
   *  in the following code:
   *  ```
   *  auto obj = dart::heap::make_object("nested", dart::heap::make_object());
   *  auto nested = obj["nested"];
   *  nested.add_field("hello", "world");
   *  ```
   *  obj is NOT modified after construction. To persist the modifications to "nested", one
   *  would have to follow it up with:
   *  ```
   *  obj.add_field("nested", std::move(nested));
   *  ```
   *  which would replace the original definition of "nested". Copy-on-write semantics
   *  mitigate the performance impact of this.
   *
   *  Finally, dart::heap has a thread-safety model in-line with std::shared_ptr.
   *  Individual dart::heap instances are NOT thread-safe, but the reference counting
   *  performed globally (and the copy-on-write semantics across packet instances) IS
   *  thread-safe.
   */
  template <template <class> class RefCount>
  class basic_heap final {

    public:

      /*----- Public Types -----*/

      using type = detail::type;

      /**
       *  @brief
       *  Class abstracts the concept of iterating over an aggregate dart::heap.
       *
       *  @details
       *  Class abstracts the concept of iteration for both objects and arrays, and
       *  so it distinguishes between iterating over a "keyspace" or "valuespace".
       *  Specifically, if the iterator was constructed (or assigned from) a call
       *  to dart::packet::begin, it will iterate over values, and if it was
       *  constructed (or assigned from) a call to dart::packet::key_begin, it will
       *  iterate over keys.
       *  The helper dart::packet::kvbegin returns a std::tuple of key and value
       *  iterators, and can be used very naturally with structured bindings in C++17.
       *
       *  @remarks
       *  Although dart::heap::iterator logically supports the operations and
       *  semantics of a Bidirectional Iterator (multipass guarantees, both incrementable
       *  and decrementable, etc), for implementation reasons, its dereference operator
       *  returns a temporary packet instance, which requires it to claim the weakest
       *  iterator category, the Input Iterator.
       */
      class iterator final {

        public:

          /*----- Public Types -----*/

          using difference_type = ssize_t;
          using value_type = basic_heap;
          using reference = value_type;
          using pointer = value_type;
          using iterator_category = std::input_iterator_tag;

          /*----- Lifecycle Functions -----*/

          iterator() = default;
          iterator(iterator const&) = default;
          iterator(iterator&&) noexcept;
          ~iterator() = default;

          /*----- Operators -----*/

          // Assignment.
          iterator& operator =(iterator const&) = default;
          auto operator =(iterator&& other) noexcept -> iterator&;

          // Comparison.
          bool operator ==(iterator const& other) const noexcept;
          bool operator !=(iterator const& other) const noexcept;

          // Increments.
          auto operator ++() noexcept -> iterator&;
          auto operator --() noexcept -> iterator&;
          auto operator ++(int) noexcept -> iterator;
          auto operator --(int) noexcept -> iterator;

          // Dereference.
          auto operator *() const noexcept -> reference;
          auto operator ->() const noexcept -> pointer;

          // Conversion.
          explicit operator bool() const noexcept;

        private:

          /*----- Private Lifecycle Functions -----*/

          iterator(detail::dynamic_iterator<RefCount> it) : impl(std::move(it)) {}

          /*----- Private Members -----*/

          shim::optional<detail::dynamic_iterator<RefCount>> impl;

          /*----- Friends -----*/

          friend class basic_heap;

      };

      using object = basic_object<basic_heap>;
      using array = basic_array<basic_heap>;
      using string = basic_string<basic_heap>;
      using number = basic_number<basic_heap>;
      using flag = basic_flag<basic_heap>;
      using null = basic_null<basic_heap>;

      // Type than can implicitly subsume this type.
      using generic_type = basic_packet<RefCount>;

      // Views of views don't work, so prevent infinite recursion
      using view = std::conditional_t<
        refcount::is_owner<RefCount>::value,
        basic_heap<view_ptr_context<RefCount>::template view_ptr>,
        basic_heap
      >;

      using size_type = size_t;
      using reverse_iterator = std::reverse_iterator<iterator>;

      /*----- Lifecycle Functions -----*/

      /**
       *  @brief
       *  Default constructor.
       *  Creates a null packet.
       */
      basic_heap() noexcept = default;

      /**
       *  @brief
       *  Copy constructor.
       *
       *  @details
       *  Increments refcount of underlying representation.
       */
      basic_heap(basic_heap const&) = default;

      /**
       *  @brief
       *  Move constructor.
       *
       *  @details
       *  Steals the refcount of an underlying implementation.
       */
      basic_heap(basic_heap&& other) noexcept;

      /**
       *  @brief
       *  Destructor. Does what you think it does.
       */
      ~basic_heap() = default;

      /*----- Operators -----*/

      /**
       *  @brief
       *  Copy assignment operator.
       *
       *  @details
       *  Increments refcount of the underlying implementation.
       */
      basic_heap& operator =(basic_heap const&) & = default;

      // An MSVC bug prevents us from deleting based on ref qualifiers.
#if !DART_USING_MSVC
      basic_heap& operator =(basic_heap const&) && = delete;
#endif

      /**
       *  @brief
       *  Move assignment operator.
       *
       *  @details
       *  Steals the refcount of the underlying implementation.
       */
      basic_heap& operator =(basic_heap&& other) & noexcept;

      // An MSVC bug prevents us from deleting based on ref qualifiers.
#if !DART_USING_MSVC
      basic_heap& operator =(basic_heap&&) && = delete;
#endif

      /**
       *  @brief
       *  Array subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within
       *  bounds, will return the requested array index as a new packet.
       */
      template <class Number>
      basic_heap operator [](basic_number<Number> const& idx) const;

      /**
       *  @brief
       *  Array subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within
       *  bounds, will return the requested array index as a new packet.
       */
      basic_heap operator [](size_type index) const;

      /**
       *  @brief
       *  Object subscript operator.
       *
       *  @details
       *  Assuming this is an object, returns the value associated with the given
       *  key, or a null packet if not such mapping exists.
       */
      template <class String>
      basic_heap operator [](basic_string<String> const& key) const;

      /**
       *  @brief
       *  Object subscript operator.
       *
       *  @details
       *  Assuming this is an object, returns the value associated with the given
       *  key, or a null packet if not such mapping exists.
       */
      basic_heap operator [](shim::string_view key) const;


      /**
       *  @brief
       *  Combined object/array subscript operator.
       *
       *  @details
       *  Assuming this is an object/array, returns the value associated with the
       *  given key/index, or a null packet/throws an exception if no such mapping exists.
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          meta::is_dartlike<KeyType const&>::value
        >
      >
      basic_heap operator [](KeyType const& identifier) const;

      /**
       *  @brief
       *  Equality operator.
       *  
       *  @details
       *  All packet types always return deep equality (aggregates included).
       *  While idiomatic for C++, this means that non-finalized object comparisons
       *  can be reasonably expensive.
       *  
       *  @remarks
       *  "Do as vector does"
       */
      template <template <class> class OtherRC>
      bool operator ==(basic_heap<OtherRC> const& other) const noexcept;

      /**
       *  @brief
       *  Inequality operator.
       *  
       *  @details
       *  All packet types always return deep equality (aggregates included).
       *  While idiomatic C++, this means that non-finalized object/array comparisons
       *  can be arbitrarily expensive.
       *  
       *  @remarks
       *  "Do as vector does"
       */
      template <template <class> class OtherRC>
      bool operator !=(basic_heap<OtherRC> const& other) const noexcept;

      /**
       *  @brief
       *  Member access operator.
       *
       *  @details
       *  Function isn't really intended to be called by the user directly, and
       *  exists as an API quirk due to the fact that dereferencing a
       *  dart::heap::iterator produces a temporary packet instance.
       */
      auto operator ->() noexcept -> basic_heap*;

      /**
       *  @brief
       *  Member access operator.
       *
       *  @details
       *  Function isn't really intended to be called by the user directly, and
       *  exists as an API quirk due to the fact that dereferencing a
       *  dart::heap::iterator produces a temporary packet instance.
       */
      auto operator ->() const noexcept -> basic_heap const*;

      /**
       *  @brief
       *  Existential operator (bool conversion operator).
       *
       *  @details
       *  Operator returns false if the current packet is null or false.
       *  Returns true in all other situations.
       */
      explicit operator bool() const noexcept;

      /**
       *  @brief
       *  Operator allows for converting the current heap instance into a
       *  non-owning, read-only, view
       */
      operator view() const& noexcept;
      operator view() && = delete;

      /**
       *  @brief
       *  String conversion operator.
       *
       *  @details
       *  Returns the string value of the current packet or throws
       *  if no such value exists.
       */
      explicit operator std::string() const;

      /**
       *  @brief
       *  String conversion operator.
       *
       *  @details
       *  Returns the string value of the current packet or throws
       *  if no such value exists.
       */
      explicit operator shim::string_view() const;

      /**
       *  @brief
       *  Integer conversion operator.
       *
       *  @details
       *  Returns the integer value of the current packet or throws
       *  if no such value exists.
       */
      explicit operator int64_t() const;

      /**
       *  @brief
       *  Decimal conversion operator.
       *
       *  @details
       *  Returns the decimal value of the current packet or throws
       *  if no such value exists.
       */
      explicit operator double() const;

      /*----- Public API -----*/

      /*----- Factory Functions -----*/

      /**
       *  @brief
       *  Object factory function.
       *  Returns a new, non-finalized, object with the given sequence of key-value pairs.
       *
       *  @details
       *  Object keys can only be strings, and so arguments must be paired such that the
       *  first of each pair is convertible to a dart::heap string, and the second of
       *  each pair is convertible into a dart::heap of any type.
       */
      template <class... Args, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          sizeof...(Args) % 2 == 0
        >
      >
      static basic_heap make_object(Args&&... pairs);

      /**
       *  @brief
       *  Object factory function.
       *  Returns a new, non-finalized, object with the given sequence of key-value pairs.
       *
       *  @details
       *  Object keys can only be strings, and so arguments must be paired such that the
       *  first of each pair is convertible to a dart::heap string, and the second of
       *  each pair is convertible into a dart::heap of any type.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_heap make_object(gsl::span<basic_heap const> pairs);

      /**
       *  @brief
       *  Object factory function.
       *  Returns a new, non-finalized, object with the given sequence of key-value pairs.
       *
       *  @details
       *  Object keys can only be strings, and so arguments must be paired such that the
       *  first of each pair is convertible to a dart::heap string, and the second of
       *  each pair is convertible into a dart::heap of any type.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_heap make_object(gsl::span<basic_buffer<RefCount> const> pairs);

      /**
       *  @brief
       *  Object factory function.
       *  Returns a new, non-finalized, object with the given sequence of key-value pairs.
       *
       *  @details
       *  Object keys can only be strings, and so arguments must be paired such that the
       *  first of each pair is convertible to a dart::heap string, and the second of
       *  each pair is convertible into a dart::heap of any type.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_heap make_object(gsl::span<basic_packet<RefCount> const> pairs);

      /**
       *  @brief
       *  Array factory function.
       *  Returns a new, non-finalized, array with the given sequence of elements.
       */
      template <class... Args, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          (
            (sizeof...(Args) > 1)
            ||
            !meta::is_span<
              meta::first_type_t<std::decay_t<Args>...>
            >::value
          )
        >
      >
      static basic_heap make_array(Args&&... elems);

      /**
       *  @brief
       *  Array factory function.
       *  Returns a new, non-finalized, array with the given sequence of elements.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_heap make_array(gsl::span<basic_heap const> elems);

      /**
       *  @brief
       *  Array factory function.
       *  Returns a new, non-finalized, array with the given sequence of elements.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_heap make_array(gsl::span<basic_buffer<RefCount> const> elems);

      /**
       *  @brief
       *  Array factory function.
       *  Returns a new, non-finalized, array with the given sequence of elements.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_heap make_array(gsl::span<basic_packet<RefCount> const> elems);

      /**
       *  @brief
       *  String factory function.
       *  Returns a new, non-finalized, string with the given contents.
       *
       *  @details
       *  If the given string is <= dart::packet::sso_bytes long (15 at the time
       *  of this writing), string will be stored inline in the class instance.
       *  Otherwise, function will allocate the memory necessary to store the given
       *  string.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_heap make_string(shim::string_view val);

      /**
       *  @brief
       *  Integer factory function.
       *  Returns a new, non-finalized, integer with the given contents.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_heap make_integer(int64_t val) noexcept;

      /**
       *  @brief
       *  Decimal factory function.
       *  Returns a new, non-finalized, decimal with the given contents.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_heap make_decimal(double val) noexcept;

      /**
       *  @brief
       *  Boolean factory function.
       *  Returns a new, non-finalized, boolean with the given contents.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_heap make_boolean(bool val) noexcept;

      /**
       *  @brief
       *  Null factory function.
       *  Returns a null packet.
       */
      static basic_heap make_null() noexcept;

      /*----- Aggregate Builder Functions -----*/

      /**
       *  @brief
       *  Function adds, or replaces, a key-value pair mapping to/in a packet.
       *
       *  @details
       *  Given key and value can be any types for which type conversions are defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *  The result of converting the key must yield a string.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto obj = dart::packet::make_object("hello", "goodbye");
       *  auto nested = obj["hello"].add_field("yes", "no").add_field("stop", "go, go, go");
       *  ```
       */
      template <class KeyType, class ValueType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<KeyType, basic_heap>::value
          &&
          convert::is_castable<ValueType, basic_heap>::value
        >
      >
      basic_heap& add_field(KeyType&& key, ValueType&& value) &;

      /**
       *  @brief
       *  Function adds, or replaces, a key-value pair mapping to/in a packet.
       *
       *  @details
       *  Given key and value can be any types for which type conversions are defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *  The result of converting the key must yield a string.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto obj = dart::packet::make_object("hello", "goodbye");
       *  auto nested = obj["hello"].add_field("yes", "no").add_field("stop", "go, go, go");
       *  ```
       */
      template <class KeyType, class ValueType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<KeyType, basic_heap>::value
          &&
          convert::is_castable<ValueType, basic_heap>::value
        >
      >
      DART_NODISCARD basic_heap&& add_field(KeyType&& key, ValueType&& value) &&;

      /**
       *  @brief
       *  Function removes a key-value pair mapping from an object.
       *
       *  @details
       *  dart::heap::add_field does not throw if the key-value pair is already present,
       *  allowing for key replacement, and so for API uniformity, this function also does
       *  not throw if the key-value pair is NOT present.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto obj = get_object();
       *  auto nested = obj["nested"].remove_field("hello");
       *  ```
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_heap& remove_field(shim::string_view key) &;

      /**
       *  @brief
       *  Function removes a key-value pair mapping from an object.
       *
       *  @details
       *  dart::heap::add_field does not throw if the key-value pair is already present,
       *  allowing for key replacement, and so for API uniformity, this function also does
       *  not throw if the key-value pair is NOT present.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto obj = get_object();
       *  auto nested = obj["nested"].remove_field("hello");
       *  ```
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      DART_NODISCARD basic_heap&& remove_field(shim::string_view key) &&;

      /**
       *  @brief
       *  Function removes a key-value pair mapping from an object.
       *
       *  @details
       *  dart::heap::add_field does not throw if the key-value pair is already present,
       *  allowing for key replacement, and so for API uniformity, this function also does
       *  not throw if the key-value pair is NOT present.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto obj = get_object();
       *  auto nested = obj["nested"].remove_field("hello");
       *  ```
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          meta::is_dartlike<KeyType const&>::value
        >
      >
      basic_heap& remove_field(KeyType const& key) &;

      /**
       *  @brief
       *  Function removes a key-value pair mapping from an object.
       *
       *  @details
       *  dart::heap::add_field does not throw if the key-value pair is already present,
       *  allowing for key replacement, and so for API uniformity, this function also does
       *  not throw if the key-value pair is NOT present.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto obj = get_object();
       *  auto nested = obj["nested"].remove_field("hello");
       *  ```
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          meta::is_dartlike<KeyType const&>::value
        >
      >
      DART_NODISCARD basic_heap&& remove_field(KeyType const& key) &&;

      /**
       *  @brief
       *  Function inserts the given element at the front of an array.
       *
       *  @details
       *  Given element can be any type for which a type conversion is defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto arr = get_array();
       *  auto nested = arr[0].push_back("testing 1, 2, 3").push_back("a, b, c");
       *  ```
       */
      template <class ValueType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<ValueType, basic_heap>::value
        >
      >
      basic_heap& push_front(ValueType&& value) &;

      /**
       *  @brief
       *  Function inserts the given element at the front of an array.
       *
       *  @details
       *  Given element can be any type for which a type conversion is defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto arr = get_array();
       *  auto nested = arr[0].push_back("testing 1, 2, 3").push_back("a, b, c");
       *  ```
       */
      template <class ValueType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<ValueType, basic_heap>::value
        >
      >
      DART_NODISCARD basic_heap&& push_front(ValueType&& value) &&;

      /**
       *  @brief
       *  Function removes the first element of a array.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto arr = get_array();
       *  auto nested = arr[0].pop_front().pop_front();
       *  ```
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_heap& pop_front() &;

      /**
       *  @brief
       *  Function removes the first element of an array.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto arr = get_array();
       *  auto nested = arr[0].pop_front().pop_front();
       *  ```
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      DART_NODISCARD basic_heap&& pop_front() &&;

      /**
       *  @brief
       *  Function inserts the given element at the end of an array.
       *
       *  @details
       *  Given element can be any type for which a type conversion is defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto arr = get_array();
       *  auto nested = arr[0].push_front("testing 1, 2, 3").push_front("a, b, c");
       *  ```
       */
      template <class ValueType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<ValueType, basic_heap>::value
        >
      >
      basic_heap& push_back(ValueType&& value) &;

      /**
       *  @brief
       *  Function inserts the given element at the end of an array.
       *
       *  @details
       *  Given element can be any type for which a type conversion is defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto arr = get_array();
       *  auto nested = arr[0].push_front("testing 1, 2, 3").push_front("a, b, c");
       *  ```
       */
      template <class ValueType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<ValueType, basic_heap>::value
        >
      >
      basic_heap&& push_back(ValueType&& value) &&;

      /**
       *  @brief
       *  Function removes the last element of an array.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto arr = get_array();
       *  auto nested = arr[0].pop_back().pop_back();
       *  ```
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_heap& pop_back() &;

      /**
       *  @brief
       *  Function removes the last element of an array.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto arr = get_array();
       *  auto nested = arr[0].pop_back().pop_back();
       *  ```
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_heap&& pop_back() &&;

      /**
       *  @brief
       *  Function adds, replaces, or inserts, a (new) (key|index)-value pair to/in an
       *  object/array.
       *
       *  @details
       *  Given key and value can be any types for which type conversions are defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *  If this is an object, the result of converting the key must yield a string.
       *  If this is an array, the result of converting the key must yield an integer.
       *
       *  @remarks
       *  Function is highly overloaded, and serves as a single point of entry for all
       *  packet mutation related to adding new values to aggregates.
       *  Function is used as a backend for dart::heap::add_field, dart::heap::push_back,
       *  dart::heap::push_front, etc
       */
      template <class KeyType, class ValueType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<KeyType, basic_heap>::value
          &&
          convert::is_castable<ValueType, basic_heap>::value
        >
      >
      auto insert(KeyType&& key, ValueType&& value) -> iterator;

      /**
       *  @brief
       *  Function adds, replaces, or inserts, a (new) (key|index)-value pair to/in an
       *  object/array.
       *
       *  @details
       *  Given value can be any type for which a type conversion is defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *  Function takes an iterator as the insertion point to establish API conformity
       *  with STL container types.
       *
       *  @remarks
       *  Iterator based insertion is actually rather complex for copy-on-write types,
       *  which means that unfortunately iterator based insertion is not more efficient
       *  than key based insertion as one might expect.
       *  Function exists to allow for easier compatibility with STL container types in
       *  generic contexts.
       */
      template <class ValueType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<ValueType, basic_heap>::value
        >
      >
      auto insert(iterator pos, ValueType&& value) -> iterator;

      /**
       *  @brief
       *  Assuming this is a non-finalized aggregate, function replaces an existing
       *  (index|key)-value pair.
       *
       *  @details
       *  Given key and value can be any types for which type conversions are defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *  The result of converting the key must yield a string.
       */
      template <class KeyType, class ValueType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<KeyType, basic_heap>::value
          &&
          convert::is_castable<ValueType, basic_heap>::value
        >
      >
      auto set(KeyType&& key, ValueType&& value) -> iterator;

      /**
       *  @brief
       *  Assuming this is a non-finalized aggregate, function replaces an existing
       *  (index|key)-value pair.
       *
       *  @details
       *  Given key and value can be any types for which type conversions are defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *  The result of converting the key must yield a string.
       */
      template <class ValueType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<ValueType, basic_heap>::value
        >
      >
      auto set(iterator pos, ValueType&& value) -> iterator;

      /**
       *  @brief
       *  Function removes a (key|index)-value pair from an object/array.
       *
       *  @details
       *  If the (key-index)-value pair is not present in the current aggregate, function
       *  returns the end iterator, otherwise it returns an iterator to one past the element
       *  removed.
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          meta::is_dartlike<KeyType const&>::value
        >
      >
      auto erase(KeyType const& key) -> iterator;

      /**
       *  @brief
       *  Function removes a (key|index)-value pair from an object/array.
       *
       *  @details
       *  If the (key-index)-value pair is not present in the current aggregate, function
       *  returns the end iterator, otherwise it returns an iterator to one past the element
       *  removed.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      auto erase(iterator pos) -> iterator;

      /**
       *  @brief
       *  Function removes a (key|index)-value pair from a non-finalized object/array.
       *
       *  @details
       *  If the (key-index)-value pair is not present in the current aggregate, function
       *  returns the end iterator, otherwise it returns an iterator to one past the element
       *  removed.
       */
      template <class String, bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      auto erase(basic_string<String> const& key) -> iterator;
      
      /**
       *  @brief
       *  Function removes a (key|index)-value pair from a non-finalized object/array.
       *
       *  @details
       *  If the (key-index)-value pair is not present in the current aggregate, function
       *  returns the end iterator, otherwise it returns an iterator to one past the element
       *  removed.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      auto erase(shim::string_view key) -> iterator;

      /**
       *  @brief
       *  Function removes a (key|index)-value pair from a non-finalized object/array.
       *
       *  @details
       *  If the (key-index)-value pair is not present in the current aggregate, function
       *  returns the end iterator, otherwise it returns an iterator to one past the element
       *  removed.
       */
      template <class Number, bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      auto erase(basic_number<Number> const& idx) -> iterator;

      /**
       *  @brief
       *  Function removes a (key|index)-value pair from a non-finalized object/array.
       *
       *  @details
       *  If the (key-index)-value pair is not present in the current aggregate, function
       *  returns the end iterator, otherwise it returns an iterator to one past the element
       *  removed.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      auto erase(size_type pos) -> iterator;

      /**
       *  @brief
       *  Function clears an aggregate of all values.
       *
       *  @details
       *  If this is object/array, will remove all subvalues.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      void clear();

      /**
       *  @brief
       *  Function "injects" a set of key-value pairs into an object without
       *  modifying it by returning a new object with the union of the keyset
       *  of the original object and the provided key-value pairs.
       *
       *  @details
       *  Function provides a uniform interface for _efficient_ injection of
       *  keys across the entire Dart API (also works for dart::buffer).
       */
      template <class... Args, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          sizeof...(Args) % 2 == 0
          &&
          meta::conjunction<
            convert::is_castable<Args, basic_heap>...
          >::value
        >
      >
      basic_heap inject(Args&&... pairs) const;

      /**
       *  @brief
       *  Function "injects" a set of key-value pairs into an object without
       *  modifying it by returning a new object with the union of the keyset
       *  of the original object and the provided key-value pairs.
       *
       *  @details
       *  Function provides a uniform interface for _efficient_ injection of
       *  keys across the entire Dart API (also works for dart::buffer).
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_heap inject(gsl::span<basic_heap const> pairs) const;

      /**
       *  @brief
       *  Function "injects" a set of key-value pairs into an object without
       *  modifying it by returning a new object with the union of the keyset
       *  of the original object and the provided key-value pairs.
       *
       *  @details
       *  Function provides a uniform interface for _efficient_ injection of
       *  keys across the entire Dart API (also works for dart::buffer).
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_heap inject(gsl::span<basic_buffer<RefCount> const> pairs) const;

      /**
       *  @brief
       *  Function "injects" a set of key-value pairs into an object without
       *  modifying it by returning a new object with the union of the keyset
       *  of the original object and the provided key-value pairs.
       *
       *  @details
       *  Function provides a uniform interface for _efficient_ injection of
       *  keys across the entire Dart API (also works for dart::buffer).
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_heap inject(gsl::span<basic_packet<RefCount> const> pairs) const;

      /**
       *  @brief
       *  Function "projects" a set of key-value pairs in the mathematical
       *  sense that it creates a new packet containing the intersection of
       *  the supplied keys, and those present in the original packet.
       *
       *  @details
       *  Function provides a uniform interface for _efficient_ projection of
       *  keys across the entire Dart API (also works for dart::buffer).
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_heap project(std::initializer_list<shim::string_view> keys) const;

      /**
       *  @brief
       *  Function "projects" a set of key-value pairs in the mathematical
       *  sense that it creates a new packet containing the intersection of
       *  the supplied keys, and those present in the original packet.
       *
       *  @details
       *  Function provides a uniform interface for _efficient_ projection of
       *  keys across the entire Dart API (also works for dart::buffer).
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_heap project(gsl::span<std::string const> keys) const;

      /**
       *  @brief
       *  Function "projects" a set of key-value pairs in the mathematical
       *  sense that it creates a new packet containing the intersection of
       *  the supplied keys, and those present in the original packet.
       *
       *  @details
       *  Function provides a uniform interface for _efficient_ projection of
       *  keys across the entire Dart API (also works for dart::buffer).
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_heap project(gsl::span<shim::string_view const> keys) const;

      /*----- State Manipulation Functions -----*/

      /**
       *  @brief
       *  Function ensures enough underlying storage for at least
       *  as many elements as specified.
       *
       *  @details
       *  If used judiciously, can increase insertion performance.
       *  If used poorly, will definitely decrease insertion performance.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      void reserve(size_type count);

      /**
       *  @brief
       *  Function sets the current capacity of the underlying array storage.
       *
       *  @details
       *  If the underlying array grows, values are initialized by casting
       *  the supplied default argument according to the usual conversion
       *  API rules.
       */
      template <class T = std::nullptr_t, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<T, basic_heap>::value
        >
      >
      void resize(size_type count, T const& def = T {});

      /**
       *  @brief
       *  Function would transition to non-finalized mode, but as dart::heap is non-finalized
       *  by definition, simply returns the current instance.
       *
       *  @details
       *  dart packets have two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart packets maintain a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart packets maintain a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::heap::finalize, the reverse can be accomplished using dart::heap::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_heap& definalize() &;

      /**
       *  @brief
       *  Function would transition to non-finalized mode, but as dart::heap is non-finalized
       *  by definition, simply returns the current instance.
       *
       *  @details
       *  dart packets have two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart packets maintain a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart packets maintain a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::heap::finalize, the reverse can be accomplished using dart::heap::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_heap const& definalize() const&;

      /**
       *  @brief
       *  Function would transition to non-finalized mode, but as dart::heap is non-finalized
       *  by definition, simply returns the current instance.
       *
       *  @details
       *  dart packets have two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart packets maintain a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart packets maintain a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::heap::finalize, the reverse can be accomplished using dart::heap::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_heap&& definalize() &&;

      /**
       *  @brief
       *  Function would transition to non-finalized mode, but as dart::heap is non-finalized
       *  by definition, simply returns the current instance.
       *
       *  @details
       *  dart packets have two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart packets maintain a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart packets maintain a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::heap::finalize, the reverse can be accomplished using dart::heap::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_heap const&& definalize() const&&;

      /**
       *  @brief
       *  Function would transition to non-finalized mode, but as dart::heap is non-finalized
       *  by definition, simply returns the current instance.
       *
       *  @details
       *  dart packets have two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart packets maintain a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart packets maintain a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::heap::finalize, the reverse can be accomplished using dart::heap::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_heap& lift() &;

      /**
       *  @brief
       *  Function would transition to non-finalized mode, but as dart::heap is non-finalized
       *  by definition, simply returns the current instance.
       *
       *  @details
       *  dart packets have two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart packets maintain a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart packets maintain a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::heap::finalize, the reverse can be accomplished using dart::heap::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_heap const& lift() const&;

      /**
       *  @brief
       *  Function would transition to non-finalized mode, but as dart::heap is non-finalized
       *  by definition, simply returns the current instance.
       *
       *  @details
       *  dart packets have two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart packets maintain a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart packets maintain a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::heap::finalize, the reverse can be accomplished using dart::heap::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_heap&& lift() &&;

      /**
       *  @brief
       *  Function would transition to non-finalized mode, but as dart::heap is non-finalized
       *  by definition, simply returns the current instance.
       *
       *  @details
       *  dart packets have two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart packets maintain a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart packets maintain a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::heap::finalize, the reverse can be accomplished using dart::heap::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_heap const&& lift() const&&;

      /**
       *  @brief
       *  Function transitions to a finalized state by returning a dart::buffer instance that
       *  describes the same object tree.
       *
       *  @details
       *  dart packets have two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart packets maintain a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart packets maintain a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::heap::finalize, the reverse can be accomplished using dart::heap::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer<RefCount> finalize() const;

      /**
       *  @brief
       *  Function transitions to a finalized state by returning a dart::buffer instance that
       *  describes the same object tree.
       *
       *  @details
       *  dart packets have two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart packets maintain a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart packets maintain a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::heap::finalize, the reverse can be accomplished using dart::heap::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer<RefCount> lower() const;

      /**
       *  @brief
       *  Function re-seats the object tree referred to by the given packet on top of the
       *  specified reference counter.
       *
       *  @details
       *  Function name is intentionally long and hard to spell to denote the fact that you
       *  shouldn't be calling it very frequently.
       *  Switching reference counters requires a deep copy of the entire tree, which means
       *  as many allocations and copies as there are nodes in the tree.
       *  Very expensive, should only be called when truly necessary for applications that
       *  need to use multiple reference counter types simultaneously.
       */
      template <template <class> class NewCount>
      static basic_heap<NewCount> transmogrify(basic_heap const& heap);

      /*----- JSON Helper Functions -----*/

#ifdef DART_USE_SAJSON
      /**
       *  @brief
       *  Function constructs an optionally finalized packet to represent the given JSON string.
       *
       *  @details
       *  Parsing is based on RapidJSON, and so exposes the same parsing customization points as
       *  RapidJSON.
       *  If your JSON has embedded comments in it, NaN or +/-Infinity values, or trailing commas,
       *  you can parse in the following ways:
       *  ```
       *  auto json = input.read();
       *  auto comments = dart::heap::from_json<dart::parse_comments>(json);
       *  auto nan_inf = dart::heap::from_json<dart::parse_nan>(json);
       *  auto commas = dart::heap::from_json<dart::parse_trailing_commas>(json);
       *  auto all_of_it = dart::heap::from_json<dart::parse_permissive>(json);
       *  ```
       */
      template <unsigned parse_stack_size = default_parse_stack_size,
               bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_heap from_json(shim::string_view json);
#elif DART_HAS_RAPIDJSON
      /**
       *  @brief
       *  Function constructs an optionally finalized packet to represent the given JSON string.
       *
       *  @details
       *  Parsing is based on RapidJSON, and so exposes the same parsing customization points as
       *  RapidJSON.
       *  If your JSON has embedded comments in it, NaN or +/-Infinity values, or trailing commas,
       *  you can parse in the following ways:
       *  ```
       *  auto json = input.read();
       *  auto comments = dart::heap::from_json<dart::parse_comments>(json);
       *  auto nan_inf = dart::heap::from_json<dart::parse_nan>(json);
       *  auto commas = dart::heap::from_json<dart::parse_trailing_commas>(json);
       *  auto all_of_it = dart::heap::from_json<dart::parse_permissive>(json);
       *  ```
       */
      template <unsigned flags = parse_default,
               bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_heap from_json(shim::string_view json);
#endif

#if DART_HAS_RAPIDJSON
      /**
       *  @brief
       *  Function serializes any packet into a JSON string.
       *
       *  @details
       *  Serialization is based on RapidJSON, and so exposes the same customization points as
       *  RapidJSON.
       *  If your packet has NaN or +/-Infinity values, you can serialize it in the following way:
       *  ```
       *  auto obj = some_object();
       *  auto json = obj.to_json<dart::write_nan>();
       *  do_something(std::move(json));
       *  ```
       */
      template <unsigned flags = write_default>
      std::string to_json() const;
#endif

      /*----- YAML Helper Functions -----*/

#if DART_HAS_YAML
      /**
       *  @brief
       *  Function constructs an optionally finalized packet to represent the given YAML string.
       *
       *  @remarks
       *  At the time of this writing, parsing logic does not support YAML anchors, this will
       *  be added soon.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_heap from_yaml(shim::string_view yaml);
#endif

      /*----- Value Accessor Functions -----*/

      /**
       *  @brief
       *  Array access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within bounds, will
       *  return the requested array index as a new packet.
       */
      template <class Number>
      basic_heap get(basic_number<Number> const& idx) const;

      /**
       *  @brief
       *  Array access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within bounds, will
       *  return the requested array index as a new packet.
       */
      basic_heap get(size_type index) const;

      /**
       *  @brief
       *  Optional array access method, similar to std::optional::value_or.
       *
       *  @details
       *  If this is an array, and contains the specified index, function returns the
       *  value associated with that index.
       *  If this is not an array, or the specified index is out of bounds, function
       *  returns the optional value cast to a dart::packet according to the usual
       *  conversion API rules.
       *
       *  @remarks
       *  Function is "logically" noexcept, but cannot be marked so as allocations necessary
       *  for converting the optional value might throw, and user defined conversions aren't
       *  required to be noexcept.
       */
      template <class Number, class T, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<T, basic_heap>::value
        >
      >
      basic_heap get_or(basic_number<Number> const& idx, T&& opt) const;

      /**
       *  @brief
       *  Optional array access method, similar to std::optional::value_or.
       *
       *  @details
       *  If this is an array, and contains the specified index, function returns the
       *  value associated with that index.
       *  If this is not an array, or the specified index is out of bounds, function
       *  returns the optional value cast to a dart::packet according to the usual
       *  conversion API rules.
       *
       *  @remarks
       *  Function is "logically" noexcept, but cannot be marked so as allocations necessary
       *  for converting the optional value might throw, and user defined conversions aren't
       *  required to be noexcept.
       */
      template <class T, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<T, basic_heap>::value
        >
      >
      basic_heap get_or(size_type index, T&& opt) const;
      
      /**
       *  @brief
       *  Object access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an object, function will return the requested key-value pair as a
       *  new packet.
       */
      template <class String>
      basic_heap get(basic_string<String> const& key) const;

      /**
       *  @brief
       *  Object access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an object, function will return the requested key-value pair as a
       *  new packet.
       */
      basic_heap get(shim::string_view key) const;

      /**
       *  @brief
       *  Optional object access method, similar to std::optional::value_or.
       *
       *  @details
       *  If this is an object, and contains the key-value pair, function returns the
       *  value associated with the specified key.
       *  If this is not an object, or the specified key-value pair is not present,
       *  function returns the optional value cast to a dart::packet according to
       *  the usual conversion API rules.
       *
       *  @remarks
       *  Function is "logically" noexcept, but cannot be marked so as allocations necessary
       *  for converting the optional value might throw, and user defined conversions aren't
       *  required to be noexcept.
       */
      template <class String, class T, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<T, basic_heap>::value
        >
      >
      basic_heap get_or(basic_string<String> const& key, T&& opt) const;

      /**
       *  @brief
       *  Optional object access method, similar to std::optional::value_or.
       *
       *  @details
       *  If this is an object, and contains the specified key-value pair,
       *  function returns the value associated with the specified key.
       *  If this is not an object, or the specified key-value pair is not present,
       *  function returns the optional value cast to a dart::packet according to
       *  the usual conversion API rules.
       *
       *  @remarks
       *  Function is "logically" noexcept, but cannot be marked so as allocations necessary
       *  for converting the optional value might throw, and user defined conversions aren't
       *  required to be noexcept.
       */
      template <class T, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<T, basic_heap>::value
        >
      >
      basic_heap get_or(shim::string_view key, T&& opt) const;

      /**
       *  @brief
       *  Combined array/object access method, precisely equivalent to the corresponding
       *  subscript operator.
       *
       *  @details
       *  Assuming this is an object/array, returns the value associated with the
       *  given key/index, or a null packet/throws an exception if no such mapping exists.
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          meta::is_dartlike<KeyType const&>::value
        >
      >
      basic_heap get(KeyType const& identifier) const;

      /**
       *  @brief
       *  Optional combined object/array access method, similar to std::optional::value_or.
       *
       *  @details
       *  If this is an object, and contains the specified key-value pair, function returns
       *  the value associated with the specified key.
       *  If this is an array, and contains the specified index, function returns the value
       *  associated with that index.
       *  If this is no an array/object, function returns the optional value cast to a
       *  dart::packet according to the usual conversion API rules.
       *  If this is an array/object, but the passed identifier is not a number/string
       *  respectively, function returns the optional value cast to a dart::packet according
       *  to the usual conversion API rules.
       *
       *  @remarks
       *  Function is "logically" noexcept, but cannot be marked so as allocations necessary
       *  for converting the optional value might throw, and anyways, user defined conversions
       *  aren't required to be noexcept.
       */
      template <class KeyType, class T, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          meta::is_dartlike<KeyType const&>::value
          &&
          convert::is_castable<T, basic_heap>::value
        >
      >
      basic_heap get_or(KeyType const& identifier, T&& opt) const;

      /**
       *  @brief
       *  Function returns the value associated with the separator delimited object path.
       */
      basic_heap get_nested(shim::string_view path, char separator = '.') const;

      /**
       *  @brief
       *  Array access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within bounds, will
       *  return the requested array index as a new packet.
       */
      template <class Number>
      basic_heap at(basic_number<Number> const& idx) const;

      /**
       *  @brief
       *  Array access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within bounds, will
       *  return the requested array index as a new packet.
       */
      basic_heap at(size_type index) const;

      /**
       *  @brief
       *  Object access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an object, function will return the requested key-value pair as a
       *  new packet.
       */
      template <class String>
      basic_heap at(basic_string<String> const& key) const;

      /**
       *  @brief
       *  Object access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an object, function will return the requested key-value pair as a
       *  new packet.
       */
      basic_heap at(shim::string_view key) const;

      /**
       *  @brief
       *  Combined array/object access method, precisely equivalent to the corresponding
       *  subscript operator.
       *
       *  @details
       *  Assuming this is an object/array, returns the value associated with the
       *  given key/index, or a null packet/throws an exception if no such mapping exists.
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          meta::is_dartlike<KeyType const&>::value
        >
      >
      basic_heap at(KeyType const& identifier) const;

      /**
       *  @brief
       *  Function returns the first element in an array.
       *
       *  @details
       *  Return the first element or throws if the array is empty.
       */
      basic_heap at_front() const;

      /**
       *  @brief
       *  Function returns the last element in an array.
       *
       *  @details
       *  Returns the last element or throws if the array is empty.
       */
      basic_heap at_back() const;

      /**
       *  @brief
       *  Function returns the first element in an array.
       *
       *  @details
       *  If this is an array with at least one thing in it,
       *  returns the first element.
       *  Throws otherwise.
       */
      basic_heap front() const;

      /**
       *  @brief
       *  Optionally returns the first element in an array.
       *
       *  @details
       *  If this is an array with at least one thing in it,
       *  returns the first element.
       *  Otherwise, returns the optional value cast to a dart::packet
       *  according to the usual conversion API rules.
       */
      template <class T, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<T, basic_heap>::value
        >
      >
      basic_heap front_or(T&& opt) const;

      /**
       *  @brief
       *  Function returns the last element in an array.
       *
       *  @details
       *  If this is an array with at least oen thing in it,
       *  returns the last element.
       *  Throws otherwise.
       */
      basic_heap back() const;

      /**
       *  @brief
       *  Optionally returns the last element in an array.
       *
       *  @details
       *  If this is an array with at least one thing in it,
       *  returns the last element.
       *  Otherwise, returns the optional value cast to a dart::packet
       *  according to the usual conversion API rules.
       */
      template <class T, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<T, basic_heap>::value
        >
      >
      basic_heap back_or(T&& opt) const;

      /**
       *  @brief
       *  Finds the requested key-value mapping in an object.
       *
       *  @details
       *  If this is in an object, function will return an
       *  iterator to the requested VALUE mapping, or the
       *  end iterator.
       *  If this is an object, throws an exception.
       */
      template <class String>
      auto find(basic_string<String> const& key) const -> iterator;

      /**
       *  @brief
       *  Finds the requested key-value mapping in an object.
       *
       *  @details
       *  If this is in an object, function will return an
       *  iterator to the requested VALUE mapping, or the
       *  end iterator.
       *  If this is an object, throws an exception.
       */
      auto find(shim::string_view key) const -> iterator;

      /**
       *  @brief
       *  Finds the requested key-value mapping in an object.
       *
       *  @details
       *  If this is in an object, function will return an
       *  iterator to the requested KEY mapping, or the
       *  end iterator.
       *  If this is an object, throws an exception.
       */
      template <class String>
      auto find_key(basic_string<String> const& key) const -> iterator;

      /**
       *  @brief
       *  Finds the requested key-value mapping in an object.
       *
       *  @details
       *  If this is in an object, function will return an
       *  iterator to the requested VALUE mapping, or the
       *  end iterator.
       *  If this is an object, throws an exception.
       */
      auto find_key(shim::string_view key) const -> iterator;

      /**
       *  @brief
       *  Returns all keys associated with the current packet.
       *
       *  @details
       *  If this is an object, returns a vector of all its keys.
       *  Throws otherwise.
       */
      std::vector<basic_heap> keys() const;

      /**
       *  @brief
       *  Returns all subvalues of the current aggregate.
       *
       *  @details
       *  If this is an object or array, returns all of the values contained
       *  within
       *  Throws otherwise.
       */
      std::vector<basic_heap> values() const;

      /**
       *  @brief
       *  Returns whether a particular key-value pair exists within an object.
       *
       *  @details
       *  If this is an object, function returns whether the given key is present
       *  Throws otherwise.
       */
      template <class String>
      bool has_key(basic_string<String> const& key) const;

      /**
       *  @brief
       *  Returns whether a particular key-value pair exists within an object.
       *
       *  @details
       *  If this is an object, function returns whether the given key is present
       *  Throws otherwise.
       */
      bool has_key(shim::string_view key) const;

      /**
       *  @brief
       *  Returns whether a particular key-value pair exists within an object.
       *
       *  @details
       *  If this is an object, function returns whether the given key is present
       *  Throws otherwise.
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          meta::is_dartlike<KeyType const&>::value
        >
      >
      bool has_key(KeyType const& key) const;

      /**
       *  @brief
       *  Function returns a null-terminated c-string representing the current packet.
       *
       *  @details
       *  If the current packet is a string, returns a c-string, otherwise throws.
       *
       *  @remarks
       *  Lifetime of the returned pointer is tied to the current instance.
       */
      char const* str() const;

      /**
       *  @brief
       *  Function optionally returns a c-string representing the current packet.
       *
       *  @details
       *  If the current packet is a string, returns a null-terminated c-string,
       *  otherwise returns the provided character pointer.
       *
       *  @remarks
       *  If this is a string, the lifetime of the returned pointer
       *  is equal to that of the current instance
       *  If this is not a string, the lifetime of the returned pointer
       *  is equivalent to the lifetime of the optional parameter.
       */
      char const* str_or(char const* opt) const noexcept;

      /**
       *  @brief
       *  Function returns a string view representing the current packet.
       *
       *  @details
       *  If the current packet is a string, returns a string view, otherwise throws.
       *
       *  @remarks
       *  Lifetime of the string view is tied to the current instance.
       */
      shim::string_view strv() const;

      /**
       *  @brief
       *  Function optionally returns a string view representing the current packet.
       *
       *  @details
       *  If the current packet is a string, returns a string view,
       *  otherwise returns the provided string view.
       *
       *  @remarks
       *  If this is a string, the lifetime of the returned string view is
       *  equal to that of the current instance.
       *  If this is not a string, the lifetime of the returned string view
       *  is equivalent to the lifetime of the optional parameter.
       */
      shim::string_view strv_or(shim::string_view opt) const noexcept;

      /**
       *  @brief
       *  Function returns an integer representing the current packet.
       *
       *  @details
       *  If the current packet is an integer, returns a it, otherwise throws.
       */
      int64_t integer() const;

      /**
       *  @brief
       *  Function optionally returns an integer representing the current packet.
       *
       *  @details
       *  If the current packet is an integer, returns a it,
       *  otherwise returns the provided integer.
       */
      int64_t integer_or(int64_t opt) const noexcept;

      /**
       *  @brief
       *  Function returns a decimal representing the current packet.
       *
       *  @details
       *  If the current packet is a decimal, returns a it, otherwise throws.
       */
      double decimal() const;

      /**
       *  @brief
       *  Function optionally returns a decimal representing the current packet.
       *
       *  @details
       *  If the current packet is a decimal, returns a it,
       *  otherwise returns the provided decimal.
       */
      double decimal_or(double opt) const noexcept;

      /**
       *  @brief
       *  Function returns a decimal representing the current packet.
       *
       *  @details
       *  If the current packet is an integer or decimal, returns a it as a decimal,
       *  otherwise throws.
       */
      double numeric() const;

      /**
       *  @brief
       *  Function optionally returns a decimal representing the current packet.
       *
       *  @details
       *  If the current packet is an integer or decimal, returns a it as a decimal,
       *  otherwise returns the provided decimal.
       */
      double numeric_or(double opt) const noexcept;

      /**
       *  @brief
       *  Function returns a decimal representing the current packet.
       *
       *  @details
       *  If the current packet is a decimal, returns a it, otherwise throws.
       */
      bool boolean() const;

      /**
       *  @brief
       *  Function optionally returns a decimal representing the current packet.
       *
       *  @details
       *  If the current packet is a decimal, returns a it,
       *  otherwise returns the provided decimal.
       */
      bool boolean_or(bool opt) const noexcept;

      /**
       *  @brief
       *  Function returns the size of the current packet, according to the semantics
       *  of its type.
       *
       *  @details
       *  If this is an object, returns the number of key-value pairs within the object.
       *  If this is an array, returns the number of elements within the array.
       *  If this is a string, returns the length of the string NOT INCLUDING the null
       *  terminator.
       */
      auto size() const -> size_type;

      /**
       *  @brief
       *  Function returns the current capacity of the underlying storage array.
       */
      auto capacity() const -> size_type;

      /**
       *  @brief
       *  Function returns whether the current packet is empty, according to the semantics
       *  of its type.
       *
       *  @details
       *  In actuality, returns whether size() == 0;
       */
      bool empty() const;

      /*----- Introspection Functions -----*/

      /**
       *  @brief
       *  Returns whether the current packet is an object or not.
       */
      bool is_object() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an array or not.
       */
      bool is_array() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an object or array or not.
       */
      bool is_aggregate() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a string or not.
       */
      bool is_str() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an integer or not.
       */
      bool is_integer() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a decimal or not.
       */
      bool is_decimal() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an integer or decimal or not.
       */
      bool is_numeric() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a boolean or not.
       */
      bool is_boolean() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is null or not.
       */
      bool is_null() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a string, integer, decimal,
       *  boolean, or null.
       */
      bool is_primitive() const noexcept;

      /**
       *  @brief
       *  Returns the type of the current packet as an enum value.
       */
      auto get_type() const noexcept -> type;

      /**
       *  @brief
       *  Returns whether the current packet is finalized.
       */
      constexpr bool is_finalized() const noexcept;

      /**
       *  @brief
       *  Returns the current reference count of the underlying reference counter.
       */
      auto refcount() const noexcept -> size_type;

      /*----- Iterator Functions -----*/

      /**
       *  @brief
       *  Function returns a dart::packet::iterator that can be used to iterate over the
       *  VALUES of the current packet.
       *
       *  @details
       *  Throws if this is not an object or array.
       */
      auto begin() const -> iterator;

      /**
       *  @brief
       *  Function returns a dart::packet::iterator that can be used to iterate over the
       *  VALUES of the current packet.
       *
       *  @details
       *  Throws if this is not an object or array.
       *
       *  @remarks
       *  Function exists for API uniformity with the STL, but is precisely equal to
       *  dart::packet::begin.
       */
      auto cbegin() const -> iterator;

      /**
       *  @brief
       *  Function returns a dart::packet::iterator one-past-the-end for iterating over
       *  the VALUES of the current packet.
       *
       *  @details
       *  Dart iterators are bidirectional, so the iterator could be decremented to iterate
       *  over the valuespace backwards, or it can be used as a sentinel value for
       *  traditional iteration.
       */
      auto end() const -> iterator;

      /**
       *  @brief
       *  Function returns a dart::packet::iterator one-past-the-end for iterating over
       *  the VALUES of the current packet.
       *
       *  @details
       *  Dart iterators are bidirectional, so the iterator could be decremented to iterate
       *  over the valuespace backwards, or it can be used as a sentinel value for
       *  traditional iteration.
       *
       *  @remarks
       *  Function exists for API uniformity with the STL, but is precisely equal to
       *  dart::packet::end.
       */
      auto cend() const -> iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator that iterates in reverse order from
       *  dart::packet::end to dart::packet::begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::begin.
       */
      auto rbegin() const -> reverse_iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator that iterates in reverse order from
       *  dart::packet::end to dart::packet::begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::begin.
       *
       *  @remarks
       *  Function exists for API uniformity with the STL, but is precisely equal to
       *  dart::packet::rbegin.
       */
      auto crbegin() const -> reverse_iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator one-past-the-end, that iterates in
       *  reverse order from dart::packet::end to dart::packet::begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::end.
       */
      auto rend() const -> reverse_iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator one-past-the-end, that iterates in
       *  reverse order from dart::packet::end to dart::packet::begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::end.
       *
       *  @remarks
       *  Function exists for API uniformity with the STL, but is precisely equal to
       *  dart::packet::rbegin.
       */
      auto crend() const -> reverse_iterator;
      
      /**
       *  @brief
       *  Function returns a dart::packet::iterator that can be used to iterate over the
       *  KEYS of the current packet.
       *
       *  @details
       *  Throws if this is not an object
       */
      auto key_begin() const -> iterator;

      /**
       *  @brief
       *  Function returns a dart::packet::iterator one-past-the-end that can be used to
       *  iterate over the KEYS of the current packet.
       *
       *  @details
       *  Dart iterators are bidirectional, so the iterator could be decremented to iterate
       *  over the keyspace backwards, or it can be used as a sentinel value for traditional
       *  iteration.
       */
      auto key_end() const -> iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator that iterates over the KEYS of the current
       *  packet in reverse order from dart::packet::key_end to dart::packet::key_begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::key_begin.
       */
      auto rkey_begin() const -> reverse_iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator one-past-the-end that iterates over the
       *  KEYS of the current packet in reverse order from dart::packet::key_end to
       *  dart::packet::key_begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::key_end.
       */
      auto rkey_end() const -> reverse_iterator;

      /**
       *  @brief
       *  Function returns a tuple of key-value iterators to allow for easy iteration
       *  over keys and values with C++17 structured bindings.
       */
      auto kvbegin() const -> std::tuple<iterator, iterator>;

      /**
       *  @brief
       *  Function returns a tuple of key-value iterators to allow for easy iteration
       *  over keys and values with C++17 structured bindings.
       */
      auto kvend() const -> std::tuple<iterator, iterator>;

      /**
       *  @brief
       *  Function returns a tuple of key-value reverse iterators to allow for easy
       *  iteration over keys and values with C++17 structured bindings.
       */
      auto rkvbegin() const -> std::tuple<reverse_iterator, reverse_iterator>;

      /**
       *  @brief
       *  Function returns a tuple of key-value reverse iterators to allow for easy
       *  iteration over keys and values with C++17 structured bindings.
       */
      auto rkvend() const -> std::tuple<reverse_iterator, reverse_iterator>;

      /*----- Member Ownership Helpers -----*/

      /**
       *  @brief
       *  Function calculates whether the current type is a non-owning view or not.
       */
      constexpr bool is_view() const noexcept;

      /**
       *  @brief
       *  Function allows one to explicitly grab full ownership of the current view.
       */
      template <bool enabled = !refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      auto as_owner() const noexcept;

    private:

      /*----- Private Types -----*/

      struct dynamic_string_layout {
        bool operator ==(dynamic_string_layout const& other) const noexcept;
        bool operator !=(dynamic_string_layout const& other) const noexcept;

        std::shared_ptr<char> ptr;
        size_type len;
      };
      struct inline_string_layout {
        bool operator ==(inline_string_layout const& other) const noexcept;
        bool operator !=(inline_string_layout const& other) const noexcept;

        std::array<char, sizeof(dynamic_string_layout) - sizeof(uint8_t)> buffer;
        uint8_t left;
      };

      using packet_fields = detail::packet_fields<RefCount>;
      using packet_elements = detail::packet_elements<RefCount>;

      using fields_rc_type = RefCount<packet_fields>;
      using elements_rc_type = RefCount<packet_elements>;

      using fields_type = shareable_ptr<fields_rc_type>;
      using elements_type = shareable_ptr<elements_rc_type>;

      using type_data = shim::variant<
        shim::monostate,
        fields_type,
        elements_type,
        dynamic_string_layout,
        inline_string_layout,
        int64_t,
        double,
        bool
      >;

      /*----- Private Lifecycle Functions -----*/

      template <class TypeData>
      basic_heap(detail::view_tag, TypeData const& data);
      basic_heap(detail::object_tag) :
        data(make_shareable<RefCount<packet_fields>>())
      {}
      basic_heap(detail::array_tag) :
        data(make_shareable<RefCount<packet_elements>>())
      {}
      basic_heap(detail::string_tag, shim::string_view val);
      basic_heap(detail::integer_tag, int64_t val) noexcept : data(val) {}
      basic_heap(detail::decimal_tag, double val) noexcept : data(val) {}
      basic_heap(detail::boolean_tag, bool val) noexcept : data(val) {}
      basic_heap(detail::null_tag) noexcept {}

      /*----- Private Helpers -----*/

      template <bool consume, class Span>
      static void push_elems(basic_heap& arr, Span elems);

      template <bool consume, class Span>
      static void inject_pairs(basic_heap& obj, Span pairs);

      template <class Spannable>
      basic_heap project_keys(Spannable const& keys) const;

      void copy_on_write(size_type overcount = 1);
      auto upper_bound() const -> size_type;
      auto layout(gsl::byte* buffer) const noexcept -> size_type;
      detail::raw_type get_raw_type() const noexcept;

      template <class Deref>
      auto erase_key_impl(shim::string_view key, Deref&& deref, fields_type safeguard) -> iterator;

      auto iterator_index(iterator pos) const -> size_type;
      shim::string_view iterator_key(iterator pos) const;

      auto get_fields() -> packet_fields&;
      auto get_fields() const -> packet_fields const&;
      auto try_get_fields() noexcept -> packet_fields*;
      auto try_get_fields() const noexcept -> packet_fields const*;
      auto get_elements() -> packet_elements&;
      auto get_elements() const -> packet_elements const&;
      auto try_get_elements() noexcept -> packet_elements*;
      auto try_get_elements() const noexcept -> packet_elements const*;

      /*----- Private Members -----*/

      type_data data;

      static constexpr auto sso_bytes = sizeof(inline_string_layout::buffer);
      static constexpr auto max_aggregate_size = detail::object_layout::max_offset;

      /*----- Friends -----*/

      // Allow type conversion logic to work.
      friend struct convert::detail::caster_impl<convert::detail::null_tag>;
      friend struct convert::detail::caster_impl<convert::detail::boolean_tag>;
      friend struct convert::detail::caster_impl<convert::detail::integer_tag>;
      friend struct convert::detail::caster_impl<convert::detail::decimal_tag>;
      friend struct convert::detail::caster_impl<convert::detail::string_tag>;

      template <template <class> class LhsRC, template <class> class RhsRC>
      friend bool operator ==(basic_buffer<LhsRC> const&, basic_heap<RhsRC> const&);
      friend size_t detail::sso_bytes<RefCount>();
      friend class detail::object<RefCount>;
      friend class detail::array<RefCount>;
      
      template <template <class> class RC>
      friend class basic_heap;
      friend class basic_buffer<RefCount>;
      template <template <class> class RC>
      friend class basic_packet;

  };

  /**
   *  @brief
   *  dart::buffer implements the finalized subset of behavior exported by dart::packet,
   *  and can be used when wishing to explicitly document in code that you have an immutable
   *  packet.
   *
   *  @details
   *  dart packet objects have two logically distinct modes: finalized and dynamic.
   *
   *  While in dynamic mode, dart packets maintain a heap-based object tree which can
   *  be used to traverse, or mutate, arbitrary data representations in a reasonably
   *  efficient manner.
   *
   *  While in finalized mode, dart packets maintain a contiguously allocated, flattened
   *  object tree designed specifically for efficient/cache-friendly immutable interaction,
   *  and readiness to be distributed via network/shared-memory/filesystem/etc.
   *
   *  Switching from dynamic to finalized mode is accomplished via a call to
   *  dart::heap::finalize. The current mode can be queried using dart::packet::is_finalized
   */
  template <template <class> class RefCount>
  class basic_buffer final {

    public:

      /*----- Public Types -----*/

      using type = detail::type;

      /**
       *  @brief
       *  Class abstracts the concept of iterating over an aggregate dart::buffer.
       *
       *  @details
       *  Class abstracts the concept of iteration for both objects and arrays, and
       *  so it distinguishes between iterating over a "keyspace" or "valuespace".
       *  Specifically, if the iterator was constructed (or assigned from) a call
       *  to dart::packet::begin, it will iterate over values, and if it was
       *  constructed (or assigned from) a call to dart::packet::key_begin, it will
       *  iterate over keys.
       *  The helper dart::packet::kvbegin returns a std::tuple of key and value
       *  iterators, and can be used very naturally with structured bindings in C++17.
       *
       *  @remarks
       *  Although dart::buffer::iterator logically supports the operations and
       *  semantics of a Bidirectional Iterator (multipass guarantees, both incrementable
       *  and decrementable, etc), for implementation reasons, its dereference operator
       *  returns a temporary packet instance, which requires it to claim the weakest
       *  iterator category, the Input Iterator.
       */
      class iterator final {

        public:

          /*----- Public Types -----*/

          using difference_type = ssize_t;
          using value_type = basic_buffer;
          using reference = value_type;
          using pointer = value_type;
          using iterator_category = std::input_iterator_tag;

          /*----- Lifecycle Functions -----*/

          iterator() = default;
          iterator(iterator const&) = default;
          iterator(iterator&& other) noexcept;
          ~iterator() = default;

          /*----- Operators -----*/

          // Assignment.
          iterator& operator =(iterator const&) = default;
          auto operator =(iterator&& other) noexcept -> iterator&;

          // Comparison.
          bool operator ==(iterator const& other) const noexcept;
          bool operator !=(iterator const& other) const noexcept;

          // Increments.
          auto operator ++() noexcept -> iterator&;
          auto operator --() noexcept -> iterator&;
          auto operator ++(int) noexcept -> iterator;
          auto operator --(int) noexcept -> iterator;

          // Dereference.
          auto operator *() const noexcept -> reference;
          auto operator ->() const noexcept -> pointer;

          // Conversion.
          explicit operator bool() const noexcept;

        private:

          /*----- Private Lifecycle Functions -----*/

          iterator(value_type pkt, detail::ll_iterator<RefCount> it) :
            pkt(std::move(pkt)),
            impl(std::move(it))
          {}

          /*----- Private Members -----*/

          value_type pkt;
          shim::optional<detail::ll_iterator<RefCount>> impl;

          /*----- Friends -----*/

          friend class basic_buffer;

      };

      using object = basic_object<basic_buffer>;
      using array = basic_array<basic_buffer>;
      using string = basic_string<basic_buffer>;
      using number = basic_number<basic_buffer>;
      using flag = basic_flag<basic_buffer>;
      using null = basic_null<basic_buffer>;

      // Type than can implicitly subsume this type.
      using generic_type = basic_packet<RefCount>;

      // Views of views don't work, so prevent infinite recursion
      using view = std::conditional_t<
        refcount::is_owner<RefCount>::value,
        basic_buffer<view_ptr_context<RefCount>::template view_ptr>,
        basic_buffer
      >;

      using size_type = size_t;
      using reverse_iterator = std::reverse_iterator<iterator>;

      /*----- Lifecycle Functions -----*/

      /**
       *  @brief
       *  Default constructor.
       *  Creates a null non-finalized packet.
       */
      basic_buffer() noexcept :
        raw({detail::raw_type::null, nullptr}),
        buffer_ref(nullptr)
      {}

      /**
       *  @brief
       *  Converting constructor.
       *  Explicitly converts a dart::heap into a dart::buffer.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      explicit basic_buffer(basic_heap<RefCount> const& heap);

      /**
       *  @brief
       *  Copying network object constructor.
       *
       *  @details
       *  Reconstitutes a previously finalized packet from a buffer of bytes
       *  by allocating space for, and copying, the given buffer.
       *  Given buffer pointer need not be well aligned, function will
       *  internally handle alignment requirements.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      explicit basic_buffer(gsl::span<gsl::byte const> buffer) :
        raw({detail::raw_type::object, buffer.data()}),
        buffer_ref(allocate_pointer(buffer))
      {}

      /**
       *  @brief
       *  Network object constructor.
       *
       *  @details
       *  Reconstitutes a previously finalized packet from a buffer of bytes.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      explicit basic_buffer(shareable_ptr<RefCount<gsl::byte const>> buffer) :
        raw({detail::raw_type::object, buffer.get()}),
        buffer_ref(validate_pointer(std::move(buffer)))
      {}

      /**
       *  @brief
       *  Network object constructor.
       *
       *  @details
       *  Reconstitutes a previously finalized packet from a buffer of bytes.
       *
       *  @remarks
       *  Function is so heavily overloaded because the portability of
       *  std::unique_ptr converting constructors has been pretty flakey in practice.
       */
      template <class Del, bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      explicit basic_buffer(std::unique_ptr<gsl::byte const[], Del>&& buffer) :
        raw({detail::raw_type::object, buffer.get()}),
        buffer_ref(normalize(validate_pointer(std::move(buffer))))
      {}

      /**
       *  @brief
       *  Network object constructor.
       *  
       *  @details
       *  Reconstitutes a previously finalized packet from a buffer of bytes.
       *
       *  @remarks
       *  Function is so heavily overloaded because the portability of
       *  std::unique_ptr converting constructors has been pretty flakey in practice.
       */
      template <class Del, bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      explicit basic_buffer(std::unique_ptr<gsl::byte const, Del>&& buffer) :
        raw({detail::raw_type::object, buffer.get()}),
        buffer_ref(normalize(validate_pointer(std::move(buffer))))
      {}

      /**
       *  @brief
       *  Network object constructor.
       *  
       *  @details
       *  Reconstitutes a previously finalized packet from a buffer of bytes.
       *
       *  @remarks
       *  Function is so heavily overloaded because the portability of
       *  std::unique_ptr converting constructors has been pretty flakey in practice.
       */
      template <class Del, bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      explicit basic_buffer(std::unique_ptr<gsl::byte, Del>&& buffer) :
        raw({detail::raw_type::object, buffer.get()}),
        buffer_ref(normalize(validate_pointer(std::move(buffer))))
      {}

      /**
       *  @brief
       *  Copy constructor.
       *
       *  @details
       *  Increments refcount of underlying network buffer.
       */
      basic_buffer(basic_buffer const&) = default;

      /**
       *  @brief
       *  Move constructor.
       *
       *  @details
       *  Steals the refcount of underlying network buffer.
       */
      basic_buffer(basic_buffer&& other) noexcept;

      /**
       *  @brief
       *  Destructor. Does what you think it does.
       */
      ~basic_buffer() = default;

      /*----- Operators -----*/

      /**
       *  @brief
       *  Copy assignment operator.
       *
       *  @details
       *  Increments refcount of underlying network buffer.
       */
      basic_buffer& operator =(basic_buffer const&) & = default;

      // An MSVC bug prevents us from deleting based on ref qualifiers.
#if !DART_USING_MSVC
      basic_buffer& operator =(basic_buffer const&) && = delete;
#endif

      /**
       *  @brief
       *  Move assignment operator.
       *
       *  @details
       *  Steals refcount of an expiring network buffer.
       */
      basic_buffer& operator =(basic_buffer&& other) & noexcept;

      // An MSVC bug prevents us from deleting based on ref qualifiers.
#if !DART_USING_MSVC
      basic_buffer& operator =(basic_buffer&&) && = delete;
#endif

      /**
       *  @brief
       *  Array subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within
       *  bounds, will return the requested array index as a new packet.
       */
      template <class Number>
      basic_buffer operator [](basic_number<Number> const& idx) const&;

      /**
       *  @brief
       *  Array subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within
       *  bounds, will return the requested array index as a new packet.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto arr = some_array();
       *  auto deep = arr[0][1][2];
       *  ```
       */
      template <class Number, bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer&& operator [](basic_number<Number> const& idx) &&;

      /**
       *  @brief
       *  Array subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within
       *  bounds, will return the requested array index as a new packet.
       */
      basic_buffer operator [](size_type index) const&;

      /**
       *  @brief
       *  Array subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within
       *  bounds, will return the requested array index as a new packet.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto arr = some_array();
       *  auto deep = arr[0][1][2];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer&& operator [](size_type index) &&;

      /**
       *  @brief
       *  Object subscript operator.
       *
       *  @details
       *  Assuming this is an object, returns the value associated with the given
       *  key, or a null packet if not such mapping exists.
       */
      template <class String>
      basic_buffer operator [](basic_string<String> const& key) const&;

      /**
       *  @brief
       *  Object subscript operator.
       *
       *  @details
       *  Assuming this is an object, returns the value associated with the given
       *  key, or a null packet if not such mapping exists.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto obj = some_object();
       *  auto deep = obj["first"]["second"]["third"];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <class String, bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer&& operator [](basic_string<String> const& key) &&;

      /**
       *  @brief
       *  Object subscript operator.
       *
       *  @details
       *  Assuming this is an object, returns the value associated with the given
       *  key, or a null packet if not such mapping exists.
       */
      basic_buffer operator [](shim::string_view key) const&;

      /**
       *  @brief
       *  Object subscript operator.
       *
       *  @details
       *  Assuming this is an object, returns the value associated with the given
       *  key, or a null packet if not such mapping exists.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto obj = some_object();
       *  auto deep = obj["first"]["second"]["third"];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer&& operator [](shim::string_view key) &&;

      /**
       *  @brief
       *  Combined object/array subscript operator.
       *
       *  @details
       *  Assuming this is an object/array, returns the value associated with the
       *  given key/index, or a null packet/throws an exception if no such mapping exists.
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          meta::is_dartlike<KeyType const&>::value
        >
      >
      basic_buffer operator [](KeyType const& indentifier) const&;

      /**
       *  @brief
       *  Combined object/array subscript operator.
       *
       *  @details
       *  Assuming this is an object/array, returns the value associated with the
       *  given key/index, or a null packet/throws an exception if no such mapping exists.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto obj = some_object();
       *  auto deep = obj["first"]["second"]["third"];
       *  ```
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          meta::is_dartlike<KeyType const&>::value
        >
      >
      basic_buffer&& operator [](KeyType const& identifier) &&;

      /**
       *  @brief
       *  Equality operator.
       *  
       *  @details
       *  All packet types always return deep equality (aggregates included).
       *  For finalized packets like dart::buffer, this is implemented as memcmp
       *  of the underlying buffer, and is _stupdendously_ fast (think gigabytes
       *  per/second).
       *  
       *  @remarks
       *  "Do as vector does"
       */
      template <template <class> class OtherRC>
      bool operator ==(basic_buffer<OtherRC> const& other) const noexcept;

      /**
       *  @brief
       *  Inequality operator.
       *  
       *  @details
       *  All packet types always return deep equality (aggregates included).
       *  For finalized packets like dart::buffer, this is implemented as memcmp
       *  of the underlying buffer, and is _stupdendously_ fast (think gigabytes
       *  per/second).
       *  
       *  @remarks
       *  "Do as vector does"
       */
      template <template <class> class OtherRC>
      bool operator !=(basic_buffer<OtherRC> const& other) const noexcept;

      /**
       *  @brief
       *  Member access operator.
       *
       *  @details
       *  Function isn't really intended to be called by the user directly, and
       *  exists as an API quirk due to the fact that dereferencing a
       *  dart::packet::iterator produces a temporary packet instance.
       */
      auto operator ->() noexcept -> basic_buffer*;

      /**
       *  @brief
       *  Member access operator.
       *
       *  @details
       *  Function isn't really intended to be called by the user directly, and
       *  exists as an API quirk due to the fact that dereferencing a
       *  dart::packet::iterator produces a temporary packet instance.
       */
      auto operator ->() const noexcept -> basic_buffer const*;

      /**
       *  @brief
       *  Existential operator (bool conversion operator).
       *
       *  @details
       *  Operator returns false if the current packet is null or false.
       *  Returns true in all other situations.
       */
      explicit operator bool() const noexcept;

      /**
       *  @brief
       *  Operator allows for converting the current buffer instance into a
       *  non-owning, read-only, view
       */
      operator view() const& noexcept;
      operator view() && = delete;

      /**
       *  @brief
       *  String conversion operator.
       *
       *  @details
       *  Returns the string value of the current packet or throws
       *  if no such value exists.
       */
      explicit operator std::string() const;

      /**
       *  @brief
       *  String conversion operator.
       *
       *  @details
       *  Returns the string value of the current packet or throws
       *  if no such value exists.
       */
      explicit operator shim::string_view() const;

      /**
       *  @brief
       *  Integer conversion operator.
       *
       *  @details
       *  Returns the integer value of the current packet or throws
       *  if no such value exists.
       */
      explicit operator int64_t() const;

      /**
       *  @brief
       *  Decimal conversion operator.
       *
       *  @details
       *  Returns the decimal value of the current packet or throws
       *  if no such value exists.
       */
      explicit operator double() const;

      /**
       *  @brief
       *  dart::heap conversion operator.
       *
       *  @details
       *  Function walks across the flattened object tree and lifts it
       *  into a dynamic heap representation, making at least as many
       *  allocations as nodes in the tree during the process.
       */
      explicit operator basic_heap<RefCount>() const;

      /*----- Public API -----*/

      /*----- Factory Functions -----*/

      /**
       *  @brief
       *  Object factory function.
       *  Returns a new, non-finalized, object with the given sequence of key-value pairs.
       *
       *  @details
       *  Object keys can only be strings, and so arguments must be paired such that the
       *  first of each pair is convertible to a dart::packet string, and the second of
       *  each pair is convertible into a dart::packet of any type.
       */
      template <class... Args, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          sizeof...(Args) % 2 == 0
        >
      >
      static basic_buffer make_object(Args&&... pairs);

      /**
       *  @brief
       *  Object factory function.
       *  Returns a new, non-finalized, object with the given sequence of key-value pairs.
       *
       *  @details
       *  Object keys can only be strings, and so arguments must be paired such that the
       *  first of each pair is convertible to a dart::heap string, and the second of
       *  each pair is convertible into a dart::heap of any type.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_buffer make_object(gsl::span<basic_heap<RefCount> const> pairs);

      /**
       *  @brief
       *  Object factory function.
       *  Returns a new, non-finalized, object with the given sequence of key-value pairs.
       *
       *  @details
       *  Object keys can only be strings, and so arguments must be paired such that the
       *  first of each pair is convertible to a dart::heap string, and the second of
       *  each pair is convertible into a dart::heap of any type.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_buffer make_object(gsl::span<basic_buffer<RefCount> const> pairs);

      /**
       *  @brief
       *  Object factory function.
       *  Returns a new, non-finalized, object with the given sequence of key-value pairs.
       *
       *  @details
       *  Object keys can only be strings, and so arguments must be paired such that the
       *  first of each pair is convertible to a dart::heap string, and the second of
       *  each pair is convertible into a dart::heap of any type.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_buffer make_object(gsl::span<basic_packet<RefCount> const> pairs);

      /**
       *  @brief
       *  Null factory function.
       *  Returns a null packet.
       */
      static basic_buffer make_null() noexcept;

      /*----- Aggregate Builder Functions -----*/

      /**
       *  @brief
       *  Function "injects" a set of key-value pairs into an object without
       *  modifying it by returning a new object with the union of the keyset
       *  of the original object and the provided key-value pairs.
       *
       *  @details
       *  Function provides a uniform interface for _efficient_ injection of
       *  keys across the entire Dart API (also works for dart::buffer).
       */
      template <class... Args, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          sizeof...(Args) % 2 == 0
          &&
          meta::conjunction<
            convert::is_castable<Args, basic_packet<RefCount>>...
          >::value
        >
      >
      basic_buffer inject(Args&&... pairs) const;

      /**
       *  @brief
       *  Function "injects" a set of key-value pairs into an object without
       *  modifying it by returning a new object with the union of the keyset
       *  of the original object and the provided key-value pairs.
       *
       *  @details
       *  Function provides a uniform interface for _efficient_ injection of
       *  keys across the entire Dart API (also works for dart::buffer).
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer inject(gsl::span<basic_heap<RefCount> const> pairs) const;

      /**
       *  @brief
       *  Function "injects" a set of key-value pairs into an object without
       *  modifying it by returning a new object with the union of the keyset
       *  of the original object and the provided key-value pairs.
       *
       *  @details
       *  Function provides a uniform interface for _efficient_ injection of
       *  keys across the entire Dart API (also works for dart::buffer).
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer inject(gsl::span<basic_buffer const> pairs) const;

      /**
       *  @brief
       *  Function "injects" a set of key-value pairs into an object without
       *  modifying it by returning a new object with the union of the keyset
       *  of the original object and the provided key-value pairs.
       *
       *  @details
       *  Function provides a uniform interface for _efficient_ injection of
       *  keys across the entire Dart API (also works for dart::buffer).
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer inject(gsl::span<basic_packet<RefCount> const> pairs) const;

      /**
       *  @brief
       *  Function "projects" a set of key-value pairs in the mathematical
       *  sense that it creates a new packet containing the intersection of
       *  the supplied keys, and those present in the original packet.
       *
       *  @details
       *  Function provides a uniform interface for _efficient_ projection of
       *  keys across the entire Dart API (also works for dart::buffer).
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer project(std::initializer_list<shim::string_view> keys) const;

      /**
       *  @brief
       *  Function "projects" a set of key-value pairs in the mathematical
       *  sense that it creates a new packet containing the intersection of
       *  the supplied keys, and those present in the original packet.
       *
       *  @details
       *  Function provides a uniform interface for _efficient_ projection of
       *  keys across the entire Dart API (also works for dart::buffer).
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer project(gsl::span<std::string const> keys) const;

      /**
       *  @brief
       *  Function "projects" a set of key-value pairs in the mathematical
       *  sense that it creates a new packet containing the intersection of
       *  the supplied keys, and those present in the original packet.
       *
       *  @details
       *  Function provides a uniform interface for _efficient_ projection of
       *  keys across the entire Dart API (also works for dart::buffer).
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer project(gsl::span<shim::string_view const> keys) const;

      /*----- State Manipulation Functions -----*/

      /**
       *  @brief
       *  Function transitions to a non-finalized state by returning a dart::heap instance
       *  that describes the same object tree.
       *
       *  @details
       *  dart packets have two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart packets maintain a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart packets maintain a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::buffer::finalize, the reverse can be accomplished using dart::buffer::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_heap<RefCount> definalize() const;

      /**
       *  @brief
       *  Function transitions to a non-finalized state by returning a dart::heap instance
       *  that describes the same object tree.
       *
       *  @details
       *  dart packets have two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart packets maintain a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart packets maintain a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::buffer::finalize, the reverse can be accomplished using dart::buffer::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_heap<RefCount> lift() const;

      /**
       *  @brief
       *  Function would transition to finalized mode, but as dart::buffer is finalized
       *  by definition, simply returns the current packet instance.
       *
       *  @details
       *  dart packets have two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart packets maintain a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart packets maintain a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::heap::finalize, the reverse can be accomplished using dart::heap::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer& finalize() &;

      /**
       *  @brief
       *  Function would transition to finalized mode, but as dart::buffer is finalized
       *  by definition, simply returns the current packet instance.
       *
       *  @details
       *  dart packets have two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart packets maintain a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart packets maintain a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::heap::finalize, the reverse can be accomplished using dart::heap::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer const& finalize() const&;

      /**
       *  @brief
       *  Function would transition to finalized mode, but as dart::buffer is finalized
       *  by definition, simply returns the current packet instance.
       *
       *  @details
       *  dart packets have two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart packets maintain a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart packets maintain a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::heap::finalize, the reverse can be accomplished using dart::heap::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer&& finalize() &&;

      /**
       *  @brief
       *  Function would transition to finalized mode, but as dart::buffer is finalized
       *  by definition, simply returns the current packet instance.
       *
       *  @details
       *  dart packets have two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart packets maintain a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart packets maintain a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::heap::finalize, the reverse can be accomplished using dart::heap::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer const&& finalize() const&&;

      /**
       *  @brief
       *  Function would transition to finalized mode, but as dart::buffer is finalized
       *  by definition, simply returns the current packet instance.
       *
       *  @details
       *  dart packets have two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart packets maintain a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart packets maintain a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::heap::finalize, the reverse can be accomplished using dart::heap::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer& lower() &;

      /**
       *  @brief
       *  Function would transition to finalized mode, but as dart::buffer is finalized
       *  by definition, simply returns the current packet instance.
       *
       *  @details
       *  dart packets have two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart packets maintain a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart packets maintain a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::heap::finalize, the reverse can be accomplished using dart::heap::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer const& lower() const&;

      /**
       *  @brief
       *  Function would transition to finalized mode, but as dart::buffer is finalized
       *  by definition, simply returns the current packet instance.
       *
       *  @details
       *  dart packets have two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart packets maintain a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart packets maintain a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::heap::finalize, the reverse can be accomplished using dart::heap::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer&& lower() &&;

      /**
       *  @brief
       *  Function would transition to finalized mode, but as dart::buffer is finalized
       *  by definition, simply returns the current packet instance.
       *
       *  @details
       *  dart packets have two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart packets maintain a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart packets maintain a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::heap::finalize, the reverse can be accomplished using dart::heap::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer const&& lower() const&&;

      /**
       *  @brief
       *  Function re-seats the network buffer referred to by the given packet on top of the
       *  specified reference counter.
       *
       *  @details
       *  Function name is intentionally long and hard to spell to denote the fact that you
       *  shouldn't be calling it very frequently.
       *  Switching reference counters requires a deep copy of the entire tree, which means
       *  as many allocations and copies as there are nodes in the tree.
       *  Very expensive, should only be called when truly necessary for applications that
       *  need to use multiple reference counter types simultaneously.
       */
      template <template <class> class NewCount>
      static basic_buffer<NewCount> transmogrify(basic_buffer<RefCount> const& buffer);

      /*----- JSON Helper Functions -----*/

#ifdef DART_USE_SAJSON
      /**
       *  @brief
       *  Function constructs an optionally finalized packet to represent the given JSON string.
       *
       *  @details
       *  Parsing is based on RapidJSON, and so exposes the same parsing customization points as
       *  RapidJSON.
       *  If your JSON has embedded comments in it, NaN or +/-Infinity values, or trailing commas,
       *  you can parse in the following ways:
       *  ```
       *  auto json = input.read();
       *  auto comments = dart::heap::from_json<dart::parse_comments>(json);
       *  auto nan_inf = dart::heap::from_json<dart::parse_nan>(json);
       *  auto commas = dart::heap::from_json<dart::parse_trailing_commas>(json);
       *  auto all_of_it = dart::heap::from_json<dart::parse_permissive>(json);
       *  ```
       */
      template <unsigned parse_stack_size = default_parse_stack_size,
               bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_buffer from_json(shim::string_view json);
#elif DART_HAS_RAPIDJSON
      /**
       *  @brief
       *  Function constructs an optionally finalized packet to represent the given JSON string.
       *
       *  @details
       *  Parsing is based on RapidJSON, and so exposes the same parsing customization points as
       *  RapidJSON.
       *  If your JSON has embedded comments in it, NaN or +/-Infinity values, or trailing commas,
       *  you can parse in the following ways:
       *  ```
       *  auto json = input.read();
       *  auto comments = dart::buffer::from_json<dart::parse_comments>(json);
       *  auto nan_inf = dart::buffer::from_json<dart::parse_nan>(json);
       *  auto commas = dart::buffer::from_json<dart::parse_trailing_commas>(json);
       *  auto all_of_it = dart::buffer::from_json<dart::parse_permissive>(json);
       *  ```
       */
      template <unsigned flags = parse_default,
               bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_buffer from_json(shim::string_view json);
#endif

#if DART_HAS_RAPIDJSON
      /**
       *  @brief
       *  Function serializes any packet into a JSON string.
       *
       *  @details
       *  Serialization is based on RapidJSON, and so exposes the same customization points as
       *  RapidJSON.
       *  If your packet has NaN or +/-Infinity values, you can serialize it in the following way:
       *  ```
       *  auto obj = some_object();
       *  auto json = obj.to_json<dart::write_nan>();
       *  do_something(std::move(json));
       *  ```
       */
      template <unsigned flags = write_default>
      std::string to_json() const;
#endif

      /*----- YAML Helper Functions -----*/

#if DART_HAS_YAML
      /**
       *  @brief
       *  Function constructs an optionally finalized packet to represent the given YAML string.
       *
       *  @remarks
       *  At the time of this writing, parsing logic does not support YAML anchors, this will
       *  be added soon.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_buffer from_yaml(shim::string_view yaml);
#endif

      /*----- Value Accessor Functions -----*/

      /**
       *  @brief
       *  Array access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within bounds, will
       *  return the requested array index as a new packet.
       */
      template <class Number>
      basic_buffer get(basic_number<Number> const& idx) const&;

      /**
       *  @brief
       *  Array access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within bounds, will
       *  return the requested array index as a new packet.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto arr = some_array();
       *  auto deep = arr[0][1][2];
       *  ```
       */
      template <class Number, bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer&& get(basic_number<Number> const& idx) &&;

      /**
       *  @brief
       *  Array access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within bounds, will
       *  return the requested array index as a new packet.
       */
      basic_buffer get(size_type index) const&;

      /**
       *  @brief
       *  Array access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within bounds, will
       *  return the requested array index as a new packet.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto arr = some_array();
       *  auto deep = arr[0][1][2];
       *  ```
       */
      basic_buffer&& get(size_type index) &&;

      /**
       *  @brief
       *  Object access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an object, function will return the requested key-value pair as a
       *  new packet.
       */
      template <class String>
      basic_buffer get(basic_string<String> const& key) const&;

      /**
       *  @brief
       *  Object access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an object, function will return the requested key-value pair as a
       *  new packet.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto obj = some_object();
       *  auto deep = obj["first"]["second"]["third"];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <class String, bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer&& get(basic_string<String> const& key) &&;

      /**
       *  @brief
       *  Object access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an object, function will return the requested key-value pair as a
       *  new packet.
       */
      basic_buffer get(shim::string_view key) const&;

      /**
       *  @brief
       *  Object access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an object, function will return the requested key-value pair as a
       *  new packet.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto obj = some_object();
       *  auto deep = obj["first"]["second"]["third"];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer&& get(shim::string_view key) &&;

      /**
       *  @brief
       *  Combined array/object access method, precisely equivalent to the corresponding
       *  subscript operator.
       *
       *  @details
       *  Assuming this is an object/array, returns the value associated with the
       *  given key/index, or a null packet/throws an exception if no such mapping exists.
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          meta::is_dartlike<KeyType const&>::value
        >
      >
      basic_buffer get(KeyType const& identifier) const&;

      /**
       *  @brief
       *  Combined array/object access method, precisely equivalent to the corresponding
       *  subscript operator.
       *
       *  @details
       *  Assuming this is an object/array, returns the value associated with the
       *  given key/index, or a null packet/throws an exception if no such mapping exists.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto obj = some_object();
       *  auto deep = obj["first"]["second"]["third"];
       *  ```
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          meta::is_dartlike<KeyType const&>::value
        >
      >
      basic_buffer&& get(KeyType const& identifier) &&;

      /**
       *  @brief
       *  Function returns the value associated with the separator delimited object path.
       */
      basic_buffer get_nested(shim::string_view path, char separator = '.') const;

      /**
       *  @brief
       *  Array access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within bounds, will
       *  return the requested array index as a new packet.
       */
      template <class Number>
      basic_buffer at(basic_number<Number> const& idx) const&;

      /**
       *  @brief
       *  Array access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within bounds, will
       *  return the requested array index as a new packet.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto arr = some_array();
       *  auto deep = arr[0][1][2];
       *  ```
       */
      template <class Number, bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer&& at(basic_number<Number> const& idx) &&;

      /**
       *  @brief
       *  Array access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within bounds, will
       *  return the requested array index as a new packet.
       */
      basic_buffer at(size_type index) const&;

      /**
       *  @brief
       *  Array access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within bounds, will
       *  return the requested array index as a new packet.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto arr = some_array();
       *  auto deep = arr[0][1][2];
       *  ```
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer&& at(size_type index) &&;

      /**
       *  @brief
       *  Object access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an object, function will return the requested key-value pair as a
       *  new packet.
       */
      template <class String>
      basic_buffer at(basic_string<String> const& key) const&;

      /**
       *  @brief
       *  Object access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an object, function will return the requested key-value pair as a
       *  new packet.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto obj = some_object();
       *  auto deep = obj["first"]["second"]["third"];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <class String, bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer&& at(basic_string<String> const& key) &&;

      /**
       *  @brief
       *  Object access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an object, function will return the requested key-value pair as a
       *  new packet.
       */
      basic_buffer at(shim::string_view key) const&;

      /**
       *  @brief
       *  Object access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an object, function will return the requested key-value pair as a
       *  new packet.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto obj = some_object();
       *  auto deep = obj["first"]["second"]["third"];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer&& at(shim::string_view key) &&;

      /**
       *  @brief
       *  Combined array/object access method, precisely equivalent to the corresponding
       *  subscript operator.
       *
       *  @details
       *  Assuming this is an object/array, returns the value associated with the
       *  given key/index, or a null packet/throws an exception if no such mapping exists.
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          meta::is_dartlike<KeyType const&>::value
        >
      >
      basic_buffer at(KeyType const& identifier) const&;

      /**
       *  @brief
       *  Combined array/object access method, precisely equivalent to the corresponding
       *  subscript operator.
       *
       *  @details
       *  Assuming this is an object/array, returns the value associated with the
       *  given key/index, or a null packet/throws an exception if no such mapping exists.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto obj = some_object();
       *  auto deep = obj["first"]["second"]["third"];
       *  ```
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          meta::is_dartlike<KeyType const&>::value
        >
      >
      basic_buffer&& at(KeyType const& identifier) &&;

      /**
       *  @brief
       *  Function returns the first element in an array.
       *
       *  @details
       *  Return the first element or throws if the array is empty.
       */
      basic_buffer at_front() const&;

      /**
       *  @brief
       *  Function returns the first element in an array.
       *
       *  @details
       *  Return the first element or throws if the array is empty.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto obj = some_object();
       *  auto deep = obj["first"]["second"]["third"];
       *  ```
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer&& at_front() &&;

      /**
       *  @brief
       *  Function returns the last element in an array.
       *
       *  @details
       *  Return the last element or throws if the array is empty.
       */
      basic_buffer at_back() const&;

      /**
       *  @brief
       *  Function returns the last element in an array.
       *
       *  @details
       *  Return the last element or throws if the array is empty.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto obj = some_object();
       *  auto deep = obj["first"]["second"]["third"];
       *  ```
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer&& at_back() &&;

      /**
       *  @brief
       *  Function returns the first element in an array.
       *
       *  @details
       *  If this is an array with at least one thing in it,
       *  returns the first element.
       *  Throws otherwise.
       */
      basic_buffer front() const&;

      /**
       *  @brief
       *  Function returns the first element in an array.
       *
       *  @details
       *  If this is an array with at least one thing in it,
       *  returns the first element.
       *  Throws otherwise.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer&& front() &&;

      /**
       *  @brief
       *  Function returns the last element in an array.
       *
       *  @details
       *  If this is an array with at least oen thing in it,
       *  returns the last element.
       *  Throws otherwise.
       */
      basic_buffer back() const&;

      /**
       *  @brief
       *  Function returns the last element in an array.
       *
       *  @details
       *  If this is an array with at least oen thing in it,
       *  returns the last element.
       *  Throws otherwise.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_buffer&& back() &&;

      /**
       *  @brief
       *  Finds the requested key-value mapping in an object.
       *
       *  @details
       *  If this is in an object, function will return an
       *  iterator to the requested VALUE mapping, or the
       *  end iterator.
       *  If this is an object, throws an exception.
       */
      template <class String>
      auto find(basic_string<String> const& key) const -> iterator;

      /**
       *  @brief
       *  Finds the requested key-value mapping in an object.
       *
       *  @details
       *  If this is in an object, function will return an
       *  iterator to the requested VALUE mapping, or the
       *  end iterator.
       *  If this is an object, throws an exception.
       */
      auto find(shim::string_view key) const -> iterator;

      /**
       *  @brief
       *  Finds the requested key-value mapping in an object.
       *
       *  @details
       *  If this is in an object, function will return an
       *  iterator to the requested KEY mapping, or the
       *  end iterator.
       *  If this is an object, throws an exception.
       */
      template <class String>
      auto find_key(basic_string<String> const& key) const -> iterator;

      /**
       *  @brief
       *  Finds the requested key-value mapping in an object.
       *
       *  @details
       *  If this is in an object, function will return an
       *  iterator to the requested VALUE mapping, or the
       *  end iterator.
       *  If this is an object, throws an exception.
       */
      auto find_key(shim::string_view key) const -> iterator;

      /**
       *  @brief
       *  Returns all keys associated with the current packet.
       *
       *  @details
       *  If this is an object, returns a vector of all its keys.
       *  Throws otherwise.
       */
      std::vector<basic_buffer> keys() const;

      /**
       *  @brief
       *  Returns all subvalues of the current aggregate.
       *
       *  @details
       *  If this is an object or array, returns all of the values contained
       *  within
       *  Throws otherwise.
       */
      std::vector<basic_buffer> values() const;

      /**
       *  @brief
       *  Returns whether a particular key-value pair exists within an object.
       *
       *  @details
       *  If this is an object, function returns whether the given key is present
       *  Throws otherwise.
       */
      template <class String>
      bool has_key(basic_string<String> const& key) const;

      /**
       *  @brief
       *  Returns whether a particular key-value pair exists within an object.
       *
       *  @details
       *  If this is an object, function returns whether the given key is present
       *  Throws otherwise.
       */
      bool has_key(shim::string_view key) const;

      /**
       *  @brief
       *  Returns whether a particular key-value pair exists within an object.
       *
       *  @details
       *  If this is an object, function returns whether the given key is present
       *  Throws otherwise.
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          meta::is_dartlike<KeyType const&>::value
        >
      >
      bool has_key(KeyType const& key) const;

      /**
       *  @brief
       *  Function returns a null-terminated c-string representing the current packet.
       *
       *  @details
       *  If the current packet is a string, returns a c-string, otherwise throws.
       *
       *  @remarks
       *  Lifetime of the returned pointer is tied to the current instance.
       */
      char const* str() const;

      /**
       *  @brief
       *  Function optionally returns a c-string representing the current packet.
       *
       *  @details
       *  If the current packet is a string, returns a null-terminated c-string,
       *  otherwise returns the provided character pointer.
       *
       *  @remarks
       *  If this is a string, the lifetime of the returned pointer
       *  is equal to that of the current instance
       *  If this is not a string, the lifetime of the returned pointer
       *  is equivalent to the lifetime of the optional parameter.
       */
      char const* str_or(char const* opt) const noexcept;

      /**
       *  @brief
       *  Function returns a string view representing the current packet.
       *
       *  @details
       *  If the current packet is a string, returns a string view, otherwise throws.
       *
       *  @remarks
       *  Lifetime of the string view is tied to the current instance.
       */
      shim::string_view strv() const;

      /**
       *  @brief
       *  Function optionally returns a string view representing the current packet.
       *
       *  @details
       *  If the current packet is a string, returns a string view,
       *  otherwise returns the provided string view.
       *
       *  @remarks
       *  If this is a string, the lifetime of the returned string view is
       *  equal to that of the current instance.
       *  If this is not a string, the lifetime of the returned string view
       *  is equivalent to the lifetime of the optional parameter.
       */
      shim::string_view strv_or(shim::string_view opt) const noexcept;

      /**
       *  @brief
       *  Function returns an integer representing the current packet.
       *
       *  @details
       *  If the current packet is an integer, returns a it, otherwise throws.
       */
      int64_t integer() const;

      /**
       *  @brief
       *  Function optionally returns an integer representing the current packet.
       *
       *  @details
       *  If the current packet is an integer, returns a it,
       *  otherwise returns the provided integer.
       */
      int64_t integer_or(int64_t opt) const noexcept;

      /**
       *  @brief
       *  Function returns a decimal representing the current packet.
       *
       *  @details
       *  If the current packet is a decimal, returns a it, otherwise throws.
       */
      double decimal() const;

      /**
       *  @brief
       *  Function optionally returns a decimal representing the current packet.
       *
       *  @details
       *  If the current packet is a decimal, returns a it,
       *  otherwise returns the provided decimal.
       */
      double decimal_or(double opt) const noexcept;

      /**
       *  @brief
       *  Function returns a decimal representing the current packet.
       *
       *  @details
       *  If the current packet is an integer or decimal, returns a it as a decimal,
       *  otherwise throws.
       */
      double numeric() const;

      /**
       *  @brief
       *  Function optionally returns a decimal representing the current packet.
       *
       *  @details
       *  If the current packet is an integer or decimal, returns a it as a decimal,
       *  otherwise returns the provided decimal.
       */
      double numeric_or(double opt) const noexcept;

      /**
       *  @brief
       *  Function returns a decimal representing the current packet.
       *
       *  @details
       *  If the current packet is a decimal, returns a it, otherwise throws.
       */
      bool boolean() const;

      /**
       *  @brief
       *  Function optionally returns a decimal representing the current packet.
       *
       *  @details
       *  If the current packet is a decimal, returns a it,
       *  otherwise returns the provided decimal.
       */
      bool boolean_or(bool opt) const noexcept;

      /**
       *  @brief
       *  Function returns the size of the current packet, according to the semantics
       *  of its type.
       *
       *  @details
       *  If this is an object, returns the number of key-value pairs within the object.
       *  If this is an array, returns the number of elements within the array.
       *  If this is a string, returns the length of the string NOT INCLUDING the null
       *  terminator.
       */
      auto size() const -> size_type;

      /**
       *  @brief
       *  Function returns the capacity of the underlying storage.
       */
      auto capacity() const -> size_type;

      /**
       *  @brief
       *  Function returns whether the current packet is empty, according to the semantics
       *  of its type.
       *
       *  @details
       *  In actuality, returns whether size() == 0;
       */
      bool empty() const;

      /*----- Introspection Functions -----*/

      /**
       *  @brief
       *  Returns whether the current packet is an object or not.
       */
      bool is_object() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an array or not.
       */
      bool is_array() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an object or array or not.
       */
      bool is_aggregate() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a string or not.
       */
      bool is_str() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an integer or not.
       */
      bool is_integer() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a decimal or not.
       */
      bool is_decimal() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an integer or decimal or not.
       */
      bool is_numeric() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a boolean or not.
       */
      bool is_boolean() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is null or not.
       */
      bool is_null() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a string, integer, decimal,
       *  boolean, or null.
       */
      bool is_primitive() const noexcept;

      /**
       *  @brief
       *  Returns the type of the current packet as an enum value.
       */
      auto get_type() const noexcept -> type;

      /**
       *  @brief
       *  Returns whether the current packet is finalized.
       */
      constexpr bool is_finalized() const noexcept;

      /**
       *  @brief
       *  Returns the current reference count of the underlying reference counter.
       */
      auto refcount() const noexcept -> size_type;

      /*----- Network Buffer Accessors -----*/

      /**
       *  @brief
       *  Function returns the network-level buffer for a finalized packet.
       *
       *  @details
       *  gsl::span is a non-owning view type, like std::string_view, and so the
       *  lifetype of the returned view is equal to the lifetime of the current
       *  packet.
       */
      gsl::span<gsl::byte const> get_bytes() const;

      /**
       *  @brief
       *  Function allows the network buffer of the current packet to be exported
       *  directly, without copies.
       *
       *  @details
       *  Function has a fairly awkward API, requiring the user to pass an existing
       *  instance (potentially default-constructed) of the reference counter type,
       *  which it then destroys and reconstructs such that it is a copy of the
       *  current reference counter.
       *  Function is written this way as the reference counter type cannot be assumed
       *  to be either copyable or moveable, and at this time I don't want to expose
       *  dart::shareable_ptr externally.
       */
      size_t share_bytes(RefCount<gsl::byte const>& bytes) const;

      /**
       *  @brief
       *  Function returns an owning copy of the network-level buffer for a
       *  finalized packet.
       *
       *  @details
       *  The primary difference between this and dart::packet::get_bytes is that
       *  get_bytes returns a non-owning view into the packet's buffer, whereas
       *  dup_bytes copies the buffer out into a newly allocated region, and returns
       *  that.
       */
      std::unique_ptr<gsl::byte const[], void (*) (gsl::byte const*)> dup_bytes() const;

      /**
       *  @brief
       *  Function returns an owning copy of the network-level buffer for a
       *  finalized packet.
       *
       *  @details
       *  The primary difference between this and dart::packet::get_bytes is that
       *  get_bytes returns a non-owning view into the packet's buffer, whereas
       *  dup_bytes copies the buffer out into a newly allocated region, and returns
       *  that.
       *
       *  @remarks
       *  Out-parameters aren't great, but it seemed like the easiest way to do this.
       */
      std::unique_ptr<gsl::byte const[], void (*) (gsl::byte const*)> dup_bytes(size_type& len) const;

      /*----- Iterator Functions -----*/

      /**
       *  @brief
       *  Function returns a dart::packet::iterator that can be used to iterate over the
       *  VALUES of the current packet.
       *
       *  @details
       *  Throws if this is not an object or array.
       */
      auto begin() const -> iterator;

      /**
       *  @brief
       *  Function returns a dart::packet::iterator that can be used to iterate over the
       *  VALUES of the current packet.
       *
       *  @details
       *  Throws if this is not an object or array.
       *
       *  @remarks
       *  Function exists for API uniformity with the STL, but is precisely equal to
       *  dart::packet::begin.
       */
      auto cbegin() const -> iterator;

      /**
       *  @brief
       *  Function returns a dart::packet::iterator one-past-the-end for iterating over
       *  the VALUES of the current packet.
       *
       *  @details
       *  Dart iterators are bidirectional, so the iterator could be decremented to iterate
       *  over the valuespace backwards, or it can be used as a sentinel value for
       *  traditional iteration.
       */
      auto end() const -> iterator;

      /**
       *  @brief
       *  Function returns a dart::packet::iterator one-past-the-end for iterating over
       *  the VALUES of the current packet.
       *
       *  @details
       *  Dart iterators are bidirectional, so the iterator could be decremented to iterate
       *  over the valuespace backwards, or it can be used as a sentinel value for
       *  traditional iteration.
       *
       *  @remarks
       *  Function exists for API uniformity with the STL, but is precisely equal to
       *  dart::packet::end.
       */
      auto cend() const -> iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator that iterates in reverse order from
       *  dart::packet::end to dart::packet::begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::begin.
       */
      auto rbegin() const -> reverse_iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator that iterates in reverse order from
       *  dart::packet::end to dart::packet::begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::begin.
       *
       *  @remarks
       *  Function exists for API uniformity with the STL, but is precisely equal to
       *  dart::packet::rbegin.
       */
      auto crbegin() const -> reverse_iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator one-past-the-end, that iterates in
       *  reverse order from dart::packet::end to dart::packet::begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::end.
       */
      auto rend() const -> reverse_iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator one-past-the-end, that iterates in
       *  reverse order from dart::packet::end to dart::packet::begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::end.
       *
       *  @remarks
       *  Function exists for API uniformity with the STL, but is precisely equal to
       *  dart::packet::rbegin.
       */
      auto crend() const -> reverse_iterator;

      /**
       *  @brief
       *  Function returns a dart::packet::iterator that can be used to iterate over the
       *  KEYS of the current packet.
       *
       *  @details
       *  Throws if this is not an object
       */
      auto key_begin() const -> iterator;

      /**
       *  @brief
       *  Function returns a dart::packet::iterator one-past-the-end that can be used to
       *  iterate over the KEYS of the current packet.
       *
       *  @details
       *  Dart iterators are bidirectional, so the iterator could be decremented to iterate
       *  over the keyspace backwards, or it can be used as a sentinel value for traditional
       *  iteration.
       */
      auto key_end() const -> iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator that iterates over the KEYS of the current
       *  packet in reverse order from dart::packet::key_end to dart::packet::key_begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::key_begin.
       */
      auto rkey_begin() const -> reverse_iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator one-past-the-end that iterates over the
       *  KEYS of the current packet in reverse order from dart::packet::key_end to
       *  dart::packet::key_begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::key_end.
       */
      auto rkey_end() const -> reverse_iterator;

      /**
       *  @brief
       *  Function returns a tuple of key-value iterators to allow for easy iteration
       *  over keys and values with C++17 structured bindings.
       */
      auto kvbegin() const -> std::tuple<iterator, iterator>;

      /**
       *  @brief
       *  Function returns a tuple of key-value iterators to allow for easy iteration
       *  over keys and values with C++17 structured bindings.
       */
      auto kvend() const -> std::tuple<iterator, iterator>;

      /**
       *  @brief
       *  Function returns a tuple of key-value reverse iterators to allow for easy
       *  iteration over keys and values with C++17 structured bindings.
       */
      auto rkvbegin() const -> std::tuple<reverse_iterator, reverse_iterator>;

      /**
       *  @brief
       *  Function returns a tuple of key-value reverse iterators to allow for easy
       *  iteration over keys and values with C++17 structured bindings.
       */
      auto rkvend() const -> std::tuple<reverse_iterator, reverse_iterator>;

      /*----- Member Ownership Helpers -----*/

      /**
       *  @brief
       *  Function calculates whether the current type is a non-owning view or not.
       */
      constexpr bool is_view() const noexcept;

      /**
       *  @brief
       *  Function allows one to explicitly grab full ownership of the current view.
       */
      template <bool enabled = !refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      auto as_owner() const noexcept;

    private:

      /*----- Private Types -----*/

      using buffer_ref_type = detail::buffer_refcount_type<RefCount>;
      using ref_type = typename buffer_ref_type::value_type;

      /*----- Private Lifecycle Functions -----*/

      basic_buffer(detail::raw_element raw, buffer_ref_type ref);

      auto allocate_pointer(gsl::span<gsl::byte const> buffer) const -> buffer_ref_type;
      template <class Pointer>
      Pointer&& validate_pointer(Pointer&& ptr) const;
      template <class Pointer>
      auto normalize(Pointer ptr) const -> buffer_ref_type;

      template <class Span>
      static basic_buffer dynamic_make_object(Span pairs);

      /*----- Private Members -----*/

      detail::raw_element raw;
      buffer_ref_type buffer_ref;

      /*----- Friends -----*/

      template <template <class> class RC>
      friend class basic_buffer;
      template <template <class> class RC>
      friend class basic_packet;
      friend struct detail::buffer_builder<RefCount>;
      template <template <class> class LhsRC, template <class> class RhsRC>
      friend bool operator ==(basic_buffer<LhsRC> const&, basic_heap<RhsRC> const&);

  };

  /**
   *  @brief
   *  dart::packet is the most flexible and general purpose class exposed by libdart
   *  for interacting with packets regardless of type/representation. It is likely to
   *  be the only class many users interact with.
   *
   *  @details
   *  dart::packet has two logically distinct modes: finalized and dynamic.
   *
   *  While in dynamic mode, dart::packet maintains a heap-based object tree which can
   *  be used to traverse, or mutate, arbitrary data representations in a reasonably
   *  efficient manner.
   *
   *  While in finalized mode, dart::packet maintains a contiguously allocated, flattened
   *  object tree designed specifically for efficient/cache-friendly immutable interaction,
   *  and readiness to be distributed via network/shared-memory/filesystem/etc.
   *
   *  Switching from dynamic to finalized mode is accomplished via a call to
   *  dart::packet::finalize. The current mode can be queried using dart::packet::is_finalized
   *
   *  @remarks
   *  Dart is distinct from other JSON libraries in that only aggregates (objects and arrays)
   *  can be mutated directly, and whenever accessing data within an aggregate, logically
   *  independent subtrees are returned. To give a concrete example of this,
   *  in the following code:
   *  ```
   *  auto obj = dart::packet::make_object("nested", dart::packet::make_object());
   *  auto nested = obj["nested"];
   *  nested.add_field("hello", "world");
   *  ```
   *  obj is NOT modified after construction. To persist the modifications to "nested", one
   *  would have to follow it up with:
   *  ```
   *  obj.add_field("nested", std::move(nested));
   *  ```
   *  which would replace the original definition of "nested". Copy-on-write semantics
   *  mitigate the performance impact of this.
   *
   *  Finally, dart::packet has a thread-safety model in-line with std::shared_ptr.
   *  Individual dart::packet instances are NOT thread-safe, but the reference counting
   *  performed globally (and the copy-on-write semantics across packet instances) IS
   *  thread-safe.
   */
  template <template <class> class RefCount>
  class basic_packet final {

    public:

      /*----- Public Types -----*/

      // Bring into the scope of the main class.
      using type = detail::type;

      /**
       *  @brief
       *  Class abstracts the concept of iterating over an aggregate dart::packet.
       *
       *  @details
       *  Class abstracts the concept of iteration for both objects and arrays, and
       *  so it distinguishes between iterating over a "keyspace" or "valuespace".
       *  Specifically, if the iterator was constructed (or assigned from) a call
       *  to dart::packet::begin, it will iterate over values, and if it was
       *  constructed (or assigned from) a call to dart::packet::key_begin, it will
       *  iterate over keys.
       *  The helper dart::packet::kvbegin returns a std::tuple of key and value
       *  iterators, and can be used very naturally with structured bindings in C++17.
       *
       *  @remarks
       *  Although dart::packet::iterator logically supports the operations and
       *  semantics of a Bidirectional Iterator (multipass guarantees, both incrementable
       *  and decrementable, etc), for implementation reasons, its dereference operator
       *  returns a temporary packet instance, which requires it to claim the weakest
       *  iterator category, the Input Iterator.
       */
      class iterator final {

        public:

          /*----- Publicly Declared Types -----*/

          using difference_type = ssize_t;
          using value_type = basic_packet;
          using reference = value_type;
          using pointer = value_type;
          using iterator_category = std::input_iterator_tag;

          /*----- Lifecycle Functions -----*/

          iterator() = default;
          iterator(typename basic_heap<RefCount>::iterator it) : impl(std::move(it)) {}
          iterator(typename basic_buffer<RefCount>::iterator it) : impl(std::move(it)) {}
          iterator(iterator const&) = default;
          iterator(iterator&& other) noexcept = default;
          ~iterator() = default;

          /*----- Operators -----*/

          // Assignment.
          iterator& operator =(iterator const&) = default;
          iterator& operator =(iterator&& other) noexcept = default;

          // Comparison.
          bool operator ==(iterator const& other) const noexcept;
          bool operator !=(iterator const& other) const noexcept;

          // Increments.
          auto operator ++() noexcept -> iterator&;
          auto operator --() noexcept -> iterator&;
          auto operator ++(int) noexcept -> iterator;
          auto operator --(int) noexcept -> iterator;

          // Dereference.
          auto operator *() const noexcept -> reference;
          auto operator ->() const noexcept -> pointer;

          // Conversion.
          explicit operator bool() const noexcept;

        private:

          /*----- Private Types -----*/

          using implerator = shim::variant<
            typename basic_heap<RefCount>::iterator,
            typename basic_buffer<RefCount>::iterator
          >;

          /*----- Private Members -----*/

          implerator impl;

          /*----- Friends -----*/

          friend class basic_packet;

      };

      using object = basic_object<basic_packet>;
      using array = basic_array<basic_packet>;
      using string = basic_string<basic_packet>;
      using number = basic_number<basic_packet>;
      using flag = basic_flag<basic_packet>;
      using null = basic_null<basic_packet>;

      // Type than can implicitly subsume this type.
      using generic_type = basic_packet;

      // Views of views don't work, so prevent infinite recursion
      using view = std::conditional_t<
        refcount::is_owner<RefCount>::value,
        basic_packet<view_ptr_context<RefCount>::template view_ptr>,
        basic_packet
      >;

      using size_type = size_t;
      using reverse_iterator = std::reverse_iterator<iterator>;

      /*----- Lifecycle Functions -----*/

      /**
       *  @brief
       *  Default constructor.
       *  Creates a null non-finalized packet.
       */
      basic_packet() noexcept : impl(basic_heap<RefCount>::make_null()) {}

      /**
       *  @brief
       *  Converting constructor.
       *  Implicitly converts a dart::heap into a dart::packet.
       *
       *  @details
       *  Constructor will not allocate memory, and cannot fail, so it is
       *  allowed implicitly.
       */
      basic_packet(basic_heap<RefCount> impl) noexcept : impl(std::move(impl)) {}

      /**
       *  @brief
       *  Converting constructor.
       *  Implicitly converts a dart::buffer into a dart::packet.
       *
       *  @details
       *  Constructor will not allocate memory, and cannot fail, so it is
       *  allowed implicitly.
       */
      basic_packet(basic_buffer<RefCount> impl) noexcept : impl(std::move(impl)) {}

      /**
       *  @brief
       *  Copying network object constructor
       *
       *  @details
       *  Reconstitutes a previously finalized packet from a buffer of bytes
       *  by allocating space for, and copying, the given buffer.
       *  Given buffer pointer need not be well aligned, function will
       *  internally handle alignment requirements.
       */
      template <bool enable = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enable
        >
      >
      explicit basic_packet(gsl::span<gsl::byte const> buffer) :
        impl(basic_buffer<RefCount>(buffer))
      {}

      /**
       *  @brief
       *  Network object constructor.
       *
       *  @details
       *  Reconstitutes a previously finalized packet from a buffer of bytes.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      explicit basic_packet(shareable_ptr<RefCount<gsl::byte const>> buffer) :
        impl(basic_buffer<RefCount>(std::move(buffer)))
      {}

      /**
       *  @brief
       *  Network object constructor.
       *
       *  @details
       *  Reconstitutes a previously finalized packet from a buffer of bytes.
       */
      template <class Del, bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      explicit basic_packet(std::unique_ptr<gsl::byte const[], Del>&& buffer) :
        impl(basic_buffer<RefCount>(std::move(buffer)))
      {}

      /**
       *  @brief
       *  Network object constructor.
       *
       *  @details
       *  Reconstitutes a previously finalized packet from a buffer of bytes.
       */
      template <class Del, bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      explicit basic_packet(std::unique_ptr<gsl::byte const, Del>&& buffer) :
        impl(basic_buffer<RefCount>(std::move(buffer)))
      {}

      /**
       *  @brief
       *  Network object constructor.
       *
       *  @details
       *  Reconstitutes a previously finalized packet from a buffer of bytes.
       */
      template <class Del, bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      explicit basic_packet(std::unique_ptr<gsl::byte, Del>&& buffer) :
        impl(basic_buffer<RefCount>(std::move(buffer)))
      {}

      /**
       *  @brief
       *  Copy constructor.
       *
       *  @details
       *  Increments refcount of underlying dart::heap or dart::buffer.
       */
      basic_packet(basic_packet const&) = default;

      /**
       *  @brief
       *  Move constructor.
       *
       *  @details
       *  Steals the refcount of an expiring dart::heap or dart::buffer.
       */
      basic_packet(basic_packet&&) noexcept = default;

      /**
       *  @brief
       *  Destructor. Does what you think it does.
       */
      ~basic_packet() = default;

      /*----- Operators -----*/

      /**
       *  @brief
       *  Copy assignment operator.
       *
       *  @details
       *  Increments refcount of underlying dart::heap or dart::buffer.
       */
      basic_packet& operator =(basic_packet const& other) & = default;

      // An MSVC bug prevents us from deleting based on ref qualifiers.
#if !DART_USING_MSVC
      basic_packet& operator =(basic_packet const&) && = delete;
#endif

      /**
       *  @brief
       *  Move assignment operator.
       *
       *  @details
       *  Steals refcount of an expiring dart::heap or dart::buffer.
       */
      basic_packet& operator =(basic_packet&& other) & = default;

      // An MSVC bug prevents us from deleting based on ref qualifiers.
#if !DART_USING_MSVC
      basic_packet& operator =(basic_packet&&) && = delete;
#endif

      /**
       *  @brief
       *  Array subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within
       *  bounds, will return the requested array index as a new packet.
       */
      template <class Number>
      basic_packet operator [](basic_number<Number> const& idx) const&;

      /**
       *  @brief
       *  Array subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within
       *  bounds, will return the requested array index as a new packet.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto arr = some_array();
       *  auto deep = arr[0][1][2];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <class Number, bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet&& operator [](basic_number<Number> const& idx) &&;

      /**
       *  @brief
       *  Array subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within
       *  bounds, will return the requested array index as a new packet.
       */
      basic_packet operator [](size_type index) const&;

      /**
       *  @brief
       *  Array subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within
       *  bounds, will return the requested array index as a new packet.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto arr = some_array();
       *  auto deep = arr[0][1][2];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet&& operator [](size_type index) &&;

      /**
       *  @brief
       *  Object subscript operator.
       *
       *  @details
       *  Assuming this is an object, returns the value associated with the given
       *  key, or a null packet if not such mapping exists.
       */
      template <class String>
      basic_packet operator [](basic_string<String> const& key) const&;

      /**
       *  @brief
       *  Object subscript operator.
       *
       *  @details
       *  Assuming this is an object, returns the value associated with the given
       *  key, or a null packet if not such mapping exists.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto obj = some_object();
       *  auto deep = obj["first"]["second"]["third"];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <class String, bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet&& operator [](basic_string<String> const& key) &&;

      /**
       *  @brief
       *  Object subscript operator.
       *
       *  @details
       *  Assuming this is an object, returns the value associated with the given
       *  key, or a null packet if not such mapping exists.
       */
      basic_packet operator [](shim::string_view key) const&;

      /**
       *  @brief
       *  Object subscript operator.
       *
       *  @details
       *  Assuming this is an object, returns the value associated with the given
       *  key, or a null packet if not such mapping exists.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto obj = some_object();
       *  auto deep = obj["first"]["second"]["third"];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet&& operator [](shim::string_view key) &&;

      /**
       *  @brief
       *  Combined object/array subscript operator.
       *
       *  @details
       *  Assuming this is an object/array, returns the value associated with the
       *  given key/index, or a null packet/throws an exception if no such mapping exists.
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          meta::is_dartlike<KeyType const&>::value
        >
      >
      basic_packet operator [](KeyType const& identifier) const&;

      /**
       *  @brief
       *  Combined object/array subscript operator.
       *
       *  @details
       *  Assuming this is an object/array, returns the value associated with the
       *  given key/index, or a null packet/throws an exception if no such mapping exists.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto obj = some_object();
       *  auto deep = obj["first"]["second"]["third"];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          meta::is_dartlike<KeyType const&>::value
        >
      >
      basic_packet&& operator [](KeyType const& identifier) &&;

      /**
       *  @brief
       *  Equality operator.
       *  
       *  @details
       *  All packet types always return deep equality (aggregates included).
       *  While idiomatic for C++, this means that non-finalized object comparisons
       *  can be reasonably expensive.
       *  Finalized comparisons, however, use memcmp to compare the underlying
       *  byte buffers, and is _stupendously_ fast.
       *  
       *  @remarks
       *  "Do as vector does"
       *  Operator == is overloaded to ensure comparability with the rest of
       *  the dart types.
       *  We want comparability across refcounters, but conversions aren't
       *  considered for template parameters.
       */
      bool operator ==(basic_packet const& other) const noexcept;

      /**
       *  @brief
       *  Equality operator.
       *  
       *  @details
       *  All packet types always return deep equality (aggregates included).
       *  While idiomatic for C++, this means that non-finalized object comparisons
       *  can be reasonably expensive.
       *  Finalized comparisons, however, use memcmp to compare the underlying
       *  byte buffers, and is _stupendously_ fast.
       *  
       *  @remarks
       *  "Do as vector does"
       */
      template <template <class> class OtherRC>
      bool operator ==(basic_packet<OtherRC> const& other) const noexcept;

      /**
       *  @brief
       *  Equality operator.
       *  
       *  @details
       *  All packet types always return deep equality (aggregates included).
       *  While idiomatic for C++, this means that non-finalized object comparisons
       *  can be reasonably expensive.
       *  Finalized comparisons, however, use memcmp to compare the underlying
       *  byte buffers, and is _stupendously_ fast.
       *  
       *  @remarks
       *  "Do as vector does"
       *  Operator == is overloaded to ensure comparability with the rest of
       *  the dart types.
       *  We want comparability across refcounters, but conversions aren't
       *  considered for template parameters.
       */
      bool operator !=(basic_packet const& other) const noexcept;

      /**
       *  @brief
       *  Inequality operator.
       *  
       *  @details
       *  All packet types always return deep equality (aggregates included).
       *  While idiomatic C++, this means that non-finalized object/array comparisons
       *  can be arbitrarily expensive.
       *  Finalized comparisons, however, use memcmp to compare the underlying
       *  byte buffers, and are _stupendously_ fast.
       *  
       *  @remarks
       *  "Do as vector does"
       */
      template <template <class> class OtherRC>
      bool operator !=(basic_packet<OtherRC> const& other) const noexcept;

      /**
       *  @brief
       *  Member access operator.
       *
       *  @details
       *  Function isn't really intended to be called by the user directly, and
       *  exists as an API quirk due to the fact that dereferencing a
       *  dart::packet::iterator produces a temporary packet instance.
       */
      auto operator ->() noexcept -> basic_packet*;

      /**
       *  @brief
       *  Member access operator.
       *
       *  @details
       *  Function isn't really intended to be called by the user directly, and
       *  exists as an API quirk due to the fact that dereferencing a
       *  dart::packet::iterator produces a temporary packet instance.
       */
      auto operator ->() const noexcept -> basic_packet const*;

      /**
       *  @brief
       *  Existential operator (bool conversion operator).
       *
       *  @details
       *  Operator returns false if the current packet is null or false.
       *  Returns true in all other situations.
       */
      explicit operator bool() const noexcept;

      /**
       *  @brief
       *  Operator allows for converting the current packet instance into a
       *  non-owning, read-only, view
       */
      operator view() const& noexcept;
      operator view() && = delete;

      /**
       *  @brief
       *  String conversion operator.
       *
       *  @details
       *  Returns the string value of the current packet or throws
       *  if no such value exists.
       */
      explicit operator std::string() const;

      /**
       *  @brief
       *  String conversion operator.
       *
       *  @details
       *  Returns the string value of the current packet or throws
       *  if no such value exists.
       */
      explicit operator shim::string_view() const;

      /**
       *  @brief
       *  Integer conversion operator.
       *
       *  @details
       *  Returns the integer value of the current packet or throws
       *  if no such value exists.
       */
      explicit operator int64_t() const;

      /**
       *  @brief
       *  Decimal conversion operator.
       *
       *  @details
       *  Returns the decimal value of the current packet or throws
       *  if no such value exists.
       */
      explicit operator double() const;

      /**
       *  @brief
       *  dart::heap conversion operator.
       *
       *  @details
       *  If the current packet isn't finalized, function simply unwraps and returns
       *  a copy of the current instance (incrementing the reference counter).
       *  If the current packet IS finalized, function walks across the flattened
       *  object tree and lifts it into a dynamic heap representation,
       *  making at least as many allocations as nodes in the tree during the process.
       */
      explicit operator basic_heap<RefCount>() const&;

      /**
       *  @brief
       *  dart::heap conversion operator.
       *
       *  @details
       *  If the current packet isn't finalized, function simply unwraps and returns
       *  a copy of the current instance (incrementing the reference counter).
       *  If the current packet IS finalized, function walks across the flattened
       *  object tree and lifts it into a dynamic heap representation,
       *  making at least as many allocations as nodes in the tree during the process.
       *
       *  @remarks
       *  Rvalue ref overload exists to forward through conversions.
       */
      explicit operator basic_heap<RefCount>() &&;

      /**
       *  @brief
       *  dart::buffer conversion operator.
       *
       *  @details
       *  If the current packet isn't finalized, function walks across the dynamic
       *  object tree and lowers it into a flattened buffer of bytes, making precisely
       *  one allocation during the process.
       *  If the current packet IS finalized, function simply unwraps and returns a copy
       *  of the current instance (incrementing the reference counter).
       */
      explicit operator basic_buffer<RefCount>() const&;

      /**
       *  @brief
       *  dart::buffer conversion operator.
       *
       *  @details
       *  If the current packet isn't finalized, function walks across the dynamic
       *  object tree and lowers it into a flattened buffer of bytes, making precisely
       *  one allocation during the process.
       *  If the current packet IS finalized, function simply unwraps and returns a copy
       *  of the current instance (incrementing the reference counter).
       *
       *  @remarks
       *  Rvalue ref overload exists to forward through conversions.
       */
      explicit operator basic_buffer<RefCount>() &&;

      /*----- Public API -----*/

      /*----- Factory Functions -----*/

      /**
       *  @brief
       *  Object factory function.
       *  Returns a new, non-finalized, object with the given sequence of key-value pairs.
       *
       *  @details
       *  Object keys can only be strings, and so arguments must be paired such that the
       *  first of each pair is convertible to a dart::packet string, and the second of
       *  each pair is convertible into a dart::packet of any type.
       */
      template <class... Args, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          sizeof...(Args) % 2 == 0
        >
      >
      static basic_packet make_object(Args&&... pairs);

      /**
       *  @brief
       *  Object factory function.
       *  Returns a new, non-finalized, object with the given sequence of key-value pairs.
       *
       *  @details
       *  Object keys can only be strings, and so arguments must be paired such that the
       *  first of each pair is convertible to a dart::heap string, and the second of
       *  each pair is convertible into a dart::heap of any type.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_packet make_object(gsl::span<basic_heap<RefCount> const> pairs);

      /**
       *  @brief
       *  Object factory function.
       *  Returns a new, non-finalized, object with the given sequence of key-value pairs.
       *
       *  @details
       *  Object keys can only be strings, and so arguments must be paired such that the
       *  first of each pair is convertible to a dart::heap string, and the second of
       *  each pair is convertible into a dart::heap of any type.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_packet make_object(gsl::span<basic_buffer<RefCount> const> pairs);

      /**
       *  @brief
       *  Object factory function.
       *  Returns a new, non-finalized, object with the given sequence of key-value pairs.
       *
       *  @details
       *  Object keys can only be strings, and so arguments must be paired such that the
       *  first of each pair is convertible to a dart::heap string, and the second of
       *  each pair is convertible into a dart::heap of any type.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_packet make_object(gsl::span<basic_packet<RefCount> const> pairs);

      /**
       *  @brief
       *  Array factory function.
       *  Returns a new, non-finalized, array with the given sequence of elements.
       */
      template <class... Args, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          (
            (sizeof...(Args) > 1)
            ||
            !meta::is_span<
              meta::first_type_t<std::decay_t<Args>...>
            >::value
          )
        >
      >
      static basic_packet make_array(Args&&... elems);

      /**
       *  @brief
       *  Array factory function.
       *  Returns a new, non-finalized, array with the given sequence of elements.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_packet make_array(gsl::span<basic_heap<RefCount> const> elems);

      /**
       *  @brief
       *  Array factory function.
       *  Returns a new, non-finalized, array with the given sequence of elements.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_packet make_array(gsl::span<basic_buffer<RefCount> const> elems);

      /**
       *  @brief
       *  Array factory function.
       *  Returns a new, non-finalized, array with the given sequence of elements.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_packet make_array(gsl::span<basic_packet const> elems);

      /**
       *  @brief
       *  String factory function.
       *  Returns a new, non-finalized, string with the given contents.
       *
       *  @details
       *  If the given string is <= dart::packet::sso_bytes long (15 at the time
       *  of this writing), string will be stored inline in the class instance.
       *  Otherwise, function will allocate the memory necessary to store the given
       *  string.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_packet make_string(shim::string_view val);

      /**
       *  @brief
       *  Integer factory function.
       *  Returns a new, non-finalized, integer with the given contents.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_packet make_integer(int64_t val) noexcept;

      /**
       *  @brief
       *  Decimal factory function.
       *  Returns a new, non-finalized, decimal with the given contents.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_packet make_decimal(double val) noexcept;

      /**
       *  @brief
       *  Boolean factory function.
       *  Returns a new, non-finalized, boolean with the given contents.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_packet make_boolean(bool val) noexcept;

      /**
       *  @brief
       *  Null factory function.
       *  Returns a null packet.
       */
      static basic_packet make_null() noexcept;

      /*----- Aggregate Builder Functions -----*/

      /**
       *  @brief
       *  Function adds, or replaces, a key-value pair mapping to/in a non-finalized packet.
       *
       *  @details
       *  Given key and value can be any types for which type conversions are defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *  The result of converting the key must yield a string.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto obj = dart::packet::make_object("hello", "goodbye");
       *  auto nested = obj["hello"].add_field("yes", "no").add_field("stop", "go, go, go");
       *  ```
       */
      template <class KeyType, class ValueType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<KeyType, basic_packet>::value
          &&
          convert::is_castable<ValueType, basic_packet>::value
        >
      >
      basic_packet& add_field(KeyType&& key, ValueType&& value) &;

      /**
       *  @brief
       *  Function adds, or replaces, a key-value pair mapping to/in a non-finalized object.
       *
       *  @details
       *  Given key and value can be any types for which type conversions are defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *  The result of converting the key must yield a string.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto obj = dart::packet::make_object("hello", "goodbye");
       *  auto nested = obj["hello"].add_field("yes", "no").add_field("stop", "go, go, go");
       *  ```
       */
      template <class KeyType, class ValueType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<KeyType, basic_packet>::value
          &&
          convert::is_castable<ValueType, basic_packet>::value
        >
      >
      DART_NODISCARD basic_packet&& add_field(KeyType&& key, ValueType&& value) &&;

      /**
       *  @brief
       *  Function removes a key-value pair mapping from a non-finalized object.
       *
       *  @details
       *  dart::packet::add_field does not throw if the key-value pair is already present,
       *  allowing for key replacement, and so for API uniformity, this function also does
       *  not throw if the key-value pair is NOT present.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto obj = get_object();
       *  auto nested = obj["nested"].remove_field("hello");
       *  ```
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet& remove_field(shim::string_view key) &;

      /**
       *  @brief
       *  Function removes a key-value pair mapping from a non-finalized object.
       *
       *  @details
       *  dart::packet::add_field does not throw if the key-value pair is already present,
       *  allowing for key replacement, and so for API uniformity, this function also does
       *  not throw if the key-value pair is NOT present.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto obj = get_object();
       *  auto nested = obj["nested"].remove_field("hello");
       *  ```
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      DART_NODISCARD basic_packet&& remove_field(shim::string_view key) &&;

      /**
       *  @brief
       *  Function removes a key-value pair mapping from a non-finalized object.
       *
       *  @details
       *  dart::packet::add_field does not throw if the key-value pair is already present,
       *  allowing for key replacement, and so for API uniformity, this function also does
       *  not throw if the key-value pair is NOT present.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto obj = get_object();
       *  auto nested = obj["nested"].remove_field("hello");
       *  ```
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          meta::is_dartlike<KeyType const&>::value
        >
      >
      basic_packet& remove_field(KeyType const& key) &;

      /**
       *  @brief
       *  Function removes a key-value pair mapping from a non-finalized object.
       *
       *  @details
       *  dart::packet::add_field does not throw if the key-value pair is already present,
       *  allowing for key replacement, and so for API uniformity, this function also does
       *  not throw if the key-value pair is NOT present.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto obj = get_object();
       *  auto nested = obj["nested"].remove_field("hello");
       *  ```
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          meta::is_dartlike<KeyType const&>::value
        >
      >
      DART_NODISCARD basic_packet&& remove_field(KeyType const& key) &&;

      /**
       *  @brief
       *  Function inserts the given element at the front of a non-finalized array.
       *
       *  @details
       *  Given element can be any type for which a type conversion is defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto arr = get_array();
       *  auto nested = arr[0].push_front("testing 1, 2, 3").push_front("a, b, c");
       *  ```
       */
      template <class ValueType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<ValueType, basic_packet>::value
        >
      >
      basic_packet& push_front(ValueType&& value) &;

      /**
       *  @brief
       *  Function inserts the given element at the front of a non-finalized array.
       *
       *  @details
       *  Given element can be any type for which a type conversion is defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto arr = get_array();
       *  auto nested = arr[0].push_front("testing 1, 2, 3").push_front("a, b, c");
       *  ```
       */
      template <class ValueType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<ValueType, basic_packet>::value
        >
      >
      DART_NODISCARD basic_packet&& push_front(ValueType&& value) &&;

      /**
       *  @brief
       *  Function removes the first element of a non-finalized array.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto arr = get_array();
       *  auto nested = arr[0].pop_front().pop_front();
       *  ```
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet& pop_front() &;

      /**
       *  @brief
       *  Function removes the first element of a non-finalized array.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto arr = get_array();
       *  auto nested = arr[0].pop_front().pop_front();
       *  ```
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      DART_NODISCARD basic_packet&& pop_front() &&;

      /**
       *  @brief
       *  Function inserts the given element at the end of a non-finalized array.
       *
       *  @details
       *  Given element can be any type for which a type conversion is defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto arr = get_array();
       *  auto nested = arr[0].push_front("testing 1, 2, 3").push_front("a, b, c");
       *  ```
       */
      template <class ValueType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<ValueType, basic_packet>::value
        >
      >
      basic_packet& push_back(ValueType&& value) &;

      /**
       *  @brief
       *  Function inserts the given element at the end of a non-finalized array.
       *
       *  @details
       *  Given element can be any type for which a type conversion is defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto arr = get_array();
       *  auto nested = arr[0].push_front("testing 1, 2, 3").push_front("a, b, c");
       *  ```
       */
      template <class ValueType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<ValueType, basic_packet>::value
        >
      >
      DART_NODISCARD basic_packet&& push_back(ValueType&& value) &&;

      /**
       *  @brief
       *  Function removes the last element of a non-finalized array.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto arr = get_array();
       *  auto nested = arr[0].pop_back().pop_back();
       *  ```
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet& pop_back() &;

      /**
       *  @brief
       *  Function removes the last element of a non-finalized array.
       *
       *  @remarks
       *  Function returns the current instance, and is overloaded for both lvalues and rvalues,
       *  allowing for efficient and expressive chaining:
       *  ```
       *  auto arr = get_array();
       *  auto nested = arr[0].pop_back().pop_back();
       *  ```
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      DART_NODISCARD basic_packet&& pop_back() &&;

      /**
       *  @brief
       *  Function adds, replaces, or inserts, a (new) (key|index)-value pair to/in a
       *  non-finalized object/array.
       *
       *  @details
       *  Given key and value can be any types for which type conversions are defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *  If this is an object, the result of converting the key must yield a string.
       *  If this is an array, the result of converting the key must yield an integer.
       *
       *  @remarks
       *  Function is highly overloaded, and serves as a single point of entry for all
       *  packet mutation related to adding new values to aggregates.
       *  Function is used as a backend for dart::packet::add_field, dart::packet::push_back,
       *  dart::packet::push_front, etc
       */
      template <class KeyType, class ValueType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<KeyType, basic_packet>::value
          &&
          convert::is_castable<ValueType, basic_packet>::value
        >
      >
      auto insert(KeyType&& key, ValueType&& value) -> iterator;

      /**
       *  @brief
       *  Function adds, replaces, or inserts, a (new) (key|index)-value pair to/in a
       *  non-finalized object/array.
       *
       *  @details
       *  Given value can be any type for which a type conversion is defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *  Function takes an iterator as the insertion point to establish API conformity
       *  with STL container types.
       *
       *  @remarks
       *  Iterator based insertion is actually rather complex for copy-on-write types,
       *  which means that unfortunately iterator based insertion is not more efficient
       *  than key based insertion as one might expect.
       *  Function exists to allow for easier compatibility with STL container types in
       *  generic contexts.
       */
      template <class ValueType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<ValueType, basic_packet>::value
        >
      >
      auto insert(iterator pos, ValueType&& value) -> iterator;

      /**
       *  @brief
       *  Assuming this is a non-finalized aggregate, function replaces an existing
       *  (index|key)-value pair.
       *
       *  @details
       *  Given key and value can be any types for which type conversions are defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *  The result of converting the key must yield a string.
       */
      template <class KeyType, class ValueType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<KeyType, basic_packet>::value
          &&
          convert::is_castable<ValueType, basic_packet>::value
        >
      >
      auto set(KeyType&& key, ValueType&& value) -> iterator;

      /**
       *  @brief
       *  Assuming this is a non-finalized aggregate, function replaces an existing
       *  (index|key)-value pair.
       *
       *  @details
       *  Given key and value can be any types for which type conversions are defined
       *  (built-in types, STL containers of built-in types, user types for which an
       *  explicit conversion has been defined, and STL containers of such user types).
       *  The result of converting the key must yield a string.
       */
      template <class ValueType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<ValueType, basic_packet>::value
        >
      >
      auto set(iterator pos, ValueType&& value) -> iterator;

      /**
       *  @brief
       *  Function removes a (key|index)-value pair from a non-finalized object/array.
       *
       *  @details
       *  If the (key-index)-value pair is not present in the current aggregate, function
       *  returns the end iterator, otherwise it returns an iterator to one past the element
       *  removed.
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          meta::is_dartlike<KeyType const&>::value
        >
      >
      auto erase(KeyType const& key) -> iterator;

      /**
       *  @brief
       *  Function removes a (key|index)-value pair from a non-finalized object/array.
       *
       *  @details
       *  If the (key-index)-value pair is not present in the current aggregate, function
       *  returns the end iterator, otherwise it returns an iterator to one past the element
       *  removed.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      auto erase(iterator pos) -> iterator;

      /**
       *  @brief
       *  Function removes a (key|index)-value pair from a non-finalized object/array.
       *
       *  @details
       *  If the (key-index)-value pair is not present in the current aggregate, function
       *  returns the end iterator, otherwise it returns an iterator to one past the element
       *  removed.
       */
      template <class String, bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      auto erase(basic_string<String> const& key) -> iterator;

      /**
       *  @brief
       *  Function removes a (key|index)-value pair from a non-finalized object/array.
       *
       *  @details
       *  If the (key-index)-value pair is not present in the current aggregate, function
       *  returns the end iterator, otherwise it returns an iterator to one past the element
       *  removed.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      auto erase(shim::string_view key) -> iterator;

      /**
       *  @brief
       *  Function removes a (key|index)-value pair from a non-finalized object/array.
       *
       *  @details
       *  If the (key-index)-value pair is not present in the current aggregate, function
       *  returns the end iterator, otherwise it returns an iterator to one past the element
       *  removed.
       */
      template <class Number, bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      auto erase(basic_number<Number> const& idx) -> iterator;

      /**
       *  @brief
       *  Function removes a (key|index)-value pair from a non-finalized object/array.
       *
       *  @details
       *  If the (key-index)-value pair is not present in the current aggregate, function
       *  returns the end iterator, otherwise it returns an iterator to one past the element
       *  removed.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      auto erase(size_type pos) -> iterator;

      /**
       *  @brief
       *  Function clears an aggregate of all values.
       *
       *  @details
       *  If this is a non-finalized object/array, function will remove all subvalues.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      void clear();

      /**
       *  @brief
       *  Function "injects" a set of key-value pairs into an object without
       *  modifying it by returning a new object with the union of the keyset
       *  of the original object and the provided key-value pairs.
       *
       *  @details
       *  Function provides a uniform interface for _efficient_ injection of
       *  keys across the entire Dart API (also works for dart::buffer).
       */
      template <class... Args, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          sizeof...(Args) % 2 == 0
          &&
          meta::conjunction<
            convert::is_castable<Args, basic_packet>...
          >::value
        >
      >
      basic_packet inject(Args&&... pairs) const;

      /**
       *  @brief
       *  Function "injects" a set of key-value pairs into an object without
       *  modifying it by returning a new object with the union of the keyset
       *  of the original object and the provided key-value pairs.
       *
       *  @details
       *  Function provides a uniform interface for _efficient_ injection of
       *  keys across the entire Dart API (also works for dart::buffer).
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet inject(gsl::span<basic_heap<RefCount> const> pairs) const;

      /**
       *  @brief
       *  Function "injects" a set of key-value pairs into an object without
       *  modifying it by returning a new object with the union of the keyset
       *  of the original object and the provided key-value pairs.
       *
       *  @details
       *  Function provides a uniform interface for _efficient_ injection of
       *  keys across the entire Dart API (also works for dart::buffer).
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet inject(gsl::span<basic_buffer<RefCount> const> pairs) const;

      /**
       *  @brief
       *  Function "injects" a set of key-value pairs into an object without
       *  modifying it by returning a new object with the union of the keyset
       *  of the original object and the provided key-value pairs.
       *
       *  @details
       *  Function provides a uniform interface for _efficient_ injection of
       *  keys across the entire Dart API (also works for dart::buffer).
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet inject(gsl::span<basic_packet const> pairs) const;

      /**
       *  @brief
       *  Function "projects" a set of key-value pairs in the mathematical
       *  sense that it creates a new packet containing the intersection of
       *  the supplied keys, and those present in the original packet.
       *
       *  @details
       *  Function provides a uniform interface for _efficient_ projection of
       *  keys across the entire Dart API (also works for dart::buffer).
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet project(std::initializer_list<shim::string_view> keys) const;

      /**
       *  @brief
       *  Function "projects" a set of key-value pairs in the mathematical
       *  sense that it creates a new packet containing the intersection of
       *  the supplied keys, and those present in the original packet.
       *
       *  @details
       *  Function provides a uniform interface for _efficient_ projection of
       *  keys across the entire Dart API (also works for dart::buffer).
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet project(gsl::span<std::string const> keys) const;

      /**
       *  @brief
       *  Function "projects" a set of key-value pairs in the mathematical
       *  sense that it creates a new packet containing the intersection of
       *  the supplied keys, and those present in the original packet.
       *
       *  @details
       *  Function provides a uniform interface for _efficient_ projection of
       *  keys across the entire Dart API (also works for dart::buffer).
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet project(gsl::span<shim::string_view const> keys) const;

      /*----- State Manipulation Functions -----*/

      /**
       *  @brief
       *  Function ensures enough underlying storage for at least
       *  as many elements as specified.
       *
       *  @details
       *  If used judiciously, can increase insertion performance.
       *  If used poorly, will definitely decrease insertion performance.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      void reserve(size_type count);

      /**
       *  @brief
       *  Function sets the current capacity of the underlying array storage.
       *
       *  @details
       *  If the underlying array grows, values are initialized by casting
       *  the supplied default argument according to the usual conversion
       *  API rules.
       */
      template <class T = std::nullptr_t, class EnableIf =
        std::enable_if_t<
          convert::is_castable<T, basic_packet>::value
        >
      >
      void resize(size_type count, T const& def = T {});

      /**
       *  @brief
       *  Function transitions the current packet from being finalized to non-finalized in place.
       *
       *  @details
       *  dart::packet has two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart::packet maintains a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart::packet maintains a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::packet::finalize, the reverse can be accomplished using dart::packet::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet& definalize() &;

      /**
       *  @brief
       *  Function transitions the current packet from being finalized to non-finalized in place.
       *
       *  @details
       *  dart::packet has two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart::packet maintains a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart::packet maintains a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::packet::finalize, the reverse can be accomplished using dart::packet::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet&& definalize() &&;

      /**
       *  @brief
       *  Function transitions the current packet from being finalized to non-finalized in place.
       *
       *  @details
       *  dart::packet has two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart::packet maintains a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart::packet maintains a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::packet::finalize, the reverse can be accomplished using dart::packet::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet& lift() &;

      /**
       *  @brief
       *  Function transitions the current packet from being finalized to non-finalized in place.
       *
       *  @details
       *  dart::packet has two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart::packet maintains a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart::packet maintains a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::packet::finalize, the reverse can be accomplished using dart::packet::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet&& lift() &&;

      /**
       *  @brief
       *  Function transitions the current packet from being non-finalized to finalized in place.
       *
       *  @details
       *  dart::packet has two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart::packet maintains a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart::packet maintains a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::packet::finalize, the reverse can be accomplished using dart::packet::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet& finalize() &;

      /**
       *  @brief
       *  Function transitions the current packet from being non-finalized to finalized in place.
       *
       *  @details
       *  dart::packet has two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart::packet maintains a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart::packet maintains a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::packet::finalize, the reverse can be accomplished using dart::packet::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet&& finalize() &&;

      /**
       *  @brief
       *  Function transitions the current packet from being non-finalized to finalized in place.
       *
       *  @details
       *  dart::packet has two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart::packet maintains a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart::packet maintains a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::packet::finalize, the reverse can be accomplished using dart::packet::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet& lower() &;

      /**
       *  @brief
       *  Function transitions the current packet from being non-finalized to finalized in place.
       *
       *  @details
       *  dart::packet has two logically distinct modes: finalized and dynamic.
       *
       *  While in dynamic mode, dart::packet maintains a heap-based object tree which can
       *  be used to traverse, or mutate, arbitrary data representations in a reasonably
       *  efficient manner.
       *
       *  While in finalized mode, dart::packet maintains a contiguously allocated, flattened
       *  object tree designed specifically for efficient/cache-friendly immutable interaction,
       *  and readiness to be distributed via network/shared-memory/filesystem/etc.
       *
       *  Switching from dynamic to finalized mode is accomplished via a call to
       *  dart::packet::finalize, the reverse can be accomplished using dart::packet::definalize.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet&& lower() &&;

      /**
       *  @brief
       *  Function re-seats the object tree referred to by the given packet on top of the
       *  specified reference counter.
       *
       *  @details
       *  Function name is intentionally long and hard to spell to denote the fact that you
       *  shouldn't be calling it very frequently.
       *  Switching reference counters requires a deep copy of the entire tree, which, in the
       *  case of a finalized packet means a single allocation and a memcpy, but in the case
       *  of a non-finalized packet, it means as many allocations and copies as there are
       *  nodes in the tree.
       *  Very expensive, should only be called when truly necessary for applications that
       *  need to use multiple reference counter types simultaneously.
       */
      template <template <class> class NewCount>
      static basic_packet<NewCount> transmogrify(basic_packet<RefCount> const& packet);

      /*----- JSON Helper Functions -----*/

#ifdef DART_USE_SAJSON
      /**
       *  @brief
       *  Function constructs an optionally finalized packet to represent the given JSON string.
       *
       *  @details
       *  Parsing is based on RapidJSON, and so exposes the same parsing customization points as
       *  RapidJSON.
       *  If your JSON has embedded comments in it, NaN or +/-Infinity values, or trailing commas,
       *  you can parse in the following ways:
       *  ```
       *  auto json = input.read();
       *  auto comments = dart::heap::from_json<dart::parse_comments>(json);
       *  auto nan_inf = dart::heap::from_json<dart::parse_nan>(json);
       *  auto commas = dart::heap::from_json<dart::parse_trailing_commas>(json);
       *  auto all_of_it = dart::heap::from_json<dart::parse_permissive>(json);
       *  ```
       */
      template <unsigned parse_stack_size = default_parse_stack_size,
               bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_packet from_json(shim::string_view json, bool finalized = true);
#elif DART_HAS_RAPIDJSON
      /**
       *  @brief
       *  Function constructs an optionally finalized packet to represent the given JSON string.
       *
       *  @details
       *  Parsing is based on RapidJSON, and so exposes the same parsing customization points as
       *  RapidJSON.
       *  If your JSON has embedded comments in it, NaN or +/-Infinity values, or trailing commas,
       *  you can parse in the following ways:
       *  ```
       *  auto json = input.read();
       *  auto comments = dart::packet::from_json<dart::parse_comments>(json);
       *  auto nan_inf = dart::packet::from_json<dart::parse_nan>(json);
       *  auto commas = dart::packet::from_json<dart::parse_trailing_commas>(json);
       *  auto all_of_it = dart::packet::from_json<dart::parse_permissive>(json);
       *  ```
       */
      template <unsigned flags = parse_default,
               bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_packet from_json(shim::string_view json, bool finalize = false);
#endif

#if DART_HAS_RAPIDJSON
      /**
       *  @brief
       *  Function serializes any packet into a JSON string.
       *
       *  @details
       *  Serialization is based on RapidJSON, and so exposes the same customization points as
       *  RapidJSON.
       *  If your packet has NaN or +/-Infinity values, you can serialize it in the following way:
       *  ```
       *  auto obj = some_object();
       *  auto json = obj.to_json<dart::write_nan>();
       *  do_something(std::move(json));
       *  ```
       */
      template <unsigned flags = write_default>
      std::string to_json() const;
#endif

      /*----- YAML Helper Functions -----*/

#if DART_HAS_YAML
      /**
       *  @brief
       *  Function constructs an optionally finalized packet to represent the given YAML string.
       *
       *  @remarks
       *  At the time of this writing, parsing logic does not support YAML anchors, this will
       *  be added soon.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      static basic_packet from_yaml(shim::string_view yaml, bool finalized = true);
#endif

      /*----- Value Accessor Functions -----*/

      /**
       *  @brief
       *  Array access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within bounds, will
       *  return the requested array index as a new packet.
       */
      template <class Number>
      basic_packet get(basic_number<Number> const& idx) const&;

      /**
       *  @brief
       *  Array access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within bounds, will
       *  return the requested array index as a new packet.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto arr = some_array();
       *  auto deep = arr[0][1][2];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <class Number, bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet&& get(basic_number<Number> const& idx) &&;

      /**
       *  @brief
       *  Array access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within bounds, will
       *  return the requested array index as a new packet.
       */
      basic_packet get(size_type index) const&;

      /**
       *  @brief
       *  Array access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within bounds, will
       *  return the requested array index as a new packet.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto arr = some_array();
       *  auto deep = arr[0][1][2];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet&& get(size_type index) &&;

      /**
       *  @brief
       *  Optional array access method, similar to std::optional::value_or.
       *
       *  @details
       *  If this is an array, and contains the specified index, function returns the
       *  value associated with that index.
       *  If this is not an array, or the specified index is out of bounds, function
       *  returns the optional value cast to a dart::packet according to the usual
       *  conversion API rules.
       *
       *  @remarks
       *  Function is "logically" noexcept, but cannot be marked so as allocations necessary
       *  for converting the optional value might throw, and user defined conversions aren't
       *  required to be noexcept.
       */
      template <class Number, class T, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<T, basic_packet>::value
        >
      >
      basic_packet get_or(basic_number<Number> const& idx, T&& opt) const;

      /**
       *  @brief
       *  Optional array access method, similar to std::optional::value_or.
       *
       *  @details
       *  If this is an array, and contains the specified index, function returns the
       *  value associated with that index.
       *  If this is not an array, or the specified index is out of bounds, function
       *  returns the optional value cast to a dart::packet according to the usual
       *  conversion API rules.
       *
       *  @remarks
       *  Function is "logically" noexcept, but cannot be marked so as allocations necessary
       *  for converting the optional value might throw, and user defined conversions aren't
       *  required to be noexcept.
       */
      template <class T, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<T, basic_packet>::value
        >
      >
      basic_packet get_or(size_type index, T&& opt) const;

      /**
       *  @brief
       *  Object access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an object, function will return the requested key-value pair as a
       *  new packet.
       */
      template <class String>
      basic_packet get(basic_string<String> const& key) const&;

      /**
       *  @brief
       *  Object access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an object, function will return the requested key-value pair as a
       *  new packet.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto obj = some_object();
       *  auto deep = obj["first"]["second"]["third"];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <class String, bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet&& get(basic_string<String> const& key) &&;

      /**
       *  @brief
       *  Object access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an object, function will return the requested key-value pair as a
       *  new packet.
       */
      basic_packet get(shim::string_view key) const&;

      /**
       *  @brief
       *  Object access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an object, function will return the requested key-value pair as a
       *  new packet.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto obj = some_object();
       *  auto deep = obj["first"]["second"]["third"];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet&& get(shim::string_view key) &&;

      /**
       *  @brief
       *  Optional object access method, similar to std::optional::value_or.
       *
       *  @details
       *  If this is an object, and contains the key-value pair, function returns the
       *  value associated with the specified key.
       *  If this is not an object, or the specified key-value pair is not present,
       *  function returns the optional value cast to a dart::packet according to
       *  the usual conversion API rules.
       *
       *  @remarks
       *  Function is "logically" noexcept, but cannot be marked so as allocations necessary
       *  for converting the optional value might throw, and user defined conversions aren't
       *  required to be noexcept.
       */
      template <class String, class T, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<T, basic_packet>::value
        >
      >
      basic_packet get_or(basic_string<String> const& key, T&& opt) const;

      /**
       *  @brief
       *  Optional object access method, similar to std::optional::value_or.
       *
       *  @details
       *  If this is an object, and contains the specified key-value pair,
       *  function returns the value associated with the specified key.
       *  If this is not an object, or the specified key-value pair is not present,
       *  function returns the optional value cast to a dart::packet according to
       *  the usual conversion API rules.
       *
       *  @remarks
       *  Function is "logically" noexcept, but cannot be marked so as allocations necessary
       *  for converting the optional value might throw, and user defined conversions aren't
       *  required to be noexcept.
       */
      template <class T, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<T, basic_packet>::value
        >
      >
      basic_packet get_or(shim::string_view key, T&& opt) const;

      /**
       *  @brief
       *  Combined array/object access method, precisely equivalent to the corresponding
       *  subscript operator.
       *
       *  @details
       *  Assuming this is an object/array, returns the value associated with the
       *  given key/index, or a null packet/throws an exception if no such mapping exists.
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          meta::is_dartlike<KeyType const&>::value
        >
      >
      basic_packet get(KeyType const& identifier) const&;

      /**
       *  @brief
       *  Combined array/object access method, precisely equivalent to the corresponding
       *  subscript operator.
       *
       *  @details
       *  Assuming this is an object/array, returns the value associated with the
       *  given key/index, or a null packet/throws an exception if no such mapping exists.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto obj = some_object();
       *  auto deep = obj["first"]["second"]["third"];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          meta::is_dartlike<KeyType const&>::value
        >
      >
      basic_packet&& get(KeyType const& identifier) &&;

      /**
       *  @brief
       *  Optional combined object/array access method, similar to std::optional::value_or.
       *
       *  @details
       *  If this is an object, and contains the specified key-value pair, function returns
       *  the value associated with the specified key.
       *  If this is an array, and contains the specified index, function returns the value
       *  associated with that index.
       *  If this is no an array/object, function returns the optional value cast to a
       *  dart::packet according to the usual conversion API rules.
       *  If this is an array/object, but the passed identifier is not a number/string
       *  respectively, function returns the optional value cast to a dart::packet according
       *  to the usual conversion API rules.
       *
       *  @remarks
       *  Function is "logically" noexcept, but cannot be marked so as allocations necessary
       *  for converting the optional value might throw, and anyways, user defined conversions
       *  aren't required to be noexcept.
       */
      template <class KeyType, class T, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          meta::is_dartlike<KeyType const&>::value
          &&
          convert::is_castable<T, basic_packet>::value
        >
      >
      basic_packet get_or(KeyType const& identifier, T&& opt) const;

      /**
       *  @brief
       *  Function returns the value associated with the separator delimited object path.
       */
      basic_packet get_nested(shim::string_view path, char separator = '.') const;

      /**
       *  @brief
       *  Array access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within bounds, will
       *  return the requested array index as a new packet.
       */
      template <class Number>
      basic_packet at(basic_number<Number> const& idx) const&;

      /**
       *  @brief
       *  Array access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within bounds, will
       *  return the requested array index as a new packet.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto arr = some_array();
       *  auto deep = arr[0][1][2];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <class Number, bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet&& at(basic_number<Number> const& idx) &&;

      /**
       *  @brief
       *  Array access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within bounds, will
       *  return the requested array index as a new packet.
       */
      basic_packet at(size_type index) const&;

      /**
       *  @brief
       *  Array access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an array, and the requested index is within bounds, will
       *  return the requested array index as a new packet.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto arr = some_array();
       *  auto deep = arr[0][1][2];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet&& at(size_type index) &&;

      /**
       *  @brief
       *  Object access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an object, function will return the requested key-value pair as a
       *  new packet.
       */
      template <class String>
      basic_packet at(basic_string<String> const& key) const&;

      /**
       *  @brief
       *  Object access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an object, function will return the requested key-value pair as a
       *  new packet.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto obj = some_object();
       *  auto deep = obj["first"]["second"]["third"];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <class String, bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet&& at(basic_string<String> const& key) &&;

      /**
       *  @brief
       *  Object access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an object, function will return the requested key-value pair as a
       *  new packet.
       */
      basic_packet at(shim::string_view key) const&;

      /**
       *  @brief
       *  Object access method, precisely equivalent to the corresponding subscript operator.
       *
       *  @details
       *  Assuming this is an object, function will return the requested key-value pair as a
       *  new packet.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto obj = some_object();
       *  auto deep = obj["first"]["second"]["third"];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet&& at(shim::string_view key) &&;

      /**
       *  @brief
       *  Combined array/object access method, precisely equivalent to the corresponding
       *  subscript operator.
       *
       *  @details
       *  Assuming this is an object/array, returns the value associated with the
       *  given key/index, or a null packet/throws an exception if no such mapping exists.
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          meta::is_dartlike<KeyType const&>::value
        >
      >
      basic_packet at(KeyType const& identifier) const&;

      /**
       *  @brief
       *  Combined array/object access method, precisely equivalent to the corresponding
       *  subscript operator.
       *
       *  @details
       *  Assuming this is an object/array, returns the value associated with the
       *  given key/index, or a null packet/throws an exception if no such mapping exists.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto obj = some_object();
       *  auto deep = obj["first"]["second"]["third"];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          meta::is_dartlike<KeyType const&>::value
        >
      >
      basic_packet&& at(KeyType const& identifier) &&;

      /**
       *  @brief
       *  Function returns the first element in an array.
       *
       *  @details
       *  Return the first element or throws if the array is empty.
       */
      basic_packet at_front() const&;

      /**
       *  @brief
       *  Function returns the first element in an array.
       *
       *  @details
       *  Return the first element or throws if the array is empty.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto obj = some_object();
       *  auto deep = obj["first"]["second"]["third"];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet&& at_front() &&;

      /**
       *  @brief
       *  Function returns the last element in an array.
       *
       *  @details
       *  Return the last element or throws if the array is empty.
       */
      basic_packet at_back() const&;

      /**
       *  @brief
       *  Function returns the last element in an array.
       *
       *  @details
       *  Return the last element or throws if the array is empty.
       *
       *  @remarks
       *  dart::buffer is able to profitably use an rvalue overloaded subscript
       *  operator to skip unnecessary refcount increments/decrements in expressions
       *  like the following:
       *  ```
       *  auto obj = some_object();
       *  auto deep = obj["first"]["second"]["third"];
       *  ```
       *  and so all classes that can wrap dart::buffer also implement this overload.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet&& at_back() &&;

      /**
       *  @brief
       *  Function returns the first element in an array.
       *
       *  @details
       *  If this is an array with at least one thing in it,
       *  returns the first element.
       *  Throws otherwise.
       */
      basic_packet front() const&;

      /**
       *  @brief
       *  Function returns the first element in an array.
       *
       *  @details
       *  If this is an array with at least one thing in it,
       *  returns the first element.
       *  Throws otherwise.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet&& front() &&;

      /**
       *  @brief
       *  Optionally returns the first element in an array.
       *
       *  @details
       *  If this is an array with at least one thing in it,
       *  returns the first element.
       *  Otherwise, returns the optional value cast to a dart::packet
       *  according to the usual conversion API rules.
       */
      template <class T, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<T, basic_packet>::value
        >
      >
      basic_packet front_or(T&& opt) const;

      /**
       *  @brief
       *  Function returns the last element in an array.
       *
       *  @details
       *  If this is an array with at least oen thing in it,
       *  returns the last element.
       *  Throws otherwise.
       */
      basic_packet back() const&;

      /**
       *  @brief
       *  Function returns the last element in an array.
       *
       *  @details
       *  If this is an array with at least oen thing in it,
       *  returns the last element.
       *  Throws otherwise.
       */
      template <bool enabled = refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      basic_packet&& back() &&;

      /**
       *  @brief
       *  Optionally returns the last element in an array.
       *
       *  @details
       *  If this is an array with at least one thing in it,
       *  returns the last element.
       *  Otherwise, returns the optional value cast to a dart::packet
       *  according to the usual conversion API rules.
       */
      template <class T, class EnableIf =
        std::enable_if_t<
          refcount::is_owner<RefCount>::value
          &&
          convert::is_castable<T, basic_packet>::value
        >
      >
      basic_packet back_or(T&& opt) const;

      /**
       *  @brief
       *  Finds the requested key-value mapping in an object.
       *
       *  @details
       *  If this is in an object, function will return an
       *  iterator to the requested VALUE mapping, or the
       *  end iterator.
       *  If this is an object, throws an exception.
       */
      template <class String>
      auto find(basic_string<String> const& key) const -> iterator;

      /**
       *  @brief
       *  Finds the requested key-value mapping in an object.
       *
       *  @details
       *  If this is in an object, function will return an
       *  iterator to the requested VALUE mapping, or the
       *  end iterator.
       *  If this is an object, throws an exception.
       */
      auto find(shim::string_view key) const -> iterator;

      /**
       *  @brief
       *  Finds the requested key-value mapping in an object.
       *
       *  @details
       *  If this is in an object, function will return an
       *  iterator to the requested KEY mapping, or the
       *  end iterator.
       *  If this is an object, throws an exception.
       */
      template <class String>
      auto find_key(basic_string<String> const& key) const -> iterator;

      /**
       *  @brief
       *  Finds the requested key-value mapping in an object.
       *
       *  @details
       *  If this is in an object, function will return an
       *  iterator to the requested VALUE mapping, or the
       *  end iterator.
       *  If this is an object, throws an exception.
       */
      auto find_key(shim::string_view key) const -> iterator;

      /**
       *  @brief
       *  Returns all keys associated with the current packet.
       *
       *  @details
       *  If this is an object, returns a vector of all its keys.
       *  Throws otherwise.
       */
      std::vector<basic_packet> keys() const;

      /**
       *  @brief
       *  Returns all subvalues of the current aggregate.
       *
       *  @details
       *  If this is an object or array, returns all of the values contained
       *  within
       *  Throws otherwise.
       */
      std::vector<basic_packet> values() const;

      /**
       *  @brief
       *  Returns whether a particular key-value pair exists within an object.
       *
       *  @details
       *  If this is an object, function returns whether the given key is present
       *  Throws otherwise.
       */
      template <class String>
      bool has_key(basic_string<String> const& key) const;

      /**
       *  @brief
       *  Returns whether a particular key-value pair exists within an object.
       *
       *  @details
       *  If this is an object, function returns whether the given key is present
       *  Throws otherwise.
       */
      bool has_key(shim::string_view key) const;

      /**
       *  @brief
       *  Returns whether a particular key-value pair exists within an object.
       *
       *  @details
       *  If this is an object, function returns whether the given key is present
       *  Throws otherwise.
       */
      template <class KeyType, class EnableIf =
        std::enable_if_t<
          meta::is_dartlike<KeyType const&>::value
        >
      >
      bool has_key(KeyType const& key) const;

      /**
       *  @brief
       *  Function returns a null-terminated c-string representing the current packet.
       *
       *  @details
       *  If the current packet is a string, returns a c-string, otherwise throws.
       *
       *  @remarks
       *  Lifetime of the returned pointer is tied to the current instance.
       */
      char const* str() const;

      /**
       *  @brief
       *  Function optionally returns a c-string representing the current packet.
       *
       *  @details
       *  If the current packet is a string, returns a null-terminated c-string,
       *  otherwise returns the provided character pointer.
       *
       *  @remarks
       *  If this is a string, the lifetime of the returned pointer
       *  is equal to that of the current instance
       *  If this is not a string, the lifetime of the returned pointer
       *  is equivalent to the lifetime of the optional parameter.
       */
      char const* str_or(char const* opt) const noexcept;

      /**
       *  @brief
       *  Function returns a string view representing the current packet.
       *
       *  @details
       *  If the current packet is a string, returns a string view, otherwise throws.
       *
       *  @remarks
       *  Lifetime of the string view is tied to the current instance.
       */
      shim::string_view strv() const;

      /**
       *  @brief
       *  Function optionally returns a string view representing the current packet.
       *
       *  @details
       *  If the current packet is a string, returns a string view,
       *  otherwise returns the provided string view.
       *
       *  @remarks
       *  If this is a string, the lifetime of the returned string view is
       *  equal to that of the current instance.
       *  If this is not a string, the lifetime of the returned string view
       *  is equivalent to the lifetime of the optional parameter.
       */
      shim::string_view strv_or(shim::string_view opt) const noexcept;

      /**
       *  @brief
       *  Function returns an integer representing the current packet.
       *
       *  @details
       *  If the current packet is an integer, returns a it, otherwise throws.
       */
      int64_t integer() const;

      /**
       *  @brief
       *  Function optionally returns an integer representing the current packet.
       *
       *  @details
       *  If the current packet is an integer, returns a it,
       *  otherwise returns the provided integer.
       */
      int64_t integer_or(int64_t opt) const noexcept;

      /**
       *  @brief
       *  Function returns a decimal representing the current packet.
       *
       *  @details
       *  If the current packet is a decimal, returns a it, otherwise throws.
       */
      double decimal() const;

      /**
       *  @brief
       *  Function optionally returns a decimal representing the current packet.
       *
       *  @details
       *  If the current packet is a decimal, returns a it,
       *  otherwise returns the provided decimal.
       */
      double decimal_or(double opt) const noexcept;

      /**
       *  @brief
       *  Function returns a decimal representing the current packet.
       *
       *  @details
       *  If the current packet is an integer or decimal, returns a it as a decimal,
       *  otherwise throws.
       */
      double numeric() const;

      /**
       *  @brief
       *  Function optionally returns a decimal representing the current packet.
       *
       *  @details
       *  If the current packet is an integer or decimal, returns a it as a decimal,
       *  otherwise returns the provided decimal.
       */
      double numeric_or(double opt) const noexcept;

      /**
       *  @brief
       *  Function returns a boolean representing the current packet.
       *
       *  @details
       *  If the current packet is a boolean, returns a it, otherwise throws.
       */
      bool boolean() const;

      /**
       *  @brief
       *  Function optionally returns a boolean representing the current packet.
       *
       *  @details
       *  If the current packet is a boolean, returns a it,
       *  otherwise returns the provided boolean.
       */
      bool boolean_or(bool opt) const noexcept;

      /**
       *  @brief
       *  Function returns the size of the current packet, according to the semantics
       *  of its type.
       *
       *  @details
       *  If this is an object, returns the number of key-value pairs within the object.
       *  If this is an array, returns the number of elements within the array.
       *  If this is a string, returns the length of the string NOT INCLUDING the null
       *  terminator.
       */
      auto size() const -> size_type;

      /**
       *  @brief
       *  Function returns the current capacity of the underlying storage array.
       *
       *  @details
       *  If this is finalized, capacity is fixed.
       */
      auto capacity() const -> size_type;

      /**
       *  @brief
       *  Function returns whether the current packet is empty, according to the semantics
       *  of its type.
       *
       *  @details
       *  In actuality, returns whether size() == 0;
       */
      bool empty() const;

      /*----- Introspection Functions -----*/

      /**
       *  @brief
       *  Returns whether the current packet is an object or not.
       */
      bool is_object() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an array or not.
       */
      bool is_array() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an object or array or not.
       */
      bool is_aggregate() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a string or not.
       */
      bool is_str() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an integer or not.
       */
      bool is_integer() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a decimal or not.
       */
      bool is_decimal() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is an integer or decimal or not.
       */
      bool is_numeric() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a boolean or not.
       */
      bool is_boolean() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is null or not.
       */
      bool is_null() const noexcept;

      /**
       *  @brief
       *  Returns whether the current packet is a string, integer, decimal,
       *  boolean, or null.
       */
      bool is_primitive() const noexcept;

      /**
       *  @brief
       *  Returns the type of the current packet as an enum value.
       */
      auto get_type() const noexcept -> type;

      /**
       *  @brief
       *  Returns whether the current packet is finalized.
       */
      bool is_finalized() const noexcept;

      /**
       *  @brief
       *  Returns the current reference count of the underlying reference counter.
       */
      auto refcount() const noexcept -> size_type;

      /*----- Network Buffer Accessors -----*/

      /**
       *  @brief
       *  Function returns the network-level buffer for a finalized packet.
       *
       *  @details
       *  gsl::span is a non-owning view type, like std::string_view, and so the
       *  lifetype of the returned view is equal to the lifetime of the current
       *  packet.
       */
      gsl::span<gsl::byte const> get_bytes() const;

      /**
       *  @brief
       *  Function allows the network buffer of the current packet to be exported
       *  directly, without copies.
       *
       *  @details
       *  Function has a fairly awkward API, requiring the user to pass an existing
       *  instance (potentially default-constructed) of the reference counter type,
       *  which it then destroys and reconstructs such that it is a copy of the
       *  current reference counter.
       *  Function is written this way as the reference counter type cannot be assumed
       *  to be either copyable or moveable, and at this time I don't want to expose
       *  dart::shareable_ptr externally.
       */
      size_t share_bytes(RefCount<gsl::byte const>& bytes) const;

      /**
       *  @brief
       *  Function returns an owning copy of the network-level buffer for a
       *  finalized packet.
       *
       *  @details
       *  The primary difference between this and dart::packet::get_bytes is that
       *  get_bytes returns a non-owning view into the packet's buffer, whereas
       *  dup_bytes copies the buffer out into a newly allocated region, and returns
       *  that.
       */
      std::unique_ptr<gsl::byte const[], void (*) (gsl::byte const*)> dup_bytes() const;

      /**
       *  @brief
       *  Function returns an owning copy of the network-level buffer for a
       *  finalized packet.
       *
       *  @details
       *  The primary difference between this and dart::packet::get_bytes is that
       *  get_bytes returns a non-owning view into the packet's buffer, whereas
       *  dup_bytes copies the buffer out into a newly allocated region, and returns
       *  that.
       *
       *  @remarks
       *  Out-parameters aren't great, but it seemed like the easiest way to do this.
       */
      std::unique_ptr<gsl::byte const[], void (*) (gsl::byte const*)> dup_bytes(size_type& len) const;

      /*----- Iterator Functions -----*/

      /**
       *  @brief
       *  Function returns a dart::packet::iterator that can be used to iterate over the
       *  VALUES of the current packet.
       *
       *  @details
       *  Throws if this is not an object or array.
       */
      auto begin() const -> iterator;

      /**
       *  @brief
       *  Function returns a dart::packet::iterator that can be used to iterate over the
       *  VALUES of the current packet.
       *
       *  @details
       *  Throws if this is not an object or array.
       *
       *  @remarks
       *  Function exists for API uniformity with the STL, but is precisely equal to
       *  dart::packet::begin.
       */
      auto cbegin() const -> iterator;

      /**
       *  @brief
       *  Function returns a dart::packet::iterator one-past-the-end for iterating over
       *  the VALUES of the current packet.
       *
       *  @details
       *  Dart iterators are bidirectional, so the iterator could be decremented to iterate
       *  over the valuespace backwards, or it can be used as a sentinel value for
       *  traditional iteration.
       */
      auto end() const -> iterator;

      /**
       *  @brief
       *  Function returns a dart::packet::iterator one-past-the-end for iterating over
       *  the VALUES of the current packet.
       *
       *  @details
       *  Dart iterators are bidirectional, so the iterator could be decremented to iterate
       *  over the valuespace backwards, or it can be used as a sentinel value for
       *  traditional iteration.
       *
       *  @remarks
       *  Function exists for API uniformity with the STL, but is precisely equal to
       *  dart::packet::end.
       */
      auto cend() const -> iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator that iterates in reverse order from
       *  dart::packet::end to dart::packet::begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::begin.
       */
      auto rbegin() const -> reverse_iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator that iterates in reverse order from
       *  dart::packet::end to dart::packet::begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::begin.
       *
       *  @remarks
       *  Function exists for API uniformity with the STL, but is precisely equal to
       *  dart::packet::rbegin.
       */
      auto crbegin() const -> reverse_iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator one-past-the-end, that iterates in
       *  reverse order from dart::packet::end to dart::packet::begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::end.
       */
      auto rend() const -> reverse_iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator one-past-the-end, that iterates in
       *  reverse order from dart::packet::end to dart::packet::begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::end.
       *
       *  @remarks
       *  Function exists for API uniformity with the STL, but is precisely equal to
       *  dart::packet::rbegin.
       */
      auto crend() const -> reverse_iterator;

      /**
       *  @brief
       *  Function returns a dart::packet::iterator that can be used to iterate over the
       *  KEYS of the current packet.
       *
       *  @details
       *  Throws if this is not an object
       */
      auto key_begin() const -> iterator;

      /**
       *  @brief
       *  Function returns a dart::packet::iterator one-past-the-end that can be used to
       *  iterate over the KEYS of the current packet.
       *
       *  @details
       *  Dart iterators are bidirectional, so the iterator could be decremented to iterate
       *  over the keyspace backwards, or it can be used as a sentinel value for traditional
       *  iteration.
       */
      auto key_end() const -> iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator that iterates over the KEYS of the current
       *  packet in reverse order from dart::packet::key_end to dart::packet::key_begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::key_begin.
       */
      auto rkey_begin() const -> reverse_iterator;

      /**
       *  @brief
       *  Function returns a reverse iterator one-past-the-end that iterates over the
       *  KEYS of the current packet in reverse order from dart::packet::key_end to
       *  dart::packet::key_begin.
       *
       *  @details
       *  Despite the fact that you're iterating backwards, function otherwise behaves
       *  like dart::packet::key_end.
       */
      auto rkey_end() const -> reverse_iterator;

      /**
       *  @brief
       *  Function returns a tuple of key-value iterators to allow for easy iteration
       *  over keys and values with C++17 structured bindings.
       */
      auto kvbegin() const -> std::tuple<iterator, iterator>;

      /**
       *  @brief
       *  Function returns a tuple of key-value iterators to allow for easy iteration
       *  over keys and values with C++17 structured bindings.
       */
      auto kvend() const -> std::tuple<iterator, iterator>;

      /**
       *  @brief
       *  Function returns a tuple of key-value reverse iterators to allow for easy
       *  iteration over keys and values with C++17 structured bindings.
       */
      auto rkvbegin() const -> std::tuple<reverse_iterator, reverse_iterator>;

      /**
       *  @brief
       *  Function returns a tuple of key-value reverse iterators to allow for easy
       *  iteration over keys and values with C++17 structured bindings.
       */
      auto rkvend() const -> std::tuple<reverse_iterator, reverse_iterator>;

      /*----- Member Ownership Helpers -----*/

      /**
       *  @brief
       *  Function calculates whether the current type is a non-owning view or not.
       */
      constexpr bool is_view() const noexcept;

      /**
       *  @brief
       *  Function allows one to explicitly grab full ownership of the current view.
       */
      template <bool enabled = !refcount::is_owner<RefCount>::value, class EnableIf =
        std::enable_if_t<
          enabled
        >
      >
      auto as_owner() const noexcept;

    private:

      /*----- Private Types -----*/

      using implementation = shim::variant<
        basic_heap<RefCount>,
        basic_buffer<RefCount>
      >;

      /*----- Private Helpers -----*/

      size_t upper_bound() const noexcept;
      auto layout(gsl::byte* buffer) const noexcept -> size_type;
      detail::raw_type get_raw_type() const noexcept;

      basic_heap<RefCount>& get_heap();
      basic_heap<RefCount> const& get_heap() const;
      basic_heap<RefCount>* try_get_heap();
      basic_heap<RefCount> const* try_get_heap() const;
      basic_buffer<RefCount>& get_buffer();
      basic_buffer<RefCount> const& get_buffer() const;
      basic_buffer<RefCount>* try_get_buffer();
      basic_buffer<RefCount> const* try_get_buffer() const;

      /*----- Private Members -----*/

      implementation impl;

      /*----- Friends -----*/

      // Allow type conversion logic to work.
      friend struct convert::detail::caster_impl<convert::detail::null_tag>;
      friend struct convert::detail::caster_impl<convert::detail::boolean_tag>;
      friend struct convert::detail::caster_impl<convert::detail::integer_tag>;
      friend struct convert::detail::caster_impl<convert::detail::decimal_tag>;
      friend struct convert::detail::caster_impl<convert::detail::string_tag>;

      template <template <class> class LhsRC, template <class> class RhsRC>
      friend bool operator ==(basic_buffer<LhsRC> const&, basic_packet<RhsRC> const&);
      template <template <class> class LhsRC, template <class> class RhsRC>
      friend bool operator ==(basic_heap<LhsRC> const&, basic_packet<RhsRC> const&);
      template <template <class> class RC>
      friend class dart::basic_packet;
      friend class detail::object<RefCount>;
      friend struct detail::buffer_builder<RefCount>;

  };

  using heap = basic_heap<std::shared_ptr>;
  using buffer = basic_buffer<std::shared_ptr>;
  using packet = basic_packet<std::shared_ptr>;

  using object = packet::object;
  using array = packet::array;
  using string = packet::string;
  using number = packet::number;
  using flag = packet::flag;

  // Make sure everything has noexcept moves as expected to avoid unnecessary bottlenecks.
  static_assert(std::is_nothrow_move_constructible<heap>::value
      && std::is_nothrow_move_assignable<heap>::value, "dart library is misconfigured");
  static_assert(std::is_nothrow_move_constructible<heap::object>::value
      && std::is_nothrow_move_assignable<heap::object>::value, "dart library is misconfigured");
  static_assert(std::is_nothrow_move_constructible<heap::array>::value
      && std::is_nothrow_move_assignable<heap::array>::value, "dart library is misconfigured");
  static_assert(std::is_nothrow_move_constructible<heap::string>::value
      && std::is_nothrow_move_assignable<heap::string>::value, "dart library is misconfigured");
  static_assert(std::is_nothrow_move_constructible<heap::number>::value
      && std::is_nothrow_move_assignable<heap::number>::value, "dart library is misconfigured");
  static_assert(std::is_nothrow_move_constructible<heap::flag>::value
      && std::is_nothrow_move_assignable<heap::flag>::value, "dart library is misconfigured");
  static_assert(std::is_nothrow_move_constructible<heap::null>::value
      && std::is_nothrow_move_assignable<heap::null>::value, "dart library is misconfigured");
  static_assert(std::is_nothrow_move_constructible<buffer>::value
      && std::is_nothrow_move_assignable<buffer>::value, "dart library is misconfigured");
  static_assert(std::is_nothrow_move_constructible<buffer::object>::value
      && std::is_nothrow_move_assignable<buffer::object>::value, "dart library is misconfigured");
  static_assert(std::is_nothrow_move_constructible<buffer::array>::value
      && std::is_nothrow_move_assignable<buffer::array>::value, "dart library is misconfigured");
  static_assert(std::is_nothrow_move_constructible<buffer::string>::value
      && std::is_nothrow_move_assignable<buffer::string>::value, "dart library is misconfigured");
  static_assert(std::is_nothrow_move_constructible<buffer::number>::value
      && std::is_nothrow_move_assignable<buffer::number>::value, "dart library is misconfigured");
  static_assert(std::is_nothrow_move_constructible<buffer::flag>::value
      && std::is_nothrow_move_assignable<buffer::flag>::value, "dart library is misconfigured");
  static_assert(std::is_nothrow_move_constructible<buffer::null>::value
      && std::is_nothrow_move_assignable<buffer::null>::value, "dart library is misconfigured");
  static_assert(std::is_nothrow_move_constructible<packet>::value
      && std::is_nothrow_move_assignable<packet>::value, "dart library is misconfigured");
  static_assert(std::is_nothrow_move_constructible<packet::object>::value
      && std::is_nothrow_move_assignable<packet::object>::value, "dart library is misconfigured");
  static_assert(std::is_nothrow_move_constructible<packet::array>::value
      && std::is_nothrow_move_assignable<packet::array>::value, "dart library is misconfigured");
  static_assert(std::is_nothrow_move_constructible<packet::string>::value
      && std::is_nothrow_move_assignable<packet::string>::value, "dart library is misconfigured");
  static_assert(std::is_nothrow_move_constructible<packet::number>::value
      && std::is_nothrow_move_assignable<packet::number>::value, "dart library is misconfigured");
  static_assert(std::is_nothrow_move_constructible<packet::flag>::value
      && std::is_nothrow_move_assignable<packet::flag>::value, "dart library is misconfigured");
  static_assert(std::is_nothrow_move_constructible<packet::null>::value
      && std::is_nothrow_move_assignable<packet::null>::value, "dart library is misconfigured");

  /*----- Free Operator Declarations -----*/

  inline namespace literals {

    inline packet operator ""_dart(char const* val, size_t len);
    inline packet operator ""_dart(unsigned long long val);
    inline packet operator ""_dart(long double val);

  }

  /*----- Global Free Functions -----*/

#ifdef DART_USE_SAJSON
  /**
   *  @brief
   *  Function constructs an optionally finalized packet to represent the given JSON string.
   *
   *  @details
   *  Parsing is based on sajson, and so exposes the same parsing customization points as
   *  sajson.
   */
  template <unsigned parse_stack_size = default_parse_stack_size>
  packet from_json(shim::string_view json, bool finalize = false) {
    return packet::from_json<parse_stack_size>(json, finalize);
  }

  /**
   *  @brief
   *  Function constructs an optionally finalized packet to represent the given JSON string.
   *
   *  @details
   *  Parsing is based on sajson, and so exposes the same parsing customization points as
   *  sajson.
   */
  template <unsigned parse_stack_size = default_parse_stack_size>
  packet parse(shim::string_view json, bool finalize = false) {
    return from_json<parse_stack_size>(json, finalize);
  }
#elif DART_HAS_RAPIDJSON
  /**
   *  @brief
   *  Function constructs an optionally finalized packet to represent the given JSON string.
   *
   *  @details
   *  Parsing is based on RapidJSON, and so exposes the same parsing customization points as
   *  RapidJSON.
   *  If your JSON has embedded comments in it, NaN or +/-Infinity values, or trailing commas,
   *  you can parse in the following ways:
   *  ```
   *  auto json = input.read();
   *  auto comments = dart::heap::from_json<dart::parse_comments>(json);
   *  auto nan_inf = dart::heap::from_json<dart::parse_nan>(json);
   *  auto commas = dart::heap::from_json<dart::parse_trailing_commas>(json);
   *  auto all_of_it = dart::heap::from_json<dart::parse_permissive>(json);
   *  ```
   */
  template <unsigned flags = parse_default>
  packet from_json(shim::string_view json, bool finalize = false) {
    return packet::from_json<flags>(json, finalize);
  }

  /**
   *  @brief
   *  Function constructs an optionally finalized packet to represent the given JSON string.
   *
   *  @details
   *  Parsing is based on RapidJSON, and so exposes the same parsing customization points as
   *  RapidJSON.
   *  If your JSON has embedded comments in it, NaN or +/-Infinity values, or trailing commas,
   *  you can parse in the following ways:
   *  ```
   *  auto json = input.read();
   *  auto comments = dart::heap::from_json<dart::parse_comments>(json);
   *  auto nan_inf = dart::heap::from_json<dart::parse_nan>(json);
   *  auto commas = dart::heap::from_json<dart::parse_trailing_commas>(json);
   *  auto all_of_it = dart::heap::from_json<dart::parse_permissive>(json);
   *  ```
   */
  template <unsigned flags = parse_default>
  packet parse(shim::string_view json, bool finalize = false) {
    return from_json<flags>(json, finalize);
  }
#endif

}

/*----- Function Template Implementations -----*/

#include "dart/api.tcc"
#include "dart/array.tcc"
#include "dart/iterator.tcc"
#include "dart/object.tcc"
#include "dart/operators.tcc"
#include "dart/primitive.tcc"
#include "dart/string.tcc"
#include "dart/heap/heap.h"
#include "dart/buffer/buffer.h"
#include "dart/packet/packet.h"
#include "dart/connector/json.tcc"
#include "dart/connector/yaml.tcc"

#endif
