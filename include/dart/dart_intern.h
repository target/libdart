#ifndef DART_INTERN_H
#define DART_INTERN_H

// Automatically detect libraries if possible.
#if defined(__has_include)
# ifndef DART_HAS_RAPIDJSON
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
#include <cstdlib>
#include <unistd.h>
#include <stdarg.h>
#include <limits.h>
#include <algorithm>

#if DART_HAS_RAPIDJSON
#include <rapidjson/reader.h>
#include <rapidjson/writer.h>
#endif

/*----- Local Includes -----*/

#include "dart_shim.h"
#include "dart_meta.h"
#include "ordered.h"

/*----- Macro Definitions -----*/

#define DART_FROM_THIS                                                                                        \
  reinterpret_cast<gsl::byte const*>(this)

#define DART_FROM_THIS_MUT                                                                                    \
  reinterpret_cast<gsl::byte*>(this)

#define DART_STRINGIFY_IMPL(x) #x
#define DART_STRINGIFY(x) DART_STRINGIFY_IMPL(x)

#ifndef NDEBUG
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

  struct fatal_error : std::runtime_error {
    fatal_error(char const* msg) : runtime_error(msg) {}
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
    enum class type {
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
    enum class raw_type {
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
     *  Helper-struct for sorting `dart::packet`s within `std::map`s.
     */
    template <template <class> class RefCount>
    struct map_comparator {
      using is_transparent = shim::string_view;
      bool operator ()(basic_heap<RefCount> const& lhs, shim::string_view rhs) const;
      bool operator ()(shim::string_view lhs, basic_heap<RefCount> const& rhs) const;
      bool operator ()(basic_heap<RefCount> const& lhs, basic_heap<RefCount> const& rhs) const;
    };

    /**
     *  @brief
     *  Helper-struct for safely comparing objects of any type.
     */
    struct typeless_comparator {
      template <class Lhs, class Rhs>
      bool operator ()(Lhs&& lhs, Rhs&& rhs) const noexcept;
    };

    template <class>
    class prefix_entry;
    template <class>
    struct table_layout {
      alignas(4) little_order<uint32_t> meta;
    };
    template <class Prefix>
    struct table_layout<prefix_entry<Prefix>> {
      static_assert(sizeof(Prefix) <= 4,
          "vtable prefix caching can use at most 4 bytes");
      alignas(4) little_order<uint32_t> meta;
      alignas(4) Prefix prefix;
    };

    // Class implements an entry in the vtable of an object or array, encoding the byte-offset
    // that the particular element begins at, along with its type.
    template <class T>
    class vtable_entry {

      public:

        /*----- Lifecycle Functions -----*/

        vtable_entry(detail::raw_type type, uint32_t offset);
        vtable_entry(vtable_entry const&) = default;

        /*----- Operators -----*/

        vtable_entry& operator =(vtable_entry const&) = default;

        /*----- Public API -----*/

        detail::raw_type get_type() const noexcept;
        uint32_t get_offset() const noexcept;

        /*----- Public Members -----*/

        static constexpr int offset_bits = 24;
        static constexpr uint32_t offset_mask = (1 << offset_bits) - 1;

      protected:

        /*----- Protected Members -----*/

        table_layout<T> layout;

    };


    template <class Prefix>
    class prefix_entry : public vtable_entry<prefix_entry<Prefix>> {

      public:

        /*----- Public Types -----*/

        using prefix_type = Prefix;

        /*----- Lifecycle Functions -----*/

        prefix_entry() noexcept = default;
        prefix_entry(detail::raw_type type, uint32_t offset, shim::string_view prefix) noexcept;
        prefix_entry(prefix_entry const&) = default;

        /*----- Operators -----*/

        prefix_entry& operator =(prefix_entry const&) = default;

        /*----- Public API -----*/

        int prefix_compare(shim::string_view str) const noexcept;

      private:

        /*----- Private Types -----*/

        using storage_t = std::aligned_storage_t<sizeof(prefix_type), 4>;

    };

    using array_entry = vtable_entry<void>;
    using object_entry = prefix_entry<uint16_t>;
    static_assert(std::is_standard_layout<array_entry>::value, "dart library is misconfigured");
    static_assert(std::is_standard_layout<object_entry>::value, "dart library is misconfigured");

    // Aliases for STL structures.
    template <template <class> class RefCount>
    using packet_elements = std::vector<basic_heap<RefCount>>;
    template <template <class> class RefCount>
    using packet_fields = std::map<basic_heap<RefCount>, basic_heap<RefCount>, map_comparator<RefCount>>;

    // Struct encodes all state information about what this packet is, and where, if anywhere, its
    // network buffer is.
    struct raw_element {
      raw_type type;
      gsl::byte const* buffer;
    };

    // Used by aggregate classes as a thin-wrapper for iteration over vtables.
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

    template <template <class> class RefCount>
    struct dn_iterator {

      /*----- Types -----*/

      using value_type = basic_heap<RefCount>;
      using fields_deref_func = value_type (typename packet_fields<RefCount>::const_iterator const&);
      using elements_deref_func = value_type (typename packet_elements<RefCount>::const_iterator const&);

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

      auto operator *() const noexcept -> value_type;

      /*----- Members -----*/

      implerator impl;

    };

    template <template <class> class RefCount>
    struct null_iterator {

      /*----- Types -----*/

      using value_type = basic_packet<RefCount>;

      /*----- Lifecycle Functions -----*/

      null_iterator() noexcept = default;
      null_iterator(null_iterator const&) = default;
      null_iterator(null_iterator&&) noexcept = default;
      ~null_iterator() = default;

      /*----- Operators -----*/

      null_iterator& operator =(null_iterator const&) = default;
      null_iterator& operator =(null_iterator&&) noexcept = default;

      bool operator ==(null_iterator const&) const noexcept;
      bool operator !=(null_iterator const& other) const noexcept;

      auto operator ++() noexcept -> null_iterator&;
      auto operator --() noexcept -> null_iterator&;
      auto operator ++(int) noexcept -> null_iterator;
      auto operator --(int) noexcept -> null_iterator;

      auto operator *() const noexcept -> value_type;

    };

    // Class represents the contents of a packet object in the network buffer.
    template <template <class> class RefCount>
    class object {

      public:

        /*----- Lifecycle Functions -----*/

        object() = delete;
        object(packet_fields<RefCount> const* fields) noexcept;
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

        auto get_key(shim::string_view const key) const noexcept -> raw_element;
        auto get_value(shim::string_view const key) const noexcept -> raw_element;

        static auto load_key(gsl::byte const* base, size_t idx) noexcept -> typename ll_iterator<RefCount>::value_type;
        static auto load_value(gsl::byte const* base, size_t idx) noexcept -> typename ll_iterator<RefCount>::value_type;

        /*----- Public Members -----*/

        static constexpr auto alignment = sizeof(int64_t);

      private:

        /*----- Private Helpers -----*/

        object_entry* vtable() noexcept;
        object_entry const* vtable() const noexcept;

        /*----- Private Members -----*/

        alignas(4) little_order<uint32_t> bytes;
        alignas(4) little_order<uint32_t> elems;

    };
    static_assert(std::is_standard_layout<object<std::shared_ptr>>::value, "dart library is misconfigured");

    // Class represents the contents of a packet array in the network buffer.
    template <template <class> class RefCount>
    class array {

      public:

        /*----- Lifecycle Functions -----*/

        array() = delete;
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

        auto get_elem(size_t index) const -> raw_element;

        static auto load_elem(gsl::byte const* base, size_t idx) noexcept -> typename ll_iterator<RefCount>::value_type;

        /*----- Public Members -----*/

        static constexpr auto alignment = sizeof(int64_t);

      private:

        /*----- Private Helpers -----*/

        array_entry* vtable() noexcept;
        array_entry const* vtable() const noexcept;

        /*----- Private Members -----*/

        alignas(4) little_order<uint32_t> bytes;
        alignas(4) little_order<uint32_t> elems;

    };
    static_assert(std::is_standard_layout<array<std::shared_ptr>>::value, "dart library is misconfigured");

    // Class represents the contents of a packet string in the network buffer.
    template <class SizeType>
    class basic_string {

      public:

        /*----- Public Types -----*/

        using size_type = SizeType;

        /*----- Lifecycle Functions -----*/

        basic_string() = delete;
        basic_string(char const* str, size_t len) noexcept;
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

    // Class template can represent the contents of any primitive (int, double, bool, etc) packet
    // in the network buffer.
    template <class T>
    class primitive {

      public:

        /*----- Lifecycle Functions -----*/

        primitive() = delete;
        primitive(T data) noexcept : data(data) {}
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

    // Used for tag dispatch from factory functions.
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

    // Maps from raw_type to public type.
    // This is implemented as an inline switch table purely for performance reasons.
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

    template <template <class> class RefCount>
    object<RefCount> const* get_object(raw_element raw) {
      if (simplify_type(raw.type) == type::object) {
        DART_ASSERT(raw.buffer != nullptr);
        return shim::launder(reinterpret_cast<object<RefCount> const*>(raw.buffer));
      } else {
        throw type_error("dart::buffer is not a finalized object and cannot be accessed as such");
      }
    }

    template <template <class> class RefCount>
    array<RefCount> const* get_array(raw_element raw) {
      if (simplify_type(raw.type) == type::array) {
        DART_ASSERT(raw.buffer != nullptr);
        return shim::launder(reinterpret_cast<array<RefCount> const*>(raw.buffer));
      } else {
        throw type_error("dart::buffer is not a finalized array and cannot be accessed as such");
      }
    }

    inline string const* get_string(raw_element raw) {
      if (raw.type == raw_type::small_string || raw.type == raw_type::string) {
        DART_ASSERT(raw.buffer != nullptr);
        return shim::launder(reinterpret_cast<string const*>(raw.buffer));
      } else {
        throw type_error("dart::buffer is not a finalized string and cannot be accessed as such");
      }
    }

    inline big_string const* get_big_string(raw_element raw) {
      if (raw.type == raw_type::big_string) {
        DART_ASSERT(raw.buffer != nullptr);
        return shim::launder(reinterpret_cast<big_string const*>(raw.buffer));
      } else {
        throw type_error("dart::buffer is not a finalized string and cannot be accessed as such");
      }
    }

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
      return match_generic(
        shim::compose_together(
          [&] (meta::types<object_tag, array_tag>) {
            return aggregate_deref<RefCount>(std::forward<Callback>(cb), raw);
          },
          [&] (string_tag) {
            return string_deref(std::forward<Callback>(cb), raw);
          },
          [&] (integer_tag) {
            return integer_deref(std::forward<Callback>(cb), raw);
          },
          [&] (decimal_tag) {
            return decimal_deref(std::forward<Callback>(cb), raw);
          },
          [&] (boolean_tag) {
            return std::forward<Callback>(cb)(*get_primitive<bool>(raw));
          },
          [] (null_tag) -> int {
            throw type_error("dart::buffer is null, and has no value to access");
          }
        )
      , raw.type);
    }

    // Functions enforce invariant that keys must be strings.
    inline void require_string(shim::string_view val) {
      if (val.size() > std::numeric_limits<uint16_t>::max()) {
        throw std::invalid_argument("dart::packet keys cannot be longer than UINT16_MAX");
      }
    }
    template <template <template <class> class> class Packet, template <class> class RefCount>
    void require_string(Packet<RefCount> const& key) {
      if (!key.is_str()) {
        throw std::invalid_argument("dart::packet object keys must be strings.");
      } else if (key.size() > std::numeric_limits<uint16_t>::max()) {
        throw std::invalid_argument("dart::packet keys cannot be longer than UINT16_MAX");
      }
    }
    template <class Packet>
    void require_string(dart::basic_string<Packet> const& val) {
      if (val.size() > std::numeric_limits<uint16_t>::max()) {
        throw std::invalid_argument("dart::packet keys cannot be longer than UINT16_MAX");
      }
    }
    template <class B = std::false_type>
    void require_string(...) {
      static_assert(B::value, "dart::packet object keys must be strings.");
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

#if DART_USING_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wnoexcept-type"
#elif DART_USING_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++17-compat-mangling"
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

  }

}

// Include main header file for all implementation files.
#include "../dart.h"

#endif
