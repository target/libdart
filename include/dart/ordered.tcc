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
#define DART_SWAP_BYTES(val, bits)                                                                          \
  [&] {                                                                                                     \
    auto* __attribute__ ((__may_alias__)) punned = reinterpret_cast<uint##bits##_t*>(&val);                 \
    auto swapped = __builtin_bswap##bits(*punned);                                                          \
    decltype(val)* __attribute__ ((__may_alias__)) retval = reinterpret_cast<decltype(val)*>(&swapped);     \
    return *retval;                                                                                         \
  }()

/*----- Function Implementations -----*/

namespace dart {
  namespace detail {

    template <class T>
    T swapper<T>::swap(T val) noexcept {
      if (sizeof(val) == 8) {
        return DART_SWAP_BYTES(val, 64);
      } else if (sizeof(val) == 4) {
        return DART_SWAP_BYTES(val, 32);
      } else if (sizeof(val) == 2) {
        return DART_SWAP_BYTES(val, 16);
      } else {
        return val;
      }
    }
#undef DART_SWAP_BYTES

    template <class T>
    T* swapper<T*>::swap(T* val) noexcept {
      // This is pretty bad too.
      return reinterpret_cast<T*>(__builtin_bswap64(reinterpret_cast<uintptr_t>(val)));
    }

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
      return increment(op);
    }

    template <class T, class O>
    template <class Operand, class>
    auto ordered<T, O>::operator -=(Operand op) noexcept -> ordered& {
      return increment(-op);
    }

    template <class T, class O>
    template <class Operand, class>
    auto ordered<T, O>::operator *=(Operand op) noexcept -> ordered& {
      return scale(op);
    }

    template <class T, class O>
    template <class Operand, class>
    auto ordered<T, O>::operator /=(Operand op) noexcept -> ordered& {
      return shrink(op);
    }

    template <class T, class O>
    template <class Operand, class>
    auto ordered<T, O>::operator &=(Operand op) noexcept -> ordered& {
      return mask(op);
    }

    template <class T, class O>
    template <class Operand, class>
    auto ordered<T, O>::operator |=(Operand op) noexcept -> ordered& {
      return fill(op);
    }

    template <class T, class O>
    template <class Operand, class>
    auto ordered<T, O>::operator ^=(Operand op) noexcept -> ordered& {
      return flip(op);
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
      return should_swap() ? perform_swap(managed) : managed;
    }

    template <class T, class O>
    auto ordered<T, O>::set(value_type val) noexcept -> value_type {
      if (should_swap()) managed = perform_swap(val);
      else managed = val;
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
    constexpr auto ordered<T, O>::perform_swap(value_type val) noexcept -> value_type {
      return swapper<T>::swap(val);
    }

  }
}
#endif
