#ifndef DART_API_IMPL_H
#define DART_API_IMPL_H

/*----- System Includes -----*/

#include <assert.h>

/*----- Local Includes -----*/

#include "dart_intern.h"

/*----- Function Implementations -----*/

namespace dart {

  namespace detail {

    template <class Packet>
    std::vector<Packet> values_impl(Packet const& that) {
      std::vector<Packet> packets;
      packets.reserve(that.size());
      for (auto entry : that) packets.push_back(std::move(entry));
      return packets;
    }

    template <class T>
    decltype(auto) maybe_dereference(T&& maybeptr, std::true_type) {
      return *std::forward<T>(maybeptr);
    }
    template <class T>
    decltype(auto) maybe_dereference(T&& maybeptr, std::false_type) {
      return std::forward<T>(maybeptr);
    }

  }

  template <template <class> class RefCount>
  basic_heap<RefCount>::basic_heap(basic_heap&& other) noexcept : data(std::move(other.data)) {
    other.data = shim::monostate {};
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount>::basic_buffer(basic_heap<RefCount> const& heap) {
    if (!heap.is_object()) {
      throw type_error("dart::buffer can only be constructed from an object heap");
    }

    // Calculate the maximum amount of memory that could be required to represent this dart::packet and
    // allocate the whole thing in one go.
    size_t bytes = heap.upper_bound();

    // Allocate an aligned region.
    gsl::byte* tmp;
    int retval = posix_memalign(reinterpret_cast<void**>(&tmp),
        detail::alignment_of<RefCount>(detail::raw_type::object), bytes);
    if (retval) throw std::bad_alloc();

    // Forgive me this one indiscretion.
    auto* del = +[] (gsl::byte const* ptr) { free(const_cast<gsl::byte*>(ptr)); };
    std::unique_ptr<gsl::byte, void (*) (gsl::byte const*)> block {tmp, del};

    // XXX: std::fill_n is REQUIRED here so that we can perform memcmps for finalized packets.
    // Recursively layout the dart::packet.
    std::fill_n(tmp, bytes, gsl::byte {});
    heap.layout(tmp);

    // Set ourselves up as a finalized dart::packet.
    buffer_ref = std::move(block);
    raw = {detail::raw_type::object, buffer_ref.get()};
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount>::basic_buffer(basic_buffer&& other) noexcept :
    raw(other.raw),
    buffer_ref(std::move(other.buffer_ref))
  {
    other.raw = {detail::raw_type::null, nullptr};
    other.buffer_ref = nullptr;
  }

  template <template <class> class RefCount>
  basic_heap<RefCount>& basic_heap<RefCount>::operator =(basic_heap&& other) & noexcept {
    if (this == &other) return *this;

    // Defer to destructor and move constructor to keep from copying stateful, easy to screw up,
    // code all over the place.
    this->~basic_heap();
    new(this) basic_heap(std::move(other));
    return *this;
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount>& basic_buffer<RefCount>::operator =(basic_buffer&& other) & noexcept {
    // Check to make sure we aren't assigning to ourselves.
    if (this == &other) return *this;

    // Defer to destructor and move constructor to keep from copying stateful, easy to screw up,
    // code all over the place.
    this->~basic_buffer();
    new(this) basic_buffer(std::move(other));
    return *this;
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  basic_heap<RefCount> basic_heap<RefCount>::operator [](KeyType const& identifier) const {
    return get(identifier);
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  basic_buffer<RefCount> basic_buffer<RefCount>::operator [](KeyType const& identifier) const& {
    return get(identifier);
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  basic_packet<RefCount> basic_packet<RefCount>::operator [](KeyType const& identifier) const& {
    return get(identifier);
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::operator [](KeyType const& identifier) && {
    return std::move(*this).get(identifier);
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::operator [](KeyType const& identifier) && {
    return std::move(*this).get(identifier);
  }

  template <class Object>
  bool basic_object<Object>::operator ==(basic_object const& other) const noexcept {
    return val == other.val;
  }

  template <class Array>
  bool basic_array<Array>::operator ==(basic_array const& other) const noexcept {
    return val == other.val;
  }

  template <class String>
  bool basic_string<String>::operator ==(basic_string const& other) const noexcept {
    return val == other.val;
  }

  template <class Number>
  bool basic_number<Number>::operator ==(basic_number const& other) const noexcept {
    return val == other.val;
  }

  template <class Boolean>
  bool basic_flag<Boolean>::operator ==(basic_flag const& other) const noexcept {
    return val == other.val;
  }

  template <class Null>
  constexpr bool basic_null<Null>::operator ==(basic_null const&) const noexcept {
    return true;
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::operator ==(basic_heap const& other) const noexcept {
    // Check if we're comparing against ourselves.
    if (this == &other) return true;

    // Check if we're even the same type.
    if (is_null() && other.is_null()) return true;
    else if (get_type() != other.get_type()) return false;

    // Defer to our underlying representation.
    return shim::visit([] (auto& lhs, auto& rhs) {
      using Lhs = std::decay_t<decltype(lhs)>;
      using Rhs = std::decay_t<decltype(rhs)>;

      // Have to compare through pointers here, and we make the decision purely off of the
      // ability to dereference the incoming type to try to allow compatibility with custom
      // external smart-pointers that otherwise conform to the std::shared_ptr interface.
      auto comparator = detail::typeless_comparator {};
      auto& lval = detail::maybe_dereference(lhs, meta::is_dereferenceable<Lhs> {});
      auto& rval = detail::maybe_dereference(rhs, meta::is_dereferenceable<Rhs> {});
      return comparator(lval, rval);
    }, data, other.data);
  }

  template <template <class> class RefCount>
  bool basic_buffer<RefCount>::operator ==(basic_buffer const& other) const noexcept {
    // Check if we're comparing against ourselves.
    if (this == &other) return true;

    // Check if we're sure we're even the same type.
    if (is_null() && other.is_null()) return true;
    else if (get_type() != other.get_type()) return false;
    else if (raw.buffer == other.raw.buffer) return true;

    // Fall back on a comparison of the underlying buffers.
    return detail::buffer_equal<RefCount>(raw, other.raw);
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::operator ==(basic_packet const& other) const noexcept {
    // Check if we're comparing against ourselves.
    if (this == &other) return true;
    return shim::visit([] (auto& lhs, auto& rhs) { return lhs == rhs; }, impl, other.impl);
  }

  template <class Object>
  bool basic_object<Object>::operator !=(basic_object const& other) const noexcept {
    return !(*this == other);
  }

  template <class Array>
  bool basic_array<Array>::operator !=(basic_array const& other) const noexcept {
    return !(*this == other);
  }

  template <class String>
  bool basic_string<String>::operator !=(basic_string const& other) const noexcept {
    return !(*this == other);
  }

  template <class Number>
  bool basic_number<Number>::operator !=(basic_number const& other) const noexcept {
    return !(*this == other);
  }

  template <class Boolean>
  bool basic_flag<Boolean>::operator !=(basic_flag const& other) const noexcept {
    return !(*this == other);
  }

  template <class Null>
  constexpr bool basic_null<Null>::operator !=(basic_null const& other) const noexcept {
    return !(*this == other);
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::operator !=(basic_heap const& other) const noexcept {
    return !(*this == other);
  }

  template <template <class> class RefCount>
  bool basic_buffer<RefCount>::operator !=(basic_buffer const& other) const noexcept {
    return !(*this == other);
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::operator !=(basic_packet const& other) const noexcept {
    return !(*this == other);
  }

  template <class String>
  shim::string_view basic_string<String>::operator *() const noexcept {
    return strv();
  }

  template <class Number>
  double basic_number<Number>::operator *() const noexcept {
    return numeric();
  }

  template <class Boolean>
  bool basic_flag<Boolean>::operator *() const noexcept {
    return boolean();
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::operator ->() noexcept -> basic_heap* {
    return this;
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::operator ->() noexcept -> basic_buffer* {
    return this;
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::operator ->() noexcept -> basic_packet* {
    return this;
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::operator ->() const noexcept -> basic_heap const* {
    return this;
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::operator ->() const noexcept -> basic_buffer const* {
    return this;
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::operator ->() const noexcept -> basic_packet const* {
    return this;
  }

  template <class Object>
  basic_object<Object>::operator value_type() const& noexcept {
    return val;
  }

  template <class Object>
  basic_object<Object>::operator value_type() && noexcept {
    return std::move(val);
  }

  template <class Array>
  basic_array<Array>::operator value_type() const& noexcept {
    return val;
  }

  template <class Array>
  basic_array<Array>::operator value_type() && noexcept {
    return std::move(val);
  }

  template <class String>
  basic_string<String>::operator value_type() const& noexcept {
    return val;
  }

  template <class String>
  basic_string<String>::operator value_type() && noexcept {
    return std::move(val);
  }

  template <class Number>
  basic_number<Number>::operator value_type() const& noexcept {
    return val;
  }

  template <class Number>
  basic_number<Number>::operator value_type() && noexcept {
    return std::move(val);
  }

  template <class Boolean>
  basic_flag<Boolean>::operator value_type() const& noexcept {
    return val;
  }

  template <class Boolean>
  basic_flag<Boolean>::operator value_type() && noexcept {
    return std::move(val);
  }

  template <class Null>
  basic_null<Null>::operator value_type() const noexcept {
    return value_type::make_null();
  }

  template <class Object>
  basic_object<Object>::operator bool() const noexcept {
    return !is_null();
  }

  template <class Array>
  basic_array<Array>::operator bool() const noexcept {
    return !is_null();
  }

  template <class String>
  basic_string<String>::operator bool() const noexcept {
    return !is_null();
  }

  template <class Number>
  basic_number<Number>::operator bool() const noexcept {
    return !is_null();
  }

  template <class Boolean>
  basic_flag<Boolean>::operator bool() const noexcept {
    return !is_null() && boolean();
  }

  template <class Null>
  constexpr basic_null<Null>::operator bool() const noexcept {
    return false;
  }

  template <template <class> class RefCount>
  basic_heap<RefCount>::operator bool() const noexcept {
    if (!is_boolean()) return !is_null();
    else return boolean();
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount>::operator bool() const noexcept {
    if (!is_boolean()) return !is_null();
    else return boolean();
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>::operator bool() const noexcept {
    if (!is_boolean()) return !is_null();
    else return boolean();
  }

  template <class String>
  basic_string<String>::operator std::string() const {
    return std::string {val};
  }

  template <template <class> class RefCount>
  basic_heap<RefCount>::operator std::string() const {
    return std::string {strv()};
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount>::operator std::string() const {
    return std::string {strv()};
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>::operator std::string() const {
    return std::string {strv()};
  }

  template <class String>
  basic_string<String>::operator shim::string_view() const noexcept {
    return shim::string_view {val};
  }

  template <template <class> class RefCount>
  basic_heap<RefCount>::operator shim::string_view() const {
    return strv();
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount>::operator shim::string_view() const {
    return strv();
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>::operator shim::string_view() const {
    return strv();
  }

  template <class Number>
  basic_number<Number>::operator int64_t() const noexcept {
    if (is_integer()) return integer();
    else return decimal();
  }

  template <template <class> class RefCount>
  basic_heap<RefCount>::operator int64_t() const {
    return integer();
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount>::operator int64_t() const {
    return integer();
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>::operator int64_t() const {
    return integer();
  }

  template <class Number>
  basic_number<Number>::operator double() const noexcept {
    return numeric();
  }

  template <template <class> class RefCount>
  basic_heap<RefCount>::operator double() const {
    return decimal();
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount>::operator double() const {
    return decimal();
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>::operator double() const {
    return decimal();
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount>::operator basic_heap<RefCount>() const {
    switch (get_type()) {
      case type::object:
        {
          iterator k, v;
          std::tie(k, v) = kvbegin();
          auto obj = basic_heap<RefCount>::make_object();
          while (v != end()) {
            obj.add_field(basic_heap<RefCount> {*k}, basic_heap<RefCount> {*v});
            ++k, ++v;
          }
          return obj;
        }
      case type::array:
        {
          auto arr = basic_heap<RefCount>::make_array();
          for (auto elem : *this) arr.push_back(basic_heap<RefCount> {std::move(elem)});
          return arr;
        }
      case type::string:
        return basic_heap<RefCount>::make_string(strv());
      case type::integer:
        return basic_heap<RefCount>::make_integer(integer());
      case type::decimal:
        return basic_heap<RefCount>::make_decimal(decimal());
      case type::boolean:
        return basic_heap<RefCount>::make_boolean(boolean());
      default:
        DART_ASSERT(get_type() == type::null);
        return basic_heap<RefCount>::make_null();
    }
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>::operator basic_heap<RefCount>() const {
    auto* pkt = shim::get_if<basic_heap<RefCount>>(&impl);
    if (pkt) return *pkt;
    else return basic_heap<RefCount> {shim::get<basic_buffer<RefCount>>(impl)};
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>::operator basic_buffer<RefCount>() const {
    auto* pkt = shim::get_if<basic_buffer<RefCount>>(&impl);
    if (pkt) return *pkt;
    else return basic_buffer<RefCount> {shim::get<basic_heap<RefCount>>(impl)};
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> basic_heap<RefCount>::make_null() noexcept {
    return basic_heap(detail::null_tag {});
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> basic_buffer<RefCount>::make_null() noexcept {
    return basic_buffer {};
  }

  template <template <class> class RefCount>
  basic_packet<RefCount> basic_packet<RefCount>::make_null() noexcept {
    return basic_packet(detail::null_tag {});
  }

  template <template <class> class RefCount>
  template <class KeyType, class ValueType, class>
  auto basic_heap<RefCount>::insert(KeyType&& key, ValueType&& value) -> iterator {
    // Perform our copy on write if our heap is shared.
    copy_on_write();

    // Cast and forward the key/value into something we can insert.
    auto&& tmp_key = convert::cast<basic_heap>(std::forward<KeyType>(key));
    auto&& tmp_value = convert::cast<basic_heap>(std::forward<ValueType>(value));

    // Perform the insertion.
    if (tmp_key.is_str()) {
      // Check to make sure we'll be able to encode the given string.
      if (tmp_key.size() > std::numeric_limits<uint16_t>::max()) {
        throw std::invalid_argument("dart::heap keys cannot be longer than UINT16_MAX");
      }

      // Attempt the insertion.
      // Sucks that we have to copy the value here, but insert might fail,
      // and the standard doesn't guarantee that tmp_value wouldn't be consumed if so.
      auto val = std::make_pair(std::forward<decltype(tmp_key)>(tmp_key), tmp_value);
      auto res = get_fields().insert(std::move(val));

      // If the key was already present, insert will fail without consuming our value.
      // Overwrite it.
      if (!res.second) res.first->second = std::move(tmp_value);
      return detail::dn_iterator<RefCount> {res.first, [] (auto& it) { return it->second; }};
    } else if (tmp_key.is_integer()) {
      auto& elements = get_elements();
      auto pos = static_cast<size_type>(tmp_key.integer());
      if (pos > elements.size()) throw std::out_of_range("dart::heap cannot insert at out of range index");
      auto new_it = elements.insert(elements.begin() + pos, std::forward<decltype(tmp_value)>(tmp_value));
      return detail::dn_iterator<RefCount> {new_it, [] (auto& it) { return *it; }};
    } else {
      throw type_error("dart::heap cannot insert keys with non string/integer types");
    }
  }

  template <template <class> class RefCount>
  template <class KeyType, class ValueType, class>
  auto basic_packet<RefCount>::insert(KeyType&& key, ValueType&& value) -> iterator {
    return get_heap().insert(std::forward<KeyType>(key), std::forward<ValueType>(value));
  }

  template <template <class> class RefCount>
  template <class ValueType, class>
  auto basic_heap<RefCount>::insert(iterator pos, ValueType&& value) -> iterator {
    // Dig all the way down and get the underlying iterator layout.
    using elements_layout = typename detail::dn_iterator<RefCount>::elements_layout;

    // Make sure our iterator can be used.
    if (!pos) throw std::invalid_argument("dart::heap cannot insert from a valueless iterator");

    // Map the interator's index, and perform the insertion.
    if (is_object()) return insert(iterator_key(pos), std::forward<ValueType>(value));
    else return insert(iterator_index(pos), std::forward<ValueType>(value));
  }

  template <template <class> class RefCount>
  template <class ValueType, class>
  auto basic_packet<RefCount>::insert(iterator pos, ValueType&& value) -> iterator {
    if (!pos) throw std::invalid_argument("dart::packet cannot insert from a valueless iterator");
    auto* it = shim::get_if<typename basic_heap<RefCount>::iterator>(&pos.impl);
    if (it) return get_heap().insert(*it, std::forward<ValueType>(value));
    else throw type_error("dart::packet cannot insert iterators from other/finalized packets");
  }

  template <template <class> class RefCount>
  template <class KeyType, class ValueType, class>
  auto basic_heap<RefCount>::set(KeyType&& key, ValueType&& value) -> iterator {
    // Perform our copy on write if our heap is shared.
    copy_on_write();

    // Cast and forward the key/value into something we can use.
    auto&& tmp_key = convert::cast<basic_heap>(std::forward<KeyType>(key));
    auto&& tmp_val = convert::cast<basic_heap>(std::forward<ValueType>(value));

    // Update the given value IF present.
    if (tmp_key.is_str()) {
      if (tmp_key.size() > std::numeric_limits<uint16_t>::max()) {
        throw std::invalid_argument("dart::heap keys cannot be longer than UINT16_MAX");
      }

      // Update the mapping IF it exists.
      auto& fields = get_fields();
      auto it = fields.find(tmp_key);
      if (it == fields.end()) throw std::out_of_range("dart::heap cannot set a non-existent key");
      it->second = std::forward<decltype(tmp_val)>(tmp_val);
      return detail::dn_iterator<RefCount>{it, [] (auto& it) { return it->second; }};
    } else if (tmp_key.is_integer()) {
      auto& elements = get_elements();
      auto pos = static_cast<size_type>(tmp_key.integer());
      if (pos >= elements.size()) throw std::out_of_range("dart::heap cannot set a value at out of range index");
      auto it = elements.begin() + pos;
      *it = std::forward<decltype(tmp_val)>(tmp_val);
      return detail::dn_iterator<RefCount> {it, [] (auto& it) { return *it; }};
    } else {
      throw type_error("dart::heap cannot set keys with non string/integer types");
    }
  }

  template <template <class> class RefCount>
  template <class KeyType, class ValueType, class>
  auto basic_packet<RefCount>::set(KeyType&& key, ValueType&& value) -> iterator {
    return get_heap().set(std::forward<KeyType>(key), std::forward<ValueType>(value));
  }

  template <template <class> class RefCount>
  template <class ValueType, class>
  auto basic_heap<RefCount>::set(iterator pos, ValueType&& value) -> iterator {
    // Dig all the way down and get the underlying iterator layout.
    using elements_layout = typename detail::dn_iterator<RefCount>::elements_layout;

    // Make sure our iterator can be used.
    if (!pos) throw std::invalid_argument("dart::heap cannot insert from a valueless iterator");

    // Map the interator's index, and perform the insertion.
    if (is_object()) return set(iterator_key(pos), std::forward<ValueType>(value));
    else return set(iterator_index(pos), std::forward<ValueType>(value));
  }

  template <template <class> class RefCount>
  template <class ValueType, class>
  auto basic_packet<RefCount>::set(iterator pos, ValueType&& value) -> iterator {
    if (!pos) throw std::invalid_argument("dart::packet cannot set from a valueless iterator");
    auto* it = shim::get_if<typename basic_heap<RefCount>::iterator>(&pos.impl);
    if (it) return get_heap().set(*it, std::forward<ValueType>(value));
    else throw type_error("dart::packet cannot set iterators from other/finalized packets");
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  auto basic_heap<RefCount>::erase(KeyType const& identifier) -> iterator {
    switch (identifier.get_type()) {
      case type::string:
        return erase(identifier.strv());
      case type::integer:
        return erase(identifier.integer());
      default:
        throw type_error("dart::heap cannot erase values with non-string/integer type.");
    }
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  auto basic_packet<RefCount>::erase(KeyType const& identifier) -> iterator {
    switch (identifier.get_type()) {
      case type::string:
        return erase(identifier.strv());
      case type::integer:
        return erase(identifier.integer());
      default:
        throw type_error("dart::packet cannot erase values with non-string/integer type.");
    }
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::erase(iterator pos) -> iterator {
    // Dig all the way down and get the underlying iterator layout.
    using fields_layout = typename detail::dn_iterator<RefCount>::fields_layout;

    // Make sure our iterator can be used.
    if (!pos) throw std::invalid_argument("dart::heap cannot erase from a valueless iterator");

    // Map the interator's key/index, and perform the insertion.
    // I wish we could do better than this, but copy-on-write semantics make it extraordinarily
    // difficult to work with STL iterators like this.
    if (is_object()) {
      try {
        return erase_key_impl(iterator_key(pos),
            shim::get<fields_layout>(pos.impl->impl).deref, shim::get<fields_type>(data));
      } catch (...) {
        throw type_error("dart::heap cannot erase with iterator of wrong type");
      }
    } else {
      return erase(iterator_index(pos));
    }
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::erase(iterator pos) -> iterator {
    if (!pos) throw std::invalid_argument("dart::packet cannot erase from a valueless iterator");
    auto* it = shim::get_if<typename basic_heap<RefCount>::iterator>(&pos.impl);
    if (it) return get_heap().erase(*it);
    else throw type_error("dart::packet cannot erase iterators from other/finalized packets");
  }

  template <class Object>
  decltype(auto) basic_object<Object>::definalize() & {
    return val.definalize();
  }

  template <class Object>
  decltype(auto) basic_object<Object>::definalize() && {
    return std::move(val).definalize();
  }

  template <template <class> class RefCount>
  basic_heap<RefCount>& basic_heap<RefCount>::definalize() & {
    return *this;
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> const& basic_heap<RefCount>::definalize() const& {
    return *this;
  }

  template <template <class> class RefCount>
  basic_heap<RefCount>&& basic_heap<RefCount>::definalize() && {
    return std::move(*this);
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> const&& basic_heap<RefCount>::definalize() const&& {
    return std::move(*this);
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> basic_buffer<RefCount>::definalize() const {
    return basic_heap<RefCount> {*this};
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>& basic_packet<RefCount>::definalize() & {
    if (is_finalized()) impl = basic_heap<RefCount> {shim::get<basic_buffer<RefCount>>(impl)};
    return *this;
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>&& basic_packet<RefCount>::definalize() && {
    definalize();
    return std::move(*this);
  }

  template <class Object>
  decltype(auto) basic_object<Object>::lift() & {
    return val.lift();
  }

  template <class Object>
  decltype(auto) basic_object<Object>::lift() && {
    return std::move(val).lift();
  }

  template <template <class> class RefCount>
  basic_heap<RefCount>& basic_heap<RefCount>::lift() & {
    return definalize();
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> const& basic_heap<RefCount>::lift() const& {
    return definalize();
  }

  template <template <class> class RefCount>
  basic_heap<RefCount>&& basic_heap<RefCount>::lift() && {
    return std::move(*this).definalize();
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> const&& basic_heap<RefCount>::lift() const&& {
    return std::move(*this).definalize();
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> basic_buffer<RefCount>::lift() const {
    return definalize();
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>& basic_packet<RefCount>::lift() & {
    return definalize();
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>&& basic_packet<RefCount>::lift() && {
    return std::move(*this).definalize();
  }

  template <class Object>
  decltype(auto) basic_object<Object>::finalize() & {
    return val.finalize();
  }

  template <class Object>
  decltype(auto) basic_object<Object>::finalize() && {
    return std::move(val).finalize();
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> basic_heap<RefCount>::finalize() const {
    return basic_buffer<RefCount> {*this};
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount>& basic_buffer<RefCount>::finalize() & {
    return *this;
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> const& basic_buffer<RefCount>::finalize() const& {
    return *this;
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::finalize() && {
    return std::move(*this);
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> const&& basic_buffer<RefCount>::finalize() const&& {
    return std::move(*this);
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>& basic_packet<RefCount>::finalize() & {
    if (!is_finalized()) impl = basic_buffer<RefCount> {shim::get<basic_heap<RefCount>>(impl)};
    return *this;
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>&& basic_packet<RefCount>::finalize() && {
    finalize();
    return std::move(*this);
  }

  template <class Object>
  decltype(auto) basic_object<Object>::lower() & {
    return val.lower();
  }

  template <class Object>
  decltype(auto) basic_object<Object>::lower() && {
    return std::move(val).lower();
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> basic_heap<RefCount>::lower() const {
    return finalize();
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount>& basic_buffer<RefCount>::lower() & {
    return finalize();
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> const& basic_buffer<RefCount>::lower() const& {
    return finalize();
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::lower() && {
    return std::move(*this).finalize();
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> const&& basic_buffer<RefCount>::lower() const&& {
    return std::move(*this).finalize();
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>& basic_packet<RefCount>::lower() & {
    return finalize();
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>&& basic_packet<RefCount>::lower() && {
    return std::move(*this).finalize();
  }
  template <template <class> class RefCount>
  template <template <class> class NewCount>
  basic_heap<NewCount> basic_heap<RefCount>::transmogrify(basic_heap const& heap) {
    switch (heap.get_type()) {
      case type::object:
        {
          basic_heap::iterator k, v;
          std::tie(k, v) = heap.kvbegin();
          auto obj = basic_heap<NewCount>::make_object();
          while (v != heap.end()) {
            obj.add_field(transmogrify<NewCount>(*k), transmogrify<NewCount>(*v));
            ++k, ++v;
          }
          return obj;
        }
      case type::array:
        {
          auto arr = basic_heap<NewCount>::make_array();
          for (auto elem : heap) arr.push_back(transmogrify<NewCount>(std::move(elem)));
          return arr;
        }
      case type::string:
        return basic_heap<NewCount>::make_string(heap.strv());
      case type::integer:
        return basic_heap<NewCount>::make_integer(heap.integer());
      case type::decimal:
        return basic_heap<NewCount>::make_decimal(heap.decimal());
      case type::boolean:
        return basic_heap<NewCount>::make_boolean(heap.boolean());
      default:
        DART_ASSERT(heap.get_type() == type::null);
        return basic_heap<NewCount>::make_null();
    }
  }

  template <template <class> class RefCount>
  template <template <class> class NewCount>
  basic_buffer<NewCount> basic_buffer<RefCount>::transmogrify(basic_buffer const& buffer) {
    return basic_buffer<NewCount> {buffer.dup_bytes()};
  }

  template <template <class> class RefCount>
  template <template <class> class NewCount>
  basic_packet<NewCount> basic_packet<RefCount>::transmogrify(basic_packet const& packet) {
    return shim::visit([] (auto& impl) -> basic_packet<NewCount> {
      using pkt = std::remove_reference_t<decltype(impl)>;
      return pkt::template transmogrify<NewCount>(impl);
    }, packet.impl);
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  basic_heap<RefCount> basic_heap<RefCount>::get(KeyType const& identifier) const {
    switch (identifier.get_type()) {
      case type::string:
        return get(identifier.strv());
      case type::integer:
        return get(identifier.integer());
      default:
        throw type_error("dart::heap cannot retrieve values with non-string/integer type.");
    }
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  basic_buffer<RefCount> basic_buffer<RefCount>::get(KeyType const& identifier) const& {
    switch (identifier.get_type()) {
      case type::string:
        return get(identifier.strv());
      case type::integer:
        return get(identifier.integer());
      default:
        throw type_error("dart::buffer cannot retrieve values with non-string/integer type.");
    }
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  basic_packet<RefCount> basic_packet<RefCount>::get(KeyType const& identifier) const& {
    return shim::visit([&] (auto& v) -> basic_packet { return v.get(identifier); }, impl);
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::get(KeyType const& identifier) && {
    switch (identifier.get_type()) {
      case type::string:
        return std::move(*this).get(identifier.strv());
      case type::integer:
        return std::move(*this).get(identifier.integer());
      default:
        throw type_error("dart::buffer cannot retrieve values with non-string/integer type.");
    }
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::get(KeyType const& identifier) && {
    shim::visit([&] (auto& v) { v = std::move(v).get(identifier); }, impl);
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <class KeyType, class T, class>
  basic_heap<RefCount> basic_heap<RefCount>::get_or(KeyType const& identifier, T&& opt) const {
    // Need an extra check for objects because objects return null if the key is missing.
    // Array access with throw if the identifier isn't within or range, or isn't an integer.
    if (is_object() && has_key(identifier)) {
      return get(identifier);
    } else if (is_array() && size() > static_cast<size_t>(identifier.integer())) {
      return get(identifier);
    } else {
      return convert::cast<basic_heap>(std::forward<T>(opt));
    }
  }

  template <template <class> class RefCount>
  template <class KeyType, class T, class>
  basic_packet<RefCount> basic_packet<RefCount>::get_or(KeyType const& identifier, T&& opt) const {
    if (is_object() && has_key(identifier)) {
      return get(identifier);
    } else if (is_array() && size() > static_cast<size_t>(identifier.integer())) {
      return get(identifier);
    } else {
      return convert::cast<basic_packet>(std::forward<T>(opt));
    }
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  basic_heap<RefCount> basic_heap<RefCount>::at(KeyType const& identifier) const {
    switch (identifier.at_type()) {
      case type::string:
        return at(identifier.strv());
      case type::integer:
        return at(identifier.integer());
      default:
        throw type_error("dart::heap cannot retrieve values with non-string/integer type.");
    }
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  basic_buffer<RefCount> basic_buffer<RefCount>::at(KeyType const& identifier) const& {
    switch (identifier.at_type()) {
      case type::string:
        return at(identifier.strv());
      case type::integer:
        return at(identifier.integer());
      default:
        throw type_error("dart::buffer cannot retrieve values with non-string/integer type.");
    }
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  basic_packet<RefCount> basic_packet<RefCount>::at(KeyType const& identifier) const& {
    return shim::visit([&] (auto& v) -> basic_packet { return v.at(identifier); }, impl);
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::at(KeyType const& identifier) && {
    switch (identifier.at_type()) {
      case type::string:
        return std::move(*this).at(identifier.strv());
      case type::integer:
        return std::move(*this).at(identifier.integer());
      default:
        throw type_error("dart::buffer cannot retrieve values with non-string/integer type.");
    }
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::at(KeyType const& identifier) && {
    shim::visit([&] (auto& v) { v = std::move(v).at(identifier); }, impl);
    return std::move(*this);
  }

  template <class Object>
  auto basic_object<Object>::values() const -> std::vector<value_type> {
    return val.values();
  }

  template <class Array>
  auto basic_array<Array>::values() const -> std::vector<value_type> {
    return val.values();
  }

  template <template <class> class RefCount>
  std::vector<basic_heap<RefCount>> basic_heap<RefCount>::values() const {
    return detail::values_impl(*this);
  }

  template <template <class> class RefCount>
  std::vector<basic_buffer<RefCount>> basic_buffer<RefCount>::values() const {
    return detail::values_impl(*this);
  }

  template <template <class> class RefCount>
  std::vector<basic_packet<RefCount>> basic_packet<RefCount>::values() const {
    std::vector<basic_packet> packets;
    packets.reserve(size());
    shim::visit([&] (auto& v) {
      for (auto& p : v.values()) packets.push_back(std::move(p));
    }, impl);
    return packets;
  }

  template <class Object>
  template <class, class>
  auto basic_object<Object>::get_bytes() const {
    return val.get_bytes();
  }

  template <template <class> class RefCount>
  gsl::span<gsl::byte const> basic_buffer<RefCount>::get_bytes() const {
    if (is_null()) throw type_error("dart::buffer is null and has no network buffer");
    auto len = detail::find_sizeof<RefCount>({detail::raw_type::object, buffer_ref.get()});
    return gsl::make_span(buffer_ref.get(), len);
  }

  template <template <class> class RefCount>
  gsl::span<gsl::byte const> basic_packet<RefCount>::get_bytes() const {
    return get_buffer().get_bytes();
  }

  template <class Object>
  template <class RC, class>
  auto basic_object<Object>::share_bytes(RC& bytes) const {
    return val.share_bytes(bytes);
  }

  template <template <class> class RefCount>
  size_t basic_buffer<RefCount>::share_bytes(RefCount<gsl::byte const>& bytes) const {
    if (is_null()) throw type_error("dart::buffer is null and has no network buffer");

    // Destroy and initialize the given reference counter instance as a copy of our buffer.
    buffer_ref.share(bytes);

    // Return the size of the packet.
    return detail::find_sizeof<RefCount>({detail::raw_type::object, buffer_ref.get()});
  }

  template <template <class> class RefCount>
  size_t basic_packet<RefCount>::share_bytes(RefCount<gsl::byte const>& bytes) const {
    return get_buffer().share_bytes(bytes);
  }

  template <class Object>
  template <class, class>
  auto basic_object<Object>::dup_bytes() const {
    size_type dummy;
    return val.dup_bytes(dummy);
  }

  template <template <class> class RefCount>
  std::unique_ptr<gsl::byte const[], void (*) (gsl::byte const*)> basic_buffer<RefCount>::dup_bytes() const {
    size_type dummy;
    return dup_bytes(dummy);
  }

  template <template <class> class RefCount>
  std::unique_ptr<gsl::byte const[], void (*) (gsl::byte const*)> basic_packet<RefCount>::dup_bytes() const {
    size_type dummy;
    return dup_bytes(dummy);
  }

  template <class Object>
  template <class, class>
  auto basic_object<Object>::dup_bytes(size_type& len) const {
    return val.dup_bytes(len);
  }

  template <template <class> class RefCount>
  std::unique_ptr<gsl::byte const[], void (*) (gsl::byte const*)> basic_buffer<RefCount>::dup_bytes(size_type& len) const {
    using tmp_ptr = std::unique_ptr<gsl::byte const[], void (*) (gsl::byte const*)>;

    // Allocate an aligned region.
    gsl::byte* tmp;
    auto buf = get_bytes();
    int retval = posix_memalign(reinterpret_cast<void**>(&tmp),
        detail::alignment_of<RefCount>(detail::raw_type::object), buf.size());
    if (retval) throw std::bad_alloc();

    // Copy and return.
    tmp_ptr block {tmp, +[] (gsl::byte const* ptr) { free(const_cast<gsl::byte*>(ptr)); }};
    std::copy(buf.begin(), buf.end(), tmp);
    len = buf.size();
    return block;
  }

  template <template <class> class RefCount>
  std::unique_ptr<gsl::byte const[], void (*) (gsl::byte const*)> basic_packet<RefCount>::dup_bytes(size_type& len) const {
    return get_buffer().dup_bytes(len);
  }

  template <class Object>
  auto basic_object<Object>::size() const noexcept -> size_type {
    return val.size();
  }

  template <class Array>
  auto basic_array<Array>::size() const noexcept -> size_type {
    return val.size();
  }

  template <class String>
  auto basic_string<String>::size() const noexcept -> size_type {
    return val.size();
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::size() const -> size_type {
    // Validate sanity.
    if (!is_aggregate() && !is_str()) {
      throw type_error("dart::heap is a primitive, and has no size.");
    }

    // Get our size.
    if (is_object()) {
      return try_get_fields()->size();
    } else if (is_array()) {
      return try_get_elements()->size();
    } else {
      return strv().size();
    }
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::size() const -> size_type {
    // Validate sanity.
    if (!is_aggregate() && !is_str()) {
      throw type_error("dart::buffer is a primitive, and has no size.");
    }

    // Get our size.
    if (is_aggregate()) {
      return detail::aggregate_deref<RefCount>([] (auto& aggr) { return aggr.size(); }, raw);
    } else {
      return detail::string_deref([] (auto& str) { return str.size(); }, raw);
    }
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::size() const -> size_type {
    return shim::visit([] (auto& v) { return v.size(); }, impl);
  }

  template <class Object>
  bool basic_object<Object>::empty() const noexcept {
    return val.empty();
  }

  template <class Array>
  bool basic_array<Array>::empty() const noexcept {
    return val.empty();
  }

  template <class String>
  bool basic_string<String>::empty() const noexcept {
    return val.empty();
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::empty() const {
    return size() == 0ULL;
  }

  template <template <class> class RefCount>
  bool basic_buffer<RefCount>::empty() const {
    return size() == 0ULL;
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::empty() const {
    return size() == 0ULL;
  }

  template <class Object>
  auto basic_object<Object>::dynamic() const noexcept -> value_type const& {
    return val;
  }

  template <class Array>
  auto basic_array<Array>::dynamic() const noexcept -> value_type const& {
    return val;
  }

  template <class String>
  auto basic_string<String>::dynamic() const noexcept -> value_type const& {
    return val;
  }

  template <class Number>
  auto basic_number<Number>::dynamic() const noexcept -> value_type const& {
    return val;
  }

  template <class Boolean>
  auto basic_flag<Boolean>::dynamic() const noexcept -> value_type const& {
    return val;
  }

  template <class Null>
  auto basic_null<Null>::dynamic() const noexcept -> value_type const& {
    return val;
  }

  template <class Object>
  bool basic_object<Object>::is_object() const noexcept {
    return val.is_object();
  }

  template <class Array>
  constexpr bool basic_array<Array>::is_object() const noexcept {
    return false;
  }

  template <class String>
  constexpr bool basic_string<String>::is_object() const noexcept {
    return false;
  }

  template <class Number>
  constexpr bool basic_number<Number>::is_object() const noexcept {
    return false;
  }

  template <class Boolean>
  constexpr bool basic_flag<Boolean>::is_object() const noexcept {
    return false;
  }

  template <class Null>
  constexpr bool basic_null<Null>::is_object() const noexcept {
    return false;
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::is_object() const noexcept {
    return shim::holds_alternative<fields_type>(data);
  }

  template <template <class> class RefCount>
  bool basic_buffer<RefCount>::is_object() const noexcept {
    return detail::simplify_type(raw.type) == type::object;
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::is_object() const noexcept {
    return shim::visit([] (auto& v) { return v.is_object(); }, impl);
  }

  template <class Object>
  constexpr bool basic_object<Object>::is_array() const noexcept {
    return false;
  }

  template <class Array>
  bool basic_array<Array>::is_array() const noexcept {
    return val.is_array();
  }

  template <class String>
  constexpr bool basic_string<String>::is_array() const noexcept {
    return false;
  }

  template <class Number>
  constexpr bool basic_number<Number>::is_array() const noexcept {
    return false;
  }

  template <class Boolean>
  constexpr bool basic_flag<Boolean>::is_array() const noexcept {
    return false;
  }

  template <class Null>
  constexpr bool basic_null<Null>::is_array() const noexcept {
    return false;
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::is_array() const noexcept {
    return shim::holds_alternative<elements_type>(data);
  }

  template <template <class> class RefCount>
  bool basic_buffer<RefCount>::is_array() const noexcept {
    return detail::simplify_type(raw.type) == type::array;
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::is_array() const noexcept {
    return shim::visit([] (auto& v) { return v.is_array(); }, impl);
  }

  template <class Object>
  bool basic_object<Object>::is_aggregate() const noexcept {
    return is_object();
  }

  template <class Array>
  bool basic_array<Array>::is_aggregate() const noexcept {
    return is_array();
  }

  template <class String>
  constexpr bool basic_string<String>::is_aggregate() const noexcept {
    return false;
  }

  template <class Number>
  constexpr bool basic_number<Number>::is_aggregate() const noexcept {
    return false;
  }

  template <class Boolean>
  constexpr bool basic_flag<Boolean>::is_aggregate() const noexcept {
    return false;
  }

  template <class Null>
  constexpr bool basic_null<Null>::is_aggregate() const noexcept {
    return false;
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::is_aggregate() const noexcept {
    return is_object() || is_array();
  }

  template <template <class> class RefCount>
  bool basic_buffer<RefCount>::is_aggregate() const noexcept {
    return is_object() || is_array();
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::is_aggregate() const noexcept {
    return is_object() || is_array();
  }

  template <class Object>
  constexpr bool basic_object<Object>::is_str() const noexcept {
    return false;
  }

  template <class Array>
  constexpr bool basic_array<Array>::is_str() const noexcept {
    return false;
  }

  template <class String>
  bool basic_string<String>::is_str() const noexcept {
    return val.is_str();
  }

  template <class Number>
  constexpr bool basic_number<Number>::is_str() const noexcept {
    return false;
  }

  template <class Boolean>
  constexpr bool basic_flag<Boolean>::is_str() const noexcept {
    return false;
  }

  template <class Null>
  constexpr bool basic_null<Null>::is_str() const noexcept {
    return false;
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::is_str() const noexcept {
    return shim::holds_alternative<dynamic_string_layout>(data)
      || shim::holds_alternative<inline_string_layout>(data);
  }

  template <template <class> class RefCount>
  bool basic_buffer<RefCount>::is_str() const noexcept {
    return detail::simplify_type(raw.type) == type::string;
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::is_str() const noexcept {
    return shim::visit([] (auto& v) { return v.is_str(); }, impl);
  }

  template <class Object>
  constexpr bool basic_object<Object>::is_integer() const noexcept {
    return false;
  }

  template <class Array>
  constexpr bool basic_array<Array>::is_integer() const noexcept {
    return false;
  }

  template <class String>
  constexpr bool basic_string<String>::is_integer() const noexcept {
    return false;
  }

  template <class Number>
  bool basic_number<Number>::is_integer() const noexcept {
    return val.is_integer();
  }

  template <class Boolean>
  constexpr bool basic_flag<Boolean>::is_integer() const noexcept {
    return false;
  }

  template <class Null>
  constexpr bool basic_null<Null>::is_integer() const noexcept {
    return false;
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::is_integer() const noexcept {
    return shim::holds_alternative<int64_t>(data);
  }

  template <template <class> class RefCount>
  bool basic_buffer<RefCount>::is_integer() const noexcept {
    return detail::simplify_type(raw.type) == type::integer;
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::is_integer() const noexcept {
    return shim::visit([] (auto& v) { return v.is_integer(); }, impl);
  }

  template <class Object>
  constexpr bool basic_object<Object>::is_decimal() const noexcept {
    return false;
  }

  template <class Array>
  constexpr bool basic_array<Array>::is_decimal() const noexcept {
    return false;
  }

  template <class String>
  constexpr bool basic_string<String>::is_decimal() const noexcept {
    return false;
  }

  template <class Number>
  bool basic_number<Number>::is_decimal() const noexcept {
    return val.is_decimal();
  }

  template <class Boolean>
  constexpr bool basic_flag<Boolean>::is_decimal() const noexcept {
    return false;
  }

  template <class Null>
  constexpr bool basic_null<Null>::is_decimal() const noexcept {
    return false;
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::is_decimal() const noexcept {
    return shim::holds_alternative<double>(data);
  }

  template <template <class> class RefCount>
  bool basic_buffer<RefCount>::is_decimal() const noexcept {
    return detail::simplify_type(raw.type) == type::decimal;
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::is_decimal() const noexcept {
    return shim::visit([] (auto& v) { return v.is_decimal(); }, impl);
  }

  template <class Object>
  constexpr bool basic_object<Object>::is_numeric() const noexcept {
    return false;
  }

  template <class Array>
  constexpr bool basic_array<Array>::is_numeric() const noexcept {
    return false;
  }

  template <class String>
  constexpr bool basic_string<String>::is_numeric() const noexcept {
    return false;
  }

  template <class Number>
  bool basic_number<Number>::is_numeric() const noexcept {
    return !is_null();
  }

  template <class Boolean>
  constexpr bool basic_flag<Boolean>::is_numeric() const noexcept {
    return false;
  }

  template <class Null>
  constexpr bool basic_null<Null>::is_numeric() const noexcept {
    return false;
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::is_numeric() const noexcept {
    return is_integer() || is_decimal();
  }

  template <template <class> class RefCount>
  bool basic_buffer<RefCount>::is_numeric() const noexcept {
    return is_integer() || is_decimal();
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::is_numeric() const noexcept {
    return is_integer() || is_decimal();
  }

  template <class Object>
  constexpr bool basic_object<Object>::is_boolean() const noexcept {
    return false;
  }

  template <class Array>
  constexpr bool basic_array<Array>::is_boolean() const noexcept {
    return false;
  }

  template <class String>
  constexpr bool basic_string<String>::is_boolean() const noexcept {
    return false;
  }

  template <class Number>
  constexpr bool basic_number<Number>::is_boolean() const noexcept {
    return false;
  }

  template <class Boolean>
  bool basic_flag<Boolean>::is_boolean() const noexcept {
    return val.is_boolean();
  }

  template <class Null>
  constexpr bool basic_null<Null>::is_boolean() const noexcept {
    return false;
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::is_boolean() const noexcept {
    return shim::holds_alternative<bool>(data);
  }

  template <template <class> class RefCount>
  bool basic_buffer<RefCount>::is_boolean() const noexcept {
    return detail::simplify_type(raw.type) == type::boolean;
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::is_boolean() const noexcept {
    return shim::visit([] (auto& v) { return v.is_boolean(); }, impl);
  }

  template <class Object>
  bool basic_object<Object>::is_null() const noexcept {
    return val.is_null();
  }

  template <class Array>
  bool basic_array<Array>::is_null() const noexcept {
    return val.is_null();
  }

  template <class String>
  bool basic_string<String>::is_null() const noexcept {
    return val.is_null();
  }

  template <class Number>
  bool basic_number<Number>::is_null() const noexcept {
    return val.is_null();
  }

  template <class Boolean>
  bool basic_flag<Boolean>::is_null() const noexcept {
    return val.is_null();
  }

  template <class Null>
  constexpr bool basic_null<Null>::is_null() const noexcept {
    return true;
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::is_null() const noexcept {
    return shim::holds_alternative<shim::monostate>(data);
  }

  template <template <class> class RefCount>
  bool basic_buffer<RefCount>::is_null() const noexcept {
    return detail::simplify_type(raw.type) == type::null;
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::is_null() const noexcept {
    return shim::visit([] (auto& v) { return v.is_null(); }, impl);
  }

  template <class Object>
  constexpr bool basic_object<Object>::is_primitive() const noexcept {
    return false;
  }

  template <class Array>
  constexpr bool basic_array<Array>::is_primitive() const noexcept {
    return false;
  }

  template <class String>
  constexpr bool basic_string<String>::is_primitive() const noexcept {
    return true;
  }

  template <class Number>
  constexpr bool basic_number<Number>::is_primitive() const noexcept {
    return true;
  }

  template <class Boolean>
  constexpr bool basic_flag<Boolean>::is_primitive() const noexcept {
    return true;
  }

  template <class Null>
  constexpr bool basic_null<Null>::is_primitive() const noexcept {
    return true;
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::is_primitive() const noexcept {
    return !is_object() && !is_array() && !is_null();
  }

  template <template <class> class RefCount>
  bool basic_buffer<RefCount>::is_primitive() const noexcept {
    return !is_object() && !is_array() && !is_null();
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::is_primitive() const noexcept {
    return !is_object() && !is_array() && !is_null();
  }

  template <class Object>
  auto basic_object<Object>::get_type() const noexcept -> type {
    return val.get_type();
  }

  template <class Array>
  auto basic_array<Array>::get_type() const noexcept -> type {
    return val.get_type();
  }

  template <class String>
  auto basic_string<String>::get_type() const noexcept -> type {
    return val.get_type();
  }

  template <class Number>
  auto basic_number<Number>::get_type() const noexcept -> type {
    return val.get_type();
  }

  template <class Boolean>
  auto basic_flag<Boolean>::get_type() const noexcept -> type {
    return val.get_type();
  }

  template <class Null>
  constexpr auto basic_null<Null>::get_type() const noexcept -> type {
    return type::null;
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::get_type() const noexcept -> type {
    if (is_object()) {
      return type::object;
    } else if (is_array()) {
      return type::array;
    } else if (is_str()) {
      return type::string;
    } else if (is_integer()) {
      return type::integer;
    } else if (is_decimal()) {
      return type::decimal;
    } else if (is_boolean()) {
      return type::boolean;
    } else {
      DART_ASSERT(is_null());
      return type::null;
    }
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::get_type() const noexcept -> type {
    return detail::simplify_type(raw.type);
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::get_type() const noexcept -> type {
    return shim::visit([] (auto& v) { return v.get_type(); }, impl);
  }

  template <class Object>
  bool basic_object<Object>::is_finalized() const noexcept {
    return val.is_finalized();
  }

  template <class Array>
  bool basic_array<Array>::is_finalized() const noexcept {
    return val.is_finalized();
  }

  template <class String>
  bool basic_string<String>::is_finalized() const noexcept {
    return val.is_finalized();
  }

  template <class Number>
  bool basic_number<Number>::is_finalized() const noexcept {
    return val.is_finalized();
  }

  template <class Boolean>
  bool basic_flag<Boolean>::is_finalized() const noexcept {
    return val.is_finalized();
  }

  template <class Null>
  bool basic_null<Null>::is_finalized() const noexcept {
    return value_type::make_null().is_finalized();
  }

  template <template <class> class RefCount>
  constexpr bool basic_heap<RefCount>::is_finalized() const noexcept {
    return false;
  }

  template <template <class> class RefCount>
  constexpr bool basic_buffer<RefCount>::is_finalized() const noexcept {
    return true;
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::is_finalized() const noexcept {
    return shim::holds_alternative<basic_buffer<RefCount>>(impl);
  }

  template <class Object>
  auto basic_object<Object>::refcount() const noexcept -> size_type {
    return val.refcount();
  }

  template <class Array>
  auto basic_array<Array>::refcount() const noexcept -> size_type {
    return val.refcount();
  }

  template <class String>
  auto basic_string<String>::refcount() const noexcept -> size_type {
    return val.refcount();
  }

  template <class Number>
  auto basic_number<Number>::refcount() const noexcept -> size_type {
    return val.refcount();
  }

  template <class Boolean>
  auto basic_flag<Boolean>::refcount() const noexcept -> size_type {
    return val.refcount();
  }

  template <class Null>
  auto basic_null<Null>::refcount() const noexcept -> size_type {
    return value_type::make_null().refcount();
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::refcount() const noexcept -> size_type {
    if (is_object()) {
      return shim::get<fields_type>(data).use_count();
    } else if (is_array()) {
      return shim::get<elements_type>(data).use_count();
    } else {
      auto* str = shim::get_if<dynamic_string_layout>(&data);
      if (str) return str->ptr.use_count();
      else if (is_null()) return 0;
      else return 1;
    }
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::refcount() const noexcept -> size_type {
    return buffer_ref.use_count();
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::refcount() const noexcept -> size_type {
    return shim::visit([] (auto& v) { return v.refcount(); }, impl);
  }

  template <class Object>
  auto basic_object<Object>::begin() const -> iterator {
    return val.begin();
  }

  template <class Array>
  auto basic_array<Array>::begin() const -> iterator {
    return val.begin();
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::begin() const -> iterator {
    if (is_object()) {
      auto deref = [] (auto& it) { return it->second; };
      return iterator(detail::dn_iterator<RefCount>(try_get_fields()->begin(), deref));
    } else if (is_array()) {
      auto deref = [] (auto& it) { return *it; };
      return iterator(detail::dn_iterator<RefCount>(try_get_elements()->begin(), deref));
    } else {
      throw type_error("dart::heap isn't an aggregate and cannot be iterated over");
    }
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::begin() const -> iterator {
    // Construct an iterator that knows how to find its memory.
    auto impl = detail::aggregate_deref<RefCount>([] (auto& aggr) { return aggr.begin(); }, raw);
    return iterator(*this, impl);
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::begin() const -> iterator {
    return shim::visit([] (auto& v) -> iterator { return v.begin(); }, impl);
  }

  template <class Object>
  auto basic_object<Object>::cbegin() const -> iterator {
    return val.cbegin();
  }

  template <class Array>
  auto basic_array<Array>::cbegin() const -> iterator {
    return val.cbegin();
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::cbegin() const -> iterator {
    return begin();
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::cbegin() const -> iterator {
    return begin();
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::cbegin() const -> iterator {
    return begin();
  }

  template <class Object>
  auto basic_object<Object>::end() const -> iterator {
    return val.end();
  }

  template <class Array>
  auto basic_array<Array>::end() const -> iterator {
    return val.end();
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::end() const -> iterator {
    if (is_object()) {
      auto deref = [] (auto& it) { return it->second; };
      return iterator(detail::dn_iterator<RefCount>(try_get_fields()->end(), deref));
    } else if (is_array()) {
      auto deref = [] (auto& it) { return *it; };
      return iterator(detail::dn_iterator<RefCount>(try_get_elements()->end(), deref));
    } else {
      throw type_error("dart::heap isn't an aggregate and cannot be iterated over");
    }
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::end() const -> iterator {
    // Construct an iterator that knows how to find its memory.
    auto impl = detail::aggregate_deref<RefCount>([] (auto& aggr) { return aggr.end(); }, raw);
    return iterator(*this, impl);
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::end() const -> iterator {
    return shim::visit([] (auto& v) -> iterator { return v.end(); }, impl);
  }

  template <class Object>
  auto basic_object<Object>::cend() const -> iterator {
    return val.cend();
  }

  template <class Array>
  auto basic_array<Array>::cend() const -> iterator {
    return val.cend();
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::cend() const -> iterator {
    return end();
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::cend() const -> iterator {
    return end();
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::cend() const -> iterator {
    return end();
  }

  template <class Object>
  auto basic_object<Object>::rbegin() const -> reverse_iterator {
    return val.rbegin();
  }

  template <class Array>
  auto basic_array<Array>::rbegin() const -> reverse_iterator {
    return val.rbegin();
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::rbegin() const -> reverse_iterator {
    return reverse_iterator {end()};
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::rbegin() const -> reverse_iterator {
    return reverse_iterator {end()};
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::rbegin() const -> reverse_iterator {
    return reverse_iterator {end()};
  }

  template <class Object>
  auto basic_object<Object>::rend() const -> reverse_iterator {
    return val.rend();
  }

  template <class Array>
  auto basic_array<Array>::rend() const -> reverse_iterator {
    return val.rend();
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::rend() const -> reverse_iterator {
    return reverse_iterator {begin()};
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::rend() const -> reverse_iterator {
    return reverse_iterator {begin()};
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::rend() const -> reverse_iterator {
    return reverse_iterator {begin()};
  }

  template <class Object>
  auto basic_object<Object>::key_begin() const -> iterator {
    return val.key_begin();
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::key_begin() const -> iterator {
    if (is_object()) {
      auto deref = [] (auto& it) { return it->first; };
      return iterator(detail::dn_iterator<RefCount>(try_get_fields()->begin(), deref));
    } else {
      throw type_error("dart::heap is not an object and cannot iterate over keys");
    }
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::key_begin() const -> iterator {
    return iterator(*this, detail::get_object<RefCount>(raw)->key_begin());
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::key_begin() const -> iterator {
    return shim::visit([] (auto& v) -> iterator { return v.key_begin(); }, impl);
  }

  template <class Object>
  auto basic_object<Object>::rkey_begin() const -> reverse_iterator {
    return val.rkey_begin();
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::rkey_begin() const -> reverse_iterator {
    return reverse_iterator {key_end()};
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::rkey_begin() const -> reverse_iterator {
    return reverse_iterator {key_end()};
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::rkey_begin() const -> reverse_iterator {
    return reverse_iterator {key_end()};
  }

  template <class Object>
  auto basic_object<Object>::key_end() const -> iterator {
    return val.key_end();
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::key_end() const -> iterator {
    if (is_object()) {
      auto deref = [] (auto& it) { return it->first; };
      return iterator(detail::dn_iterator<RefCount>(try_get_fields()->end(), deref));
    } else {
      throw type_error("dart::heap is not an object and cannot iterate over keys");
    }
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::key_end() const -> iterator {
    return iterator(*this, detail::get_object<RefCount>(raw)->key_end());
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::key_end() const -> iterator {
    return shim::visit([] (auto& v) -> iterator { return v.key_end(); }, impl);
  }

  template <class Object>
  auto basic_object<Object>::rkey_end() const -> reverse_iterator {
    return val.rkey_end();
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::rkey_end() const -> reverse_iterator {
    return reverse_iterator {key_begin()};
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::rkey_end() const -> reverse_iterator {
    return reverse_iterator {key_begin()};
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::rkey_end() const -> reverse_iterator {
    return reverse_iterator {key_begin()};
  }

  template <class Object>
  auto basic_object<Object>::kvbegin() const -> std::tuple<iterator, iterator> {
    return val.kvbegin();
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::kvbegin() const -> std::tuple<iterator, iterator> {
    return std::tuple<iterator, iterator> {key_begin(), begin()};
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::kvbegin() const -> std::tuple<iterator, iterator> {
    return std::tuple<iterator, iterator> {key_begin(), begin()};
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::kvbegin() const -> std::tuple<iterator, iterator> {
    return std::tuple<iterator, iterator> {key_begin(), begin()};
  }

  template <class Object>
  auto basic_object<Object>::kvend() const -> std::tuple<iterator, iterator> {
    return val.kvend();
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::kvend() const -> std::tuple<iterator, iterator> {
    return std::tuple<iterator, iterator> {key_end(), end()};
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::kvend() const -> std::tuple<iterator, iterator> {
    return std::tuple<iterator, iterator> {key_end(), end()};
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::kvend() const -> std::tuple<iterator, iterator> {
    return std::tuple<iterator, iterator> {key_end(), end()};
  }

  template <class Object>
  auto basic_object<Object>::rkvbegin() const -> std::tuple<reverse_iterator, reverse_iterator> {
    return val.rkvbegin();
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::rkvbegin() const -> std::tuple<reverse_iterator, reverse_iterator> {
    return std::tuple<reverse_iterator, reverse_iterator> {rkey_begin(), rbegin()};
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::rkvbegin() const -> std::tuple<reverse_iterator, reverse_iterator> {
    return std::tuple<reverse_iterator, reverse_iterator> {rkey_begin(), rbegin()};
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::rkvbegin() const -> std::tuple<reverse_iterator, reverse_iterator> {
    return std::tuple<reverse_iterator, reverse_iterator> {rkey_begin(), rbegin()};
  }

  template <class Object>
  auto basic_object<Object>::rkvend() const -> std::tuple<reverse_iterator, reverse_iterator> {
    return val.rkvend();
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::rkvend() const -> std::tuple<reverse_iterator, reverse_iterator> {
    return std::tuple<reverse_iterator, reverse_iterator> {rkey_end(), rend()};
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::rkvend() const -> std::tuple<reverse_iterator, reverse_iterator> {
    return std::tuple<reverse_iterator, reverse_iterator> {rkey_end(), rend()};
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::rkvend() const -> std::tuple<reverse_iterator, reverse_iterator> {
    return std::tuple<reverse_iterator, reverse_iterator> {rkey_end(), rend()};
  }

  inline namespace literals {

    inline packet operator ""_dart(char const* val, size_t len) {
      return packet::make_string({val, len});
    }

    inline packet operator ""_dart(unsigned long long val) {
      return packet::make_integer(val);
    }

    inline packet operator ""_dart(long double val) {
      return packet::make_decimal(val);
    }

  }

}

#endif
