#ifndef DART_BUFFER_IMPL_DETAILS_H
#define DART_BUFFER_IMPL_DETAILS_H

/*----- Project Includes -----*/

#include "../common.h"

/*----- Function Implementations -----*/

namespace dart {

  template <template <class> class RefCount>
  basic_buffer<RefCount>::basic_buffer(detail::raw_element raw, buffer_ref_type ref) :
    raw(raw),
    buffer_ref(std::move(ref))
  {
    if (is_null()) buffer_ref.reset();
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::allocate_pointer(gsl::span<gsl::byte const> buffer) const -> buffer_ref_type {
    if (buffer.empty()) throw std::invalid_argument("dart::packet buffer must not be empty");

    // Copy the data into a new buffer.
    auto owner = detail::aligned_alloc<RefCount>(buffer.size(), detail::raw_type::object, [&] (auto* bytes) {
      std::copy(std::begin(buffer), std::end(buffer), bytes);
    });
    return buffer_ref_type {std::move(owner)};
  }

  template <template <class> class RefCount>
  template <class Pointer>
  Pointer&& basic_buffer<RefCount>::validate_pointer(Pointer&& ptr) const {
    // Validate arguments.
    if (!ptr) {
      throw std::invalid_argument("dart::packet pointer must not be null.");
    } else if (detail::align_pointer<RefCount>(ptr.get(), detail::raw_type::object) != ptr.get()) {
      throw std::invalid_argument("dart::packet pointer must be aligned to a 64-bit word boundary");
    }
    return std::forward<Pointer>(ptr);
  }

  template <template <class> class RefCount>
  template <class Pointer>
  auto basic_buffer<RefCount>::normalize(Pointer ptr) const -> buffer_ref_type {
    // This whole business is necessary to allow std::unique_ptr<gsl::byte const[]>
    // to convert to std::shared_ptr<gsl::byte const>, which the STL doesn't define
    // a conversion for, but is convenient for this library.
    buffer_ref_type tmp(ptr.get(), std::move(ptr.get_deleter()));
    ptr.release();
    return tmp;
  }

  template <template <class> class RefCount>
  template <class Span>
  basic_buffer<RefCount> basic_buffer<RefCount>::dynamic_make_object(Span pairs) {
    using pair_storage = std::vector<detail::packet_pair<RefCount>>;

    // Make sure we've been given something we can use.
    if (pairs.size() & 1) {
      throw std::invalid_argument("dart::buffer objects can only be constructed from a sequence of key-value PAIRS");
    }

    // Break our arguments up into pairs.
    pair_storage storage;
    auto end = std::end(pairs);
    auto it = std::begin(pairs);
    storage.reserve(pairs.size());
    while (it != end) {
      if (!it->is_str()) throw std::invalid_argument("dart::buffer object keys must be strings");
      auto& k = *it++;
      auto& v = *it++;
      storage.emplace_back(k, v);
    }

    // Pass off to the low level code.
    return detail::buffer_builder<RefCount>::build_buffer(gsl::make_span(storage));
  }

}

#endif
