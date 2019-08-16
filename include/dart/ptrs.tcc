#ifndef DART_POINTERS_IMPL_H
#define DART_POINTERS_IMPL_H

/*----- Local Includes -----*/

#include "ptrs.h"

/*----- Template Implementations -----*/

namespace dart {

  namespace detail {

    template <class T>
    void managed_ptr_deleter<T>::operator ()(T ptr) {
      delete ptr;
    }

    template <class T>
    void managed_ptr_deleter<T[]>::operator ()(std::remove_extent_t<T>* ptr) {
      delete[] ptr;
    }

    template <class Counter, class PtrType, class Deleter>
    void managed_ptr_eraser<Counter, PtrType, Deleter>::destroy() {
      deleter(reinterpret_cast<std::decay_t<PtrType>>(this->ptr));
    }

    template <class T, class Counter>
    template <class U, class>
    counted_ptr_base<T, Counter>::counted_ptr_base(counted_ptr_base<U, Counter> const& other) noexcept :
      value(other.value)
    {
      if (value) ++value->use_count;
    }

    template <class T, class Counter>
    template <class U, class>
    counted_ptr_base<T, Counter>::counted_ptr_base(counted_ptr_base<U, Counter>&& other) noexcept :
      value(other.value)
    {
      other.value = nullptr;
    }

    template <class T, class Counter>
    template <class U, class Deleter, class>
    counted_ptr_base<T, Counter>::counted_ptr_base(std::unique_ptr<U, Deleter>&& ptr) :
      value(
        new managed_ptr_eraser<
          Counter,
          ptr_type<typename std::unique_ptr<U, Deleter>::element_type>,
          Deleter
        >(ptr.get(), 1, std::move(ptr.get_deleter()))
      )
    {
      ptr.release();
    }

    template <class T, class Counter>
    counted_ptr_base<T, Counter>::counted_ptr_base(counted_ptr_base const& other) noexcept : value(other.value) {
      if (value) ++value->use_count;
    }

    template <class T, class Counter>
    counted_ptr_base<T, Counter>::counted_ptr_base(counted_ptr_base&& other) noexcept : value(other.value) {
      other.value = nullptr;
    }

    template <class T, class Counter>
    counted_ptr_base<T, Counter>::~counted_ptr_base() noexcept {
      if (value && --value->use_count == 0) {
        value->destroy();
        delete value;
      }
    }

    template <class T, class Counter>
    template <class U, class>
    counted_ptr_base<T, Counter>&
    counted_ptr_base<T, Counter>::operator =(counted_ptr_base<U, Counter> const& other) noexcept {
      if (value == other.value) return *this;

      // Leverage copy-constructor and move-assignment for exception safety.
      auto tmp = other;
      *this = std::move(tmp);
      return *this;
    }

    template <class T, class Counter>
    template <class U, class>
    counted_ptr_base<T, Counter>&
    counted_ptr_base<T, Counter>::operator =(counted_ptr_base<U, Counter>&& other) noexcept {
      if (value == other.value) return *this;

      // Defer to destructor and move constructor to keep from copying stateful, easy to screw up, code
      // all over the place.
      this->~counted_ptr_base();
      new(this) counted_ptr_base(std::move(other));
      return *this;
    }

    template <class T, class Counter>
    counted_ptr_base<T, Counter>& counted_ptr_base<T, Counter>::operator =(counted_ptr_base const& other) noexcept {
      if (this == &other) return *this;

      // Leverage copy-constructor and move-assignment for exception safety.
      auto tmp = other;
      *this = std::move(tmp);
      return *this;
    }

    template <class T, class Counter>
    counted_ptr_base<T, Counter>& counted_ptr_base<T, Counter>::operator =(counted_ptr_base&& other) noexcept {
      if (this == &other) return *this;

      // Defer to destructor and move constructor to keep from copying stateful, easy to screw up, code
      // all over the place.
      this->~counted_ptr_base();
      new(this) counted_ptr_base(std::move(other));
      return *this;
    }

    template <class T, class Counter>
    template <class U, class Deleter, class>
    counted_ptr_base<T, Counter>& counted_ptr_base<T, Counter>::operator =(std::unique_ptr<U, Deleter>&& ptr)
      noexcept(std::is_nothrow_move_constructible<Deleter>::value)
    {
      // Leverage move-assignment for exception safety.
      counted_ptr_base tmp(std::move(ptr));
      *this = std::move(tmp);
      return *this;
    }

    template <class T, class Counter>
    auto counted_ptr_impl<T, Counter>::operator *() const noexcept -> element_type& {
      return *reinterpret_cast<element_type*>(this->value->ptr);
    }

    template <class T, class Counter>
    auto counted_ptr_impl<T, Counter>::operator ->() const noexcept -> element_type* {
      return reinterpret_cast<element_type*>(this->value->ptr);
    }

    template <class T, class Counter>
    bool counted_ptr_base<T, Counter>::operator ==(counted_ptr_base const& other) const noexcept {
      return value == other.value;
    }

    template <class T, class Counter>
    bool counted_ptr_base<T, Counter>::operator !=(counted_ptr_base const& other) const noexcept {
      return !(*this == other);
    }

    template <class T, class Counter>
    bool counted_ptr_base<T, Counter>::operator <(counted_ptr_base const& other) const noexcept {
      return value < other.value;
    }

    template <class T, class Counter>
    bool counted_ptr_base<T, Counter>::operator <=(counted_ptr_base const& other) const noexcept {
      return value <= other.value;
    }

    template <class T, class Counter>
    bool counted_ptr_base<T, Counter>::operator >(counted_ptr_base const& other) const noexcept {
      return value > other.value;
    }

    template <class T, class Counter>
    bool counted_ptr_base<T, Counter>::operator >=(counted_ptr_base const& other) const noexcept {
      return value >= other.value;
    }

    template <class T, class Counter>
    counted_ptr_base<T, Counter>::operator bool() const noexcept {
      return value != nullptr;
    }

    template <class T, class Counter>
    std::remove_extent_t<typename counted_ptr_base<T, Counter>::element_type>*
    counted_ptr_base<T, Counter>::get() const noexcept {
      if (value) return reinterpret_cast<std::remove_extent_t<element_type>*>(value->ptr);
      else return nullptr;
    }

    template <class T, class Counter>
    bool counted_ptr_base<T, Counter>::unique() const noexcept {
      return use_count() == 1;
    }

    template <class T, class Counter>
    int64_t counted_ptr_base<T, Counter>::use_count() const noexcept {
      if (value) return value->use_count;
      else return 0;
    }

    template <class T, class Counter>
    void counted_ptr_base<T, Counter>::reset() noexcept {
      this->~counted_ptr_base();
      new(this) counted_ptr_base();
    }

    template <class T, class Counter>
    template <class U, class>
    void counted_ptr_base<T, Counter>::reset(U ptr) {
      counted_ptr_base tmp(ptr);
      *this = std::move(tmp);
    }

    template <class T, class Counter>
    template <class U, class Deleter, class>
    void counted_ptr_base<T, Counter>::reset(U ptr, Deleter&& deleter) {
      counted_ptr_base tmp(ptr, std::move(deleter));
      *this = std::move(tmp);
    }

    template <class T, class Counter>
    typename counted_array_ptr_impl<T, Counter>::element_type&
    counted_array_ptr_impl<T, Counter>::operator [](index_type idx) const noexcept {
      return reinterpret_cast<element_type*>(this->value->ptr)[idx];
    }

  }

  template <class T>
  shareable_ptr<T>::shareable_ptr(T const& other) noexcept(refcount_traits<T>::is_nothrow_copyable::value) {
    refcount_traits<T>::copy(&impl, other);
  }

  template <class T>
  shareable_ptr<T>::shareable_ptr(T&& other) noexcept(refcount_traits<T>::is_nothrow_moveable::value) {
    refcount_traits<T>::move(&impl, std::move(other));
  }

  template <class T>
  template <class U, class D, class>
  shareable_ptr<T>::shareable_ptr(U* owner, D&& del)
    noexcept(refcount_traits<T>::template can_nothrow_take<U*, D>::value)
  {
    refcount_traits<T>::take(&impl, owner, std::forward<D>(del));
  }

  template <class T>
  template <class U, class D, class>
  shareable_ptr<T>::shareable_ptr(std::unique_ptr<U, D>&& ptr)
    noexcept(refcount_traits<T>::template can_nothrow_take<U*, D>::value)
  {
    refcount_traits<T>::take(&impl, ptr.get(), std::move(ptr.get_deleter()));
    ptr.release();
  }

  template <class T>
  shareable_ptr<T>::shareable_ptr()
    noexcept(refcount_traits<T>::template can_nothrow_take<element_type*, std::default_delete<element_type>>::value)
  {
    refcount_traits<T>::take(&impl, nullptr, std::default_delete<element_type> {});
  }

  template <class T>
  shareable_ptr<T>::shareable_ptr(shareable_ptr const& other) noexcept(refcount_traits<T>::is_nothrow_copyable::value) {
    refcount_traits<T>::copy(&impl, other.impl);
  }

  template <class T>
  shareable_ptr<T>::shareable_ptr(shareable_ptr&& other) noexcept(refcount_traits<T>::is_nothrow_moveable::value) {
    refcount_traits<T>::move(&impl, std::move(other.impl));
  }

  template <class T>
  shareable_ptr<T>::~shareable_ptr() noexcept {
    try {
      refcount_traits<T>::reset(impl);
    } catch (...) {
      // Oops...
    }
    impl.~value_type();
  }

  template <class T>
  template <class U, class D, class>
  shareable_ptr<T>& shareable_ptr<T>::operator =(std::unique_ptr<U, D>&& ptr)
    noexcept(refcount_traits<T>::template can_nothrow_take<U*, D>::value)
  {
    shareable_ptr tmp {std::move(ptr)};
    *this = std::move(tmp);
    return *this;
  }

  template <class T>
  shareable_ptr<T>& shareable_ptr<T>::operator =(T const& other) noexcept(is_nothrow_assignable::value) {
    shareable_ptr tmp {other};
    *this = std::move(tmp);
    return *this;
  }

  template <class T>
  shareable_ptr<T>& shareable_ptr<T>::operator =(T&& other) noexcept {
    // XXX: If you have a throwing move constructor, you get what you deserve.
    // I'm not allocating to avoid this, and I'm definitely not adding a "valueless by exception" state.
    // Good people don't throw from move constructors.
    this->~shareable_ptr();
    new(this) shareable_ptr(std::move(other));
    return *this;
  }

  template <class T>
  shareable_ptr<T>& shareable_ptr<T>::operator =(std::nullptr_t)
    noexcept(refcount_traits<T>::template can_nothrow_take<element_type*, std::default_delete<element_type>>::value)
  {
    shareable_ptr tmp;
    *this = std::move(tmp);
    return *this;
  }

  template <class T>
  shareable_ptr<T>& shareable_ptr<T>::operator =(shareable_ptr const& other) noexcept(is_nothrow_assignable::value) {
    if (this == &other) return *this;
    auto tmp {other};
    *this = std::move(tmp);
    return *this;
  }

  template <class T>
  shareable_ptr<T>& shareable_ptr<T>::operator =(shareable_ptr&& other) noexcept {
    // XXX: If you have a throwing move constructor, you get what you deserve.
    // I'm not allocating to avoid this, and I'm definitely not adding a "valueless by exception" state.
    // Good people don't throw from move constructors.
    if (this == &other) return *this;
    this->~shareable_ptr();
    new(this) shareable_ptr(std::move(other));
    return *this;
  }

  template <class T>
  auto shareable_ptr<T>::operator *() const noexcept(refcount_traits<T>::is_nothrow_unwrappable::value) -> element_type& {
    return *get();
  }

  template <class T>
  auto shareable_ptr<T>::operator ->() const noexcept(refcount_traits<T>::is_nothrow_unwrappable::value) -> element_type* {
    return get();
  }

  template <class T>
  bool shareable_ptr<T>::operator ==(shareable_ptr const& other) const
    noexcept(refcount_traits<T>::is_nothrow_unwrappable::value)
  {
    return get() == other.get();
  }

  template <class T>
  bool shareable_ptr<T>::operator !=(shareable_ptr const& other) const
    noexcept(refcount_traits<T>::is_nothrow_unwrappable::value)
  {
    return !(*this == other);
  }

  template <class T>
  bool shareable_ptr<T>::operator <(shareable_ptr const& other) const
    noexcept(refcount_traits<T>::is_nothrow_unwrappable::value)
  {
    return get() < other.get();
  }

  template <class T>
  bool shareable_ptr<T>::operator <=(shareable_ptr const& other) const
    noexcept(refcount_traits<T>::is_nothrow_unwrappable::value)
  {
    return !(other < *this);
  }

  template <class T>
  bool shareable_ptr<T>::operator >(shareable_ptr const& other) const
    noexcept(refcount_traits<T>::is_nothrow_unwrappable::value)
  {
    return other < *this;
  }

  template <class T>
  bool shareable_ptr<T>::operator >=(shareable_ptr const& other) const
    noexcept(refcount_traits<T>::is_nothrow_unwrappable::value)
  {
    return !(*this < other);
  }

  template <class T>
  shareable_ptr<T>::operator bool() const noexcept(refcount_traits<T>::is_nothrow_unwrappable::value) {
    return !refcount_traits<T>::is_null(impl);
  }

  template <class T>
  auto shareable_ptr<T>::get() const noexcept(refcount_traits<T>::is_nothrow_unwrappable::value) -> element_type* {
    return refcount_traits<T>::unwrap(impl);
  }

  template <class T>
  bool shareable_ptr<T>::unique() const noexcept(refcount_traits<T>::has_nothrow_use_count::value) {
    return use_count() == 1;
  }

  template <class T>
  int64_t shareable_ptr<T>::use_count() const noexcept(refcount_traits<T>::has_nothrow_use_count::value) {
    return refcount_traits<T>::use_count(impl);
  }

  template <class T>
  void shareable_ptr<T>::reset() noexcept(refcount_traits<T>::has_nothrow_reset::value) {
    refcount_traits<T>::reset(impl);
  }

  template <class T>
  void shareable_ptr<T>::share(T& ptr) const noexcept(refcount_traits<T>::is_nothrow_copyable::value) {
    // Write operation in terms of copy and move for exception safety.
    auto tmp = *this;
    std::move(tmp).transfer(ptr);
  }

  template <class T>
  void shareable_ptr<T>::transfer(T& ptr) && noexcept(refcount_traits<T>::is_nothrow_moveable::value) {
    // Destroy the instance we were given.
    // We DID take it by mutable refernece, so hopefully the user won't be surprised.
    ptr.~T();
    
    // Move ourselves into it.
    // If you have a throwing move constructor, you get what you deserve.
    refcount_traits<T>::move(&ptr, std::move(impl));
  }

  template <class T>
  auto shareable_ptr<T>::raw() noexcept -> value_type& {
    return impl;
  }

  template <class T>
  auto shareable_ptr<T>::raw() const noexcept -> value_type const& {
    return impl;
  }

  template <class T, class... Args>
  shareable_ptr<T> make_shareable(Args&&... the_args) {
    shareable_ptr<T> ptr(typename shareable_ptr<T>::partial_construction_tag {});
    refcount_traits<T>::construct(&ptr.impl, std::forward<Args>(the_args)...);
    return ptr;
  }

  template <template <class> class RefCount>
  template <class T>
  view_ptr_context<RefCount>::view_ptr<T>::view_ptr(T*) {
    throw std::logic_error("dart::view_ptr cannot be passed an owning raw pointer");
  }

  template <template <class> class RefCount>
  template <class T>
  template <class Del>
  view_ptr_context<RefCount>::view_ptr<T>::view_ptr(T*, Del&&) {
    throw std::logic_error("dart::view_ptr cannot be passed an owning raw pointer");
  }

  template <template <class> class RefCount>
  template <class T>
  view_ptr_context<RefCount>::view_ptr<T>::view_ptr(view_ptr&& other) noexcept : impl(other.impl) {
    // XXX: I don't know if this is the right call or not, since view_ptr doesn't generally have
    // memory ownership semantics, but I figure it's the least surprising thing.
    other.impl = nullptr;
  }

  template <template <class> class RefCount>
  template <class T>
  auto view_ptr_context<RefCount>::view_ptr<T>::operator =(refcount_type const& owner) noexcept -> view_ptr& {
    if (impl == &owner) return *this;
    this->~view_ptr();
    new(this) view_ptr(owner);
    return *this;
  }

  template <template <class> class RefCount>
  template <class T>
  auto view_ptr_context<RefCount>::view_ptr<T>::operator =(view_ptr&& other) noexcept -> view_ptr& {
    if (this == &other) return *this;
    this->~view_ptr();
    new(this) view_ptr(std::move(other));
    return *this;
  }

  template <template <class> class RefCount>
  template <class T>
  auto view_ptr_context<RefCount>::view_ptr<T>::operator *() const
    noexcept(refcount_traits<refcount_type>::is_nothrow_unwrappable::value)
    -> element_type&
  {
    return *get();
  }

  template <template <class> class RefCount>
  template <class T>
  auto view_ptr_context<RefCount>::view_ptr<T>::operator ->() const
    noexcept(refcount_traits<refcount_type>::is_nothrow_unwrappable::value)
    -> element_type*
  {
    return get();
  }

  template <template <class> class RefCount>
  template <class T>
  bool view_ptr_context<RefCount>::view_ptr<T>::operator ==(view_ptr const& other) const
    noexcept(refcount_traits<refcount_type>::is_nothrow_unwrappable::value)
  {
    return get() == other.get();
  }

  template <template <class> class RefCount>
  template <class T>
  bool view_ptr_context<RefCount>::view_ptr<T>::operator !=(view_ptr const& other) const
    noexcept(refcount_traits<refcount_type>::is_nothrow_unwrappable::value)
  {
    return !(*this == other);
  }

  template <template <class> class RefCount>
  template <class T>
  bool view_ptr_context<RefCount>::view_ptr<T>::operator <(view_ptr const& other) const
    noexcept(refcount_traits<refcount_type>::is_nothrow_unwrappable::value)
  {
    return get() < other.get();
  }

  template <template <class> class RefCount>
  template <class T>
  bool view_ptr_context<RefCount>::view_ptr<T>::operator <=(view_ptr const& other) const
    noexcept(refcount_traits<refcount_type>::is_nothrow_unwrappable::value)
  {
    return !(other < *this);
  }

  template <template <class> class RefCount>
  template <class T>
  bool view_ptr_context<RefCount>::view_ptr<T>::operator >(view_ptr const& other) const
    noexcept(refcount_traits<refcount_type>::is_nothrow_unwrappable::value)
  {
    return other < *this;
  }

  template <template <class> class RefCount>
  template <class T>
  bool view_ptr_context<RefCount>::view_ptr<T>::operator >=(view_ptr const& other) const
    noexcept(refcount_traits<refcount_type>::is_nothrow_unwrappable::value)
  {
    return !(*this < other);
  }

  template <template <class> class RefCount>
  template <class T>
  view_ptr_context<RefCount>::view_ptr<T>::operator bool() const
    noexcept(refcount_traits<refcount_type>::is_nothrow_unwrappable::value)
  {
    return get() != nullptr;
  }

  template <template <class> class RefCount>
  template <class T>
  view_ptr_context<RefCount>::view_ptr<T>::operator refcount_type() const {
    return raw();
  }

  template <template <class> class RefCount>
  template <class T>
  auto view_ptr_context<RefCount>::view_ptr<T>::get() const
    noexcept(refcount_traits<refcount_type>::is_nothrow_unwrappable::value)
    -> element_type*
  {
    if (impl) return refcount_traits<RefCount<T>>::unwrap(*impl);
    else return nullptr;
  }

  template <template <class> class RefCount>
  template <class T>
  size_t view_ptr_context<RefCount>::view_ptr<T>::use_count() const
    noexcept(refcount_traits<refcount_type>::has_nothrow_use_count::value)
  {
    // XXX: view_ptr doesn't have any memory ownership semantics, and so layers
    // above it should assume it always has at least one unless it's in the null state
    if (impl) {
      auto count = refcount_traits<RefCount<T>>::use_count(*impl);
      return count ? count : 1;
    } else {
      return 0;
    }
  }

  template <template <class> class RefCount>
  template <class T>
  void view_ptr_context<RefCount>::view_ptr<T>::reset() noexcept {
    impl = nullptr;
  }

  template <template <class> class RefCount>
  template <class T>
  auto view_ptr_context<RefCount>::view_ptr<T>::raw() const noexcept -> refcount_type const& {
    return *impl;
  }

  template <class T, class>
  unsafe_ptr<T> make_unsafe(size_t idx) {
    return unsafe_ptr<T>(new std::remove_extent_t<T>[idx]);
  }

  template <class T, class... Args>
  std::enable_if_t<!std::is_array<T>::value, unsafe_ptr<T>> make_unsafe(Args&&... the_args) {
    return unsafe_ptr<T>(new T(std::forward<Args>(the_args)...));
  }

  template <class T, class>
  skinny_ptr<T> make_skinny(size_t idx) {
    return skinny_ptr<T>(new std::remove_extent_t<T>[idx]);
  }

  template <class T, class... Args>
  std::enable_if_t<!std::is_array<T>::value, skinny_ptr<T>> make_skinny(Args&&... the_args) {
    return skinny_ptr<T>(new T(std::forward<Args>(the_args)...));
  }

}

#endif
