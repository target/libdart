#ifndef DART_TESTS_H
#define DART_TESTS_H

/*----- Local Includes -----*/

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
    auto slice_tuple(std::tuple<Types...>& tuple, std::index_sequence<idxs...>)
    {
      using sliced_tuple = std::tuple<decltype(std::get<idxs>(std::move(tuple)))...>;
      return sliced_tuple {std::get<idxs>(std::move(tuple))...};
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
    obtuse_ptr() = delete;
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

    // This is good for the CI pathway, as we get much better coverage,
    // but it's way too expensive for the average user.
#ifdef DART_EXTENDED_TESTS
    for_each_t<idx + 1, Rest...>(cb);
#endif
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
  void heap_string_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_string<heap>,
      basic_string<unsafe_heap>,
      basic_string<obtuse_heap>
    >(cb);
    cont(std::integral_constant<size_t, idx + 3> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_heap_string_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_string<heap>
    >(cb);
    cont(std::integral_constant<size_t, idx + 1> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void buffer_string_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_string<buffer>,
      basic_string<unsafe_buffer>,
      basic_string<obtuse_buffer>
    >(cb);
    cont(std::integral_constant<size_t, idx + 3> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_buffer_string_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_string<buffer>
    >(cb);
    cont(std::integral_constant<size_t, idx + 1> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void packet_string_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_string<packet>,
      basic_string<unsafe_packet>,
      basic_string<obtuse_packet>
    >(cb);
    cont(std::integral_constant<size_t, idx + 3> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_packet_string_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_string<packet>
    >(cb);
    cont(std::integral_constant<size_t, idx + 1> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void mutable_string_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    heap_string_api_test<idx>(cb, [&] (auto next) {
      packet_string_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_mutable_string_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    simple_heap_string_api_test<idx>(cb, [&] (auto next) {
      simple_packet_string_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void finalized_string_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    buffer_string_api_test<idx>(cb, [&] (auto next) {
      packet_string_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_finalized_string_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    simple_buffer_string_api_test<idx>(cb, [&] (auto next) {
      simple_packet_string_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void string_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    mutable_string_api_test<idx>(cb, [&] (auto next) {
      finalized_string_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_string_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    simple_mutable_string_api_test<idx>(cb, [&] (auto next) {
      simple_finalized_string_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void heap_number_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_number<heap>,
      basic_number<unsafe_heap>,
      basic_number<obtuse_heap>
    >(cb);
    cont(std::integral_constant<size_t, idx + 3> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_heap_number_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_number<heap>
    >(cb);
    cont(std::integral_constant<size_t, idx + 1> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void buffer_number_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_number<buffer>,
      basic_number<unsafe_buffer>,
      basic_number<obtuse_buffer>
    >(cb);
    cont(std::integral_constant<size_t, idx + 3> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_buffer_number_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_number<buffer>
    >(cb);
    cont(std::integral_constant<size_t, idx + 1> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void packet_number_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_number<packet>,
      basic_number<unsafe_packet>,
      basic_number<obtuse_packet>
    >(cb);
    cont(std::integral_constant<size_t, idx + 3> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_packet_number_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_number<packet>
    >(cb);
    cont(std::integral_constant<size_t, idx + 1> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void mutable_number_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    heap_number_api_test<idx>(cb, [&] (auto next) {
      packet_number_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_mutable_number_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    simple_heap_number_api_test<idx>(cb, [&] (auto next) {
      simple_packet_number_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void finalized_number_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    buffer_number_api_test<idx>(cb, [&] (auto next) {
      packet_number_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_finalized_number_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    simple_buffer_number_api_test<idx>(cb, [&] (auto next) {
      simple_packet_number_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void number_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    mutable_number_api_test<idx>(cb, [&] (auto next) {
      finalized_number_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_number_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    simple_mutable_number_api_test<idx>(cb, [&] (auto next) {
      simple_finalized_number_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void heap_flag_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_flag<heap>,
      basic_flag<unsafe_heap>,
      basic_flag<obtuse_heap>
    >(cb);
    cont(std::integral_constant<size_t, idx + 3> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_heap_flag_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_flag<heap>
    >(cb);
    cont(std::integral_constant<size_t, idx + 1> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void buffer_flag_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_flag<buffer>,
      basic_flag<unsafe_buffer>,
      basic_flag<obtuse_buffer>
    >(cb);
    cont(std::integral_constant<size_t, idx + 3> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_buffer_flag_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_flag<buffer>
    >(cb);
    cont(std::integral_constant<size_t, idx + 1> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void packet_flag_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_flag<packet>,
      basic_flag<unsafe_packet>,
      basic_flag<obtuse_packet>
    >(cb);
    cont(std::integral_constant<size_t, idx + 3> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_packet_flag_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_flag<packet>
    >(cb);
    cont(std::integral_constant<size_t, idx + 1> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void mutable_flag_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    heap_flag_api_test<idx>(cb, [&] (auto next) {
      packet_flag_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_mutable_flag_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    simple_heap_flag_api_test<idx>(cb, [&] (auto next) {
      simple_packet_flag_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void finalized_flag_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    buffer_flag_api_test<idx>(cb, [&] (auto next) {
      packet_flag_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_finalized_flag_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    simple_buffer_flag_api_test<idx>(cb, [&] (auto next) {
      simple_packet_flag_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void flag_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    mutable_flag_api_test<idx>(cb, [&] (auto next) {
      finalized_flag_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_flag_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    simple_mutable_flag_api_test<idx>(cb, [&] (auto next) {
      simple_finalized_flag_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void heap_null_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_null<heap>,
      basic_null<unsafe_heap>,
      basic_null<obtuse_heap>
    >(cb);
    cont(std::integral_constant<size_t, idx + 3> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_heap_null_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_null<heap>
    >(cb);
    cont(std::integral_constant<size_t, idx + 1> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void buffer_null_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_null<buffer>,
      basic_null<unsafe_buffer>,
      basic_null<obtuse_buffer>
    >(cb);
    cont(std::integral_constant<size_t, idx + 3> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_buffer_null_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_null<buffer>
    >(cb);
    cont(std::integral_constant<size_t, idx + 1> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void packet_null_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_null<packet>,
      basic_null<unsafe_packet>,
      basic_null<obtuse_packet>
    >(cb);
    cont(std::integral_constant<size_t, idx + 3> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_packet_null_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    for_each_t<
      idx,
      basic_null<packet>
    >(cb);
    cont(std::integral_constant<size_t, idx + 1> {});
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void mutable_null_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    heap_null_api_test<idx>(cb, [&] (auto next) {
      packet_null_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_mutable_null_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    simple_heap_null_api_test<idx>(cb, [&] (auto next) {
      simple_packet_null_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void finalized_null_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    buffer_null_api_test<idx>(cb, [&] (auto next) {
      packet_null_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_finalized_null_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    simple_buffer_null_api_test<idx>(cb, [&] (auto next) {
      simple_packet_null_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void null_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    mutable_null_api_test<idx>(cb, [&] (auto next) {
      finalized_null_api_test<next>(cb, cont);
    });
  }

  template <size_t idx = 0, class Callback, class Continuation = noop>
  void simple_null_api_test(Callback&& cb, Continuation&& cont = Continuation {}) {
    simple_mutable_null_api_test<idx>(cb, [&] (auto next) {
      simple_finalized_null_api_test<next>(cb, cont);
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

  template <int64_t low = std::numeric_limits<int64_t>::min(),
           int64_t high = std::numeric_limits<int64_t>::max()>
  int64_t rand_int() {
    static std::mt19937 engine(std::random_device {}());
    static std::uniform_int_distribution<int64_t> dist(low, high);
    return dist(engine);
  }

  inline std::string rand_string(size_t len = rand_int<0, 32>(), dart::shim::string_view prefix = "") {
    constexpr int low = 0, high = 25;
    static std::vector<char> alpha {
      'a', 'b', 'c', 'd', 'e', 'f', 'g',
      'h', 'i', 'j', 'k', 'l', 'm', 'n',
      'o', 'p', 'q', 'r', 's', 't', 'u',
      'v', 'w', 'x', 'y', 'z'
    };

    std::string retval {prefix};
    retval.resize(len, '\0');
    std::generate(retval.begin() + prefix.size(), retval.end(), [&] {
      return alpha[rand_int<low, high>()];
    });
    return retval;
  }

  template <class Callback>
  void n_times(int n, Callback&& cb) {
    for (auto i = 0; i < n; ++i) cb();
  }

}

#endif
