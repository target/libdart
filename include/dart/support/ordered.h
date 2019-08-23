#ifndef DART_ORDERED_VALUE_H
#define DART_ORDERED_VALUE_H

/*----- System Includes -----*/

#include <cstdint>
#include <type_traits>

/*----- Local Includes -----*/

#include "../meta.h"

/*----- Type Declarations -----*/

namespace dart {

  namespace detail {

    template <class T>
    struct is_arithmeticish {
      static constexpr auto value = std::is_arithmetic<T>::value || std::is_pointer<T>::value;
    };

    template <class T, class U>
    struct both_are_arithmetic {
      static constexpr auto value = std::is_arithmetic<T>::value && std::is_arithmetic<U>::value;
    };

    struct little_endian_tag {
      static constexpr auto value = __ORDER_LITTLE_ENDIAN__;
    };
    struct big_endian_tag {
      static constexpr auto value = __ORDER_BIG_ENDIAN__;
    };

    template <class T, class O>
    class ordered {

      static_assert(std::is_scalar<T>::value,
          "dart::ordered only supports scalar types");
      static_assert(sizeof(T) <= sizeof(int64_t),
          "dart::ordered only supports types from 8-64 bits.");

      public:

        /*----- Public Types -----*/

        using value_type = T;
        using order_type = O;

        /*----- Lifecycle Functions -----*/

        ordered() noexcept : managed() {}
        ordered(value_type val) noexcept;
        ordered(ordered const&) = default;
        ordered(ordered&&) = default;
        ~ordered() = default;

        /*----- Operators -----*/

        auto operator =(value_type val) noexcept -> ordered&;
        ordered& operator =(ordered const&) = default;
        ordered& operator =(ordered&&) = default;

        template <class U, class V, class =
          std::enable_if_t<
            std::is_convertible<value_type, U>::value
            &&
            std::is_convertible<U, value_type>::value
          >
        >
        bool operator ==(ordered<U, V> const& other) const noexcept;
        template <class U, class V, class =
          std::enable_if_t<
            std::is_convertible<value_type, U>::value
            &&
            std::is_convertible<U, value_type>::value
          >
        >
        bool operator !=(ordered<U, V> const& other) const noexcept;

        template <class Operand, class =
          std::enable_if_t<
            std::is_arithmetic<Operand>::value
            &&
            detail::is_arithmeticish<value_type>::value
          >
        >
        auto operator +=(Operand op) noexcept -> ordered&;
        template <class Operand, class =
          std::enable_if_t<
            std::is_arithmetic<Operand>::value
            &&
            detail::is_arithmeticish<value_type>::value
          >
        >
        auto operator -=(Operand op) noexcept -> ordered&;
        template <class Operand, class =
          std::enable_if_t<
            detail::both_are_arithmetic<value_type, Operand>::value
          >
        >
        auto operator *=(Operand op) noexcept -> ordered&;
        template <class Operand, class =
          std::enable_if_t<
            detail::both_are_arithmetic<value_type, Operand>::value
          >
        >
        auto operator /=(Operand op) noexcept -> ordered&;
        template <class Operand, class =
          std::enable_if_t<
            detail::both_are_arithmetic<value_type, Operand>::value
          >
        >
        auto operator &=(Operand op) noexcept -> ordered&;
        template <class Operand, class =
          std::enable_if_t<
            detail::both_are_arithmetic<value_type, Operand>::value
          >
        >
        auto operator |=(Operand op) noexcept -> ordered&;
        template <class Operand, class =
          std::enable_if_t<
            detail::both_are_arithmetic<value_type, Operand>::value
          >
        >
        auto operator ^=(Operand op) noexcept -> ordered&;

        template <class Value = value_type, class =
          std::enable_if_t<
            detail::is_arithmeticish<Value>::value
          >
        >
        auto operator ++() noexcept -> ordered&;
        template <class Value = value_type, class =
          std::enable_if_t<
            detail::is_arithmeticish<Value>::value
          >
        >
        auto operator --() noexcept -> ordered&;
        template <class Value = value_type, class =
          std::enable_if_t<
            detail::is_arithmeticish<Value>::value
          >
        >
        auto operator ++(int) noexcept -> value_type;
        template <class Value = value_type, class =
          std::enable_if_t<
            detail::is_arithmeticish<Value>::value
          >
        >
        auto operator --(int) noexcept -> value_type;

        template <class Value = value_type, class =
          std::enable_if_t<
            std::is_pointer<Value>::value
          >
        >
        auto operator *() const noexcept -> std::remove_pointer_t<Value>;
        template <class Value = value_type, class =
          std::enable_if_t<
            std::is_pointer<Value>::value
          >
        >
        auto operator ->() const noexcept -> value_type;

        operator value_type() const noexcept;

        /*----- Public API -----*/

        // Available for all types.
        auto get() const noexcept -> value_type;
        auto set(value_type val) noexcept -> value_type;

        // Available for arithmetic-ish types.
        template <class Value = value_type, class =
          std::enable_if_t<
            detail::is_arithmeticish<Value>::value
          >
        >
        auto increment(value_type val) noexcept -> value_type;

        // Available for arithmetic types.
        template <class Value = value_type, class =
          std::enable_if_t<
            std::is_arithmetic<Value>::value
          >
        >
        auto scale(value_type val) noexcept -> value_type;
        template <class Value = value_type, class =
          std::enable_if_t<
            std::is_arithmetic<Value>::value
          >
        >
        auto shrink(value_type val) noexcept -> value_type;
        template <class Value = value_type, class =
          std::enable_if_t<
            std::is_arithmetic<Value>::value
          >
        >
        auto mask(value_type val) noexcept -> value_type;
        template <class Value = value_type, class =
          std::enable_if_t<
            std::is_arithmetic<Value>::value
          >
        >
        auto fill(value_type val) noexcept -> value_type;
        template <class Value = value_type, class =
          std::enable_if_t<
            std::is_arithmetic<Value>::value
          >
        >
        auto flip(value_type val) noexcept -> value_type;

      private:

        /*----- Private Types -----*/

        // FIXME: Come up something better than this.
        // Yuck, but it'll do for now.
        using storage_type = std::conditional_t<
          sizeof(T) == 1,
          uint8_t,
          std::conditional_t<
            sizeof(T) == 2,
            uint16_t,
            std::conditional_t<
              sizeof(T) == 4,
              uint32_t,
              uint64_t
            >
          >
        >;

        /*----- Private Helpers -----*/

        template <class Callback>
        auto mutate(Callback&& cb) noexcept -> value_type;

        void copy_in(value_type val) noexcept;
        auto copy_out() const noexcept -> value_type;

        void swap_in(value_type val) noexcept;
        auto swap_out() const noexcept -> value_type;

        static constexpr bool should_swap() noexcept;

        /*----- Private Members -----*/

        storage_type managed;

    };

  }

  template <class T>
  using little_order = detail::ordered<T, detail::little_endian_tag>;

  template <class T>
  using big_order = detail::ordered<T, detail::big_endian_tag>;

}

/*----- Template Implementations -----*/

#include "ordered.tcc"

#endif
