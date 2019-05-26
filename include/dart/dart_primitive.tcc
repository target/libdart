#ifndef DART_PRIMITIVE_IMPL_H
#define DART_PRIMITIVE_IMPL_H

/*----- Local Includes -----*/

#include "dart_intern.h"

/*----- Function Implementations -----*/

namespace dart {

  template <class Number>
  template <class Num, class>
  basic_number<Number>::basic_number(Num&& val) {
    if (!val.is_numeric()) {
      throw type_error("dart::packet::number can only be constructed from a numeric value");
    }
    this->val = std::forward<Num>(val);
  }

  template <class Boolean>
  template <class Bool, class>
  basic_flag<Boolean>::basic_flag(Bool&& val) {
    if (!val.is_boolean()) {
      throw type_error("dart::packet::flag can only be constructed from a boolean value");
    }
    this->val = std::forward<Bool>(val);
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> basic_heap<RefCount>::make_integer(int64_t val) noexcept {
    return basic_heap(detail::integer_tag {}, val);
  }

  template <template <class> class RefCount>
  basic_packet<RefCount> basic_packet<RefCount>::make_integer(int64_t val) noexcept {
    return basic_packet(detail::integer_tag {}, val);
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> basic_heap<RefCount>::make_decimal(double val) noexcept {
    return basic_heap(detail::decimal_tag {}, val);
  }

  template <template <class> class RefCount>
  basic_packet<RefCount> basic_packet<RefCount>::make_decimal(double val) noexcept {
    return basic_packet(detail::decimal_tag {}, val);
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> basic_heap<RefCount>::make_boolean(bool val) noexcept {
    return basic_heap(detail::boolean_tag {}, val);
  }

  template <template <class> class RefCount>
  basic_packet<RefCount> basic_packet<RefCount>::make_boolean(bool val) noexcept {
    return basic_packet(detail::boolean_tag {}, val);
  }

  template <class Number>
  int64_t basic_number<Number>::integer() const {
    return val.integer();
  }

  template <template <class> class RefCount>
  int64_t basic_heap<RefCount>::integer() const {
    if (is_integer()) return shim::get<int64_t>(data);
    else throw type_error("dart::heap has no integer value");
  }

  template <template <class> class RefCount>
  int64_t basic_buffer<RefCount>::integer() const {
    return detail::integer_deref([] (auto& val) { return val.get_data(); }, raw);
  }

  template <template <class> class RefCount>
  int64_t basic_packet<RefCount>::integer() const {
    return shim::visit([] (auto& v) { return v.integer(); }, impl);
  }

  template <template <class> class RefCount>
  int64_t basic_heap<RefCount>::integer_or(int64_t opt) const noexcept {
    return detail::safe_optional_access(*this, opt, &basic_heap::is_integer, &basic_heap::integer);
  }

  template <template <class> class RefCount>
  int64_t basic_packet<RefCount>::integer_or(int64_t opt) const noexcept {
    return detail::safe_optional_access(*this, opt, &basic_packet::is_integer, &basic_packet::integer);
  }

  template <class Number>
  double basic_number<Number>::decimal() const {
    return val.decimal();
  }

  template <template <class> class RefCount>
  double basic_heap<RefCount>::decimal() const {
    if (is_decimal()) return shim::get<double>(data);
    else throw type_error("dart::heap has no decimal value");
  }

  template <template <class> class RefCount>
  double basic_buffer<RefCount>::decimal() const {
    return detail::decimal_deref([] (auto& val) { return val.get_data(); }, raw);
  }

  template <template <class> class RefCount>
  double basic_packet<RefCount>::decimal() const {
    return shim::visit([] (auto& v) { return v.decimal(); }, impl);
  }

  template <template <class> class RefCount>
  double basic_heap<RefCount>::decimal_or(double opt) const noexcept {
    return detail::safe_optional_access(*this, opt, &basic_heap::is_decimal, &basic_heap::decimal);
  }

  template <template <class> class RefCount>
  double basic_packet<RefCount>::decimal_or(double opt) const noexcept {
    return detail::safe_optional_access(*this, opt, &basic_packet::is_decimal, &basic_packet::decimal);
  }

  template <class Number>
  double basic_number<Number>::numeric() const noexcept {
    return val.numeric();
  }

  template <template <class> class RefCount>
  double basic_heap<RefCount>::numeric() const {
    switch (get_type()) {
      case type::integer:
        return integer();
      case type::decimal:
        return decimal();
      default:
        throw type_error("dart::heap has no numeric value");
    }
  }

  template <template <class> class RefCount>
  double basic_buffer<RefCount>::numeric() const {
    switch (get_type()) {
      case type::integer:
        return integer();
      case type::decimal:
        return decimal();
      default:
        throw type_error("dart::buffer has no numeric value");
    }
  }

  template <template <class> class RefCount>
  double basic_packet<RefCount>::numeric() const {
    return shim::visit([] (auto& v) { return v.numeric(); }, impl);
  }

  template <template <class> class RefCount>
  double basic_heap<RefCount>::numeric_or(double opt) const noexcept {
    return detail::safe_optional_access(*this, opt, &basic_heap::is_numeric, &basic_heap::numeric);
  }

  template <template <class> class RefCount>
  double basic_packet<RefCount>::numeric_or(double opt) const noexcept {
    return detail::safe_optional_access(*this, opt, &basic_packet::is_numeric, &basic_packet::numeric);
  }

  template <class Boolean>
  bool basic_flag<Boolean>::boolean() const noexcept {
    return val.boolean();
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::boolean() const {
    if (is_boolean()) return shim::get<bool>(data);
    else throw type_error("dart::heap has no boolean value");
  }

  template <template <class> class RefCount>
  bool basic_buffer<RefCount>::boolean() const {
    return detail::get_primitive<bool>(raw)->get_data();
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::boolean() const {
    return shim::visit([] (auto& v) { return v.boolean(); }, impl);
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::boolean_or(bool opt) const noexcept {
    return detail::safe_optional_access(*this, opt, &basic_heap::is_boolean, &basic_heap::boolean);
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::boolean_or(bool opt) const noexcept {
    return detail::safe_optional_access(*this, opt, &basic_packet::is_boolean, &basic_packet::boolean);
  }

  namespace detail {

    template <class T>
    size_t primitive<T>::get_sizeof() const noexcept {
      return sizeof(T);
    }

    template <class T>
    T primitive<T>::get_data() const noexcept {
      return data;
    }

    template <class T>
    size_t primitive<T>::static_sizeof() noexcept {
      return sizeof(primitive<T>);
    }

  }

}

#endif
