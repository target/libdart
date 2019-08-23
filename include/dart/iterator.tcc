#ifndef DART_ITERATOR_H
#define DART_ITERATOR_H

/*----- Local Includes -----*/

#include "common.h"

/*----- Function Implementations -----*/

namespace dart {

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
