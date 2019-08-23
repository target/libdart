#ifndef DART_COMMON_H
#define DART_COMMON_H

// Automatically detect libraries if possible.
#if defined(__has_include)
# if !defined(DART_HAS_RAPIDJSON)
#   define DART_HAS_RAPIDJSON __has_include(<rapidjson/reader.h>) && __has_include(<rapidjson/writer.h>)
# endif
# ifndef DART_HAS_YAML
#   define DART_HAS_YAML __has_include(<yaml.h>)
# endif
#endif

/*----- System Includes -----*/

#include <map>
#include <vector>
#include <math.h>
#include <gsl/gsl>
#include <errno.h>
#include <cstdlib>
#include <unistd.h>
#include <stdarg.h>
#include <limits.h>
#include <algorithm>

#if DART_HAS_RAPIDJSON
#include <rapidjson/reader.h>
#include <rapidjson/writer.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#endif

#if DART_HAS_YAML
#include <yaml.h>
#endif

/*----- Local Includes -----*/

#include "shim.h"
#include "meta.h"
#include "support/ptrs.h"
#include "support/ordered.h"

/*----- System Includes with Compiler Flags -----*/

// Current version of sajson emits unused parameter
// warnings on Clang.
#ifdef DART_USE_SAJSON
#if DART_USING_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif
#include <sajson.h>
#if DART_USING_CLANG
#pragma clang diagnostic pop
#endif
#endif

/*----- Macro Definitions -----*/

#define DART_FROM_THIS                                                                                        \
  reinterpret_cast<gsl::byte const*>(this)

#define DART_FROM_THIS_MUT                                                                                    \
  reinterpret_cast<gsl::byte*>(this)

#define DART_STRINGIFY_IMPL(x) #x
#define DART_STRINGIFY(x) DART_STRINGIFY_IMPL(x)

#ifndef NDEBUG

/**
 *  @brief
 *  Macro customizes functionality usually provided by assert().
 *
 *  @details
 *  Not strictly necessary, but tries to provide a bit more context and
 *  information as to why I just murdered the user's program (in production, no doubt).
 *
 *  @remarks
 *  Don't actually know if Doxygen lets you document macros, guess we'll see.
 */
#define DART_ASSERT(cond)                                                                                     \
  if (__builtin_expect(!(cond), 0)) {                                                                         \
    auto& msg = "dart::packet has detected fatal memory corruption and cannot continue execution.\n"          \
      "\"" DART_STRINGIFY(cond) "\" violated.\nSee " __FILE__ ":" DART_STRINGIFY(__LINE__);                   \
    int spins {0}, written {0}, total {sizeof(msg)};                                                          \
    do {                                                                                                      \
      ssize_t ret = write(STDERR_FILENO, msg + written, total - written);                                     \
      if (ret >= 0) written += ret;                                                                           \
    } while (written != total && spins++ < 16);                                                               \
    std::abort();                                                                                             \
  }
#else
#define DART_ASSERT(cond) 
#endif

/*----- Global Declarations -----*/

namespace dart {

  template <class Object>
  class basic_object;
  template <class Array>
  class basic_array;
  template <class String>
  class basic_string;
  template <class Number>
  class basic_number;
  template <class Boolean>
  class basic_flag;
  template <class Null>
  class basic_null;

  template <template <class> class RefCount>
  class basic_heap;
  template <template <class> class RefCount>
  class basic_buffer;
  template <template <class> class RefCount>
  class basic_packet;

  struct type_error : std::logic_error {
    type_error(char const* msg) : logic_error(msg) {}
  };

  struct state_error : std::runtime_error {
    state_error(char const* msg) : runtime_error(msg) {}
  };

  struct parse_error : std::runtime_error {
    parse_error(char const* msg) : runtime_error(msg) {}
  };

  namespace detail {

    /**
     *  @brief
     *  Enum encodes the type of an individual packet instance.
     *
     *  @details
     *  Internally, dart::packet manages a much larger set of types that encode ancillary information
     *  such as precision, signedness, alignment, size, however, all internal types map onto one of those
     *  contained in dart::packet::type, and all public API functions conceptually interact with objects
     *  of these types.
     */
    enum class type : uint8_t {
      object,
      array,
      string,
      integer,
      decimal,
      boolean,
      null
    };

    /**
     *  @brief
     *  Low-level type information that encodes information about the underlying machine-type,
     *  like precision, signedness, etc.
     */
    enum class raw_type : uint8_t {
      object,
      array,
      string,
      small_string,
      big_string,
      short_integer,
      integer,
      long_integer,
      decimal,
      long_decimal,
      boolean,
      null
    };

    /**
     *  @brief
     *  Used internally in scenarios where two dart types aren't contained within
     *  some existing data structure, but they need to be paired together.
     */
    template <class Packet>
    struct basic_pair {

      /*----- Lifecycle Functions -----*/

      basic_pair() = default;
      template <class P, class =
        std::enable_if_t<
          std::is_convertible<
            P,
            Packet
          >::value
        >
      >
      basic_pair(P&& key, P&& value) : key(std::forward<P>(key)), value(std::forward<P>(value)) {}
      basic_pair(basic_pair const&) = default;
      basic_pair(basic_pair&&) = default;
      ~basic_pair() = default;

      /*----- Operators -----*/

      basic_pair& operator =(basic_pair const&) = default;
      basic_pair& operator =(basic_pair&&) = default;

      /*----- Members -----*/

      Packet key, value;

    };
    template <template <class> class RefCount>
    using heap_pair = basic_pair<basic_heap<RefCount>>;
    template <template <class> class RefCount>
    using buffer_pair = basic_pair<basic_buffer<RefCount>>;
    template <template <class> class RefCount>
    using packet_pair = basic_pair<basic_packet<RefCount>>;

    /**
     *  @brief
     *  Helper-struct for sorting `dart::packet`s within `std::map`s.
     */
    template <template <class> class RefCount>
    struct dart_comparator {
      using is_transparent = shim::string_view;

      bool operator ()(shim::string_view lhs, shim::string_view rhs) const;

      // Comparison between some packet type and a string
      template <template <template <class> class> class Packet>
      bool operator ()(Packet<RefCount> const& lhs, shim::string_view rhs) const;
      template <template <template <class> class> class Packet>
      bool operator ()(shim::string_view lhs, Packet<RefCount> const& rhs) const;

      // Comparison between a key-value pair of some packet type and a string
      template <template <template <class> class> class Packet>
      bool operator ()(basic_pair<Packet<RefCount>> const& lhs, shim::string_view rhs) const;
      template <template <template <class> class> class Packet>
      bool operator ()(shim::string_view lhs, basic_pair<Packet<RefCount>> const& rhs) const;

      // Comparison between two, potentially disparate, packet types
      template <
        template <template <class> class> class LhsPacket,
        template <template <class> class> class RhsPacket
      >
      bool operator ()(LhsPacket<RefCount> const& lhs, RhsPacket<RefCount> const& rhs) const;

      // Comparison between a key-value pair of some packet type and some,
      // potentially disparate, packet type
      template <
        template <template <class> class> class LhsPacket,
        template <template <class> class> class RhsPacket
      >
      bool operator ()(basic_pair<LhsPacket<RefCount>> const& lhs, RhsPacket<RefCount> const& rhs) const;
      template <
        template <template <class> class> class LhsPacket,
        template <template <class> class> class RhsPacket
      >
      bool operator ()(LhsPacket<RefCount> const& lhs, basic_pair<RhsPacket<RefCount>> const& rhs) const;

      // Comparison between a key-value pair of some packet type and
      // a key-value pair of some, potentially disparate, packet type.
      template <
        template <template <class> class> class LhsPacket,
        template <template <class> class> class RhsPacket
      >
      bool operator ()(basic_pair<LhsPacket<RefCount>> const& lhs, basic_pair<RhsPacket<RefCount>> const& rhs) const;
    };

    /**
     *  @brief
     *  Helper-struct for safely comparing objects of any type.
     */
    struct typeless_comparator {
      template <class Lhs, class Rhs>
      bool operator ()(Lhs&& lhs, Rhs&& rhs) const noexcept;
    };

    template <template <class> class RefCount>
    using buffer_refcount_type = shareable_ptr<RefCount<gsl::byte const>>;

    class prefix_entry;
    template <class>
    struct table_layout {
      using offset_type = uint32_t;
      static constexpr auto max_offset = std::numeric_limits<offset_type>::max();

      alignas(4) little_order<offset_type> offset;
      alignas(4) little_order<uint8_t> type;
    };
    template <>
    struct table_layout<prefix_entry> {
      using offset_type = uint32_t;
      using prefix_type = uint16_t;
      static constexpr auto max_offset = std::numeric_limits<offset_type>::max();

      alignas(4) little_order<uint32_t> offset;
      alignas(1) little_order<uint8_t> type;
      alignas(1) little_order<uint8_t> len;
      alignas(2) prefix_type prefix;
    };

    /**
     *  @brief
     *  Macro customizes functionality usually provided by assert().
     *
     *  @details
     *  Not strictly necessary, but tries to provide a bit more context and
     *  information as to why I just murdered the user's program (in production, no doubt).
     *
     *  @remarks
     *  Don't actually know if Doxygen lets you document macros, guess we'll see.
     */
    template <class T>
    class vtable_entry {

      public:

        /*----- Lifecycle Functions -----*/

        vtable_entry(detail::raw_type type, uint32_t offset);
        vtable_entry(vtable_entry const&) = default;

        /*----- Operators -----*/

        vtable_entry& operator =(vtable_entry const&) = default;

        /*----- Public API -----*/

        raw_type get_type() const noexcept;
        uint32_t get_offset() const noexcept;

        // This sucks, but we need it for dart::buffer::inject
        void adjust_offset(std::ptrdiff_t diff) noexcept;

      protected:

        /*----- Protected Members -----*/

        table_layout<T> layout;

    };

    /**
     *  @brief
     *  Class represents an entry in the vtable of an object.
     *
     *  @details
     *  Class wraps memory that is stored in little endian byte order,
     *  irrespective of the native ordering of the host machine.
     *  In other words, attempt to subvert its API at your peril.
     *
     *  @remarks
     *  Although class inherts from vtable_entry, and can be safely
     *  be used as such, it remains a standard layout type due to some
     *  hackery with its internals.
     */
    class prefix_entry : public vtable_entry<prefix_entry> {

      public:

        /*----- Public Types -----*/

        using prefix_type = table_layout<prefix_entry>::prefix_type;

        /*----- Lifecycle Functions -----*/

        inline prefix_entry(detail::raw_type type, uint32_t offset, shim::string_view prefix) noexcept;
        prefix_entry(prefix_entry const&) = default;

        /*----- Operators -----*/

        prefix_entry& operator =(prefix_entry const&) = default;

        /*----- Public API -----*/

        inline int prefix_compare(shim::string_view str) const noexcept;

      private:

        /*----- Private Helpers -----*/

        inline int compare_impl(char const* const str, size_t const len) const noexcept;

        /*----- Private Types -----*/

        using storage_t = std::aligned_storage_t<sizeof(prefix_type), 4>;

    };

    using object_entry = prefix_entry;
    using array_entry = vtable_entry<void>;
    using object_layout = table_layout<prefix_entry>;
    using array_layout = table_layout<void>;
    static_assert(std::is_standard_layout<array_entry>::value, "dart library is misconfigured");
    static_assert(std::is_standard_layout<object_entry>::value, "dart library is misconfigured");

    // Aliases for STL structures.
    template <template <class> class RefCount>
    using packet_elements = std::vector<refcount::owner_indirection_t<basic_heap, RefCount>>;
    template <template <class> class RefCount>
    using packet_fields = std::map<
      refcount::owner_indirection_t<basic_heap, RefCount>,
      refcount::owner_indirection_t<basic_heap, RefCount>,
      refcount::owner_indirection_t<dart_comparator, RefCount>
    >;

    /**
     *  @brief
     *  Struct represents the minimum context required to perform
     *  an operation on a dart::buffer object.
     *
     *  @details
     *  At its core, this is what dart::buffer "is".
     *  It just provides a nice API, and some memory safety around
     *  this structure, which is why it's so fast.
     */
    struct raw_element {
      raw_type type;
      gsl::byte const* buffer;
    };

    /**
     *  @brief
     *  Struct is the lowest level abstraction for safe iteration over a dart::buffer
     *  object/array.
     *
     *  @details
     *  Struct holds onto the context for what vtable entry it's currently on,
     *  where the base of its aggregate is, and how to dereference itself.
     *
     *  @remarks
     *  ll_iterator holds onto more state than I'd like, but it's kind of unavoidable
     *  given the fact that the vtable provides offsets from the base of the aggregate
     *  itself and the fact that we may be iterating over values OR pairs of values.
     */
    template <template <class> class RefCount>
    struct ll_iterator {

      /*----- Types -----*/

      using value_type = raw_element;
      using loading_function = value_type (gsl::byte const*, size_t idx);

      /*----- Lifecycle Functions -----*/

      ll_iterator() = delete;
      ll_iterator(size_t idx, gsl::byte const* base, loading_function* load_func) noexcept :
        idx(idx),
        base(base),
        load_func(load_func)
      {}
      ll_iterator(ll_iterator const&) = default;
      ll_iterator(ll_iterator&&) noexcept = default;
      ~ll_iterator() = default;

      /*----- Operators -----*/

      ll_iterator& operator =(ll_iterator const&) = default;
      ll_iterator& operator =(ll_iterator&&) noexcept = default;

      bool operator ==(ll_iterator const& other) const noexcept;
      bool operator !=(ll_iterator const& other) const noexcept;

      auto operator ++() noexcept -> ll_iterator&;
      auto operator --() noexcept -> ll_iterator&;
      auto operator ++(int) noexcept -> ll_iterator;
      auto operator --(int) noexcept -> ll_iterator;

      auto operator *() const noexcept -> value_type;

      /*----- Members -----*/

      size_t idx;
      gsl::byte const* base;
      loading_function* load_func;

    };

    /**
     *  @brief
     *  Struct is the lowest level abstraction for safe iteration over a dart::heap
     *  object/array.
     *
     *  @details
     *  Struct simply wraps an STL iterator of some type, and a way to dereference it.
     */
    template <template <class> class RefCount>
    struct dn_iterator {

      /*----- Types -----*/

      using value_type = basic_heap<RefCount>;
      using reference = value_type const&;
      using fields_deref_func = reference (typename packet_fields<RefCount>::const_iterator const&);
      using elements_deref_func = reference (typename packet_elements<RefCount>::const_iterator const&);

      struct fields_layout {
        fields_deref_func* deref;
        typename packet_fields<RefCount>::const_iterator it;
      };

      struct elements_layout {
        elements_deref_func* deref;
        typename packet_elements<RefCount>::const_iterator it;
      };

      using implerator = shim::variant<fields_layout, elements_layout>;

      /*----- Lifecycle Functions -----*/

      dn_iterator() = delete;
      dn_iterator(typename packet_fields<RefCount>::const_iterator it, fields_deref_func* func) noexcept :
        impl(fields_layout {func, it})
      {}
      dn_iterator(typename packet_elements<RefCount>::const_iterator it, elements_deref_func* func) noexcept :
        impl(elements_layout {func, it})
      {}
      dn_iterator(dn_iterator const&) = default;
      dn_iterator(dn_iterator&&) noexcept = default;
      ~dn_iterator() = default;

      /*----- Operators -----*/

      dn_iterator& operator =(dn_iterator const&) = default;
      dn_iterator& operator =(dn_iterator&&) noexcept = default;

      bool operator ==(dn_iterator const& other) const noexcept;
      bool operator !=(dn_iterator const& other) const noexcept;

      auto operator ++() noexcept -> dn_iterator&;
      auto operator --() noexcept -> dn_iterator&;
      auto operator ++(int) noexcept -> dn_iterator;
      auto operator --(int) noexcept -> dn_iterator;

      auto operator *() const noexcept -> reference;

      /*----- Members -----*/

      implerator impl;

    };
    template <template <class> class RefCount>
    using dynamic_iterator = refcount::owner_indirection_t<dn_iterator, RefCount>;

    /**
     *  @brief
     *  Class is the lowest level abstraction for safe interaction with
     *  a dart::buffer object.
     *
     *  @details
     *  Class wraps memory that is stored in little endian byte order,
     *  irrespective of the native ordering of the host machine.
     *  In other words, attempt to subvert its API at your peril.
     */
    template <template <class> class RefCount>
    class object {

      public:

        /*----- Lifecycle Functions -----*/

        object() = delete;

        // JSON Constructors
#ifdef DART_USE_SAJSON
        explicit object(sajson::value fields) noexcept;
#endif
#if DART_HAS_RAPIDJSON
        explicit object(rapidjson::Value const& fields) noexcept;
#endif

        // Direct constructors
        explicit object(gsl::span<packet_pair<RefCount>> pairs) noexcept;
        explicit object(packet_fields<RefCount> const* fields) noexcept;

        // Special constructors
        object(object const* base, object const* incoming) noexcept;
        template <class Key>
        object(object const* base, gsl::span<Key const*> key_ptrs) noexcept;

        object(object const&) = delete;
        ~object() = delete;

        /*----- Operators -----*/

        object& operator =(object const&) = delete;

        /*----- Public API -----*/

        size_t size() const noexcept;
        size_t get_sizeof() const noexcept;

        auto begin() const noexcept -> ll_iterator<RefCount>;
        auto key_begin() const noexcept -> ll_iterator<RefCount>;
        auto end() const noexcept -> ll_iterator<RefCount>;
        auto key_end() const noexcept -> ll_iterator<RefCount>;

        template <class Callback>
        auto get_key(shim::string_view const key, Callback&& cb) const noexcept -> raw_element;
        auto get_it(shim::string_view const key) const noexcept -> ll_iterator<RefCount>;
        auto get_key_it(shim::string_view const key) const noexcept -> ll_iterator<RefCount>;
        auto get_value(shim::string_view const key) const noexcept -> raw_element;
        auto at_value(shim::string_view const key) const -> raw_element;

        static auto load_key(gsl::byte const* base, size_t idx) noexcept -> typename ll_iterator<RefCount>::value_type;
        static auto load_value(gsl::byte const* base, size_t idx) noexcept -> typename ll_iterator<RefCount>::value_type;

        /*----- Public Members -----*/

        static constexpr auto alignment = sizeof(int64_t);

      private:

        /*----- Private Helpers -----*/

        size_t realign(size_t guess, size_t offset) noexcept;

        template <class Callback>
        auto get_value_impl(shim::string_view const key, Callback&& cb) const -> raw_element;

        object_entry* vtable() noexcept;
        object_entry const* vtable() const noexcept;

        /*----- Private Members -----*/

        alignas(4) little_order<uint32_t> bytes;
        alignas(4) little_order<uint32_t> elems;

    };
    static_assert(std::is_standard_layout<object<std::shared_ptr>>::value, "dart library is misconfigured");

    /**
     *  @brief
     *  Class is the lowest level abstraction for safe interaction with
     *  a dart::buffer array.
     *
     *  @details
     *  Class wraps memory that is stored in little endian byte order,
     *  irrespective of the native ordering of the host machine.
     *  In other words, attempt to subvert its API at your peril.
     */
    template <template <class> class RefCount>
    class array {

      public:

        /*----- Lifecycle Functions -----*/

        array() = delete;
#ifdef DART_USE_SAJSON
        explicit array(sajson::value elems) noexcept;
#endif
#if DART_HAS_RAPIDJSON
        explicit array(rapidjson::Value const& elems) noexcept;
#endif
        array(packet_elements<RefCount> const* elems) noexcept;
        array(array const&) = delete;
        ~array() = delete;

        /*----- Operators -----*/

        array& operator =(array const&) = delete;

        /*----- Public API -----*/

        size_t size() const noexcept;
        size_t get_sizeof() const noexcept;

        auto begin() const noexcept -> ll_iterator<RefCount>;
        auto end() const noexcept -> ll_iterator<RefCount>;

        auto get_elem(size_t index) const noexcept -> raw_element;
        auto at_elem(size_t index) const -> raw_element;

        static auto load_elem(gsl::byte const* base, size_t idx) noexcept -> typename ll_iterator<RefCount>::value_type;

        /*----- Public Members -----*/

        static constexpr auto alignment = sizeof(int64_t);

      private:

        /*----- Private Helpers -----*/

        auto get_elem_impl(size_t index, bool throw_if_absent) const -> raw_element;

        array_entry* vtable() noexcept;
        array_entry const* vtable() const noexcept;

        /*----- Private Members -----*/

        alignas(4) little_order<uint32_t> bytes;
        alignas(4) little_order<uint32_t> elems;

    };
    static_assert(std::is_standard_layout<array<std::shared_ptr>>::value, "dart library is misconfigured");

    /**
     *  @brief
     *  Class is the lowest level abstraction for safe interaction with
     *  a dart::buffer string.
     *
     *  @details
     *  Class wraps memory that is stored in little endian byte order,
     *  irrespective of the native ordering of the host machine.
     *  In other words, attempt to subvert its API at your peril.
     */
    template <class SizeType>
    class basic_string {

      public:

        /*----- Public Types -----*/

        using size_type = SizeType;

        /*----- Lifecycle Functions -----*/

        basic_string() = delete;
        explicit basic_string(shim::string_view strv) noexcept;
        explicit basic_string(char const* str, size_t len) noexcept;
        basic_string(basic_string const&) = delete;
        ~basic_string() = delete;

        /*----- Operators -----*/

        basic_string& operator =(basic_string const&) = delete;

        /*----- Public API -----*/

        size_t size() const noexcept;
        size_t get_sizeof() const noexcept;

        shim::string_view get_strv() const noexcept;

        static size_t static_sizeof(size_type len) noexcept;

        /*----- Public Members -----*/

        static constexpr auto alignment = sizeof(size_type);

      private:

        /*----- Private Helpers -----*/

        char* data() noexcept;
        char const* data() const noexcept;

        /*----- Private Members -----*/

        alignas(alignment) little_order<size_type> len;

    };
    using string = basic_string<uint16_t>;
    using big_string = basic_string<uint32_t>;
    static_assert(std::is_standard_layout<string>::value, "dart library is misconfigured");
    static_assert(std::is_standard_layout<big_string>::value, "dart library is misconfigured");

    /**
     *  @brief
     *  Class is the lowest level abstraction for safe interaction with
     *  a dart::buffer primitive value.
     *
     *  @details
     *  Class wraps memory that is stored in little endian byte order,
     *  irrespective of the native ordering of the host machine.
     *  In other words, attempt to subvert its API at your peril.
     */
    template <class T>
    class primitive {

      public:

        /*----- Lifecycle Functions -----*/

        primitive() = delete;
        explicit primitive(T data) noexcept : data(data) {}
        primitive(primitive const&) = delete;
        ~primitive() = delete;

        /*---- Operators -----*/

        primitive& operator =(primitive const&) = delete;

        /*----- Public API -----*/

        size_t get_sizeof() const noexcept;

        T get_data() const noexcept;

        static size_t static_sizeof() noexcept;

        /*----- Public Members -----*/

        static constexpr auto alignment = sizeof(T);

      private:

        /*----- Private Members -----*/

        alignas(alignment) little_order<T> data;

    };
    static_assert(std::is_standard_layout<primitive<int>>::value, "dart library is misconfigured");

    template <template <class> class RefCount>
    struct buffer_builder {
      using buffer = basic_buffer<RefCount>;

      template <class Span>
      static auto build_buffer(Span pairs) -> buffer;
      static auto merge_buffers(buffer const& base, buffer const& incoming) -> buffer;

      template <class Spannable>
      static auto project_keys(buffer const& base, Spannable const& keys) -> buffer;

      template <class Callback>
      static void each_unique_pair(object<RefCount> const* base, object<RefCount> const* incoming, Callback&& cb);
      template <class Key, class Callback>
      static void project_each_pair(object<RefCount> const* base, gsl::span<Key const*> key_ptrs, Callback&& cb);

      template <class Span>
      static size_t max_bytes(Span pairs);
    };

    // Used for tag dispatch from factory functions.
    struct view_tag {};
    struct object_tag {
      static constexpr auto alignment = object<std::shared_ptr>::alignment;
    };
    struct array_tag {
      static constexpr auto alignment = array<std::shared_ptr>::alignment;
    };
    struct string_tag {
      static constexpr auto alignment = big_string::alignment;
    };
    struct small_string_tag : string_tag {
      static constexpr auto alignment = string::alignment;
    };
    struct big_string_tag : string_tag {
      static constexpr auto alignment = string_tag::alignment;
    };
    struct integer_tag {
      static constexpr auto alignment = primitive<int64_t>::alignment;
    };
    struct short_integer_tag : integer_tag {
      static constexpr auto alignment = primitive<int16_t>::alignment;
    };
    struct medium_integer_tag : integer_tag {
      static constexpr auto alignment = primitive<int32_t>::alignment;
    };
    struct long_integer_tag : integer_tag {
      static constexpr auto alignment = integer_tag::alignment;
    };
    struct decimal_tag {
      static constexpr auto alignment = primitive<double>::alignment;
    };
    struct short_decimal_tag : decimal_tag {
      static constexpr auto alignment = primitive<float>::alignment;
    };
    struct long_decimal_tag : decimal_tag {
      static constexpr auto alignment = decimal_tag::alignment;
    };
    struct boolean_tag {
      static constexpr auto alignment = primitive<bool>::alignment;
    };
    struct null_tag {
      static constexpr auto alignment = 1;
    };

    /**
     *  @brief
     *  Function converts between internal type information and
     *  user-facing type information.
     *
     *  @details
     *  Dart internally has to encode significantly more complicated
     *  type information than it publicly exposes, so this function
     *  works as a an efficient go-between for the two.
     */
    inline type simplify_type(raw_type type) noexcept {
      switch (type) {
        case raw_type::object:
          return detail::type::object;
        case raw_type::array:
          return detail::type::array;
        case raw_type::small_string:
        case raw_type::string:
        case raw_type::big_string:
          return detail::type::string;
        case raw_type::short_integer:
        case raw_type::integer:
        case raw_type::long_integer:
          return detail::type::integer;
        case raw_type::decimal:
        case raw_type::long_decimal:
          return detail::type::decimal;
        case raw_type::boolean:
          return detail::type::boolean;
        default:
          DART_ASSERT(type == raw_type::null);
          return detail::type::null;
      }
    }

    /**
     *  @brief
     *  Function provides a "safe" bridge between the high level
     *  and low level object apis.
     *
     *  @details
     *  Don't pass it null.
     *  Tries its damndest not to invoke UB, but god knows.
     */
    template <template <class> class RefCount>
    object<RefCount> const* get_object(raw_element raw) {
      if (simplify_type(raw.type) == type::object) {
        DART_ASSERT(raw.buffer != nullptr);
        return shim::launder(reinterpret_cast<object<RefCount> const*>(raw.buffer));
      } else {
        throw type_error("dart::buffer is not a finalized object and cannot be accessed as such");
      }
    }

    /**
     *  @brief
     *  Function provides a "safe" bridge between the high level
     *  and low level array apis.
     *
     *  @details
     *  Don't pass it null.
     *  Tries its damndest not to invoke UB, but god knows.
     */
    template <template <class> class RefCount>
    array<RefCount> const* get_array(raw_element raw) {
      if (simplify_type(raw.type) == type::array) {
        DART_ASSERT(raw.buffer != nullptr);
        return shim::launder(reinterpret_cast<array<RefCount> const*>(raw.buffer));
      } else {
        throw type_error("dart::buffer is not a finalized array and cannot be accessed as such");
      }
    }

    /**
     *  @brief
     *  Function provides a "safe" bridge between the high level
     *  and low level string apis.
     *
     *  @details
     *  Don't pass it null.
     *  Tries its damndest not to invoke UB, but god knows.
     */
    inline string const* get_string(raw_element raw) {
      if (raw.type == raw_type::small_string || raw.type == raw_type::string) {
        DART_ASSERT(raw.buffer != nullptr);
        return shim::launder(reinterpret_cast<string const*>(raw.buffer));
      } else {
        throw type_error("dart::buffer is not a finalized string and cannot be accessed as such");
      }
    }

    /**
     *  @brief
     *  Function provides a "safe" bridge between the high level
     *  and low level string apis.
     *
     *  @details
     *  Don't pass it null.
     *  Tries its damndest not to invoke UB, but god knows.
     */
    inline big_string const* get_big_string(raw_element raw) {
      if (raw.type == raw_type::big_string) {
        DART_ASSERT(raw.buffer != nullptr);
        return shim::launder(reinterpret_cast<big_string const*>(raw.buffer));
      } else {
        throw type_error("dart::buffer is not a finalized string and cannot be accessed as such");
      }
    }

    /**
     *  @brief
     *  Function provides a "safe" bridge between the high level
     *  and low level primitive apis.
     *
     *  @details
     *  Don't pass it null.
     *  Tries its damndest not to invoke UB, but god knows.
     */
    template <class T>
    primitive<T> const* get_primitive(raw_element raw) {
      auto simple = simplify_type(raw.type);
      if (simple == type::integer || simple == type::decimal || simple == type::boolean) {
        DART_ASSERT(raw.buffer != nullptr);
        return shim::launder(reinterpret_cast<primitive<T> const*>(raw.buffer));
      } else {
        throw type_error("dart::buffer is not a finalized primitive and cannot be accessed as such");
      }
    }

    template <template <class> class RefCount, class Callback>
    auto aggregate_deref(Callback&& cb, raw_element raw)
      -> std::common_type_t<
        decltype(std::forward<Callback>(cb)(std::declval<object<RefCount>&>())),
        decltype(std::forward<Callback>(cb)(std::declval<array<RefCount>&>()))
      >
    {
      switch (raw.type) {
        case raw_type::object:
          return std::forward<Callback>(cb)(*get_object<RefCount>(raw));
        case raw_type::array:
          return std::forward<Callback>(cb)(*get_array<RefCount>(raw));
        default:
          throw type_error("dart::buffer is not a finalized aggregate and cannot be accessed as such");
      }
    }

    template <class Callback>
    auto string_deref(Callback&& cb, raw_element raw)
      -> std::common_type_t<
        decltype(std::forward<Callback>(cb)(std::declval<string&>())),
        decltype(std::forward<Callback>(cb)(std::declval<big_string&>()))
      >
    {
      switch (raw.type) {
        case raw_type::small_string:
        case raw_type::string:
          return std::forward<Callback>(cb)(*get_string(raw));
        case raw_type::big_string:
          return std::forward<Callback>(cb)(*get_big_string(raw));
        default:
          throw type_error("dart::buffer is not a finalized string and cannot be accessed as such");
      }
    }

    template <class Callback>
    auto integer_deref(Callback&& cb, raw_element raw)
      -> std::common_type_t<
        decltype(std::forward<Callback>(cb)(std::declval<primitive<int16_t>&>())),
        decltype(std::forward<Callback>(cb)(std::declval<primitive<int32_t>&>())),
        decltype(std::forward<Callback>(cb)(std::declval<primitive<int64_t>&>()))
      >
    {
      switch (raw.type) {
        case raw_type::short_integer:
          return std::forward<Callback>(cb)(*get_primitive<int16_t>(raw));
        case raw_type::integer:
          return std::forward<Callback>(cb)(*get_primitive<int32_t>(raw));
        case raw_type::long_integer:
          return std::forward<Callback>(cb)(*get_primitive<int64_t>(raw));
        default:
          throw type_error("dart::buffer is not a finalized integer and cannot be accessed as such");
      }
    }

    template <class Callback>
    auto decimal_deref(Callback&& cb, raw_element raw)
      -> std::common_type_t<
        decltype(std::forward<Callback>(cb)(std::declval<primitive<float>&>())),
        decltype(std::forward<Callback>(cb)(std::declval<primitive<double>&>()))
      >
    {
      switch (raw.type) {
        case raw_type::decimal:
          return std::forward<Callback>(cb)(*get_primitive<float>(raw));
        case raw_type::long_decimal:
          return std::forward<Callback>(cb)(*get_primitive<double>(raw));
        default:
          throw type_error("dart::buffer is not a finalized decimal and cannot be accessed as such");
      }
    }

    template <class Callback>
    auto match_generic(Callback&& cb, raw_type raw)
      -> std::common_type_t<
        decltype(std::forward<Callback>(cb)(object_tag {})),
        decltype(std::forward<Callback>(cb)(array_tag {})),
        decltype(std::forward<Callback>(cb)(small_string_tag {})),
        decltype(std::forward<Callback>(cb)(big_string_tag {})),
        decltype(std::forward<Callback>(cb)(short_integer_tag {})),
        decltype(std::forward<Callback>(cb)(medium_integer_tag {})),
        decltype(std::forward<Callback>(cb)(long_integer_tag {})),
        decltype(std::forward<Callback>(cb)(short_decimal_tag {})),
        decltype(std::forward<Callback>(cb)(long_decimal_tag {})),
        decltype(std::forward<Callback>(cb)(boolean_tag {})),
        decltype(std::forward<Callback>(cb)(null_tag {}))
      >
    {
      switch (raw) {
        case raw_type::object:
          return std::forward<Callback>(cb)(object_tag {});
        case raw_type::array:
          return std::forward<Callback>(cb)(array_tag {});
        case raw_type::small_string:
        case raw_type::string:
          return std::forward<Callback>(cb)(small_string_tag {});
        case raw_type::big_string:
          return std::forward<Callback>(cb)(big_string_tag {});
        case raw_type::short_integer:
          return std::forward<Callback>(cb)(short_integer_tag {});
        case raw_type::integer:
          return std::forward<Callback>(cb)(medium_integer_tag {});
        case raw_type::long_integer:
          return std::forward<Callback>(cb)(long_integer_tag {});
        case raw_type::decimal:
          return std::forward<Callback>(cb)(short_decimal_tag {});
        case raw_type::long_decimal:
          return std::forward<Callback>(cb)(long_decimal_tag {});
        case raw_type::boolean:
          return std::forward<Callback>(cb)(boolean_tag {});
        default:
          DART_ASSERT(raw == raw_type::null);
          return std::forward<Callback>(cb)(null_tag {});
      }
    }

    template <template <class> class RefCount, class Callback>
    auto generic_deref(Callback&& cb, raw_element raw)
      -> std::common_type_t<
        decltype(aggregate_deref<RefCount>(std::forward<Callback>(cb), raw)),
        decltype(string_deref(std::forward<Callback>(cb), raw)),
        decltype(integer_deref(std::forward<Callback>(cb), raw)),
        decltype(decimal_deref(std::forward<Callback>(cb), raw)),
        decltype(std::forward<Callback>(cb)(std::declval<primitive<bool>&>()))
      >
    {
      switch (raw.type) {
        case raw_type::object:
        case raw_type::array:
          return aggregate_deref<RefCount>(std::forward<Callback>(cb), raw);
        case raw_type::small_string:
        case raw_type::string:
        case raw_type::big_string:
          return string_deref(std::forward<Callback>(cb), raw);
        case raw_type::short_integer:
        case raw_type::integer:
        case raw_type::long_integer:
          return integer_deref(std::forward<Callback>(cb), raw);
        case raw_type::decimal:
        case raw_type::long_decimal:
          return decimal_deref(std::forward<Callback>(cb), raw);
        case raw_type::boolean:
          return std::forward<Callback>(cb)(*get_primitive<bool>(raw));
        default:
          DART_ASSERT(raw.type == raw_type::null);
          throw type_error("dart::buffer is null, and has no value to access");
      }
    }

    // Returns the native alignment requirements of the given type.
    template <template <class> class RefCount>
    constexpr size_t alignment_of(raw_type type) noexcept {
      return match_generic([] (auto v) { return decltype(v)::alignment; }, type);
    }

    // Function takes a pointer and returns a pointer to the "next" (greater than OR equal to) boundary
    // conforming to the native alignment of the given type.
    // Assumes that all alignment requests are for powers of two.
    template <template <class> class RefCount, class T>
    constexpr T* align_pointer(T* ptr, raw_type type) noexcept {
      // Get the required alignment for a pointer of this type.
      auto alignment = alignment_of<RefCount>(type);

      // Bump forward to the next boundary of the given alignment requirement.
      uintptr_t offset = reinterpret_cast<uintptr_t>(ptr);
      return reinterpret_cast<T*>((offset + (alignment - 1)) & ~(alignment - 1));
    }

    template <template <class> class RefCount, class T>
    constexpr T pad_bytes(T bytes, raw_type type) noexcept {
      // Get the required alignment for a pointer of this type.
      auto alignment = alignment_of<RefCount>(type);

      // Pad the given bytes to ensure its end lands on an alignment boundary.
      return (bytes + (alignment - 1)) & ~(alignment - 1);
    }

    template <template <class> class RefCount>
    size_t find_sizeof(raw_element elem) noexcept {
      if (elem.type != raw_type::null) {
        return generic_deref<RefCount>([] (auto& v) { return v.get_sizeof(); }, elem);
      } else {
        return 0;
      }
    }

    template <template <class> class RefCount>
    bool buffer_equal(raw_element lhs, raw_element rhs) {
      auto lhs_size = find_sizeof<RefCount>(lhs), rhs_size = find_sizeof<RefCount>(rhs);
      if (lhs_size == rhs_size) {
        return std::equal(lhs.buffer, lhs.buffer + lhs_size, rhs.buffer);
      } else {
        return false;
      }
    }

    template <template <class> class RefCount>
    constexpr size_t sso_bytes() {
      return basic_heap<RefCount>::sso_bytes;
    }

    template <template <class> class RefCount>
    raw_type identify_string(shim::string_view val) noexcept {
      if (val.size() > std::numeric_limits<uint16_t>::max()) {
        return detail::raw_type::big_string;
      } else if (val.size() > sso_bytes<RefCount>()) {
        return detail::raw_type::string;
      } else {
        return detail::raw_type::small_string;
      }
    }

    constexpr raw_type identify_integer(int64_t val) noexcept {
      if (val > INT32_MAX || val < INT32_MIN) return raw_type::long_integer;
      else if (val > INT16_MAX || val < INT16_MAX) return raw_type::integer;
      else return raw_type::short_integer;
    }

    // Returns the smallest floating point type capable of precisely representing the given
    // value.
    constexpr raw_type identify_decimal(double val) noexcept {
      if (val != static_cast<float volatile>(val)) return raw_type::long_decimal;
      else return raw_type::decimal;
    }

// Dart is header-only, so I think the scenarios where the ABI stability of
// symbol mangling will be relevant are relatively few.
#if DART_USING_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wnoexcept-type"
#elif DART_USING_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wc++17-compat-mangling"
#pragma clang diagnostic ignored "-Wc++1z-compat"
#endif

    template <class Packet, class T>
    T safe_optional_access(Packet const& that, T opt,
        bool (Packet::* guard) () const noexcept, T (Packet::* accessor) () const) {
      if (!(that.*guard)()) {
        return opt;
      } else try {
        return (that.*accessor)();
      } catch (...) {
        return opt;
      }
    }

#if DART_USING_GCC
#pragma GCC diagnostic pop
#elif DART_USING_CLANG
#pragma clang diagnostic pop
#endif

    template <template <class> class RefCount, class Owner = buffer_refcount_type<RefCount>, class Callback>
    Owner aligned_alloc(size_t bytes, raw_type type, Callback&& cb) {
      // Make an aligned allocation.
      gsl::byte* tmp;
      int retval = posix_memalign(reinterpret_cast<void**>(&tmp), alignment_of<RefCount>(type), bytes);
      if (retval) throw std::bad_alloc();

      // Associate it with an owner in case anything goes wrong.
      Owner ref {tmp, +[] (gsl::byte const* ptr) { free(const_cast<gsl::byte*>(ptr)); }};

      // Hand out mutable access and return.
      cb(tmp);
      return ref;
    }

    template <template <class> class RefCount, size_t static_elems = 8, class Spannable, class Callback>
    decltype(auto) sort_spannable(Spannable const& elems, Callback&& cb) {
      using value_type = typename Spannable::value_type;

      // Check if we can perform the sorting statically.
      dart_comparator<RefCount> comp;
      auto const size = static_cast<size_t>(elems.size());
      if (size <= static_elems) {
        // We've got enough static space, setup a local array.
        std::array<value_type const*, static_elems> ptrs;
        std::transform(std::begin(elems), std::end(elems), std::begin(ptrs), [] (auto& e) { return &e; });

        // Sort the pointers and pass back to the user.
        std::sort(ptrs.data(), ptrs.data() + size, [&] (auto* lhs, auto* rhs) { return comp(*lhs, *rhs); });
        return cb(gsl::make_span(ptrs.data(), size));
      } else {
        // Dynamic fallback.
        std::vector<value_type const*> ptrs(size, nullptr);
        std::transform(std::begin(elems), std::end(elems), std::begin(ptrs), [] (auto& e) { return &e; });

        // Sort and pass back.
        std::sort(std::begin(ptrs), std::end(ptrs), [&] (auto* lhs, auto* rhs) { return comp(*lhs, *rhs); });
        return cb(gsl::make_span(ptrs));
      }
    }

#ifdef DART_USE_SAJSON
    // FIXME: Find somewhere better to put these functions.
    template <template <class> class RefCount>
    raw_type json_identify(sajson::value curr_val) {
      switch (curr_val.get_type()) {
        case sajson::TYPE_OBJECT:
          return raw_type::object;
        case sajson::TYPE_ARRAY:
          return raw_type::array;
        case sajson::TYPE_STRING:
          // Figure out what type of string we've been given.
          return identify_string<RefCount>({curr_val.as_cstring(), curr_val.get_string_length()});
        case sajson::TYPE_INTEGER:
          return identify_integer(curr_val.get_integer_value());
        case sajson::TYPE_DOUBLE:
          return identify_decimal(curr_val.get_double_value());
        case sajson::TYPE_FALSE:
        case sajson::TYPE_TRUE:
          return raw_type::boolean;
        default:
          DART_ASSERT(curr_val.get_type() == sajson::TYPE_NULL);
          return raw_type::null;
      }
    }

    template <template <class> class RefCount>
    size_t json_lower(gsl::byte* buffer, sajson::value curr_val) {
      auto raw = json_identify<RefCount>(curr_val);
      switch (raw) {
        case raw_type::object:
          new(buffer) object<RefCount>(curr_val);
          break;
        case raw_type::array:
          new(buffer) array<RefCount>(curr_val);
          break;
        case raw_type::small_string:
        case raw_type::string:
          new(buffer) string(curr_val.as_cstring(), curr_val.get_string_length());
          break;
        case raw_type::big_string:
          new(buffer) big_string(curr_val.as_cstring(), curr_val.get_string_length());
          break;
        case raw_type::short_integer:
          new(buffer) primitive<int16_t>(curr_val.get_integer_value());
          break;
        case raw_type::integer:
          new(buffer) primitive<int32_t>(curr_val.get_integer_value());
          break;
        case raw_type::long_integer:
          new(buffer) primitive<int64_t>(curr_val.get_integer_value());
          break;
        case raw_type::decimal:
          new(buffer) primitive<float>(curr_val.get_double_value());
          break;
        case raw_type::long_decimal:
          new(buffer) primitive<double>(curr_val.get_double_value());
          break;
        case raw_type::boolean:
          new(buffer) detail::primitive<bool>((curr_val.get_type() == sajson::TYPE_TRUE) ? true : false);
          break;
        default:
          DART_ASSERT(curr_val.get_type() == sajson::TYPE_NULL);
          break;
      }
      return detail::find_sizeof<RefCount>({raw, buffer});
    }
#endif

#if DART_HAS_RAPIDJSON
    // FIXME: Find somewhere better to put these functions.
    template <template <class> class RefCount>
    raw_type json_identify(rapidjson::Value const& curr_val) {
      switch (curr_val.GetType()) {
        case rapidjson::kObjectType:
          return raw_type::object;
        case rapidjson::kArrayType:
          return raw_type::array;
        case rapidjson::kStringType:
          // Figure out what type of string we've been given.
          return identify_string<RefCount>({curr_val.GetString(), curr_val.GetStringLength()});
        case rapidjson::kNumberType:
          // Figure out what type of number we've been given.
          if (curr_val.IsInt()) return identify_integer(curr_val.GetInt64());
          else return identify_decimal(curr_val.GetDouble());
        case rapidjson::kTrueType:
        case rapidjson::kFalseType:
          return raw_type::boolean;
        default:
          DART_ASSERT(curr_val.IsNull());
          return raw_type::null;
      }
    }

    template <template <class> class RefCount>
    size_t json_lower(gsl::byte* buffer, rapidjson::Value const& curr_val) {
      auto raw = json_identify<RefCount>(curr_val);
      switch (raw) {
        case raw_type::object:
          new(buffer) object<RefCount>(curr_val);
          break;
        case raw_type::array:
          new(buffer) array<RefCount>(curr_val);
          break;
        case raw_type::small_string:
        case raw_type::string:
          new(buffer) string(curr_val.GetString(), curr_val.GetStringLength());
          break;
        case raw_type::big_string:
          new(buffer) big_string(curr_val.GetString(), curr_val.GetStringLength());
          break;
        case raw_type::short_integer:
          new(buffer) primitive<int16_t>(curr_val.GetInt());
          break;
        case raw_type::integer:
          new(buffer) primitive<int32_t>(curr_val.GetInt());
          break;
        case raw_type::long_integer:
          new(buffer) primitive<int64_t>(curr_val.GetInt64());
          break;
        case raw_type::decimal:
          new(buffer) primitive<float>(curr_val.GetFloat());
          break;
        case raw_type::long_decimal:
          new(buffer) primitive<double>(curr_val.GetDouble());
          break;
        case raw_type::boolean:
          new(buffer) detail::primitive<bool>(curr_val.GetBool());
          break;
        default:
          DART_ASSERT(curr_val.IsNull());
          break;
      }
      return detail::find_sizeof<RefCount>({raw, buffer});
    }
#endif

    // Helper function handles the edge case where we're working with
    // a view type, and we need to cast it back to an owner, ONLY IF
    // we are an owner ourselves.
    template <class MaybeView, class View>
    decltype(auto) view_return_indirection(View&& view) {
      return shim::compose_together(
        [] (auto&& v, std::true_type) -> decltype(auto) {
          return std::forward<decltype(v)>(v);
        },
        [] (auto&& v, std::false_type) -> decltype(auto) {
          return std::forward<decltype(v)>(v).as_owner();
        }
      )(std::forward<View>(view), std::is_same<std::decay_t<MaybeView>, std::decay_t<View>> {});
    }

    template <class Packet>
    Packet get_nested_impl(Packet haystack, shim::string_view needle, char separator) {
      // Spin through the provided needle until we reach the end, or hit a leaf.
      auto start = needle.begin();
      typename Packet::view curr = haystack;
      while (start < needle.end() && curr.is_object()) {
        // Tokenize up the needle and drag the current packet through each one.
        auto stop = std::find(start, needle.end(), separator);
        curr = curr[needle.substr(start - needle.begin(), stop - start)];

        // Prepare for next iteration.
        stop == needle.end() ? start = stop : start = stop + 1;
      }

      // If we finished, find our final value, otherwise return null.
      if (start < needle.end()) return Packet::make_null();
      else return view_return_indirection<Packet>(curr);
    }

    template <class Packet>
    std::vector<Packet> keys_impl(Packet const& that) {
      std::vector<Packet> packets;
      packets.reserve(that.size());
      for (auto it = that.key_begin(); it != that.key_end(); ++it) {
        packets.push_back(*it);
      }
      return packets;
    }

    template <class Packet>
    std::vector<Packet> values_impl(Packet const& that) {
      std::vector<Packet> packets;
      packets.reserve(that.size());
      for (auto entry : that) packets.push_back(std::move(entry));
      return packets;
    }

    template <class T>
    decltype(auto) maybe_dereference(T&& maybeptr, std::true_type) {
      return *std::forward<T>(maybeptr);
    }
    template <class T>
    decltype(auto) maybe_dereference(T&& maybeptr, std::false_type) {
      return std::forward<T>(maybeptr);
    }

  }

}

// Include main header file for all implementation files.
#include "../dart.h"
#include "common.tcc"

#endif
