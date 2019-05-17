#ifndef DART_STRING_VIEW_IMPL_H
#define DART_STRING_VIEW_IMPL_H

/*----- Local Includes -----*/

#include "string_view.h"

/*----- Macro Definitions -----*/

#define DART_DEFINE_OPERATORS(class_name)                                                     \
  constexpr bool operator ==(class_name lhs, class_name rhs) {                                \
    return lhs.compare(rhs) == 0;                                                             \
  }                                                                                           \
  constexpr bool operator !=(class_name lhs, class_name rhs) {                                \
    return lhs.compare(rhs) != 0;                                                             \
  }                                                                                           \
  constexpr bool operator <(class_name lhs, class_name rhs) {                                 \
    return lhs.compare(rhs) < 0;                                                              \
  }                                                                                           \
  constexpr bool operator <=(class_name lhs, class_name rhs) {                                \
    return lhs.compare(rhs) <= 0;                                                             \
  }                                                                                           \
  constexpr bool operator >(class_name lhs, class_name rhs) {                                 \
    return lhs.compare(rhs) > 0;                                                              \
  }                                                                                           \
  constexpr bool operator >=(class_name lhs, class_name rhs) {                                \
    return lhs.compare(rhs) >= 0;                                                             \
  }


/*----- Type Declarations -----*/

namespace dart {

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::operator [](size_type idx) const noexcept -> const_reference {
    // No bounds checking. Gotta love the STL.
    // Even in the era of branch predictors and zero-overhead exception tables,
    // one branch is far too high a cost for basic safety.
    return data()[idx];
  }

  template <class CharT, class Traits>
  template <template <class, class> class View>
  constexpr basic_string_view<CharT, Traits>::operator View<CharT, Traits>() const noexcept {
    return View<CharT, Traits> {data(), size()};
  }

  template <class CharT, class Traits>
  template <class Allocator>
  basic_string_view<CharT, Traits>::operator std::basic_string<CharT, Traits, Allocator>() const {
    return std::basic_string<CharT, Traits, Allocator> {data(), size()};
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::at(size_type idx) const -> const_reference {
    if (idx >= size()) throw std::out_of_range("basic_string_view::at() is out of range");
    return (*this)[idx];
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::front() const -> const_reference {
    // No bounds checking. Gotta love the STL.
    // Even in the era of branch predictors and zero-overhead exception tables,
    // one branch is far too high a cost for basic safety.
    return (*this)[0];
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::back() const -> const_reference {
    // No bounds checking. Gotta love the STL.
    // Even in the era of branch predictors and zero-overhead exception tables,
    // one branch is far too high a cost for basic safety.
    // If we're the empty string, this will explode in FANTASTIC fashion.
    return (*this)[size() - 1];
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::data() const -> const_pointer {
    // No bounds checking. Gotta love the STL.
    // Even in the era of branch predictors and zero-overhead exception tables,
    // one branch is far too high a cost for basic safety.
    // If we're the empty string, this will explode in FANTASTIC fashion.
    return chars;
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::size() const noexcept -> size_type {
    return len;
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::length() const noexcept -> size_type {
    return size();
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::max_size() const noexcept -> size_type {
    return npos - 1;
  }

  template <class CharT, class Traits>
  constexpr bool basic_string_view<CharT, Traits>::empty() const noexcept {
    return size() == 0U;
  }

  template <class CharT, class Traits>
  constexpr void basic_string_view<CharT, Traits>::remove_prefix(size_type num) noexcept {
    // Once again, the standard explicitly mandates no check here.
    len -= num;
    chars += num;
  }

  template <class CharT, class Traits>
  constexpr void basic_string_view<CharT, Traits>::remove_suffix(size_type num) noexcept {
    // Once again, the standard explicitly mandates no check here.
    len -= num;
  }

  template <class CharT, class Traits>
  constexpr void basic_string_view<CharT, Traits>::swap(basic_string_view& other) noexcept {
    if (this == &other) return;
    auto tmp {*this};
    *this = other;
    other = tmp;
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::copy(pointer out, size_type count, size_type offset) const
    -> size_type
  {
    if (offset > size()) throw std::out_of_range("basic_string_view::copy() is out of range");
    auto rcount = std::min(count, size() - offset);
    std::copy_n(data() + offset, rcount, out);
    return rcount;
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::substr(size_type offset, size_type count) const
    -> basic_string_view
  {
    if (offset > size()) throw std::out_of_range("basic_string_view::substr is out of range");
    auto rcount = std::min(count, size() - offset);
    return {data() + offset, rcount};
  }

  template <class CharT, class Traits>
  constexpr int basic_string_view<CharT, Traits>::compare(basic_string_view other) const noexcept {
    // FIXME: Try to optimize this.
    auto rlen = std::min(size(), other.size());
    auto comp = traits_type::compare(data(), other.data(), rlen);
    if (comp) return comp;
    else if (size() < other.size()) return -1;
    else if (size() > other.size()) return 1;
    else return 0;
  }

  template <class CharT, class Traits>
  constexpr int basic_string_view<CharT, Traits>::compare(size_type offset,
      size_type count, basic_string_view other) const {
    return substr(offset, count).compare(other);
  }

  template <class CharT, class Traits>
  constexpr int basic_string_view<CharT, Traits>::compare(size_type offset_one,
      size_type count_one, basic_string_view other, size_type offset_two, size_type count_two) const {
    return substr(offset_one, count_one).compare(other.substr(offset_two, count_two));
  }

  template <class CharT, class Traits>
  constexpr int basic_string_view<CharT, Traits>::compare(const_pointer other) const noexcept {
    return compare(basic_string_view {other});
  }

  template <class CharT, class Traits>
  constexpr int basic_string_view<CharT, Traits>::compare(size_type offset,
      size_type count, const_pointer other) const noexcept {
    return substr(offset, count).compare(basic_string_view {other});
  }

  template <class CharT, class Traits>
  constexpr int basic_string_view<CharT, Traits>::compare(size_type offset,
      size_type count_one, const_pointer other, size_type count_two) const noexcept {
    return substr(offset, count_one).compare(basic_string_view {other, count_two});
  }

  template <class CharT, class Traits>
  constexpr bool basic_string_view<CharT, Traits>::starts_with(basic_string_view other) const noexcept {
    return size() >= other.size() && compare(0, other.size(), other) == 0;
  }

  template <class CharT, class Traits>
  constexpr bool basic_string_view<CharT, Traits>::starts_with(value_type other) const noexcept {
    return !empty() && front() == other;
  }

  template <class CharT, class Traits>
  constexpr bool basic_string_view<CharT, Traits>::starts_with(const_pointer other) const noexcept {
    return starts_with(basic_string_view {other});
  }

  template <class CharT, class Traits>
  constexpr bool basic_string_view<CharT, Traits>::ends_with(basic_string_view other) const noexcept {
    return size() >= other.size() && compare(size() - other.size(), npos, other) == 0;
  }

  template <class CharT, class Traits>
  constexpr bool basic_string_view<CharT, Traits>::ends_with(value_type other) const noexcept {
    return !empty() && back() == other;
  }

  template <class CharT, class Traits>
  constexpr bool basic_string_view<CharT, Traits>::ends_with(const_pointer other) const noexcept {
    return ends_with(basic_string_view {other});
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::find(basic_string_view target, size_type offset) const noexcept
    -> size_type
  {
    // Short circuit if we can't possibly contain the target.
    if (target.size() + offset > size()) return npos;

    // Search until we find it or walk off the end.
    while (offset + target.size() <= size()) {
      if (compare(offset, target.size(), target) == 0) return offset;
      ++offset;
    }
    return npos;
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::find(value_type target, size_type offset) const noexcept
    -> size_type
  {
    return find(basic_string_view {&target, 1}, offset);
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::find(const_pointer target, size_type offset, size_type num) const noexcept
    -> size_type
  {
    return find(basic_string_view {target, num}, offset);
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::find(const_pointer target, size_type offset) const noexcept
    -> size_type
  {
    return find(basic_string_view {target}, offset);
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::rfind(basic_string_view target, size_type offset) const noexcept
    -> size_type
  {
    // Clamp our offset.
    ssize_t safe_offset = std::min(offset, size());

    // Search until we find it or walk off the front.
    while (safe_offset - static_cast<ssize_t>(target.size()) >= 0) {
      if (compare(offset, target.size(), target) == 0) return offset;
      --offset;
    }
    return npos;
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::rfind(value_type target, size_type offset) const noexcept
    -> size_type
  {
    return rfind(basic_string_view {&target, 1}, offset);
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::rfind(const_pointer target, size_type offset, size_type num) const noexcept
    -> size_type
  {
    return rfind(basic_string_view {target, num}, offset);
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::rfind(const_pointer target, size_type offset) const noexcept
    -> size_type
  {
    return rfind(basic_string_view {target}, offset);
  }

  template <class CharT, class Traits>
  constexpr auto
  basic_string_view<CharT, Traits>::find_first_of(basic_string_view target, size_type offset) const noexcept
    -> size_type
  {
    // Short-circuit if we know this can't work.
    if (offset >= size()) return npos;

    // Try to find one of the requested characters.
    auto found = std::find_if(begin() + offset, end(), [=] (auto c) {
      return target.find(c) != npos;
    });
    return found != end() ? found - begin() : npos;
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::find_first_of(value_type target, size_type offset) const noexcept
    -> size_type
  {
    return find_first_of(basic_string_view {&target, 1}, offset);
  }

  template <class CharT, class Traits>
  constexpr auto
  basic_string_view<CharT, Traits>::find_first_of(const_pointer target, size_type offset, size_type num) const noexcept
    -> size_type
  {
    return find_first_of(basic_string_view {target, num}, offset);
  }

  template <class CharT, class Traits>
  constexpr auto
  basic_string_view<CharT, Traits>::find_first_of(const_pointer target, size_type offset) const noexcept
    -> size_type
  {
    return find_first_of(basic_string_view {target}, offset);
  }

  template <class CharT, class Traits>
  constexpr auto
  basic_string_view<CharT, Traits>::find_last_of(basic_string_view target, size_type offset) const noexcept
    -> size_type
  {
    // Short-circuit if we know this can't work.
    auto len = size();
    if (offset >= len) return npos;

    // Try to find one of the requested characters.
    auto found = std::find_if(rbegin() + offset, rend(), [=] (auto c) {
      return target.find(c) != npos;
    });
    return found != rend() ? rend() - found - 1 : npos;
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::find_last_of(value_type target, size_type offset) const noexcept
    -> size_type
  {
    return find_last_of(basic_string_view {&target, 1}, offset);
  }

  template <class CharT, class Traits>
  constexpr auto
  basic_string_view<CharT, Traits>::find_last_of(const_pointer target, size_type offset, size_type num) const noexcept
    -> size_type
  {
    return find_last_of(basic_string_view {target, num}, offset);
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::find_last_of(const_pointer target, size_type offset) const noexcept
    -> size_type
  {
    return find_last_of(basic_string_view {target}, offset);
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::find_first_not_of(basic_string_view target, size_type offset) const noexcept
    -> size_type
  {
    // Short-circuit if we know this can't work.
    if (offset >= size()) return npos;

    // Try to find one of the requested characters.
    auto found = std::find_if(begin() + offset, end(), [=] (auto c) {
      return target.find(c) == npos;
    });
    return found != end() ? begin() - found : npos;
  }

  template <class CharT, class Traits>
  constexpr auto
  basic_string_view<CharT, Traits>::find_first_not_of(value_type target, size_type offset) const noexcept
    -> size_type
  {
    return find_first_not_of(basic_string_view {&target, 1}, offset);
  }

  template <class CharT, class Traits>
  constexpr auto
  basic_string_view<CharT, Traits>::find_first_not_of(const_pointer target, size_type offset, size_type num) const noexcept
    -> size_type
  {
    return find_first_not_of(basic_string_view {target, num}, offset);
  }

  template <class CharT, class Traits>
  constexpr auto
  basic_string_view<CharT, Traits>::find_first_not_of(const_pointer target, size_type offset) const noexcept
    -> size_type
  {
    return find_first_not_of(basic_string_view {target}, offset);
  }

  template <class CharT, class Traits>
  constexpr auto
  basic_string_view<CharT, Traits>::find_last_not_of(basic_string_view target, size_type offset) const noexcept
    -> size_type
  {
    // Short-circuit if we know this can't work.
    if (offset >= size()) return npos;

    // Try to find one of the requested characters.
    auto found = std::find_if(rbegin() + offset, rend(), [=] (auto c) {
      return target.find(c) == npos;
    });
    return found != rend() ? rbegin() - found : npos;
  }

  template <class CharT, class Traits>
  constexpr auto
  basic_string_view<CharT, Traits>::find_last_not_of(value_type target, size_type offset) const noexcept
    -> size_type
  {
    return find_last_not_of(basic_string_view {&target, 1}, offset);
  }

  template <class CharT, class Traits>
  constexpr auto
  basic_string_view<CharT, Traits>::find_last_not_of(const_pointer target, size_type offset, size_type num) const noexcept
    -> size_type
  {
    return find_last_not_of(basic_string_view {target, num}, offset);
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::find_last_not_of(const_pointer target, size_type offset) const noexcept
    -> size_type
  {
    return find_last_not_of(basic_string_view {target}, offset);
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::begin() const noexcept -> iterator {
    return data();
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::cbegin() const noexcept -> iterator {
    return begin();
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::end() const noexcept -> iterator {
    return begin() + size();
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::cend() const noexcept -> iterator {
    return end();
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::rbegin() const noexcept -> reverse_iterator {
    return reverse_iterator {end()};
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::crbegin() const noexcept -> reverse_iterator {
    return rbegin();
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::rend() const noexcept -> reverse_iterator {
    return reverse_iterator {begin()};
  }

  template <class CharT, class Traits>
  constexpr auto basic_string_view<CharT, Traits>::crend() const noexcept -> reverse_iterator {
    return rend();
  }

  // Implicit conversions aren't considered during template instantiation
  // and so we need to manually define all of these.
  DART_DEFINE_OPERATORS(string_view);
  DART_DEFINE_OPERATORS(wstring_view);
  DART_DEFINE_OPERATORS(u16string_view);
  DART_DEFINE_OPERATORS(u32string_view);

}

#endif
