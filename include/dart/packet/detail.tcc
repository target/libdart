#ifndef DART_PACKET_IMPL_DETAILS_H
#define DART_PACKET_IMPL_DETAILS_H

/*----- Project Includes -----*/

#include "../common.h"

/*----- Function Implementations -----*/

namespace dart {

  template <template <class> class RefCount>
  size_t basic_packet<RefCount>::upper_bound() const noexcept {
    return shim::visit(
      shim::compose_together(
        [] (basic_buffer<RefCount> const& impl) { return detail::find_sizeof<RefCount>(impl.raw); },
        [] (basic_heap<RefCount> const& impl) { return impl.upper_bound(); }
      ),
      impl
    );
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::layout(gsl::byte* buffer) const noexcept -> size_type {
    return shim::visit(
      shim::compose_together(
        [=] (basic_buffer<RefCount> const& impl) {
          auto bytes = detail::find_sizeof<RefCount>(impl.raw);
          std::copy_n(impl.raw.buffer, bytes, buffer);
          return bytes;
        },
        [=] (basic_heap<RefCount> const& impl) {
          return impl.layout(buffer);
        }
      ),
      impl
    );
  }

  template <template <class> class RefCount>
  detail::raw_type basic_packet<RefCount>::get_raw_type() const noexcept {
    return shim::visit(
      shim::compose_together(
        [] (basic_buffer<RefCount> const& impl) {
          return impl.raw.type;
        },
        [] (basic_heap<RefCount> const& impl) {
          return impl.get_raw_type();
        }
      ),
      impl
    );
  }

  template <template <class> class RefCount>
  basic_heap<RefCount>& basic_packet<RefCount>::get_heap() {
    if (!is_finalized()) return shim::get<basic_heap<RefCount>>(impl);
    else throw state_error("dart::packet is finalized and cannot access a heap representation");
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> const& basic_packet<RefCount>::get_heap() const {
    if (!is_finalized()) return shim::get<basic_heap<RefCount>>(impl);
    else throw state_error("dart::packet is finalized and cannot access a heap representation");
  }

  template <template <class> class RefCount>
  basic_heap<RefCount>* basic_packet<RefCount>::try_get_heap() {
    return shim::get_if<basic_heap<RefCount>>(&impl);
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> const* basic_packet<RefCount>::try_get_heap() const {
    return shim::get_if<basic_heap<RefCount>>(&impl);
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount>& basic_packet<RefCount>::get_buffer() {
    if (is_finalized()) return shim::get<basic_buffer<RefCount>>(impl);
    else throw state_error("dart::packet is not finalized and cannot access a buffer representation");
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> const& basic_packet<RefCount>::get_buffer() const {
    if (is_finalized()) return shim::get<basic_buffer<RefCount>>(impl);
    else throw state_error("dart::packet is not finalized and cannot access a buffer representation");
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount>* basic_packet<RefCount>::try_get_buffer() {
    return shim::get_if<basic_buffer<RefCount>>(&impl);
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> const* basic_packet<RefCount>::try_get_buffer() const {
    return shim::get_if<basic_buffer<RefCount>>(&impl);
  }

}

#endif
