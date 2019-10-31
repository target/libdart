#ifndef DART_REFCOUNT_TRAITS_H
#define DART_REFCOUNT_TRAITS_H

/*----- System Includes -----*/

#include <type_traits>

/*----- Local Includes -----*/

#include "meta.h"

/*----- Type Declarations -----*/

namespace dart {

  // Namespace starts with a slew of "behavioral structs"
  // that allow users to specialize how specific behavior
  // is performed with their reference counter.
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

    // This section raises all of the functionality that was just declared
    // into a space where it can be SFINAEd with meta::is_detected to check
    // if it is supported for arbitrary types.
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

      // XXX: Miserable, awful, hack that allowed me to add the dart::packet::view types.
      // Unfortunately I didn't leave design space to add non-owning view types to the library,
      // and the library had already gotten so big, and I didn't see a way to add view types
      // without also adding a proper allocator model, which would've added a boatload of new code.
      // So I came up with a hack that allowed dart::basic_packet to act as its own view type,
      // allowing me to add views in at the last minute with minimal code change.
      //
      // dart::packet is actually:
      // dart::basic_packet<std::shared_ptr>
      // and dart::packet::view is actually:
      // dart::basic_packet<dart::view_ptr_context<std::shared_ptr>::view_ptr>;
      //
      // The basic idea was to come up with a version of dart::basic_packet whose refcounter
      // was actually a "do-nothing" refcounter that just cached a raw pointer to its parent's
      // reference counter, and then define some magic conversion functions to tie the whole thing
      // together and make it feel akin to std::string -> std::string_view.
      //
      // The dart::view_ptr_context::view_ptr dance is necessary because I couldn't change the
      // template signature of dart::basic_packet, which meant that view_ptr had to be parameterized
      // by only a single type, and yet it needed to also be able to remember how to convert itself
      // into a new instance of the original reference counter, because I wanted view types
      // to be able to transition back into the safe, refcounted, pathway on demand.
      //
      // The problem arose because dart::heap has to hold pointers to collections of itself
      // to make aggregates work, which meant that dart::heap::view was calculating that it
      // held a pointer to instances of dart::heap::view, when it was actually being initialized
      // with a pointer to instances of dart::heap.
      //
      // As a concrete illustration, dart::heap::view used to declare that it stored instances of:
      // dart::shareable_ptr<
      //   dart::view_ptr_context<std::shared_ptr>::view_ptr<
      //     std::vector<
      //       dart::basic_heap<
      //         dart::view_ptr_context<std::shared_ptr>::view_ptr
      //       >
      //     >
      //   >
      // >
      // Now it declares that it stores:
      // dart::shareable_ptr<
      //   dart::view_ptr_context<std::shared_ptr>::view_ptr<
      //     std::vector<
      //       dart::refcount::owner_indirection<
      //         dart::basic_heap,
      //         dart::view_ptr_context<std::shared_ptr>::view_ptr
      //       >
      //     >
      //   >
      // >
      // Which resolves to:
      // dart::shareable_ptr<
      //   dart::view_ptr_context<std::shared_ptr>::view_ptr<
      //     std::vector<
      //       dart::basic_heap<std::shared_ptr>
      //     >
      //   >
      // >
      // Which fixes the issue by forcing view types to declare that they actually contain
      // instances of their PARENT type, instead of themselves, while leaving the main types
      // to declare that they contain instances of themselves.
      //
      // The reason this is so difficult to do in C++ is because alias templates aren't like
      // type aliases when it comes to specialization. In the following code:
      //
      // template <class T>
      // using identity = T;
      // template <class T>
      // using identity_alias = identity<T>;
      //
      // template <class T>
      // struct foo {};
      //
      // template <template <class> class Tmp>
      // struct higher_foo {};
      //
      // static_assert(
      //   std::is_same<
      //     foo<identity<void>>,
      //     foo<identity_alias<void>>
      //   >::value
      // );
      //
      // static_assert(
      //   std::is_same<
      //     higher_foo<identity>,
      //     higher_foo<identity_alias>
      //   >::value
      // );
      //
      // The first static assert will pass, the second will fail.
      // identity and identity_alias are two SEPARATE templates, even though they compute
      // exactly the same space of types, and are defined in terms of each other.
      // This makes things very, very difficult if you need to extract a template-template
      // parameter from an existing type and then USE that template-template parameter to
      // recompute a previously computed type via specialization, which is what needed to happen
      // here.
      // All this to say, the damn thing works, but I hope to god it never becomes a problem.
      // Rant over.
      // XXX: Miserable, awful, hack that allowed me to add the dart::basic_packet::view types.
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

    // This section finishes the work of the file so far, lowering the
    // question of whether a particular operation is supported
    // by an arbitrary type into a compile-time boolean.
    // Classes all ultimately inherit from either std::true_type
    // or std::false_type.
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

    // Struct checks whether a Dart implementation type is a view or not.
    template <template <class> class Owner>
    struct is_owner : meta::negation<meta::is_detected<detail::nonowning_t, Owner<gsl::byte>>> {};

    // Struct handles an AWFUL edge case in the type logic surrounding classes
    // like dart::packet::view
    // For more information, check the comments above detail::owner_indirection_impl
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

  /**
   *  @brief
   *  Struct is used to centralize the concept of how to operate
   *  with a given reference counter implementation.
   *
   *  @details
   *  The basic idea of refcount_traits is to identify a minimal set of
   *  operations and semantics with which to define Dart's requirements
   *  of any reference counting implementation it uses.
   *
   *  @remarks
   *  I didn't want users to have to specialize this class directly
   *  to work with their refcounters, as they'd always have to implement
   *  every single function, even if their refcounter already had mostly
   *  sensible operators and semantics defined.
   *  Therefore, this class sits as a common touchpoint for all things
   *  related to reference counters, but defers final implementation to
   *  the behavioral structs defined at the top of this file.
   *  Allows the users to specialize only what they need, but allows
   *  the rest of the library one logical unit to work with.
   */
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

    /**
     *  @brief
     *  Function defers construction of a reference counter to a
     *  potentially user defined source.
     *
     *  @param[out] that
     *  Pointer to unconstructed memory!
     *  Placement new must be used to construct your reference counter.
     */
    template <class... Args>
    static auto construct(T* that, Args&&... the_args) {
      static_assert(refcount::is_constructible<T, Args...>::value,
          "Reference counter type must either conform to the std::shared_ptr API "
          "or specialize the necessary implementation types in the dart::refcount namespace");
      return refcount::construct<T>::perform(that, std::forward<Args>(the_args)...);
    }

    /**
     *  @brief
     *  Function defers ownership transfer of an existing pointer
     *  to a potentially user defined source.
     *
     *  @param[out] that
     *  Pointer to unconstructed memory!
     *  Placement new must be used to construct your reference counter.
     *  @param[in,out] del
     *  Deleter to be used on the given pointer.
     */
    template <class Del>
    static auto take(T* that, std::nullptr_t, Del&& del) noexcept(can_nothrow_take<element_type*, Del>::value) {
      static_assert(refcount::can_take<T, element_type*, Del>::value,
          "Reference counter type must either conform to the std::shared_ptr API "
          "or specialize the necessary implementation types in the dart::refcount namespace");
      return refcount::take<T>::perform(that, nullptr, std::forward<Del>(del));
    }

    /**
     *  @brief
     *  Function defers construction of a reference counter to a
     *  potentially user defined source.
     *
     *  @param[out] that
     *  Pointer to unconstructed memory!
     *  Placement new must be used to construct your reference counter.
     *  @param[in,out] ptr
     *  The pointer whose ownership must be transferred to a newly
     *  constructed reference counter.
     *  @param[in,out] del
     *  Deleter to be used on the given pointer.
     */
    template <class Ptr, class Del>
    static auto take(T* that, Ptr* ptr, Del&& del) noexcept(can_nothrow_take<Ptr, Del>::value) {
      static_assert(refcount::can_take<T, Ptr*, Del>::value,
          "Reference counter type must either conform to the std::shared_ptr API "
          "or specialize the necessary implementation types in the dart::refcount namespace");
      return refcount::take<T>::perform(that, ptr, std::forward<Del>(del));
    }

    /**
     *  @brief
     *  Function defers copying of a reference counter to a
     *  potentially user defined source.
     *
     *  @param[out] that
     *  Pointer to unconstructed memory!
     *  Placement new must be used to construct your reference counter.
     *  @param[in] rc
     *  The reference counter to be copied from.
     */
    static auto copy(T* that, T const& rc) noexcept(is_nothrow_copyable::value) {
      return refcount::copy<T>::perform(that, rc);
    }

    /**
     *  @brief
     *  Function defers moving of a reference counter to a
     *  potentially user defined source.
     *
     *  @param[out] that
     *  Pointer to unconstructed memory!
     *  Placement new must be used to construct your reference counter.
     *  @param[in] rc
     *  The reference counter to be moved from.
     */
    static auto move(T* that, T&& rc) noexcept(is_nothrow_moveable::value) {
      return refcount::move<T>::perform(that, std::move(rc));
    }

    /**
     *  @brief
     *  Function defers dereferencing a reference counter to a
     *  potentially user defined source.
     *
     *  @param[in] rc
     *  The reference counter to dereference
     */
    static auto& deref(T const& rc) noexcept(is_nothrow_unwrappable::value) {
      return *refcount::unwrap<T>::perform(rc);
    }

    /**
     *  @brief
     *  Function defers retrieval of the underlying pointer for a
     *  reference counter to a potentially user defined source.
     *
     *  @param[in] rc
     *  The reference counter to be unwrapped.
     */
    static auto* unwrap(T const& rc) noexcept(is_nothrow_unwrappable::value) {
      return refcount::unwrap<T>::perform(rc);
    }

    /**
     *  @brief
     *  Function defers querying the reference count of a reference counter
     *  to a potentially user defined source.
     *
     *  @param[in] rc
     *  The reference counter to query.
     */
    static auto use_count(T const& rc) noexcept(has_nothrow_use_count::value) {
      return refcount::use_count<T>::perform(rc);
    }

    /**
     *  @brief
     *  Function defers release of a reference counter to a potentially user
     *  defined source.
     *
     *  @param[in] rc
     *  The reference counter to reset.
     */
    static auto reset(T& rc) noexcept(has_nothrow_reset::value) {
      return refcount::reset<T>::perform(rc);
    }

    /**
     *  @brief
     *  Function defers null checking a reference counter to a potentially
     *  user defined source.
     *
     *  @param[in] rc
     *  The reference counter to null check.
     */
    static auto is_null(T const& rc) noexcept(is_nothrow_unwrappable::value) {
      return unwrap(rc) == nullptr;
    }

  };

}

#endif
