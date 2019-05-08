#ifndef DART_OPTIONAL_IMPL_H
#define DART_OPTIONAL_IMPL_H

/*----- Local Includes -----*/

#include "optional.h"

/*----- Template Implementations -----*/

namespace dart {

#ifndef DART_HAS_CPP17
  namespace shim {
    template <class T>
    constexpr T* launder(T* ptr) noexcept {
      return ptr;
    }
  }
#endif

  template <class T>
  optional<T>::optional(copyable_t const& other) : is_set(false) {
    if (other) construct(*other);
  }

  template <class T>
  optional<T>::optional(moveable_t&& other) noexcept(std::is_nothrow_move_constructible<T>::value) : is_set(false) {
    if (other) construct(std::move(*other));
  }

  template <class T>
  template <class U,
    typename std::enable_if<
      std::is_constructible<T, U const&>::value
      &&
      detail::constructor_conversion_check<T, U>::value
      &&
      !std::is_convertible<U const&, T>::value,
      bool
    >::type
  >
  optional<T>::optional(optional<U> const& other) : is_set(false) {
    if (other) construct(*other);
  }

  template <class T>
  template <class U,
    typename std::enable_if<
      std::is_constructible<T, U const&>::value
      &&
      detail::constructor_conversion_check<T, U>::value
      &&
      std::is_convertible<U const&, T>::value,
      bool
    >::type
  >
  optional<T>::optional(optional<U> const& other) : is_set(false) {
    if (other) construct(*other);
  }

  template <class T>
  template <class U,
    typename std::enable_if<
      std::is_constructible<T, U&&>::value
      &&
      detail::constructor_conversion_check<T, U>::value
      &&
      !std::is_convertible<U&&, T>::value,
      bool
    >::type
  >
  optional<T>::optional(optional<U>&& other) : is_set(false) {
    if (other) construct(std::move(*other));
  }

  template <class T>
  template <class U,
    typename std::enable_if<
      std::is_constructible<T, U&&>::value
      &&
      detail::constructor_conversion_check<T, U>::value
      &&
      std::is_convertible<U&&, T>::value,
      bool
    >::type
  >
  optional<T>::optional(optional<U>&& other) : is_set(false) {
    if (other) construct(std::move(*other));
  }

  template <class T>
  template <class U,
    typename std::enable_if<
      std::is_constructible<T, U&&>::value
      &&
      !std::is_same<
        optional<T>,
        std::decay_t<U>
      >::value
      &&
      !std::is_convertible<U&&, T>::value,
      bool
    >::type
  >
  optional<T>::optional(U&& val) : is_set(false) {
    construct(std::forward<U>(val));
  }

  template <class T>
  template <class U,
    typename std::enable_if<
      std::is_constructible<T, U&&>::value
      &&
      !std::is_same<
        optional<T>,
        std::decay_t<U>
      >::value
      &&
      std::is_convertible<U&&, T>::value,
      bool
    >::type
  >
  optional<T>::optional(U&& val) : is_set(false) {
    construct(std::forward<U>(val));
  }

  template <class T>
  optional<T>::~optional() noexcept {
    if (*this) destroy();
  }

  template <class T>
  optional<T>& optional<T>::operator =(nullopt_t) noexcept {
    reset();
    return *this;
  }

  template <class T>
  optional<T>& optional<T>::operator =(copy_assignable_t const& other) {
    if (this == &other) return *this;
    if (other && *this) assign(*other);
    else if (other) construct(*other);
    else if (*this) destroy();
    return *this;
  }

  template <class T>
  optional<T>& optional<T>::operator =(move_assignable_t&& other) noexcept(detail::safely_assignable<T>::value) {
    if (this == &other) return *this;
    if (other && *this) assign(std::move(*other));
    else if (other) construct(std::move(*other));
    else if (*this) destroy();
    return *this;
  }

  template <class T>
  template <class U, class>
  optional<T>& optional<T>::operator =(U&& val) {
    if (*this) assign(std::forward<U>(val));
    else construct(std::forward<U>(val));
    return *this;
  }

  template <class T>
  template <class U, class>
  optional<T>& optional<T>::operator =(optional<U> const& other) {
    if (other && *this) assign(*other);
    else if (other) construct(*other);
    else if (*this) destroy();
    return *this;
  }

  template <class T>
  template <class U, class>
  optional<T>& optional<T>::operator =(optional<U>&& other) {
    if (other && *this) assign(std::move(*other));
    else if (other) construct(std::move(*other));
    else if (*this) destroy();
    return *this;
  }

  template <class T>
  auto optional<T>::operator *() & noexcept -> value_type& {
    return *access();
  }

  template <class T>
  auto optional<T>::operator *() const& noexcept -> value_type const& {
    return *access();
  }

  template <class T>
  auto optional<T>::operator *() && noexcept -> value_type&& {
    return std::move(*access());
  }

  template <class T>
  auto optional<T>::operator *() const&& noexcept -> value_type const&& {
    return std::move(*access());
  }

  template <class T>
  auto optional<T>::operator ->() noexcept -> value_type* {
    return access();
  }

  template <class T>
  auto optional<T>::operator ->() const noexcept -> value_type const* {
    return access();
  }

  template <class T>
  optional<T>::operator bool() const noexcept {
    return has_value();
  }

  template <class T>
  bool optional<T>::has_value() const noexcept {
    return is_set;
  }

  template <class T>
  auto optional<T>::value() & -> value_type& {
    if (*this) return *access();
    else throw bad_optional_access {};
  }

  template <class T>
  auto optional<T>::value() const& -> value_type const& {
    if (*this) return *access();
    else throw bad_optional_access {};
  }

  template <class T>
  auto optional<T>::value() && -> value_type&& {
    if (*this) return *access();
    else throw bad_optional_access {};
  }

  template <class T>
  auto optional<T>::value() const&& -> value_type const&& {
    if (*this) return *access();
    else throw bad_optional_access {};
  }

  template <class T>
  template <class U, class>
  auto optional<T>::value_or(U&& def) const& -> value_type {
    return (*this) ? **this : std::forward<U>(def);
  }

  template <class T>
  template <class U, class>
  auto optional<T>::value_or(U&& def) && -> value_type {
    return (*this) ? std::move(**this) : std::forward<U>(def);
  }

  template <class T>
  void optional<T>::reset() noexcept {
    if (*this) destroy();
  }

  template <class T>
  template <class... Args>
  auto optional<T>::emplace(Args&&... the_args) -> value_type& {
    if (*this) destroy();
    construct(std::forward<Args>(the_args)...);
    return value();
  }

  template <class T>
  template <class... Args>
  void optional<T>::construct(Args&&... the_args) {
    assert(!is_set);
    new(&store) value_type(std::forward<Args>(the_args)...);
    is_set = true;
  }

  template <class T>
  template <class U>
  void optional<T>::assign(U&& other) {
    assert(is_set);
    *access() = std::forward<U>(other);
  }

  template <class T>
  void optional<T>::destroy() noexcept {
    assert(is_set);
    access()->~T();
    is_set = false;
  }

  template <class T>
  auto optional<T>::access() noexcept -> value_type* {
    return shim::launder(reinterpret_cast<T*>(&store));
  }

  template <class T>
  auto optional<T>::access() const noexcept -> value_type const* {
    return shim::launder(reinterpret_cast<T const*>(&store));
  }

}

#endif
