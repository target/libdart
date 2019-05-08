#ifndef DART_OPTIONAL_H
#define DART_OPTIONAL_H

/*----- System Includes -----*/

#include <cassert>
#include <utility>
#include <type_traits>

/*----- Local Includes -----*/

#include "dart_meta.h"

/*----- Type Declarations -----*/

namespace dart {

  template <class T>
  class optional;

  namespace detail {

    template <class T, class U>
    struct constructor_conversion_check {
      static constexpr auto value = !(
        std::is_constructible<T, optional<U>&>::value
        || std::is_constructible<T, optional<U> const&>::value
        || std::is_constructible<T, optional<U>&&>::value
        || std::is_constructible<T, optional<U> const&&>::value
        || std::is_convertible<optional<U>&, T>::value
        || std::is_convertible<optional<U> const&, T>::value
        || std::is_convertible<optional<U>&&, T>::value
        || std::is_convertible<optional<U> const&&, T>::value
      );
    };

    template <class T, class U>
    struct assignment_conversion_check {
      static constexpr auto value = !(
        constructor_conversion_check<T, U>::value
        || std::is_assignable<T&, optional<U>&>::value
        || std::is_assignable<T&, optional<U> const&>::value
        || std::is_assignable<T&, optional<U>&&>::value
        || std::is_assignable<T&, optional<U> const&&>::value
      );
    };

    template <class T>
    struct safely_assignable {
      static constexpr auto value =
        std::is_nothrow_move_assignable<T>::value && std::is_nothrow_move_constructible<T>::value;
    };

  }

  struct nullopt_t {};
  static constexpr auto nullopt = nullopt_t {};
  struct bad_optional_access : std::exception {};

  template <class T>
  class optional {

    struct nonesuch {};

    using copyable_t = std::conditional_t<
      std::is_copy_constructible<T>::value,
      optional,
      nonesuch
    >;
    using noncopyable_t = std::conditional_t<
      !std::is_copy_constructible<T>::value,
      optional,
      nonesuch
    >;
    using moveable_t = std::conditional_t<
      std::is_move_constructible<T>::value,
      optional,
      nonesuch
    >;
    using nonmoveable_t = std::conditional_t<
      !std::is_move_constructible<T>::value,
      optional,
      nonesuch
    >;
    using copy_assignable_t = std::conditional_t<
      std::is_copy_constructible<T>::value
      &&
      std::is_copy_assignable<T>::value,
      optional,
      nonesuch
    >;
    using non_copy_assignable_t = std::conditional_t<
      !std::is_copy_constructible<T>::value
      ||
      !std::is_copy_assignable<T>::value,
      optional,
      nonesuch
    >;
    using move_assignable_t = std::conditional_t<
      std::is_move_constructible<T>::value
      &&
      std::is_move_assignable<T>::value,
      optional,
      nonesuch
    >;
    using non_move_assignable_t = std::conditional_t<
      !std::is_move_constructible<T>::value
      ||
      !std::is_move_assignable<T>::value,
      optional,
      nonesuch
    >;

    public:

      /*----- Public Types -----*/

      using value_type = T;

      /*----- Lifecycle Functions -----*/

      // Default constructors.
      optional() : is_set(false) {}
      optional(nullopt_t) : is_set(false) {}

      // Copy/Move constructors.
      optional(copyable_t const& other);
      optional(noncopyable_t const&) = delete;
      optional(moveable_t&& other) noexcept(std::is_nothrow_move_constructible<T>::value);
      optional(nonmoveable_t&& other) = delete;

      // Converting copy constructors.
      template <class U,
        typename std::enable_if<
          std::is_constructible<T, U const&>::value
          &&
          detail::constructor_conversion_check<T, U>::value
          &&
          !std::is_convertible<U const&, T>::value,
          bool
        >::type = false
      >
      explicit optional(optional<U> const& other);
      template <class U,
        typename std::enable_if<
          std::is_constructible<T, U const&>::value
          &&
          detail::constructor_conversion_check<T, U>::value
          &&
          std::is_convertible<U const&, T>::value,
          bool
        >::type = false
      >
      optional(optional<U> const& other);

      // Converting move constructors.
      template <class U,
        typename std::enable_if<
          std::is_constructible<T, U&&>::value
          &&
          detail::constructor_conversion_check<T, U>::value
          &&
          !std::is_convertible<U&&, T>::value,
          bool
        >::type = false
      >
      explicit optional(optional<U>&& other);
      template <class U,
        typename std::enable_if<
          std::is_constructible<T, U&&>::value
          &&
          detail::constructor_conversion_check<T, U>::value
          &&
          std::is_convertible<U&&, T>::value,
          bool
        >::type = false
      >
      optional(optional<U>&& other);

      // Forwarding constructors.
      template <class U,
        typename std::enable_if<
          std::is_constructible<T, U&&>::value
          &&
          !std::is_same<
            optional,
            std::decay_t<U>
          >::value
          &&
          !std::is_convertible<U&&, T>::value,
          bool
        >::type = false
      >
      explicit optional(U&& val);
      template <class U,
        typename std::enable_if<
          std::is_constructible<T, U&&>::value
          &&
          !std::is_same<
            optional,
            std::decay_t<U>
          >::value
          &&
          std::is_convertible<U&&, T>::value,
          bool
        >::type = false
      >
      optional(U&& val);

      ~optional() noexcept;

      /*----- Operators -----*/

      // Assignment.
      optional& operator =(nullopt_t) noexcept;
      optional& operator =(copy_assignable_t const& other);
      optional& operator =(non_copy_assignable_t const&) = delete;
      optional& operator =(move_assignable_t&& other) noexcept(detail::safely_assignable<T>::value);
      optional& operator =(non_move_assignable_t&& other) = delete;
      template <class U, class =
        std::enable_if_t<
          !std::is_same<
            optional,
            std::decay_t<U>
          >::value
          &&
          std::is_constructible<T, U>::value
          &&
          std::is_assignable<T&, U>::value
        >
      >
      optional& operator =(U&& val);
      template <class U, class =
        std::enable_if_t<
          detail::assignment_conversion_check<T, U>::value
          &&
          std::is_constructible<T, U const&>::value
          &&
          std::is_assignable<T&, U const&>::value
        >
      >
      optional& operator =(optional<U> const& other);
      template <class U, class =
        std::enable_if_t<
          detail::assignment_conversion_check<T, U>::value
          &&
          std::is_constructible<T, U const&>::value
          &&
          std::is_assignable<T&, U const&>::value
        >
      >
      optional& operator =(optional<U>&& other);

      // Dereference.
      auto operator *() & noexcept -> value_type&;
      auto operator *() const& noexcept -> value_type const&;
      auto operator *() && noexcept -> value_type&&;
      auto operator *() const&& noexcept -> value_type const&&;
      auto operator ->() noexcept -> value_type*;
      auto operator ->() const noexcept -> value_type const*;

      // Conversion.
      explicit operator bool() const noexcept;

      /*----- Public API -----*/

      bool has_value() const noexcept;

      auto value() & -> value_type&;
      auto value() const& -> value_type const&;
      auto value() && -> value_type&&;
      auto value() const&& -> value_type const&&;

      template <class U, class =
        std::enable_if_t<
          std::is_convertible<U, T>::value
          &&
          std::is_copy_constructible<T>::value
        >
      >
      auto value_or(U&& def) const& -> value_type;
      template <class U, class =
        std::enable_if_t<
          std::is_convertible<U, T>::value
          &&
          std::is_move_constructible<T>::value
        >
      >
      auto value_or(U&& def) && -> value_type;

      void reset() noexcept;
      template <class... Args>
      auto emplace(Args&&... the_args) -> value_type&;

    private:

      /*----- Private Types -----*/

      using storage = std::aligned_storage_t<sizeof(T), alignof(T)>;

      /*----- Private Helpers -----*/

      template <class... Args>
      void construct(Args&&... the_args);
      template <class U>
      void assign(U&& other);
      void destroy() noexcept;

      auto access() noexcept -> value_type*;
      auto access() const noexcept -> value_type const*;

      /*----- Private Members -----*/

      bool is_set;
      storage store;

  };

  /*----- Free Operators -----*/

  /*----- Equality Operators -----*/

  // Optional equality.
  template <class T, class U, class =
    std::enable_if_t<
      meta::are_comparable<T const&, U const&>::value
    >
  >
  bool operator ==(optional<T> const& lhs, optional<U> const& rhs) {
    if (static_cast<bool>(lhs) != static_cast<bool>(rhs)) return false;
    else if (!lhs) return true;
    else return *lhs == *rhs;
  }
  template <class T, class U, class =
    std::enable_if_t<
      meta::are_comparable<T const&, U const&>::value
    >
  >
  bool operator !=(optional<T> const& lhs, optional<U> const& rhs) {
    return !(lhs == rhs);
  }

  // Optional vs value equality
  template <class T, class U, class =
    std::enable_if_t<
      meta::are_comparable<T const&, U const&>::value
    >
  >
  bool operator ==(optional<T> const& lhs, U const& rhs) {
    if (!lhs) return false;
    else return *lhs == rhs;
  }
  template <class T, class U, class =
    std::enable_if_t<
      meta::are_comparable<T const&, U const>::value
    >
  >
  bool operator ==(T const& lhs, optional<U> const& rhs) {
    return rhs == lhs;
  }
  template <class T, class U, class =
    std::enable_if_t<
      meta::are_comparable<T const&, U const&>::value
    >
  >
  bool operator !=(optional<T> const& lhs, U const& rhs) {
    return !(lhs == rhs);
  }
  template <class T, class U, class =
    std::enable_if_t<
      meta::are_comparable<T const&, U const>::value
    >
  >
  bool operator !=(T const& lhs, optional<U> const& rhs) {
    return !(lhs == rhs);
  }

  // Optional vs nullopt_t equality
  template <class T>
  bool operator ==(optional<T> const& lhs, nullopt_t) {
    return !lhs;
  }
  template <class T>
  bool operator ==(nullopt_t lhs, optional<T> const& rhs) {
    return rhs == lhs;
  }
  template <class T>
  bool operator !=(optional<T> const& lhs, nullopt_t rhs) {
    return !(lhs == rhs);
  }
  template <class T>
  bool operator !=(nullopt_t lhs, optional<T> const& rhs) {
    return !(lhs == rhs);
  }

  /*----- Less Than Operators -----*/

  // Optional less than ordering
  template <class T, class U, class =
    std::enable_if_t<
      meta::are_lt_comparable<T const&, U const&>::value
    >
  >
  bool operator <(optional<T> const& lhs, optional<U> const& rhs) {
    if (!rhs) return false;
    else if (!lhs) return true;
    else return *lhs < *rhs;
  }
  template <class T, class U, class =
    std::enable_if_t<
      meta::are_lte_comparable<T const&, U const&>::value
    >
  >
  bool operator <=(optional<T> const& lhs, optional<U> const& rhs) {
    if (!lhs) return true;
    else if (!rhs) return false;
    else return *lhs <= *rhs;
  }

  // Optional vs value less than ordering
  template <class T, class U, class =
    std::enable_if_t<
      meta::are_lt_comparable<T const&, U const&>::value
    >
  >
  bool operator <(optional<T> const& lhs, U const& rhs) {
    if (!lhs) return true;
    else return *lhs < rhs;
  }
  template <class T, class U, class =
    std::enable_if_t<
      meta::are_lt_comparable<T const&, U const&>::value
    >
  >
  bool operator <(T const& lhs, optional<U> const& rhs) {
    if (!rhs) return false;
    else return lhs < *rhs;
  }
  template <class T, class U, class =
    std::enable_if_t<
      meta::are_lte_comparable<T const&, U const&>::value
    >
  >
  bool operator <=(optional<T> const& lhs, U const& rhs) {
    if (!lhs) return true;
    else return *lhs <= rhs;
  }
  template <class T, class U, class =
    std::enable_if_t<
      meta::are_lte_comparable<T const&, U const&>::value
    >
  >
  bool operator <=(T const& lhs, optional<U> const& rhs) {
    if (!rhs) return false;
    else return lhs <= *rhs;
  }

  // Optional vs nullopt_t ordering
  template <class T>
  bool operator <(optional<T> const&, nullopt_t) {
    return false;
  }
  template <class T>
  bool operator <(nullopt_t, optional<T> const& rhs) {
    return static_cast<bool>(rhs);
  }
  template <class T>
  bool operator <=(optional<T> const& lhs, nullopt_t) {
    return !lhs;
  }
  template <class T>
  bool operator <=(nullopt_t, optional<T> const&) {
    return true;
  }

  // Optional greater than ordering.
  template <class T, class U, class =
    std::enable_if_t<
      meta::are_gt_comparable<T const&, U const&>::value
    >
  >
  bool operator >(optional<T> const& lhs, optional<U> const& rhs) {
    if (!lhs) return false;
    else if (!rhs) return true;
    else return *lhs > *rhs;
  }
  template <class T, class U, class =
    std::enable_if_t<
      meta::are_gte_comparable<T const&, U const&>::value
    >
  >
  bool operator >=(optional<T> const& lhs, optional<U> const& rhs) {
    if (!rhs) return true;
    else if (!lhs) return false;
    else return *lhs >= *rhs;
  }

  // Optional vs value greater than ordering.
  template <class T, class U, class =
    std::enable_if_t<
      meta::are_gt_comparable<T const&, U const&>::value
    >
  >
  bool operator >(optional<T> const& lhs, U const& rhs) {
    if (lhs) return *lhs > rhs;
    else return false;
  }
  template <class T, class U, class =
    std::enable_if_t<
      meta::are_gt_comparable<T const&, U const&>::value
    >
  >
  bool operator >(T const& lhs, optional<U> const& rhs) {
    if (rhs) return lhs > *rhs;
    else return true;
  }
  template <class T, class U, class =
    std::enable_if_t<
      meta::are_gte_comparable<T const&, U const&>::value
    >
  >
  bool operator >=(optional<T> const& lhs, U const& rhs) {
    if (lhs) return *lhs >= rhs;
    else return false;
  }
  template <class T, class U, class =
    std::enable_if_t<
      meta::are_gte_comparable<T const&, U const&>::value
    >
  >
  bool operator >=(T const& lhs, optional<U> const& rhs) {
    if (rhs) return lhs >= *rhs;
    else return true;
  }

}

/*----- Template Implementations -----*/

#include "optional.tcc"

#endif
