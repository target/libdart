#ifndef DART_PACKET_API_H
#define DART_PACKET_API_H

/*----- Project Includes -----*/

#include "../common.h"

/*----- Function Implementations -----*/

namespace dart {

  template <template <class> class RefCount>
  template <class KeyType, class>
  basic_packet<RefCount> basic_packet<RefCount>::operator [](KeyType const& identifier) const& {
    return get(identifier);
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::operator [](KeyType const& identifier) && {
    return std::move(*this).get(identifier);
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::operator ==(basic_packet const& other) const noexcept {
    return this->operator ==<RefCount>(other);
  }

  template <template <class> class RefCount>
  template <template <class> class OtherRC>
  bool basic_packet<RefCount>::operator ==(basic_packet<OtherRC> const& other) const noexcept {
    // Check if we're comparing against ourselves.
    // Cast is necessary to ensure validity if we're comparing
    // against a different refcounter.
    if (static_cast<void const*>(this) == static_cast<void const*>(&other)) return true;
    return shim::visit([] (auto& lhs, auto& rhs) { return lhs == rhs; }, impl, other.impl);
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::operator !=(basic_packet const& other) const noexcept {
    return this->operator !=<RefCount>(other);
  }

  template <template <class> class RefCount>
  template <template <class> class OtherRC>
  bool basic_packet<RefCount>::operator !=(basic_packet<OtherRC> const& other) const noexcept {
    return !(*this == other);
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::operator ->() noexcept -> basic_packet* {
    return this;
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::operator ->() const noexcept -> basic_packet const* {
    return this;
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>::operator bool() const noexcept {
    if (!is_boolean()) return !is_null();
    else return boolean();
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>::operator view() const& noexcept {
    return shim::visit([] (auto& impl) -> view {
      return typename std::decay_t<decltype(impl)>::view {impl};
    }, impl);
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>::operator std::string() const {
    return std::string {strv()};
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>::operator shim::string_view() const {
    return strv();
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>::operator int64_t() const {
    return integer();
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>::operator double() const {
    return decimal();
  }

  template <template <class> class RefCount>
  basic_packet<RefCount> basic_packet<RefCount>::make_null() noexcept {
    return basic_heap<RefCount>::make_null();
  }

  template <template <class> class RefCount>
  template <class KeyType, class ValueType, class>
  auto basic_packet<RefCount>::insert(KeyType&& key, ValueType&& value) -> iterator {
    return get_heap().insert(std::forward<KeyType>(key), std::forward<ValueType>(value));
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
  auto basic_packet<RefCount>::set(KeyType&& key, ValueType&& value) -> iterator {
    return get_heap().set(std::forward<KeyType>(key), std::forward<ValueType>(value));
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
  template <template <class> class, class>
  auto basic_packet<RefCount>::erase(iterator pos) -> iterator {
    if (!pos) throw std::invalid_argument("dart::packet cannot erase from a valueless iterator");
    auto* it = shim::get_if<typename basic_heap<RefCount>::iterator>(&pos.impl);
    if (it) return get_heap().erase(*it);
    else throw type_error("dart::packet cannot erase iterators from other/finalized packets");
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  void basic_packet<RefCount>::clear() {
    get_heap().clear();
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>& basic_packet<RefCount>::definalize() & {
    if (is_finalized()) impl = basic_heap<RefCount> {shim::get<basic_buffer<RefCount>>(impl)};
    return *this;
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::definalize() && {
    definalize();
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>& basic_packet<RefCount>::lift() & {
    return definalize();
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::lift() && {
    return std::move(*this).definalize();
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>& basic_packet<RefCount>::finalize() & {
    if (!is_finalized()) impl = basic_buffer<RefCount> {shim::get<basic_heap<RefCount>>(impl)};
    return *this;
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::finalize() && {
    finalize();
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>& basic_packet<RefCount>::lower() & {
    return finalize();
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::lower() && {
    return std::move(*this).finalize();
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
  basic_packet<RefCount> basic_packet<RefCount>::get(KeyType const& identifier) const& {
    return shim::visit([&] (auto& v) -> basic_packet { return v.get(identifier); }, impl);
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::get(KeyType const& identifier) && {
    shim::visit([&] (auto& v) { v = std::move(v).get(identifier); }, impl);
    return std::move(*this);
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
  basic_packet<RefCount> basic_packet<RefCount>::at(KeyType const& identifier) const& {
    return shim::visit([&] (auto& v) -> basic_packet { return v.at(identifier); }, impl);
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::at(KeyType const& identifier) && {
    shim::visit([&] (auto& v) { v = std::move(v).at(identifier); }, impl);
    return std::move(*this);
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

  template <template <class> class RefCount>
  gsl::span<gsl::byte const> basic_packet<RefCount>::get_bytes() const {
    return get_buffer().get_bytes();
  }

  template <template <class> class RefCount>
  size_t basic_packet<RefCount>::share_bytes(RefCount<gsl::byte const>& bytes) const {
    return get_buffer().share_bytes(bytes);
  }

  template <template <class> class RefCount>
  std::unique_ptr<gsl::byte const[], void (*) (gsl::byte const*)> basic_packet<RefCount>::dup_bytes() const {
    size_type dummy;
    return dup_bytes(dummy);
  }

  template <template <class> class RefCount>
  std::unique_ptr<gsl::byte const[], void (*) (gsl::byte const*)> basic_packet<RefCount>::dup_bytes(size_type& len) const {
    return get_buffer().dup_bytes(len);
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::size() const -> size_type {
    return shim::visit([] (auto& v) { return v.size(); }, impl);
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::empty() const {
    return size() == 0ULL;
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::is_object() const noexcept {
    return shim::visit([] (auto& v) { return v.is_object(); }, impl);
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::is_array() const noexcept {
    return shim::visit([] (auto& v) { return v.is_array(); }, impl);
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::is_aggregate() const noexcept {
    return is_object() || is_array();
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::is_str() const noexcept {
    return shim::visit([] (auto& v) { return v.is_str(); }, impl);
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::is_integer() const noexcept {
    return shim::visit([] (auto& v) { return v.is_integer(); }, impl);
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::is_decimal() const noexcept {
    return shim::visit([] (auto& v) { return v.is_decimal(); }, impl);
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::is_numeric() const noexcept {
    return is_integer() || is_decimal();
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::is_boolean() const noexcept {
    return shim::visit([] (auto& v) { return v.is_boolean(); }, impl);
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::is_null() const noexcept {
    return shim::visit([] (auto& v) { return v.is_null(); }, impl);
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::is_primitive() const noexcept {
    return !is_object() && !is_array() && !is_null();
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::get_type() const noexcept -> type {
    return shim::visit([] (auto& v) { return v.get_type(); }, impl);
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::is_finalized() const noexcept {
    return shim::holds_alternative<basic_buffer<RefCount>>(impl);
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::refcount() const noexcept -> size_type {
    return shim::visit([] (auto& v) { return v.refcount(); }, impl);
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::begin() const -> iterator {
    return shim::visit([] (auto& v) -> iterator { return v.begin(); }, impl);
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::cbegin() const -> iterator {
    return begin();
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::end() const -> iterator {
    return shim::visit([] (auto& v) -> iterator { return v.end(); }, impl);
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::cend() const -> iterator {
    return end();
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::rbegin() const -> reverse_iterator {
    return reverse_iterator {end()};
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::rend() const -> reverse_iterator {
    return reverse_iterator {begin()};
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::key_begin() const -> iterator {
    return shim::visit([] (auto& v) -> iterator { return v.key_begin(); }, impl);
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::rkey_begin() const -> reverse_iterator {
    return reverse_iterator {key_end()};
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::key_end() const -> iterator {
    return shim::visit([] (auto& v) -> iterator { return v.key_end(); }, impl);
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::rkey_end() const -> reverse_iterator {
    return reverse_iterator {key_begin()};
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::kvbegin() const -> std::tuple<iterator, iterator> {
    return std::tuple<iterator, iterator> {key_begin(), begin()};
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::kvend() const -> std::tuple<iterator, iterator> {
    return std::tuple<iterator, iterator> {key_end(), end()};
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::rkvbegin() const -> std::tuple<reverse_iterator, reverse_iterator> {
    return std::tuple<reverse_iterator, reverse_iterator> {rkey_begin(), rbegin()};
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::rkvend() const -> std::tuple<reverse_iterator, reverse_iterator> {
    return std::tuple<reverse_iterator, reverse_iterator> {rkey_end(), rend()};
  }

  template <template <class> class RefCount>
  constexpr bool basic_packet<RefCount>::is_view() const noexcept {
    return !refcount::is_owner<RefCount>::value;
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  auto basic_packet<RefCount>::as_owner() const noexcept {
    return shim::visit([] (auto& impl) -> refcount::owner_indirection_t<dart::basic_packet, RefCount> {
      return impl.as_owner();
    }, impl);
  }

}

#endif
