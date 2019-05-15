#ifndef DART_STRING_VIEW_H
#define DART_STRING_VIEW_H

/*----- System Includes -----*/

#include <string>
#include <limits>
#include <utility>
#include <algorithm>
#include <stdexcept>

/*----- Type Declarations -----*/

namespace dart {

  template <class CharT, class Traits = std::char_traits<CharT>>
  class basic_string_view {

    public:

      /*----- Public Types -----*/

      using traits_type = Traits;
      using value_type = CharT;
      using pointer = CharT*;
      using const_pointer = CharT const*;
      using reference = CharT&;
      using const_reference = CharT const&;
      using const_iterator = CharT const*;
      using iterator = const_iterator;
      using reverse_iterator = std::reverse_iterator<const_iterator>;
      using size_type = std::size_t;
      using difference_type = std::ptrdiff_t;

      /*----- Public Members -----*/

      static constexpr size_type npos = std::numeric_limits<size_type>::max();

      /*----- Lifecycle Functions -----*/

      constexpr basic_string_view() noexcept :
        len {0},
        chars {nullptr}
      {}
      template <class Allocator>
      constexpr basic_string_view(std::basic_string<CharT, Traits, Allocator> const& other) :
        len {other.size()},
        chars {other.data()}
      {}
      template <template <class, class> class View, class =
        std::enable_if_t<
          !std::is_same<
            std::decay_t<View<CharT, Traits>>,
            std::basic_string<CharT, Traits>
          >::value
        >
      >
      constexpr basic_string_view(View<CharT, Traits> other) noexcept : len {other.size()}, chars {other.data()} {}
      constexpr basic_string_view(basic_string_view const& other) noexcept = default;
      constexpr basic_string_view(const_pointer s, size_type count) noexcept : len {count}, chars {s} {}
      constexpr basic_string_view(const_pointer s) noexcept : len {traits_type::length(s)}, chars {s} {}

      /*----- Operators -----*/

      constexpr auto operator =(basic_string_view const&) noexcept -> basic_string_view& = default;

      constexpr auto operator [](size_type idx) const noexcept -> const_reference;

      template <template <class, class> class View>
      explicit constexpr operator View<CharT, Traits>() const noexcept;

      template <class Allocator>
      explicit operator std::basic_string<CharT, Traits, Allocator>() const;

      /*----- Public API -----*/

      /*----- Access -----*/

      constexpr auto at(size_type idx) const -> const_reference;
      constexpr auto front() const -> const_reference;
      constexpr auto back() const -> const_reference;
      constexpr auto data() const -> const_pointer;

      /*----- Capacity -----*/

      constexpr auto size() const noexcept -> size_type;
      constexpr auto length() const noexcept -> size_type;
      constexpr auto max_size() const noexcept -> size_type;
      constexpr bool empty() const noexcept;

      /*----- Mutation -----*/

      constexpr void remove_prefix(size_type num) noexcept;
      constexpr void remove_suffix(size_type num) noexcept;
      constexpr void swap(basic_string_view& other) noexcept;

      /*----- Operations -----*/

      // Representation Modification.
      constexpr auto copy(pointer out, size_type len, size_type offset = 0) const -> size_type;
      constexpr auto substr(size_type offset = 0, size_type len = npos) const -> basic_string_view;

      // Compare.
      constexpr int compare(basic_string_view other) const noexcept;
      constexpr int compare(size_type offset, size_type count, basic_string_view other) const;
      constexpr int compare(size_type offset_one, size_type count_one,
          basic_string_view other, size_type offset_two, size_type count_two) const;
      constexpr int compare(const_pointer other) const noexcept;
      constexpr int compare(size_type offset, size_type count, const_pointer other) const noexcept;
      constexpr int compare(size_type offset,
          size_type count_one, const_pointer other, size_type count_two) const noexcept;

      // Prefix/suffix checks.
      constexpr bool starts_with(basic_string_view other) const noexcept;
      constexpr bool starts_with(value_type other) const noexcept;
      constexpr bool starts_with(const_pointer other) const noexcept;
      constexpr bool ends_with(basic_string_view other) const noexcept;
      constexpr bool ends_with(value_type other) const noexcept;
      constexpr bool ends_with(const_pointer other) const noexcept;

      // Lookup.
      constexpr auto find(basic_string_view target, size_type offset = 0) const noexcept -> size_type;
      constexpr auto find(value_type target, size_type offset = 0) const noexcept -> size_type;
      constexpr auto find(const_pointer target, size_type offset, size_type count) const noexcept -> size_type;
      constexpr auto find(const_pointer target, size_type offset = 0) const noexcept -> size_type;
      constexpr auto rfind(basic_string_view target, size_type offset = npos) const noexcept -> size_type;
      constexpr auto rfind(value_type target, size_type offset = 0) const noexcept -> size_type;
      constexpr auto rfind(const_pointer target, size_type offset, size_type count) const noexcept -> size_type;
      constexpr auto rfind(const_pointer target, size_type offset = 0) const noexcept -> size_type;

      // Membership Checks.
      constexpr auto find_first_of(basic_string_view target, size_type offset = 0) const noexcept -> size_type;
      constexpr auto find_first_of(value_type target, size_type offset = 0) const noexcept -> size_type;
      constexpr auto find_first_of(const_pointer target, size_type offset, size_type count) const noexcept -> size_type;
      constexpr auto find_first_of(const_pointer target, size_type offset = 0) const noexcept -> size_type;
      constexpr auto find_last_of(basic_string_view target, size_type offset = 0) const noexcept -> size_type;
      constexpr auto find_last_of(value_type target, size_type offset = 0) const noexcept -> size_type;
      constexpr auto find_last_of(const_pointer target, size_type offset, size_type count) const noexcept -> size_type;
      constexpr auto find_last_of(const_pointer target, size_type offset = 0) const noexcept -> size_type;

      // Negated Membership Checks.
      constexpr auto find_first_not_of(basic_string_view target, size_type offset = 0) const noexcept -> size_type;
      constexpr auto find_first_not_of(value_type target, size_type offset = 0) const noexcept -> size_type;
      constexpr auto find_first_not_of(const_pointer target, size_type offset, size_type count) const noexcept -> size_type;
      constexpr auto find_first_not_of(const_pointer target, size_type offset = 0) const noexcept -> size_type;
      constexpr auto find_last_not_of(basic_string_view target, size_type offset = 0) const noexcept -> size_type;
      constexpr auto find_last_not_of(value_type target, size_type offset = 0) const noexcept -> size_type;
      constexpr auto find_last_not_of(const_pointer target, size_type offset, size_type count) const noexcept -> size_type;
      constexpr auto find_last_not_of(const_pointer target, size_type offset = 0) const noexcept -> size_type;

      /*----- Iteration -----*/

      constexpr auto begin() const noexcept -> iterator;
      constexpr auto cbegin() const noexcept -> const_iterator;

      constexpr auto end() const noexcept -> iterator;
      constexpr auto cend() const noexcept -> const_iterator;

      constexpr auto rbegin() const noexcept -> reverse_iterator;
      constexpr auto crbegin() const noexcept -> reverse_iterator;

      constexpr auto rend() const noexcept -> reverse_iterator;
      constexpr auto crend() const noexcept -> reverse_iterator;

    private:

      /*----- Private Members -----*/

      size_type len;
      const_pointer chars;

  };

  template <class CharT, class Traits>
  constexpr bool operator ==(basic_string_view<CharT, Traits> lhs, basic_string_view<CharT, Traits> rhs) {
    return lhs.compare(rhs) == 0;
  }

  template <class CharT, class Traits>
  constexpr bool operator !=(basic_string_view<CharT, Traits> lhs, basic_string_view<CharT, Traits> rhs) {
    return lhs.compare(rhs) != 0;
  }

  template <class CharT, class Traits>
  constexpr bool operator <(basic_string_view<CharT, Traits> lhs, basic_string_view<CharT, Traits> rhs) {
    return lhs.compare(rhs) < 0;
  }

  template <class CharT, class Traits>
  constexpr bool operator <=(basic_string_view<CharT, Traits> lhs, basic_string_view<CharT, Traits> rhs) {
    return lhs.compare(rhs) <= 0;
  }

  template <class CharT, class Traits>
  constexpr bool operator >(basic_string_view<CharT, Traits> lhs, basic_string_view<CharT, Traits> rhs) {
    return lhs.compare(rhs) > 0;
  }

  template <class CharT, class Traits>
  constexpr bool operator >=(basic_string_view<CharT, Traits> lhs, basic_string_view<CharT, Traits> rhs) {
    return lhs.compare(rhs) >= 0;
  }

  template <class CharT, class Traits>
  std::basic_ostream<CharT, Traits>&
  operator <<(std::basic_ostream<CharT, Traits>& os, basic_string_view<CharT, Traits> str) {
    // Not standard compliant, and will fail with embedded nulls, but it'll work.
    return (os << str.data());
  }

  using string_view = basic_string_view<char>;
  using wstring_view = basic_string_view<wchar_t>;
  using u16string_view = basic_string_view<char16_t>;
  using u32string_view = basic_string_view<char32_t>;

}

/*----- Template Implementations -----*/

#include "string_view.tcc"

#endif
