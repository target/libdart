#ifndef DART_ITERATOR_IMPL_H
#define DART_ITERATOR_IMPL_H

/*----- Local Includes -----*/

#include "dart_intern.h"

/*----- Function Implementations -----*/

namespace dart {

  template <template <class> class RefCount>
  basic_heap<RefCount>::iterator::iterator(iterator&& other) noexcept :
    impl(std::move(other.impl))
  {
    other.impl = shim::nullopt;
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount>::iterator::iterator(iterator&& other) noexcept :
    pkt(std::move(other.pkt)),
    impl(std::move(other.impl))
  {
    other.impl = shim::nullopt;
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::iterator::operator =(iterator&& other) noexcept -> iterator& {
    if (this == &other) return *this;
    this->~iterator();
    new(this) iterator(std::move(other));
    return *this;
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::iterator::operator =(iterator&& other) noexcept -> iterator& {
    if (this == &other) return *this;
    this->~iterator();
    new(this) iterator(std::move(other));
    return *this;
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::iterator::operator ==(iterator const& other) const noexcept {
    if (this == &other) return true;
    return impl == other.impl;
  }

  template <template <class> class RefCount>
  bool basic_buffer<RefCount>::iterator::operator ==(iterator const& other) const noexcept {
    if (this == &other) return true;
    return impl == other.impl;
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::iterator::operator ==(iterator const& other) const noexcept {
    if (this == &other) return true;
    else return shim::visit(detail::typeless_comparator {}, impl, other.impl);
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::iterator::operator !=(iterator const& other) const noexcept {
    return !(*this == other);
  }

  template <template <class> class RefCount>
  bool basic_buffer<RefCount>::iterator::operator !=(iterator const& other) const noexcept {
    return !(*this == other);
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::iterator::operator !=(iterator const& other) const noexcept {
    return !(*this == other);
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::iterator::operator ++() noexcept -> iterator& {
    ++*impl;
    return *this;
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::iterator::operator ++() noexcept -> iterator& {
    ++*impl;
    return *this;
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::iterator::operator ++() noexcept -> iterator& {
    shim::visit([] (auto& impl) { ++impl; }, impl);
    return *this;
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::iterator::operator --() noexcept -> iterator& {
    --*impl;
    return *this;
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::iterator::operator --() noexcept -> iterator& {
    --*impl;
    return *this;
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::iterator::operator --() noexcept -> iterator& {
    shim::visit([] (auto& impl) { --impl; }, impl);
    return *this;
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::iterator::operator ++(int) noexcept -> iterator {
    auto that = *this;
    ++*this;
    return that;
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::iterator::operator ++(int) noexcept -> iterator {
    auto that = *this;
    ++*this;
    return that;
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::iterator::operator ++(int) noexcept -> iterator {
    auto that = *this;
    ++*this;
    return that;
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::iterator::operator --(int) noexcept -> iterator {
    auto that = *this;
    --*this;
    return that;
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::iterator::operator --(int) noexcept -> iterator {
    auto that = *this;
    --*this;
    return that;
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::iterator::operator --(int) noexcept -> iterator {
    auto that = *this;
    --*this;
    return that;
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::iterator::operator *() const noexcept -> reference {
    return **impl;
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::iterator::operator *() const noexcept -> reference {
    return basic_buffer(**impl, pkt.buffer_ref);
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::iterator::operator *() const noexcept -> reference {
    return shim::visit([] (auto& impl) -> reference { return *impl; }, impl);
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::iterator::operator ->() const noexcept -> pointer {
    return iterator::operator *();
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::iterator::operator ->() const noexcept -> pointer {
    return iterator::operator *();
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::iterator::operator ->() const noexcept -> pointer {
    return iterator::operator *();
  }

  template <template <class> class RefCount>
  basic_heap<RefCount>::iterator::operator bool() const noexcept {
    return static_cast<bool>(impl);
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount>::iterator::operator bool() const noexcept {
    return static_cast<bool>(impl);
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>::iterator::operator bool() const noexcept {
    return shim::visit([] (auto& impl) { return static_cast<bool>(impl); }, impl);
  }

  namespace detail {

    template <template <class> class RefCount>
    bool ll_iterator<RefCount>::operator ==(ll_iterator const& other) const noexcept {
      if (this == &other) return true;
      else if (base == other.base && idx == other.idx) return true;
      else return false;
    }

    template <template <class> class RefCount>
    bool ll_iterator<RefCount>::operator !=(ll_iterator const& other) const noexcept {
      return !(*this == other);
    }

    template <template <class> class RefCount>
    auto ll_iterator<RefCount>::operator ++() noexcept -> ll_iterator& {
      idx++;
      return *this;
    }

    template <template <class> class RefCount>
    auto ll_iterator<RefCount>::operator --() noexcept -> ll_iterator& {
      idx--;
      return *this;
    }

    template <template <class> class RefCount>
    auto ll_iterator<RefCount>::operator ++(int) noexcept -> ll_iterator {
      // Get a copy of ourselves and increment.
      auto it = *this;
      idx++;
      return it;
    }

    template <template <class> class RefCount>
    auto ll_iterator<RefCount>::operator --(int) noexcept -> ll_iterator {
      // Get a copy of ourselves and decrement.
      auto it = *this;
      idx--;
      return it;
    }

    template <template <class> class RefCount>
    auto ll_iterator<RefCount>::operator *() const noexcept -> value_type {
      return load_func(base, idx);
    }

    template <template <class> class RefCount>
    bool dn_iterator<RefCount>::operator ==(dn_iterator const& other) const noexcept {
      if (this == &other) {
        return true;
      } else {
        return shim::visit([] (auto& lhs, auto& rhs) {
          return typeless_comparator {}(lhs.it, rhs.it);
        }, impl, other.impl);
      }
    }

    template <template <class> class RefCount>
    bool dn_iterator<RefCount>::operator !=(dn_iterator const& other) const noexcept {
      return !(*this == other);
    }

    template <template <class> class RefCount>
    auto dn_iterator<RefCount>::operator ++() noexcept -> dn_iterator& {
      shim::visit([] (auto& impl) { ++impl.it; }, impl);
      return *this;
    }

    template <template <class> class RefCount>
    auto dn_iterator<RefCount>::operator --() noexcept -> dn_iterator& {
      shim::visit([] (auto& impl) { --impl.it; }, impl);
      return *this;
    }

    template <template <class> class RefCount>
    auto dn_iterator<RefCount>::operator ++(int) noexcept -> dn_iterator {
      auto that = *this;
      ++*this;
      return that;
    }

    template <template <class> class RefCount>
    auto dn_iterator<RefCount>::operator --(int) noexcept -> dn_iterator {
      auto that = *this;
      --*this;
      return that;
    }

    template <template <class> class RefCount>
    auto dn_iterator<RefCount>::operator *() const noexcept -> reference {
      return shim::visit([] (auto& impl) -> auto& { return impl.deref(impl.it); }, impl);
    }

  }

}

#endif
