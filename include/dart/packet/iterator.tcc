#ifndef DART_PACKET_ITERATOR_H
#define DART_PACKET_ITERATOR_H

/*----- Project Includes -----*/

#include "../common.h"

/*----- Function Implementations -----*/

namespace dart {

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::iterator::operator ==(iterator const& other) const noexcept {
    if (this == &other) return true;
    else return shim::visit(detail::typeless_comparator {}, impl, other.impl);
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::iterator::operator !=(iterator const& other) const noexcept {
    return !(*this == other);
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::iterator::operator ++() noexcept -> iterator& {
    shim::visit([] (auto& impl) { ++impl; }, impl);
    return *this;
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::iterator::operator --() noexcept -> iterator& {
    shim::visit([] (auto& impl) { --impl; }, impl);
    return *this;
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::iterator::operator ++(int) noexcept -> iterator {
    auto that = *this;
    ++*this;
    return that;
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::iterator::operator --(int) noexcept -> iterator {
    auto that = *this;
    --*this;
    return that;
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::iterator::operator *() const noexcept -> reference {
    return shim::visit([] (auto& impl) -> reference { return *impl; }, impl);
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::iterator::operator ->() const noexcept -> pointer {
    return iterator::operator *();
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>::iterator::operator bool() const noexcept {
    return shim::visit([] (auto& impl) { return static_cast<bool>(impl); }, impl);
  }

}

#endif
