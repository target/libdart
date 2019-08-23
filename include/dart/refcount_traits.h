#ifndef DART_REFCOUNT_TRAITS_H
#define DART_REFCOUNT_TRAITS_H

/*----- System Includes -----*/

#include <type_traits>

/*----- Local Includes -----*/

#include "meta.h"

/*----- Type Declarations -----*/

namespace dart {

  namespace refcount {

    template <class T>
    struct element_type {
      using type = typename T::element_type;
    };

    template <class T>
    struct construct {

      template <class... Args, class =
        std::enable_if_t<
          std::is_constructible<T, typename T::element_type*>::value
        >
      >
      static void perform(T* that, Args&&... the_args) {
        auto* owner = new typename T::element_type(std::forward<Args>(the_args)...);
        try {
          new(that) T(owner);
        } catch (...) {
          delete owner;
          throw;
        }
      }

    };
    template <class T>
    struct construct<std::shared_ptr<T>> {

      template <class... Args>
      static void perform(std::shared_ptr<T>* that, Args&&... the_args) {
        new(that) std::shared_ptr<T>(std::make_shared<T>(std::forward<Args>(the_args)...));
      }

    };

    template <class T>
    struct take {

      template <class D, bool safe = noexcept(T(nullptr))>
      static void perform(T* that, std::nullptr_t, D&&) noexcept(safe) {
        new(that) T(nullptr);
      }

      template <class U, class D, bool safe = noexcept(T(std::declval<U*>(), std::declval<D>()))>
      static void perform(T* that, U* owned, D&& del) noexcept(safe) {
        new(that) T(owned, std::forward<D>(del));
      }

    };

    template <class T>
    struct copy {

      template <class U, class =
        std::enable_if_t<
          std::is_copy_constructible<U>::value
        >
      >
      static void perform(U* that, U const& other) noexcept(std::is_nothrow_copy_constructible<U>::value) {
        new(that) U(other);
      }

    };

    template <class T>
    struct move {

      template <class U, class =
        std::enable_if_t<
          std::is_move_constructible<U>::value
        >
      >
      static void perform(U* that, U&& other) noexcept(std::is_nothrow_move_constructible<U>::value) {
        new(that) U(std::move(other));
      }

    };

    template <class T>
    struct unwrap {

      template <class U, bool safe = noexcept(std::declval<U const&>().get())>
      static typename U::element_type* perform(U const& rc) noexcept(safe) {
        return rc.get();
      }

    };

    template <class T>
    struct use_count {

      template <class U, bool safe = noexcept(std::declval<U const&>().use_count())>
      static size_t perform(U const& rc) noexcept(safe) {
        return rc.use_count();
      }

    };

    template <class T>
    struct reset {

      template <class U, bool safe = noexcept(std::declval<U>().reset())>
      static void perform(U& rc) noexcept(safe) {
        rc.reset();
      }

    };

    namespace detail {

      template <class T>
      using element_type_t = typename element_type<T>::type;

      template <class T, class... Args>
      using construct_t = decltype(construct<T>::perform(std::declval<T*>(), std::declval<Args>()...));

      template <class T, class Ptr, class Del>
      using take_t = decltype(take<T>::perform(std::declval<T*>(), std::declval<Ptr>(), std::declval<Del>()));

      template <class T, class Ptr, class Del>
      using nothrow_take_t = std::enable_if_t<
        noexcept(take<T>::perform(std::declval<T*>(), std::declval<Ptr>(), std::declval<Del>()))
      >;

      template <class T>
      using copy_t = decltype(copy<T>::perform(std::declval<T*>(), std::declval<T const&>()));

      template <class T>
      using nothrow_copy_t = std::enable_if_t<
        noexcept(copy<T>::perform(std::declval<T*>(), std::declval<T const&>()))
      >;

      template <class T>
      using move_t = decltype(move<T>::perform(std::declval<T*>(), std::declval<T&&>()));

      template <class T>
      using nothrow_move_t = std::enable_if_t<
        noexcept(move<T>::perform(std::declval<T*>(), std::declval<T&&>()))
      >;

      template <class T>
      using unwrap_t = decltype(unwrap<T>::perform(std::declval<T const&>()));

      template <class T>
      using nothrow_unwrap_t = std::enable_if_t<
        noexcept(unwrap<T>::perform(std::declval<T&>()))
      >;

      template <class T>
      using use_count_t = decltype(use_count<T>::perform(std::declval<T const&>()));

      template <class T>
      using nothrow_use_count_t = std::enable_if_t<
        noexcept(use_count<T>::perform(std::declval<T const&>()))
      >;

      template <class T>
      using reset_t = decltype(reset<T>::perform(std::declval<T&>()));

      template <class T>
      using nothrow_reset_t = std::enable_if_t<
        noexcept(reset<T>::perform(std::declval<T&>()))
      >;

      template <class T>
      using nonowning_t = typename T::is_nonowning;

      template <bool, template <template <class> class> class Tmp, template <class> class RefCount>
      struct owner_indirection_impl {
        using type = Tmp<RefCount>;
      };
      template <template <template <class> class> class Tmp, template <class> class RefCount>
      struct owner_indirection_impl<false, Tmp, RefCount> {
        template <template <class> class Owner>
        struct rebinder {
          using type = Tmp<Owner>;
        };

        using type = typename RefCount<gsl::byte>::template refcount_rebind<rebinder>::type;
      };

    }

    template <class T>
    struct has_element_type :
      meta::is_detected<
        detail::element_type_t,
        T
      >
    {};
    template <class T, class... Args>
    struct is_constructible :
      meta::is_detected<
        detail::construct_t,
        T,
        Args...
      >
    {};
    template <class T, class Ptr, class Del>
    struct can_take :
      meta::is_detected<
        detail::take_t,
        T,
        Ptr,
        Del
      >
    {};
    template <class T, class Ptr, class Del>
    struct can_nothrow_take :
      meta::is_detected<
        detail::nothrow_take_t,
        T,
        Ptr,
        Del
      >
    {};
    template <class T>
    struct is_copyable :
      meta::is_detected<
        detail::copy_t,
        T
      >
    {};
    template <class T>
    struct is_nothrow_copyable :
      meta::is_detected<
        detail::nothrow_copy_t,
        T
      >
    {};
    template <class T>
    struct is_moveable :
      meta::is_detected<
        detail::move_t,
        T
      >
    {};
    template <class T>
    struct is_nothrow_moveable :
      meta::is_detected<
        detail::nothrow_move_t,
        T
      >
    {};
    template <class T>
    struct is_unwrappable :
      meta::is_detected<
        detail::unwrap_t,
        T
      >
    {};
    template <class T>
    struct is_nothrow_unwrappable :
      meta::is_detected<
        detail::nothrow_unwrap_t,
        T
      >
    {};
    template <class T>
    struct has_use_count :
      meta::is_detected<
        detail::use_count_t,
        T
      >
    {};
    template <class T>
    struct has_nothrow_use_count :
      meta::is_detected<
        detail::nothrow_use_count_t,
        T
      >
    {};
    template <class T>
    struct has_reset :
      meta::is_detected<
        detail::reset_t,
        T
      >
    {};
    template <class T>
    struct has_nothrow_reset :
      meta::is_detected<
        detail::nothrow_reset_t,
        T
      >
    {};

    template <template <class> class Owner>
    struct is_owner : meta::negation<meta::is_detected<detail::nonowning_t, Owner<gsl::byte>>> {};

    template <template <template <class> class> class Tmp, template <class> class RefCount>
    struct owner_indirection {
      using type = typename detail::owner_indirection_impl<
        is_owner<RefCount>::value,
        Tmp,
        RefCount
      >::type;
    };
    template <template <template <class> class> class Tmp, template <class> class RefCount>
    using owner_indirection_t = typename owner_indirection<Tmp, RefCount>::type;

  }

  template <class T>
  struct refcount_traits {

    static_assert(
      refcount::has_element_type<T>::value
      &&
      refcount::is_copyable<T>::value
      &&
      refcount::is_moveable<T>::value
      &&
      refcount::is_unwrappable<T>::value
      &&
      refcount::has_use_count<T>::value
      &&
      refcount::has_reset<T>::value,
      "Reference counter type must either conform to the std::shared_ptr API "
      "or specialize the necessary implementation types in the dart::refcount namespace"
    );

    /*----- Types -----*/

    using element_type = typename refcount::element_type<T>::type;

    template <class Ptr, class Del>
    struct can_nothrow_take : refcount::can_nothrow_take<T, Ptr, Del> {};
    struct is_nothrow_copyable : refcount::is_nothrow_copyable<T> {};
    struct is_nothrow_moveable : refcount::is_nothrow_moveable<T> {};
    struct is_nothrow_unwrappable : refcount::is_nothrow_unwrappable<T> {};
    struct has_nothrow_use_count : refcount::has_nothrow_use_count<T> {};
    struct has_nothrow_reset : refcount::has_nothrow_reset<T> {};

    /*----- Helpers -----*/

    template <class... Args>
    static auto construct(T* that, Args&&... the_args) {
      static_assert(refcount::is_constructible<T, Args...>::value,
          "Reference counter type must either conform to the std::shared_ptr API "
          "or specialize the necessary implementation types in the dart::refcount namespace");
      return refcount::construct<T>::perform(that, std::forward<Args>(the_args)...);
    }

    template <class Del>
    static auto take(T* that, std::nullptr_t, Del&& del) noexcept(can_nothrow_take<element_type*, Del>::value) {
      static_assert(refcount::can_take<T, element_type*, Del>::value,
          "Reference counter type must either conform to the std::shared_ptr API "
          "or specialize the necessary implementation types in the dart::refcount namespace");
      return refcount::take<T>::perform(that, nullptr, std::forward<Del>(del));
    }

    template <class Ptr, class Del>
    static auto take(T* that, Ptr* ptr, Del&& del) noexcept(can_nothrow_take<Ptr, Del>::value) {
      static_assert(refcount::can_take<T, Ptr*, Del>::value,
          "Reference counter type must either conform to the std::shared_ptr API "
          "or specialize the necessary implementation types in the dart::refcount namespace");
      return refcount::take<T>::perform(that, ptr, std::forward<Del>(del));
    }

    static auto copy(T* that, T const& rc) noexcept(is_nothrow_copyable::value) {
      return refcount::copy<T>::perform(that, rc);
    }

    static auto move(T* that, T&& rc) noexcept(is_nothrow_moveable::value) {
      return refcount::move<T>::perform(that, std::move(rc));
    }

    static auto& deref(T const& rc) noexcept(is_nothrow_unwrappable::value) {
      return *refcount::unwrap<T>::perform(rc);
    }

    static auto* unwrap(T const& rc) noexcept(is_nothrow_unwrappable::value) {
      return refcount::unwrap<T>::perform(rc);
    }

    static auto use_count(T const& rc) noexcept(has_nothrow_use_count::value) {
      return refcount::use_count<T>::perform(rc);
    }

    static auto reset(T& rc) noexcept(has_nothrow_reset::value) {
      return refcount::reset<T>::perform(rc);
    }

    static auto is_null(T const& rc) noexcept(is_nothrow_unwrappable::value) {
      return unwrap(rc) == nullptr;
    }

  };

}

#endif
