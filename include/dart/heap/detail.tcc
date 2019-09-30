#ifndef DART_HEAP_IMPL_DETAILS_H
#define DART_HEAP_IMPL_DETAILS_H

/*----- Project Includes -----*/

#include "../common.h"

/*----- Function Implementations -----*/

namespace dart {

  template <template <class> class RefCount>
  template <class TypeData>
  basic_heap<RefCount>::basic_heap(detail::view_tag, TypeData const& other) {
    // XXX: I have absolutely no idea why this is necessary, I just know that
    // some configurations of gcc 8.3.0 segfault while compiling this constructor
    // if I use a lambda instead of this explicit visitor
    struct mono_visitor {
      void operator ()(shim::monostate) const noexcept {}
    };

    // XXX: This is pretty not great, but the the first four types need special treatment,
    // and it's not straightforward to compute what they will be.
    // At the very least, this approach will generate compilation errors if more
    // types are added, and will likely cause errors if they're re-arranged unexpectedly
    shim::visit(
      shim::compose_together(
        mono_visitor {},
        [this] (shim::variant_alternative_t<1, TypeData> const& fields) {
          data = fields_type {fields.raw()};
        },
        [this] (shim::variant_alternative_t<2, TypeData> const& elems) {
          data = elements_type {elems.raw()};
        },
        [this] (shim::variant_alternative_t<3, TypeData> const& str) {
          data = dynamic_string_layout {str.ptr, str.len};
        },
        [this] (shim::variant_alternative_t<4, TypeData> const& str) {
          data = inline_string_layout {str.buffer, str.left};
        },
        [this] (shim::variant_alternative_t<5, TypeData> const& num) {
          data = num;
        },
        [this] (shim::variant_alternative_t<6, TypeData> const& num) {
          data = num;
        },
        [this] (shim::variant_alternative_t<7, TypeData> const& flag) {
          data = flag;
        }
      ),
      other
    );
  }

  template <template <class> class RefCount>
  void basic_heap<RefCount>::copy_on_write(size_type overcount) {
    if (refcount() > overcount) {
      if (is_object()) data = fields_type(new packet_fields(get_fields()));
      else if (is_array()) data = elements_type(new packet_elements(get_elements()));
    }
  }

  // FIXME: Audit this function. A LOT has changed since it was written.
  template <template <class> class RefCount>
  auto basic_heap<RefCount>::upper_bound() const -> size_type {
    switch (get_raw_type()) {
      case detail::raw_type::object:
        {
          // Start with the base size of the object structure, then add the size of our vtable.
          // The plus one is to account for any potentially required padding.
          auto* fields = try_get_fields();
          size_t max = sizeof(detail::object<RefCount>) + ((sizeof(detail::object_entry) * (fields->size() + 1)));

          // Now iterate over our fields and calculate the max memory required for each.
          for (auto& field : *fields) {
            // Get the maximum size of both our key and value.
            size_t key_max = field.first.upper_bound(), val_max = field.second.upper_bound();

            // Total size required for this field is the max size of the key, plus the maximum required
            // padding for the value type (minus 1), plus the max size of the value, plus the maximum
            // required padding for a subsequent key (minus 1).
            max += key_max + detail::alignment_of<RefCount>(field.second.get_raw_type()) - 1;
            max += val_max + detail::alignment_of<RefCount>(detail::raw_type::string) - 1;
          }

          // This is required so that packets can be copied into contiguous buffers
          // without ruining their alignment.
          max = detail::pad_bytes<RefCount>(max, detail::raw_type::object);

          // Make sure we aren't going to exceed the maximum offset value we can encode in our vtable.
          if (max > max_aggregate_size) {
            throw std::length_error("Offset required for encoding is too large for dart::packet vtable");
          }
          return max;
        }
      case detail::raw_type::array:
        {
          // Start with the base size of the array structure, then add the size of our vtable.
          // The plus one is to account for any potentially required padding.
          auto* elements = try_get_elements();
          size_t max = sizeof(detail::array<RefCount>) + (sizeof(detail::array_entry) * (elements->size() + 1));

          // Now iterate over each element and add their max size.
          // Max size for each element is considered to be their reported maximum size, plus the maximum required
          // padding for the next element.
          for (auto& elem : *elements) {
            max += elem.upper_bound() + detail::alignment_of<RefCount>(elem.get_raw_type()) - 1;
          }

          // Make sure we aren't going to exceed the maximum offset value we can encode in our vtable.
          if (max > max_aggregate_size) {
            throw std::length_error("Offset required for encoding is too large for dart::packet vtable");
          }
          return max;
        }
      case detail::raw_type::small_string:
      case detail::raw_type::string:
        // Maximum size in the case of a string is the base size of the string structure, plus the
        // length of the string. Structure size includes null-terminating character.
        return detail::string::static_sizeof(static_cast<detail::string::size_type>(size()));
      case detail::raw_type::big_string:
        // Maximum size in the case of a string is the base size of the string structure, plus the
        // length of the string. Structure size includes null-terminating character.
        return detail::big_string::static_sizeof(static_cast<detail::big_string::size_type>(size()));
      case detail::raw_type::short_integer:
        return detail::primitive<int16_t>::static_sizeof();
      case detail::raw_type::integer:
        return detail::primitive<int32_t>::static_sizeof();
      case detail::raw_type::long_integer:
        return detail::primitive<int64_t>::static_sizeof();
      case detail::raw_type::decimal:
        return detail::primitive<float>::static_sizeof();
      case detail::raw_type::long_decimal:
        return detail::primitive<double>::static_sizeof();
      case detail::raw_type::boolean:
        return detail::primitive<bool>::static_sizeof();
      default:
        DART_ASSERT(get_raw_type() == detail::raw_type::null);
        return 0;
    }
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::layout(gsl::byte* buffer) const noexcept -> size_type {
    // Construct a wrapper class of the correct type in the provided buffer, and return the number
    // of bytes used.
    auto raw = get_raw_type();
    switch (raw) {
      case detail::raw_type::object:
        new(buffer) detail::object<RefCount>(try_get_fields());
        break;
      case detail::raw_type::array:
        new(buffer) detail::array<RefCount>(try_get_elements());
        break;
      case detail::raw_type::small_string:
      case detail::raw_type::string:
        {
          auto view = strv();
          new(buffer) detail::string(view.data(), view.size());
          break;
        }
      case detail::raw_type::big_string:
        {
          auto view = strv();
          new(buffer) detail::big_string(view.data(), view.size());
          break;
        }
      case detail::raw_type::short_integer:
        new(buffer) detail::primitive<int16_t>(static_cast<int16_t>(integer()));
        break;
      case detail::raw_type::integer:
        new(buffer) detail::primitive<int32_t>(static_cast<int32_t>(integer()));
        break;
      case detail::raw_type::long_integer:
        new(buffer) detail::primitive<int64_t>(integer());
        break;
      case detail::raw_type::decimal:
        new(buffer) detail::primitive<float>(static_cast<float>(decimal()));
        break;
      case detail::raw_type::long_decimal:
        new(buffer) detail::primitive<double>(decimal());
        break;
      case detail::raw_type::boolean:
        new(buffer) detail::primitive<bool>(boolean());
        break;
      default:
        DART_ASSERT(raw == detail::raw_type::null);
        break;
    }
    return detail::find_sizeof<RefCount>({raw, buffer});
  }

  template <template <class> class RefCount>
  detail::raw_type basic_heap<RefCount>::get_raw_type() const noexcept {
    switch (get_type()) {
      case type::object:
        return detail::raw_type::object;
      case type::array:
        return detail::raw_type::array;
      case type::string:
        return detail::identify_string<RefCount>(strv());
      case type::integer:
        return detail::identify_integer(integer());
      case type::decimal:
        return detail::identify_decimal(decimal());
      case type::boolean:
        return detail::raw_type::boolean;
      default:
        DART_ASSERT(is_null());
        return detail::raw_type::null;
    }
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::get_fields() -> packet_fields& {
    if (is_object()) return *shim::get<fields_type>(data);
    else throw type_error("dart::heap is not an object and cannot access fields");
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::get_fields() const -> packet_fields const& {
    if (is_object()) return *shim::get<fields_type>(data);
    else throw type_error("dart::heap is not an object and cannot access fields");
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::try_get_fields() noexcept -> packet_fields* {
    if (is_object()) return shim::get<fields_type>(data).get();
    else return nullptr;
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::try_get_fields() const noexcept -> packet_fields const* {
    if (is_object()) return shim::get<fields_type>(data).get();
    else return nullptr;
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::get_elements() -> packet_elements& {
    if (is_array()) return *shim::get<elements_type>(data);
    else throw type_error("dart::heap is not an array and cannot access elements");
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::get_elements() const -> packet_elements const& {
    if (is_array()) return *shim::get<elements_type>(data);
    else throw type_error("dart::heap is not an object and cannot access elements");
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::try_get_elements() noexcept -> packet_elements* {
    if (is_array()) return shim::get<elements_type>(data).get();
    else return nullptr;
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::try_get_elements() const noexcept -> packet_elements const* {
    if (is_array()) return shim::get<elements_type>(data).get();
    else return nullptr;
  }

}

#endif
