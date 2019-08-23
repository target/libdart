#ifndef DART_ORDERED_VALUE_IMPL_H
#define DART_ORDERED_VALUE_IMPL_H

/*----- Local Includes -----*/

#include "ordered.h"

/*----- Macro Definitions -----*/

// Boy this is ugly.
// Have to prevent the compiler from inserting
// conversions and truncating floating point values,
// but there's that nasty strict-aliasing rule, so this.
// C++, ladies and gentlemen.
#define DART_PUNNED_PTR auto* __attribute__ ((__may_alias__))

#define DART_COPY_IN(ptr, bits)                                                                     \
  [&] {                                                                                             \
    DART_PUNNED_PTR punned = reinterpret_cast<uint##bits##_t*>(ptr);                                \
    return *punned;                                                                                 \
  }()

#define DART_COPY_OUT(ptr, bits)                                                                    \
  [&] {                                                                                             \
    DART_PUNNED_PTR punned = reinterpret_cast<T const*>(ptr);                                       \
    return *punned;                                                                                 \
  }()

#define DART_SWAP_IN(ptr, bits)                                                                     \
  [&] {                                                                                             \
    DART_PUNNED_PTR punned = reinterpret_cast<uint##bits##_t*>(ptr);                                \
    return __builtin_bswap##bits(*punned);                                                          \
  }()

#define DART_SWAP_OUT(ptr, bits)                                                                    \
  [&] {                                                                                             \
    auto tmp = __builtin_bswap##bits(*ptr);                                                         \
    DART_PUNNED_PTR punned = reinterpret_cast<T*>(&tmp);                                            \
    return *punned;                                                                                 \
  }()

/*----- Function Implementations -----*/

namespace dart {
  namespace detail {

    template <class T, class O>
    ordered<T, O>::ordered(value_type val) noexcept {
      set(val);
    }

    template <class T, class O>
    auto ordered<T, O>::operator =(value_type val) noexcept -> ordered& {
      set(val);
      return *this;
    }

    template <class T, class O>
    template <class U, class V, class>
    bool ordered<T, O>::operator ==(ordered<U, V> const& other) const noexcept {
      return get() == other.get();
    }

    template <class T, class O>
    template <class U, class V, class>
    bool ordered<T, O>::operator !=(ordered<U, V> const& other) const noexcept {
      return !(*this == other);
    }

    template <class T, class O>
    template <class Operand, class>
    auto ordered<T, O>::operator +=(Operand op) noexcept -> ordered& {
      increment(op);
      return *this;
    }

    template <class T, class O>
    template <class Operand, class>
    auto ordered<T, O>::operator -=(Operand op) noexcept -> ordered& {
      increment(-op);
      return *this;
    }

    template <class T, class O>
    template <class Operand, class>
    auto ordered<T, O>::operator *=(Operand op) noexcept -> ordered& {
      scale(op);
      return *this;
    }

    template <class T, class O>
    template <class Operand, class>
    auto ordered<T, O>::operator /=(Operand op) noexcept -> ordered& {
      shrink(op);
      return *this;
    }

    template <class T, class O>
    template <class Operand, class>
    auto ordered<T, O>::operator &=(Operand op) noexcept -> ordered& {
      mask(op);
      return *this;
    }

    template <class T, class O>
    template <class Operand, class>
    auto ordered<T, O>::operator |=(Operand op) noexcept -> ordered& {
      fill(op);
      return *this;
    }

    template <class T, class O>
    template <class Operand, class>
    auto ordered<T, O>::operator ^=(Operand op) noexcept -> ordered& {
      flip(op);
      return *this;
    }

    template <class T, class O>
    template <class, class>
    auto ordered<T, O>::operator ++() noexcept -> ordered& {
      increment(1);
      return *this;
    }

    template <class T, class O>
    template <class, class>
    auto ordered<T, O>::operator --() noexcept -> ordered& {
      increment(-1);
      return *this;
    }

    template <class T, class O>
    template <class, class>
    auto ordered<T, O>::operator ++(int) noexcept -> value_type {
      auto that = get();
      increment(1);
      return that;
    }

    template <class T, class O>
    template <class, class>
    auto ordered<T, O>::operator --(int) noexcept -> value_type {
      auto that = get();
      increment(-1);
      return that;
    }

    template <class T, class O>
    template <class Value, class>
    auto ordered<T, O>::operator *() const noexcept -> std::remove_pointer_t<Value> {
      return *get();
    }

    template <class T, class O>
    template <class, class>
    auto ordered<T, O>::operator ->() const noexcept -> value_type {
      return get();
    }

    template <class T, class O>
    ordered<T, O>::operator value_type() const noexcept {
      return get();
    }

    template <class T, class O>
    auto ordered<T, O>::get() const noexcept -> value_type {
      return should_swap() ? swap_out() : copy_out();
    }

    template <class T, class O>
    auto ordered<T, O>::set(value_type val) noexcept -> value_type {
      if (should_swap()) swap_in(val);
      else copy_in(val);
      return val;
    }

    template <class T, class O>
    template <class, class>
    auto ordered<T, O>::increment(value_type val) noexcept -> value_type {
      return mutate([&] (auto v) { return v + val; });
    }

    template <class T, class O>
    template <class, class>
    auto ordered<T, O>::scale(value_type val) noexcept -> value_type {
      return mutate([&] (auto v) { return v * val; });
    }

    template <class T, class O>
    template <class, class>
    auto ordered<T, O>::shrink(value_type val) noexcept -> value_type {
      return mutate([&] (auto v) { return v / val; });
    }

    template <class T, class O>
    template <class, class>
    auto ordered<T, O>::mask(value_type val) noexcept -> value_type {
      return mutate([&] (auto v) { return v & val; });
    }

    template <class T, class O>
    template <class, class>
    auto ordered<T, O>::fill(value_type val) noexcept -> value_type {
      return mutate([&] (auto v) { return v | val; });
    }

    template <class T, class O>
    template <class, class>
    auto ordered<T, O>::flip(value_type val) noexcept -> value_type {
      return mutate([&] (auto v) { return v ^ val; });
    }

    template <class T, class O>
    template <class Callback>
    auto ordered<T, O>::mutate(Callback&& cb) noexcept -> value_type {
      return set(cb(get()));
    }

    template <class T, class O>
    constexpr bool ordered<T, O>::should_swap() noexcept {
      return order_type::value != __BYTE_ORDER__;
    }

    template <class T, class O>
    void ordered<T, O>::copy_in(value_type val) noexcept {
      if (sizeof(T) == 8) {
        managed = DART_COPY_IN(&val, 64);
      } else if (sizeof(T) == 4) {
        managed = DART_COPY_IN(&val, 32);
      } else if (sizeof(T) == 2) {
        managed = DART_COPY_IN(&val, 16);
      } else {
        managed = DART_COPY_IN(&val, 8);
      }
    }

    template <class T, class O>
    auto ordered<T, O>::copy_out() const noexcept -> value_type {
      if (sizeof(T) == 8) {
        return DART_COPY_OUT(&managed, 64);
      } else if (sizeof(T) == 4) {
        return DART_COPY_OUT(&managed, 32);
      } else if (sizeof(T) == 2) {
        return DART_COPY_OUT(&managed, 16);
      } else {
        return DART_COPY_OUT(&managed, 8);
      }
    }

    template <class T, class O>
    void ordered<T, O>::swap_in(value_type val) noexcept {
      if (sizeof(T) == 8) {
        managed = DART_SWAP_IN(&val, 64);
      } else if (sizeof(T) == 4) {
        managed = DART_SWAP_IN(&val, 32);
      } else if (sizeof(T) == 2) {
        managed = DART_SWAP_IN(&val, 16);
      } else {
        managed = DART_COPY_IN(&val, 8);
      }
    }

    template <class T, class O>
    auto ordered<T, O>::swap_out() const noexcept -> value_type {
      if (sizeof(T) == 8) {
        return DART_SWAP_OUT(&managed, 64);
      } else if (sizeof(T) == 4) {
        return DART_SWAP_OUT(&managed, 32);
      } else if (sizeof(T) == 2) {
        return DART_SWAP_OUT(&managed, 16);
      } else {
        return DART_COPY_OUT(&managed, 8);
      }
    }

  }
}
#endif
