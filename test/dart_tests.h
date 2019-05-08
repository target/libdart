#ifndef DART_TESTS_H
#define DART_TESTS_H

/*----- Local Includes -----*/

// Yuck
// Bad, but fixes the problem of some platforms including
// the wrong thing if the library is already installed
#include "../include/dart.h"
#include "../include/extern/catch.h"

/*----- Macros -----*/

#define DYNAMIC_WHEN(desc, count) WHEN((desc) + std::to_string(count))
#define DYNAMIC_THEN(desc, count) THEN((desc) + std::to_string(count))

/*----- Header Begin -----*/

namespace dart {

  /*----- Types -----*/

  namespace meta {

    namespace detail {

	    template <class T, class Seq, T Begin>
      struct make_integer_range_impl;
      template <class T, T... Ints, T Begin>
      struct make_integer_range_impl<T, std::integer_sequence<T, Ints...>, Begin> {
        using type = std::integer_sequence<T, Begin + Ints...>;
      };

      template <size_t idx, bool... conds>
      struct if_constexpr_switch;
      template <size_t idx, bool... conds>
      struct if_constexpr_switch<idx, true, conds...> {
        template <class Funcs, class... Args>
        static decltype(auto) if_constexpr_switch_impl(Funcs& funcs, Args&&... the_args) {
          return std::get<idx>(funcs)(std::forward<Args>(the_args)...);
        }
      };
      template <size_t idx, bool... conds>
      struct if_constexpr_switch<idx, false, conds...> {
        template <class Funcs, class... Args>
        static decltype(auto) if_constexpr_switch_impl(Funcs& funcs, Args&&... the_args) {
          return if_constexpr_switch<idx + 1, conds...>::if_constexpr_switch_impl(funcs, std::forward<Args>(the_args)...);
        }
      };
      template <bool... conds, class Funcs, class Args, size_t... idxs>
      decltype(auto) if_constexpr_dispatch(Funcs& funcs, Args&& args, std::index_sequence<idxs...>) {
        return if_constexpr_switch<0, conds...>::if_constexpr_switch_impl(funcs, std::get<idxs>(std::move(args))...);
      }

    }

    template <class T, T Begin, T End>
    using make_integer_range = typename detail::make_integer_range_impl<
      T,
      std::make_integer_sequence<
        T,
        End - Begin
      >,
      Begin
    >::type;
    template <size_t Begin, size_t End>
    using make_index_range = make_integer_range<size_t, Begin, End>;

    template <class... Types, size_t... idxs>
    auto slice_tuple(std::tuple<Types...>& tuple, std::index_sequence<idxs...>) ->
      std::tuple<decltype(std::get<idxs>(std::move(tuple)))...>
    {
      return {std::get<idxs>(std::move(tuple))...};
    }

    template <bool... conds, class... Args>
    decltype(auto) if_constexpr(Args&&... the_args) {
      auto packed = std::forward_as_tuple(std::forward<Args>(the_args)...);
      auto funcs = slice_tuple(packed, make_index_range<0, sizeof...(conds)> {});
      auto args = slice_tuple(packed,
          make_index_range<sizeof...(conds), std::tuple_size<decltype(packed)>::value> {});
      return detail::if_constexpr_dispatch<conds...>(funcs,
          std::move(args), std::make_index_sequence<std::tuple_size<decltype(args)>::value> {});
    }

    template <class>
    struct is_heap : std::false_type {};
    template <template <class> class RefCount>
    struct is_heap<basic_heap<RefCount>> : std::true_type {};

    template <class>
    struct is_buffer : std::false_type {};
    template <template <class> class RefCount>
    struct is_buffer<basic_buffer<RefCount>> : std::true_type {};

    template <class>
    struct is_packet : std::false_type {};
    template <template <class> class RefCount>
    struct is_packet<basic_packet<RefCount>> : std::true_type {};

    template <class>
    struct packet_rebind;
    template <template <template <class> class> class TargetPacket, template <class> class TargetCount>
    struct packet_rebind<TargetPacket<TargetCount>> {
      template <template <template <class> class> class CurrPacket, template <class> class CurrCount>
      static auto rebind(CurrPacket<CurrCount> const& pkt) {
        return TargetPacket<TargetCount> {CurrPacket<CurrCount>::template transmogrify<TargetCount>(pkt)};
      }
    };

  }

  template <class T>
  struct obtuse_ptr {
    obtuse_ptr(obtuse_ptr const&) = delete;
    obtuse_ptr(obtuse_ptr&&) = delete;
    std::shared_ptr<T> impl;
  };

  namespace refcount {

    template <class T>
    struct element_type<obtuse_ptr<T>> {
      using type = T;
    };

    template <class T>
    struct construct<obtuse_ptr<T>> {
      template <class... Args>
      static void perform(obtuse_ptr<T>* that, Args&&... the_args) {
        new(that) obtuse_ptr<T> {std::make_shared<T>(std::forward<Args>(the_args)...)};
      }
    };

    template <class T>
    struct take<obtuse_ptr<T>> {
      template <class D>
      static void perform(obtuse_ptr<T>* that, std::nullptr_t, D&&) {
        new(that) obtuse_ptr<T> {std::shared_ptr<T>(nullptr)};
      }
      template <class D>
      static void perform(obtuse_ptr<T>* that, T* owned, D&& del) {
        new(that) obtuse_ptr<T> {std::shared_ptr<T>(owned, std::forward<D>(del))};
      }
    };

    template <class T>
    struct copy<obtuse_ptr<T>> {
      static void perform(obtuse_ptr<T>* that, obtuse_ptr<T> const& other) {
        new(that) obtuse_ptr<T> {other.impl};
      }
    };

    template <class T>
    struct move<obtuse_ptr<T>> {
      static void perform(obtuse_ptr<T>* that, obtuse_ptr<T>&& other) {
        new(that) obtuse_ptr<T> {std::move(other.impl)};
      }
    };

    template <class T>
    struct unwrap<obtuse_ptr<T>> {
      static T* perform(obtuse_ptr<T> const& rc) {
        return rc.impl.get();
      }
    };

    template <class T>
    struct use_count<obtuse_ptr<T>> {
      static size_t perform(obtuse_ptr<T> const& rc) {
        return rc.impl.use_count();
      }
    };

    template <class T>
    struct reset<obtuse_ptr<T>> {
      static void perform(obtuse_ptr<T>& rc) {
        rc.impl.reset();
      }
    };

  }

  using unsafe_heap = basic_heap<unsafe_ptr>;
  using obtuse_heap = basic_heap<obtuse_ptr>;
  using unsafe_buffer = basic_buffer<unsafe_ptr>;
  using obtuse_buffer = basic_buffer<obtuse_ptr>;
  using unsafe_packet = basic_packet<unsafe_ptr>;
  using obtuse_packet = basic_packet<obtuse_ptr>;

  struct noop {
    template <class T, T val>
    void operator ()(std::integral_constant<T, val>) {}
  };

  /*----- Free Functions -----*/

  template <size_t idx, class Callback>
  void for_each_t(Callback&&) {}
  template <size_t idx, class Current, class... Rest, class Callback>
  void for_each_t(Callback&& cb) {
    struct type_tag {
      using type = Current;
    };
    cb(type_tag {}, std::integral_constant<size_t, idx> {});

    // Intentionally allow cb to decay.
    // Never consumed, doesn't gain anything from forwarding.
    for_each_t<idx + 1, Rest...>(cb);
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void heap_object_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_object<heap>,
      basic_object<unsafe_heap>,
      basic_object<obtuse_heap>
    >(cb);
    cont(std::integral_constant<size_t, idx + 3> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_heap_object_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_object<heap>
    >(cb);
    cont(std::integral_constant<size_t, idx + 1> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void buffer_object_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_object<buffer>,
      basic_object<unsafe_buffer>,
      basic_object<obtuse_buffer>
    >(cb);
    cont(std::integral_constant<size_t, idx + 3> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_buffer_object_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_object<buffer>
    >(cb);
    cont(std::integral_constant<size_t, idx + 1> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void packet_object_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_object<packet>,
      basic_object<unsafe_packet>,
      basic_object<obtuse_packet>
    >(cb);
    cont(std::integral_constant<size_t, idx + 3> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_packet_object_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_object<packet>
    >(cb);
    cont(std::integral_constant<size_t, idx + 1> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void mutable_object_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    heap_object_api_test<idx>(cb, [&] (auto next) {
      packet_object_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_mutable_object_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    simple_heap_object_api_test<idx>(cb, [&] (auto next) {
      simple_packet_object_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void finalized_object_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    buffer_object_api_test<idx>(cb, [&] (auto next) {
      packet_object_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_finalized_object_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    simple_buffer_object_api_test<idx>(cb, [&] (auto next) {
      simple_packet_object_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void object_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    mutable_object_api_test<idx>(cb, [&] (auto next) {
      finalized_object_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_object_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    simple_mutable_object_api_test<idx>(cb, [&] (auto next) {
      simple_finalized_object_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void heap_array_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_array<heap>,
      basic_array<unsafe_heap>,
      basic_array<obtuse_heap>
    >(cb);
    cont(std::integral_constant<size_t, idx + 3> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_heap_array_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_array<heap>
    >(cb);
    cont(std::integral_constant<size_t, idx + 1> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void buffer_array_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_array<buffer>,
      basic_array<unsafe_buffer>,
      basic_array<obtuse_buffer>
    >(cb);
    cont(std::integral_constant<size_t, idx + 3> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_buffer_array_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_array<buffer>
    >(cb);
    cont(std::integral_constant<size_t, idx + 1> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void packet_array_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_array<packet>,
      basic_array<unsafe_packet>,
      basic_array<obtuse_packet>
    >(cb);
    cont(std::integral_constant<size_t, idx + 3> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_packet_array_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_array<packet>
    >(cb);
    cont(std::integral_constant<size_t, idx + 1> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void mutable_array_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    heap_array_api_test<idx>(cb, [&] (auto next) {
      packet_array_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_mutable_array_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    simple_heap_array_api_test<idx>(cb, [&] (auto next) {
      simple_packet_array_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void finalized_array_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    buffer_array_api_test<idx>(cb, [&] (auto next) {
      packet_array_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_finalized_array_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    simple_buffer_array_api_test<idx>(cb, [&] (auto next) {
      simple_packet_array_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void array_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    mutable_array_api_test<idx>(cb, [&] (auto next) {
      finalized_array_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_array_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    simple_mutable_array_api_test<idx>(cb, [&] (auto next) {
      simple_finalized_array_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void heap_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      heap,
      unsafe_heap,
      obtuse_heap
    >(cb);
    cont(std::integral_constant<size_t, idx + 3> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_heap_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      heap
    >(cb);
    cont(std::integral_constant<size_t, idx + 1> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void buffer_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      buffer,
      unsafe_buffer,
      obtuse_buffer
    >(cb);
    cont(std::integral_constant<size_t, idx + 3> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_buffer_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      buffer
    >(cb);
    cont(std::integral_constant<size_t, idx + 1> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void packet_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      packet,
      unsafe_packet,
      obtuse_packet
    >(cb);
    cont(std::integral_constant<size_t, idx + 3> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_packet_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      packet
    >(cb);
    cont(std::integral_constant<size_t, idx + 1> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void mutable_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    heap_api_test<idx>(cb, [&] (auto next) {
      packet_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_mutable_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    simple_heap_api_test<idx>(cb, [&] (auto next) {
      simple_packet_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void finalized_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    buffer_api_test<idx>(cb, [&] (auto next) {
      packet_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_finalized_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    simple_buffer_api_test<idx>(cb, [&] (auto next) {
      simple_packet_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    mutable_api_test<idx>(cb, [&] (auto next) {
      finalized_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    simple_mutable_api_test<idx>(cb, [&] (auto next) {
      simple_finalized_api_test<next>(cb, cont);
    });
  }

  template <class LhsPacket, class RhsPacket>
  LhsPacket conversion_helper(RhsPacket const& curr) {
    return meta::packet_rebind<LhsPacket>::rebind(curr);
  }

}

#endif
