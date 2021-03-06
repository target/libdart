#ifndef DART_HEAP_ITERATOR_H
#define DART_HEAP_ITERATOR_H

/*----- Project Includes -----*/

#include "../common.h"

/*----- Function Implementations -----*/

namespace dart {

  template <template <class> class RefCount>
  basic_heap<RefCount>::iterator::iterator(iterator&& other) noexcept :
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
  bool basic_heap<RefCount>::iterator::operator ==(iterator const& other) const noexcept {
    if (this == &other) return true;
    return impl == other.impl;
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::iterator::operator !=(iterator const& other) const noexcept {
    return !(*this == other);
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::iterator::operator ++() noexcept -> iterator& {
    ++*impl;
    return *this;
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::iterator::operator --() noexcept -> iterator& {
    --*impl;
    return *this;
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::iterator::operator ++(int) noexcept -> iterator {
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
  auto basic_heap<RefCount>::iterator::operator *() const noexcept -> reference {
    return **impl;
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::iterator::operator ->() const noexcept -> pointer {
    return iterator::operator *();
  }

  template <template <class> class RefCount>
  basic_heap<RefCount>::iterator::operator bool() const noexcept {
    return static_cast<bool>(impl);
  }

}

#endif
