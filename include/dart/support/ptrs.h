#ifndef DART_POINTERS_H
#define DART_POINTERS_H

/*----- System Includes -----*/

#include <atomic>
#include <memory>
#include <gsl/gsl>
#include <stdint.h>
#include <functional>
#include <type_traits>

/*----- System Includes -----*/

#include "../refcount_traits.h"

/*----- Type Declarations -----*/

namespace dart {

  namespace detail {

    template <class T>
    struct managed_ptr_deleter {
      void operator ()(T ptr);
    };

    template <class T>
    struct managed_ptr_deleter<T[]> {
      void operator ()(std::remove_extent_t<T>* ptr);
    };

    // Type-erasure mechanism for counted_ptr_base.
    template <class Counter>
    struct managed_ptr {
      managed_ptr(void* ptr, int64_t use_count) : ptr(ptr), use_count(use_count) {}
      virtual ~managed_ptr() = default;
      virtual void destroy() = 0;

      void* ptr;
      Counter use_count;
    };
    template <class Counter, class PtrType, class Deleter>
    struct managed_ptr_eraser : managed_ptr<Counter> {
      managed_ptr_eraser(void const* ptr, int64_t use_count) : managed_ptr<Counter>(const_cast<void*>(ptr), use_count) {}
      managed_ptr_eraser(void const* ptr, int64_t use_count, Deleter const& deleter) :
        managed_ptr<Counter>(const_cast<void*>(ptr), use_count),
        deleter(deleter)
      {}
      managed_ptr_eraser(void const* ptr, int64_t use_count, Deleter&& deleter) :
        managed_ptr<Counter>(const_cast<void*>(ptr), use_count),
        deleter(std::move(deleter))
      {}
      virtual void destroy() override;

      Deleter deleter;
    };

    template <class T, class Counter>
    class counted_ptr_base {

      public:

        /*----- Public Types -----*/

        using element_type = T;
        using counter_type = Counter;
        template <class U>
        using ptr_type = std::conditional_t<std::is_array<U>::value, U, U*>;

        /*----- Lifecycle Functions -----*/

        counted_ptr_base() noexcept : value(nullptr) {}
        counted_ptr_base(std::nullptr_t) noexcept : value(nullptr) {}
        explicit counted_ptr_base(ptr_type<T> ptr) :
          value(new managed_ptr_eraser<counter_type, ptr_type<T>, managed_ptr_deleter<ptr_type<T>>>(ptr, 1))
        {}
        template <class Deleter>
        counted_ptr_base(ptr_type<T> ptr, Deleter&& deleter) :
          value(new managed_ptr_eraser<counter_type, ptr_type<T>, Deleter>(ptr, 1, std::move(deleter)))
        {}
        template <class U, class EnableIf =
          std::enable_if_t<
            std::is_convertible<ptr_type<U>, ptr_type<T>>::value
          >
        >
        counted_ptr_base(counted_ptr_base<U, counter_type> const& other) noexcept;
        template <class U, class EnableIf =
          std::enable_if_t<
            std::is_convertible<ptr_type<U>, ptr_type<T>>::value
          >
        >
        counted_ptr_base(counted_ptr_base<U, counter_type>&& other) noexcept;
        template <class U, class Deleter, class EnableIf =
          std::enable_if_t<
            std::is_same<ptr_type<U>, ptr_type<T>>::value
            ||
            std::is_convertible<ptr_type<U>, ptr_type<T>>::value
          >
        >
        counted_ptr_base(std::unique_ptr<U, Deleter>&& ptr);
        counted_ptr_base(counted_ptr_base const& other) noexcept;
        counted_ptr_base(counted_ptr_base&& other) noexcept;
        ~counted_ptr_base() noexcept;

        /*----- Operators -----*/

        // Assignment.
        template <class U, class EnableIf =
          std::enable_if_t<
            std::is_convertible<ptr_type<U>, ptr_type<T>>::value
          >
        >
        counted_ptr_base& operator =(counted_ptr_base<U, counter_type> const& other) noexcept;
        template <class U, class EnableIf =
          std::enable_if_t<
            std::is_convertible<ptr_type<U>, ptr_type<T>>::value
          >
        >
        counted_ptr_base& operator =(counted_ptr_base<U, counter_type>&& other) noexcept;
        counted_ptr_base& operator =(counted_ptr_base const& other) noexcept;
        counted_ptr_base& operator =(counted_ptr_base&& other) noexcept;
        template <class U, class Deleter, class EnableIf =
          std::enable_if_t<
            std::is_convertible<ptr_type<U>, ptr_type<T>>::value
          >
        >
        counted_ptr_base& operator =(std::unique_ptr<U, Deleter>&& ptr)
          noexcept(std::is_nothrow_move_constructible<Deleter>::value);

        // Comparison.
        bool operator ==(counted_ptr_base const& other) const noexcept;
        bool operator !=(counted_ptr_base const& other) const noexcept;
        bool operator <(counted_ptr_base const& other) const noexcept;
        bool operator <=(counted_ptr_base const& other) const noexcept;
        bool operator >(counted_ptr_base const& other) const noexcept;
        bool operator >=(counted_ptr_base const& other) const noexcept;

        // Conversion.
        explicit operator bool() const noexcept;

        /*----- Public API -----*/

        // Accessors.
        std::remove_extent_t<element_type>* get() const noexcept;
        bool unique() const noexcept;
        int64_t use_count() const noexcept;

        // Mutators.
        void reset() noexcept;
        template <class U, class EnableIf =
          std::enable_if_t<
            std::is_convertible<U, ptr_type<T>>::value
          >
        >
        void reset(U ptr);
        template <class U, class Deleter, class EnableIf =
          std::enable_if_t<
            std::is_convertible<U, ptr_type<T>>::value
          >
        >
        void reset(U ptr, Deleter&& deleter);

      protected:

        /*----- Protected Members -----*/

        gsl::owner<managed_ptr<counter_type>*> value;

        /*----- Friends -----*/

        template <class U, class C>
        friend class counted_ptr_base;

    };

    template <class T, class Counter>
    struct counted_ptr_impl : counted_ptr_base<T, Counter> {

      /*----- Types -----*/

      using typename counted_ptr_base<T, Counter>::element_type;

      /*----- Lifecycle Functions -----*/

      using counted_ptr_base<T, Counter>::counted_ptr_base;

      /*----- Operators -----*/

      // Dereference.
      auto operator *() const noexcept -> element_type&;
      auto operator ->() const noexcept -> element_type*;

      // Comparison.
      using counted_ptr_base<T, Counter>::operator ==;
      using counted_ptr_base<T, Counter>::operator !=;
      using counted_ptr_base<T, Counter>::operator <;
      using counted_ptr_base<T, Counter>::operator <=;
      using counted_ptr_base<T, Counter>::operator >;
      using counted_ptr_base<T, Counter>::operator >=;
      using counted_ptr_base<T, Counter>::operator bool;

    };

    template <class T, class Counter>
    struct counted_array_ptr_impl : counted_ptr_base<T, Counter> {

      /*----- Types -----*/

      using index_type = std::ptrdiff_t;
      using element_type = std::remove_extent_t<typename counted_ptr_base<T, Counter>::element_type>;

      /*----- Lifecycle Functions -----*/

      using counted_ptr_base<T, Counter>::counted_ptr_base;

      /*----- Operators -----*/

      // Derefence.
      element_type& operator [](index_type idx) const noexcept;

      // Comparison.
      using counted_ptr_base<T, Counter>::operator ==;
      using counted_ptr_base<T, Counter>::operator !=;
      using counted_ptr_base<T, Counter>::operator <;
      using counted_ptr_base<T, Counter>::operator <=;
      using counted_ptr_base<T, Counter>::operator >;
      using counted_ptr_base<T, Counter>::operator >=;
      using counted_ptr_base<T, Counter>::operator bool;

    };

  }

  template <class T>
  class shareable_ptr {

    public:

      /*----- Public Types -----*/

      using value_type = T;
      using element_type = typename refcount_traits<T>::element_type;

    private:

      struct is_nothrow_assignable :
        meta::conjunction<
          typename refcount_traits<T>::is_nothrow_copyable,
          typename refcount_traits<T>::is_nothrow_moveable
        >
      {};

      // XXX: MSVC gets confused if this is used directly in a noexcept specification.
      struct is_nothrow_default_constructible :
        std::integral_constant<bool,
          refcount_traits<T>::template can_nothrow_take<element_type*, std::default_delete<element_type>>::value
        >
      {};
    
    public:

      /*----- Lifecycle Functions -----*/

      // Converting constructors.
      shareable_ptr(std::nullptr_t)
          noexcept(is_nothrow_default_constructible::value) :
        shareable_ptr()
      {}
      template <class U, class D, class EnableIf =
        std::enable_if_t<
          refcount::can_take<T, U*, D>::value
        >
      >
      explicit shareable_ptr(std::unique_ptr<U, D>&& ptr)
          noexcept(refcount_traits<T>::template can_nothrow_take<U*, D>::value);
      shareable_ptr(T const& other) noexcept(refcount_traits<T>::is_nothrow_copyable::value);
      shareable_ptr(T&& other) noexcept(refcount_traits<T>::is_nothrow_moveable::value);

      // Ownership transfer constructors.
      template <class U, class =
        std::enable_if_t<
          refcount::can_take<T, U*, std::default_delete<U>>::value
        >
      >
      explicit shareable_ptr(U* owner)
          noexcept(refcount_traits<T>::template can_nothrow_take<U*, std::default_delete<U>>::value) :
        shareable_ptr(owner, std::default_delete<U> {})
      {}
      template <class U, class D, class EnableIf =
        std::enable_if_t<
          refcount::can_take<T, U*, D>::value
        >
      >
      explicit shareable_ptr(U* owner, D&& del)
          noexcept(refcount_traits<T>::template can_nothrow_take<U*, D>::value);

      // Special Constructors.
      shareable_ptr() noexcept(is_nothrow_default_constructible::value);
      shareable_ptr(shareable_ptr const& other) noexcept(refcount_traits<T>::is_nothrow_copyable::value);
      shareable_ptr(shareable_ptr&& other) noexcept(refcount_traits<T>::is_nothrow_moveable::value);
      ~shareable_ptr() noexcept;

      /*----- Operators -----*/

      // Assignment.
      template <class U, class D, class EnableIf =
        std::enable_if_t<
          refcount::can_take<T, U*, D>::value
        >
      >
      shareable_ptr& operator =(std::unique_ptr<U, D>&& ptr)
          noexcept(refcount_traits<T>::template can_nothrow_take<U*, D>::value);
      shareable_ptr& operator =(T const& other) noexcept(is_nothrow_assignable::value);
      shareable_ptr& operator =(T&& other) noexcept;
      shareable_ptr& operator =(std::nullptr_t) noexcept(is_nothrow_default_constructible::value);
      shareable_ptr& operator =(shareable_ptr const& other) noexcept(is_nothrow_assignable::value);
      shareable_ptr& operator =(shareable_ptr&& other) noexcept;

      // Dereference.
      auto operator *() const noexcept(refcount_traits<T>::is_nothrow_unwrappable::value) -> element_type&;
      auto operator ->() const noexcept(refcount_traits<T>::is_nothrow_unwrappable::value) -> element_type*;

      // Comparison.
      bool operator ==(shareable_ptr const& other) const noexcept(refcount_traits<T>::is_nothrow_unwrappable::value);
      bool operator !=(shareable_ptr const& other) const noexcept(refcount_traits<T>::is_nothrow_unwrappable::value);
      bool operator <(shareable_ptr const& other) const noexcept(refcount_traits<T>::is_nothrow_unwrappable::value);
      bool operator <=(shareable_ptr const& other) const noexcept(refcount_traits<T>::is_nothrow_unwrappable::value);
      bool operator >(shareable_ptr const& other) const noexcept(refcount_traits<T>::is_nothrow_unwrappable::value);
      bool operator >=(shareable_ptr const& other) const noexcept(refcount_traits<T>::is_nothrow_unwrappable::value);

      // Conversion.
      explicit operator bool() const noexcept(refcount_traits<T>::is_nothrow_unwrappable::value);

      /*----- Public API -----*/

      // Accessors.
      auto get() const noexcept(refcount_traits<T>::is_nothrow_unwrappable::value) -> element_type*;
      bool unique() const noexcept(refcount_traits<T>::has_nothrow_use_count::value);
      int64_t use_count() const noexcept(refcount_traits<T>::has_nothrow_use_count::value);

      // Mutators.
      void reset() noexcept(refcount_traits<T>::has_nothrow_reset::value);

      // Extremely awkward "copy-constructor" that allows us to copy our
      // wrapped value into a raw instance of T.
      // Function has to be written this way since we can't assume T is copyable/moveable
      // The only thing we can definitely do with T is pass it into dart::refcount_traits::copy.
      void share(T& ptr) const noexcept(refcount_traits<T>::is_nothrow_copyable::value);

      // Extremely awkward "move-constructor" that allows us to move our
      // wrapped value into a raw instance of T.
      // Function has to be written this way since we can't assume T is copyable/moveable
      // The only thing we can definitely do with T is pass it into dart::refcount_traits::move.
      void transfer(T& ptr) && noexcept(refcount_traits<T>::is_nothrow_moveable::value);

      auto raw() noexcept -> value_type&;
      auto raw() const noexcept -> value_type const&;

    private:

      /*----- Private Types -----*/

      struct partial_construction_tag {};

      /*----- Private Lifecycle Functions -----*/

      shareable_ptr(partial_construction_tag) {}

      /*----- Private Members -----*/

      union {
        value_type impl;
      };

      /*----- Friends -----*/

      template <class U, class... Args>
      friend shareable_ptr<U> make_shareable(Args&&...);

  };

  template <template <class> class RefCount>
  struct view_ptr_context {

    template <class T>
    class view_ptr {

      public:

        /*----- Public Types -----*/

        using refcount_type = RefCount<T>;
        using is_nonowning = refcount_type;
        using element_type = typename refcount_traits<refcount_type>::element_type;

        // XXX: Necessary to make logic in refcount_traits work.
        template <template <template <class> class> class Binder>
        using refcount_rebind = Binder<RefCount>;

        /*----- Lifecycle Functions -----*/

        // Functions must exist for refcount concept, but will throw if called
        // as view_ptr doesn't have ownership semantics
        explicit view_ptr(T*);
        template <class Del>
        explicit view_ptr(T*, Del&&);
        
        // I'm not currently disallowing construction from temporaries
        // I can imagine scenarios where it could be useful, and generally,
        // you'll need to know what you're doing to work with the view types
        view_ptr(std::nullptr_t) noexcept : view_ptr() {}
        view_ptr(refcount_type const& owner) noexcept : impl(&owner) {}

        // Lifecycle functions mostly do nothing
        view_ptr() noexcept : impl(nullptr) {}
        view_ptr(view_ptr const&) = default;
        view_ptr(view_ptr&& other) noexcept;
        ~view_ptr() = default;

        /*----- Operators -----*/

        auto operator =(refcount_type const& owner) noexcept -> view_ptr&;

        auto operator =(view_ptr const&) -> view_ptr& = default;
        auto operator =(view_ptr&& other) noexcept -> view_ptr&;

        auto operator *() const noexcept(refcount_traits<refcount_type>::is_nothrow_unwrappable::value) -> element_type&;
        auto operator ->() const noexcept(refcount_traits<refcount_type>::is_nothrow_unwrappable::value) -> element_type*;

        bool operator ==(view_ptr const& other) const noexcept(refcount_traits<refcount_type>::is_nothrow_unwrappable::value);
        bool operator !=(view_ptr const& other) const noexcept(refcount_traits<refcount_type>::is_nothrow_unwrappable::value);
        bool operator <(view_ptr const& other) const noexcept(refcount_traits<refcount_type>::is_nothrow_unwrappable::value);
        bool operator <=(view_ptr const& other) const noexcept(refcount_traits<refcount_type>::is_nothrow_unwrappable::value);
        bool operator >(view_ptr const& other) const noexcept(refcount_traits<refcount_type>::is_nothrow_unwrappable::value);
        bool operator >=(view_ptr const& other) const noexcept(refcount_traits<refcount_type>::is_nothrow_unwrappable::value);

        explicit operator bool() const noexcept(refcount_traits<refcount_type>::is_nothrow_unwrappable::value);

        operator refcount_type const&() const;

        /*----- Public API -----*/

        auto get() const noexcept(refcount_traits<refcount_type>::is_nothrow_unwrappable::value) -> element_type*;
        size_t use_count() const noexcept(refcount_traits<refcount_type>::has_nothrow_use_count::value);

        void reset() noexcept;

        auto raw() const noexcept -> refcount_type const&;

      private:

        /*----- Private Members -----*/

        refcount_type const* impl;

    };

  };

  template <class T>
  using unsafe_ptr = std::conditional_t<
    std::is_same<
      std::remove_extent_t<T>,
      T
    >::value,
    detail::counted_ptr_impl<T, int64_t>,
    detail::counted_array_ptr_impl<T, int64_t>
  >;

  template <class T, class EnableIf =
    std::enable_if_t<
      std::is_array<T>::value
    >
  >
  unsafe_ptr<T> make_unsafe(size_t idx);

  template <class T, class... Args>
  std::enable_if_t<!std::is_array<T>::value, unsafe_ptr<T>> make_unsafe(Args&&... the_args);

  template <class T>
  using skinny_ptr = std::conditional_t<
    std::is_same<
      std::remove_extent_t<T>,
      T
    >::value,
    detail::counted_ptr_impl<T, std::atomic<int64_t>>,
    detail::counted_array_ptr_impl<T, std::atomic<int64_t>>
  >;

  template <class T, class EnableIf =
    std::enable_if_t<
      std::is_array<T>::value
    >
  >
  skinny_ptr<T> make_skinny(size_t idx);

  template <class T, class... Args>
  std::enable_if_t<!std::is_array<T>::value, skinny_ptr<T>> make_skinny(Args&&... the_args);

}

/*----- Template Implementations -----*/

#include "ptrs.tcc"

#endif
