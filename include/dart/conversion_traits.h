#ifndef DART_CONVERSION_TRAITS_H
#define DART_CONVERSION_TRAITS_H

/*----- System Includes -----*/

#include <type_traits>
#include <unordered_map>

/*----- Local Includes -----*/

#include "shim.h"
#include "meta.h"
#include "common.h"

/*----- Type Declarations -----*/

namespace dart {

  namespace convert {

    // Struct can be specialized to allow Dart to
    // interoperate with arbitrary types.
    template <class T>
    struct to_dart {
      template <class Packet>
      static Packet cast(meta::nonesuch);
    };

    namespace detail {

      struct null_tag {};
      struct boolean_tag {};
      struct integer_tag {};
      struct decimal_tag {};
      struct string_tag {};
      struct wrapper_tag {};
      struct dart_tag {};
      struct user_tag {};

      // Meta-function normalizes any conceivable type into one of
      // eight cases that can then be individually processed.
      //
      // Think of it as a switch statement, but for types.
      //
      // It's abusing overload mechanics to test different
      // properties of a given type by SFINAEing away overloads
      // that don't match.
      //
      // The ordering of the checks is important
      template <class T>
      struct normalize {
        template <class U, class =
          std::enable_if_t<
            std::is_same<
              U,
              std::nullptr_t
            >::value
          >
        >
        static null_tag detect(meta::priority_tag<6>);
        template <class U, class =
          std::enable_if_t<
            std::is_same<
              U,
              bool
            >::value
          >
        >
        static boolean_tag detect(meta::priority_tag<5>);
        template <class U, class =
          std::enable_if_t<
            std::is_integral<U>::value
          >
        >
        static integer_tag detect(meta::priority_tag<4>);
        template <class U, class =
          std::enable_if_t<
            std::is_floating_point<U>::value
          >
        >
        static decimal_tag detect(meta::priority_tag<3>);
        template <class U, class =
          std::enable_if_t<
            std::is_convertible<
              U,
              shim::string_view
            >::value
          >
        >
        static string_tag detect(meta::priority_tag<2>);
        template <class U, class =
          std::enable_if_t<
            meta::disjunction<
              meta::is_specialization_of<U, basic_object>,
              meta::is_specialization_of<U, basic_array>,
              meta::is_specialization_of<U, basic_string>,
              meta::is_specialization_of<U, basic_number>,
              meta::is_specialization_of<U, basic_flag>,
              meta::is_specialization_of<U, basic_null>
            >::value
          >
        >
        static wrapper_tag detect(meta::priority_tag<1>);
        template <class U, class =
          std::enable_if_t<
            meta::disjunction<
              meta::is_higher_specialization_of<U, basic_heap>,
              meta::is_higher_specialization_of<U, basic_buffer>,
              meta::is_higher_specialization_of<U, basic_packet>
            >::value
          >
        >
        static dart_tag detect(meta::priority_tag<0>);
        template <class>
        static user_tag detect(...);

        using type = decltype(detect<std::decay_t<T>>(meta::priority_tag<6> {}));
      };
      template <class T>
      using normalize_t = typename normalize<T>::type;

      // Makes the question of whether a user conversion is defined SFINAEable
      // so that it can be used in meta::is_detected.
      template <class T, class Packet>
      using user_cast_t = decltype(to_dart<std::decay_t<T>>::template cast<Packet>(std::declval<T>()));

      // Switch table implementation for type conversions.
      template <class T>
      struct caster_impl;
      template <>
      struct caster_impl<null_tag> {
        // This handles the case where we were given nullptr
        template <class Packet>
        static Packet cast(std::nullptr_t) {
          return Packet::make_null();
        }
      };
      template <>
      struct caster_impl<boolean_tag> {
        // This handles the case where we were given bool.
        template <class Packet>
        static Packet cast(bool val) {
          return Packet::make_boolean(val);
        }
      };
      template <>
      struct caster_impl<integer_tag> {
        // This handles the case where we were given int anything.
        template <class Packet>
        static Packet cast(int64_t val) {
          return Packet::make_integer(val);
        }
      };
      template <>
      struct caster_impl<decimal_tag> {
        // This handles the case where we were given float/double anything.
        template <class Packet>
        static Packet cast(double val) {
          return Packet::make_decimal(val);
        }
      };
      template <>
      struct caster_impl<string_tag> {
        // This handles the case where we given anything convertible to
        // std::string_view
        template <class Packet>
        static Packet cast(shim::string_view val) {
          return Packet::make_string(val);
        }
      };
      template <>
      struct caster_impl<dart_tag> {
        // This handles the case that we were given
        // the right type to start with.
        // Forwards whatever it was given back out.
        template <class TargetPacket, class Packet,
          std::enable_if_t<
            std::is_same<
              TargetPacket,
              std::decay_t<Packet>
            >::value
          >* = nullptr
        >
        static Packet&& cast(Packet&& pkt) {
          return std::forward<Packet>(pkt);
        }
        // This handles the case where we were given
        // another dart type, but not the correct type.
        template <class TargetPacket, class Packet,
          std::enable_if_t<
            !std::is_same<
              TargetPacket,
              std::decay_t<Packet>
            >::value
          >* = nullptr
        >
        static TargetPacket cast(Packet&& pkt) {
          return TargetPacket {std::forward<Packet>(pkt)};
        }
      };
      template <>
      struct caster_impl<wrapper_tag> {
        // Handles the case where we were given a Dart wrapper
        // type like basic_object or basic_array.
        // Hands off to the conversion logic for the underlying implementation type.
        template <class Packet, class Wrapper>
        static Packet cast(Wrapper&& wrapper) {
          return caster_impl<dart_tag>::cast<Packet>(std::forward<Wrapper>(wrapper).val);
        }
      };
      template <>
      struct caster_impl<user_tag> {
        // Handles the case where we were given a user type
        // that has a defined conversion.
        template <class Packet, class T>
        static Packet cast(T&& val) {
          return to_dart<std::decay_t<T>>::template cast<Packet>(std::forward<T>(val));
        }
      };

      // Calculates if two dart types are using the same reference counter
      // implementation, even if the two dart types aren't the same.
      template <class PacketOne, class PacketTwo>
      struct same_refcounter : std::false_type {};
      template <template <class> class RefCount,
               template <template <class> class> class PacketOne,
               template <template <class> class> class PacketTwo>
      struct same_refcounter<PacketOne<RefCount>, PacketTwo<RefCount>> : std::true_type {};

      // Calculates if two Dart wrapper types are using the same reference counter
      // implementation, even if those two wrapper types are using different
      // implementation types.
      template <class Packet, class Wrapper>
      struct same_wrapped_refcounter : std::false_type {};
      template <template <class> class RefCount,
               template <class> class Wrapper,
               template <template <class> class> class PacketOne,
               template <template <class> class> class PacketTwo>
      struct same_wrapped_refcounter<
        PacketOne<RefCount>,
        Wrapper<PacketTwo<RefCount>>
      > : std::true_type {};

    }

    /**
     *  @brief
     *  Function can be used to convert any registered type to a Dart packet object.
     *
     *  @details
     *  Say you had a simple custom string class along the lines of the following:
     *  ```
     *  struct my_string {
     *    std::string str;
     *  };
     *  ```
     *  And you wanted to be able to perform operations like so:
     *  ```
     *  // Add to dart objects directly.
     *  dart::packet obj = dart::packet::object("hello", my_string {"world"});
     *
     *  // Cast into a dart object directly.
     *  dart::packet str = dart::convert::cast<dart::packet>(my_string {"world"});
     *  ```
     *  You can accomplish this by specializing the struct `dart::convert::to_dart`
     *  with your custom type and implementing a cast function like so:
     *  ```
     *  namespace dart::convert {
     *    template <>
     *    struct to_dart<my_string> {
     *      template <class Packet>
     *      static Packet cast(my_string const& s) {
     *        return Packet::make_string(s.str);
     *      }
     *    };
     *  }
     *  ```
     */
    template <class Packet = dart::basic_packet<std::shared_ptr>, class T>
    decltype(auto) cast(T&& val) {
      return detail::caster_impl<detail::normalize_t<T>>::template cast<Packet>(std::forward<T>(val));
    }

    /**
     *  @brief
     *  Meta-function calculates whether a call to dart::convert::cast will be well-formed
     *  for a particular T and TargetPacket type.
     *
     *  @details
     *  The expression:
     *  ```
     *  static_assert(dart::convert::is_castable<T, dart::packet>::value);
     *  ```
     *  Will compile successfully for any T that is either a builtin machine type,
     *  is an STL container of builtin machine types, is a user type for which
     *  a conversion has been defined using the conversion API, or is an STL container
     *  of such a user type.
     */
    template <class T, class TargetPacket, class Normalized = detail::normalize_t<T>>
    struct is_castable : 
      meta::disjunction<
        // We can cast if given a builtin type
        meta::contained<
          Normalized,
          convert::detail::string_tag,
          convert::detail::decimal_tag,
          convert::detail::integer_tag,
          convert::detail::boolean_tag,
          convert::detail::null_tag
        >,

        // If we're attempting to cast a dart type...
        meta::conjunction<
          std::is_same<
            Normalized,
            convert::detail::dart_tag
          >,
          // We can do it if the reference counters are the same.
          detail::same_refcounter<
            TargetPacket,
            std::decay_t<T>
          >
        >,
        // If we're attempting to cast a dart wrapper type...
        meta::conjunction<
          std::is_same<
            Normalized,
            convert::detail::wrapper_tag
          >,
          // We can do it if the reference counters are the same
          detail::same_wrapped_refcounter<
            TargetPacket,
            std::decay_t<T>
          >
        >,
        // If we're attempting to cast a user type...
        meta::conjunction<
          std::is_same<
            Normalized,
            convert::detail::user_tag
          >,
          // We can do it if they've specialized the user cast struct
          meta::is_detected<
            detail::user_cast_t,
            T,
            TargetPacket
          >
        >
      >
    {};

    template <class Packet = dart::basic_packet<std::shared_ptr>, class Callback, class... Args, class =
      std::enable_if_t<
        meta::conjunction<
          is_castable<Args, Packet>...
        >::value
      >
    >
    decltype(auto) as_span(Callback&& cb, Args&&... the_args) {
      // Create some temporary storage for the given arguments.
      std::array<Packet, sizeof...(Args)> storage {{cast<Packet>(std::forward<Args>(the_args))...}};

      // Pass through as a span.
      return std::forward<Callback>(cb)(gsl::make_span(storage));
    }

    // Specialization for interoperability with std::vector
    template <class T, class Alloc>
    struct to_dart<std::vector<T, Alloc>> {
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      static Packet cast(std::vector<T, Alloc> const& vec) {
        auto pkt = Packet::make_array();
        for (auto& val : vec) pkt.push_back(val);
        return pkt;
      }
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      static Packet cast(std::vector<T, Alloc>&& vec) {
        auto pkt = Packet::make_array();
        for (auto& val : vec) pkt.push_back(std::move(val));
        return pkt;
      }
    };

    // Specialization for interoperability with std::array
    template <class T, size_t len>
    struct to_dart<std::array<T, len>> {
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      static Packet cast(std::array<T, len> const& arr) {
        auto pkt = Packet::make_array();
        for (auto& val : arr) pkt.push_back(val);
        return pkt;
      }
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      static Packet cast(std::array<T, len>&& arr) {
        auto pkt = Packet::make_array();
        for (auto& val : arr) pkt.push_back(std::move(val));
        return pkt;
      }
    };

    // Specialization for interoperability with std::map
    template <class Key, class Value, class Comp, class Alloc>
    struct to_dart<std::map<Key, Value, Comp, Alloc>> {
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
          &&
          convert::is_castable<Value, Packet>::value
        >
      >
      static Packet cast(std::map<Key, Value, Comp, Alloc> const& map) {
        auto obj = Packet::make_object();
        for (auto& pair : map) obj.add_field(pair.first, pair.second);
        return obj;
      }
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
          &&
          convert::is_castable<Value, Packet>::value
        >
      >
      static Packet cast(std::map<Key, Value, Comp, Alloc>&& map) {
        auto obj = Packet::make_object();
        for (auto& pair : map) obj.add_field(std::move(pair).first, std::move(pair).second);
        return obj;
      }
    };

    // Specialization for interoperability with std::unordered_map
    template <class Key, class Value, class Hash, class Equal, class Alloc>
    struct to_dart<std::unordered_map<Key, Value, Hash, Equal, Alloc>> {
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
          &&
          convert::is_castable<Value, Packet>::value
        >
      >
      static Packet cast(std::unordered_map<Key, Value, Hash, Equal, Alloc> const& map) {
        auto obj = Packet::make_object();
        for (auto& pair : map) obj.add_field(pair.first, pair.second);
        return obj;
      }
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
          &&
          convert::is_castable<Value, Packet>::value
        >
      >
      static Packet cast(std::unordered_map<Key, Value, Hash, Equal, Alloc>&& map) {
        auto obj = Packet::make_object();
        for (auto& pair : map) obj.add_field(std::move(pair).first, std::move(pair).second);
        return obj;
      }
    };

    // Specialization for interoperability with std::optional.
    template <class T>
    struct to_dart<shim::optional<T>> {
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      static Packet cast(shim::optional<T> const& opt) {
        if (opt) return convert::cast<Packet>(*opt);
        else return Packet::make_null();
      }
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      static Packet cast(shim::optional<T>&& opt) {
        if (opt) return convert::cast<Packet>(std::move(*opt));
        else return Packet::make_null();
      }
    };

    // Specialization for interoperability with std::variant.
    template <class... Ts>
    struct to_dart<shim::variant<Ts...>> {
      template <class Packet, class =
        std::enable_if_t<
          meta::conjunction<
            convert::is_castable<Ts, Packet>...
          >::value
        >
      >
      static Packet cast(shim::variant<Ts...> const& var) {
        return shim::visit([] (auto& val) { return convert::cast<Packet>(val); }, var);
      }
      template <class Packet, class =
        std::enable_if_t<
          meta::conjunction<
            convert::is_castable<Ts, Packet>...
          >::value
        >
      >
      static Packet cast(shim::variant<Ts...>&& var) {
        return shim::visit([] (auto&& val) { return convert::cast<Packet>(std::move(val)); }, std::move(var));
      }
    };

    // Specialization for interoperability with std::tuple.
    template <class... Ts>
    struct to_dart<std::tuple<Ts...>> {
      template <class Packet, class Tuple, size_t... idxs>
      static Packet unpack(Tuple&& tup, std::index_sequence<idxs...>) {
        return Packet::make_array(std::get<idxs>(std::forward<Tuple>(tup))...);
      }

      template <class Packet, class =
        std::enable_if_t<
          meta::conjunction<
            convert::is_castable<Ts, Packet>...
          >::value
        >
      >
      static Packet cast(std::tuple<Ts...> const& tup) {
        return unpack<Packet>(tup, std::index_sequence_for<Ts...> {});
      }
      template <class Packet, class =
        std::enable_if_t<
          meta::conjunction<
            convert::is_castable<Ts, Packet>...
          >::value
        >
      >
      static Packet cast(std::tuple<Ts...>&& tup) {
        return unpack<Packet>(std::move(tup), std::index_sequence_for<Ts...> {});
      }
    };

    // Bizzarely useful in some meta-programming situations.
    template <class T, T val>
    struct to_dart<std::integral_constant<T, val>> {
      template <class Packet>
      static Packet cast(std::integral_constant<T, val>) {
        return convert::cast<Packet>(val);
      }
    };

  }

}

#endif
