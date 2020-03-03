#ifndef DART_CONVERSION_TRAITS_H
#define DART_CONVERSION_TRAITS_H

/*----- System Includes -----*/

#include <type_traits>

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
    struct conversion_traits {
      template <class Packet>
      Packet to_dart(meta::nonesuch);
      template <class Packet>
      T from_dart(meta::nonesuch);
      template <class Packet>
      bool compare(Packet const&, meta::nonesuch);
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

      inline std::string type_to_string(dart::detail::type t) {
        switch (t) {
          case dart::detail::type::object:
            return "object";
          case dart::detail::type::array:
            return "array";
          case dart::detail::type::string:
            return "string";
          case dart::detail::type::integer:
            return "integer";
          case dart::detail::type::decimal:
            return "decimal";
          case dart::detail::type::boolean:
            return "boolean";
          default:
            DART_ASSERT(t == dart::detail::type::null);
            return "null";
        }
      }

      inline void report_type_mismatch(dart::detail::type expected, dart::detail::type encountered) {
        std::string errmsg = "Encountered type \"";
        errmsg += type_to_string(encountered);
        errmsg += "\" when expecting \"";
        errmsg += type_to_string(expected);
        errmsg += "\" during serialization";
        throw type_error(errmsg.c_str());
      }

      // Struct is basically a switch table of different internal Dart conversion
      // operations.
      // Allows Dart conversion operations to run via the same operator T() that
      // everything else uses.
      template <class From, class To>
      struct api_converter;
      template <template <class> class RefCount>
      struct api_converter<basic_buffer<RefCount>, basic_heap<RefCount>> {
        using heap = basic_heap<RefCount>;
        using buffer = basic_buffer<RefCount>;

        template <class Buffer>
        static heap convert(Buffer&& buff) {
          switch (buff.get_type()) {
            case dart::detail::type::object:
              {
                typename buffer::iterator k, v;
                std::tie(k, v) = buff.kvbegin();
                auto obj = heap::make_object();
                while (v != buff.end()) {
                  obj.add_field(heap {*k}, heap {*v});
                  ++k, ++v;
                }
                return obj;
              }
            case dart::detail::type::array:
              {
                auto arr = heap::make_array();
                for (auto elem : buff) arr.push_back(heap {std::move(elem)});
                return arr;
              }
            case dart::detail::type::string:
              return heap::make_string(buff.strv());
            case dart::detail::type::integer:
              return heap::make_integer(buff.integer());
            case dart::detail::type::decimal:
              return heap::make_decimal(buff.decimal());
            case dart::detail::type::boolean:
              return heap::make_boolean(buff.boolean());
            default:
              DART_ASSERT(buff.get_type() == dart::detail::type::null);
              return heap::make_null();
          }
        }
      };
      template <template <class> class RefCount>
      struct api_converter<basic_heap<RefCount>, basic_buffer<RefCount>> {
        using heap = basic_heap<RefCount>;
        using buffer = basic_buffer<RefCount>;

        template <class Heap>
        static buffer convert(Heap&& hp) {
          if (!hp.is_object()) {
            throw type_error("dart::buffer can only be constructed from an object heap");
          }

          // Calculate the maximum amount of memory that could be required to represent this dart::packet and
          // allocate the whole thing in one go.
          buffer buff;
          size_t bytes = hp.upper_bound();
          auto buftype = dart::detail::raw_type::object;
          buff.buffer_ref = dart::detail::aligned_alloc<RefCount>(bytes, buftype, [&] (auto* buff) {
            std::fill_n(buff, bytes, gsl::byte {});
            hp.layout(buff);
          });
          buff.raw = {dart::detail::raw_type::object, buff.buffer_ref.get()};
          return buff;
        }
      };
      template <template <class> class RefCount>
      struct api_converter<basic_packet<RefCount>, basic_heap<RefCount>> {
        using heap = basic_heap<RefCount>;
        using buffer = basic_buffer<RefCount>;
        using packet = basic_packet<RefCount>;

        inline static heap convert(packet const& pkt) {
          auto* h = shim::get_if<heap>(&pkt.impl);
          if (h) return *h;
          else return heap {shim::get<buffer>(pkt.impl)};
        }

        inline static heap convert(packet&& pkt) {
          auto* h = shim::get_if<heap>(&pkt.impl);
          if (h) return std::move(*h);
          else return heap {shim::get<buffer>(std::move(pkt.impl))};
        }
      };
      template <template <class> class RefCount>
      struct api_converter<basic_heap<RefCount>, basic_packet<RefCount>> {
        using heap = basic_heap<RefCount>;
        using packet = basic_packet<RefCount>;

        template <class Heap>
        static packet convert(Heap&& hp) {
          return packet {std::forward<Heap>(hp)};
        }
      };
      template <template <class> class RefCount>
      struct api_converter<basic_packet<RefCount>, basic_buffer<RefCount>> {
        using heap = basic_heap<RefCount>;
        using buffer = basic_buffer<RefCount>;
        using packet = basic_packet<RefCount>;

        inline static buffer convert(packet const& pkt) {
          auto* b = shim::get_if<buffer>(&pkt.impl);
          if (b) return *b;
          else return buffer {shim::get<heap>(pkt.impl)};
        }

        inline static buffer convert(packet&& pkt) {
          auto* b = shim::get_if<buffer>(&pkt.impl);
          if (b) return std::move(*b);
          else return buffer {shim::get<heap>(std::move(pkt.impl))};
        }
      };
      template <template <class> class RefCount>
      struct api_converter<basic_buffer<RefCount>, basic_packet<RefCount>> {
        using buffer = basic_buffer<RefCount>;
        using packet = basic_packet<RefCount>;

        template <class Buffer>
        static packet convert(Buffer&& buff) {
          return packet {std::forward<Buffer>(buff)};
        }
      };

      // Handles the case of converting a view type back into
      // its corresponding packet type
      template <class TargetPacket, class Packet>
      TargetPacket view_convert(Packet&& pkt, std::true_type) {
        return TargetPacket {std::forward<Packet>(pkt).as_owner()};
      }

      // Handles the case of converting a packet type into its
      // corresponding view type.
      template <class TargetPacket, class Packet>
      TargetPacket view_convert(Packet&& pkt, std::false_type) {
        return TargetPacket {std::forward<Packet>(pkt)};
      }

      // Struct is basically a switch table of different Dart comparison operations
      // where both lhs and rhs are known to be specializations of the same Dart
      // template, even if instantiated with different reference counters.
      template <class PacketType>
      struct typed_compare;
      template <template <class> class RefCount>
      struct typed_compare<basic_heap<RefCount>> {
        template <class OtherHeap>
        static bool compare(basic_heap<RefCount> const& lhs, OtherHeap const& rhs) noexcept {
          // Check if we're comparing against ourselves.
          // Cast is necessary to ensure validity if we're comparing
          // against a different refcounter.
          if (static_cast<void const*>(&lhs) == static_cast<void const*>(&rhs)) return true;

          // Check if we're even the same type.
          if (lhs.is_null() && rhs.is_null()) return true;
          else if (lhs.get_type() != rhs.get_type()) return false;

          // Defer to our underlying representation.
          return shim::visit([] (auto& lhs, auto& rhs) {
            using Lhs = std::decay_t<decltype(lhs)>;
            using Rhs = std::decay_t<decltype(rhs)>;

            // Have to compare through pointers here, and we make the decision purely off of the
            // ability to dereference the incoming type to try to allow compatibility with custom
            // external smart-pointers that otherwise conform to the std::shared_ptr interface.
            auto comparator = dart::detail::typeless_comparator {};
            auto& lval = dart::detail::maybe_dereference(lhs, meta::is_dereferenceable<Lhs> {});
            auto& rval = dart::detail::maybe_dereference(rhs, meta::is_dereferenceable<Rhs> {});
            return comparator(lval, rval);
          }, lhs.data, rhs.data);
        }
      };
      template <template <class> class RefCount>
      struct typed_compare<basic_buffer<RefCount>> {
        template <class OtherBuffer>
        static bool compare(basic_buffer<RefCount> const& lhs, OtherBuffer const& rhs) noexcept {
          // Check if we're comparing against ourselves.
          // Cast is necessary to ensure validity if we're comparing
          // against a different refcounter.
          if (static_cast<void const*>(&lhs) == static_cast<void const*>(&rhs)) return true;

          // Check if we're sure we're even the same type.
          auto rawlhs = lhs.raw, rawrhs = rhs.raw;
          if (lhs.is_null() && rhs.is_null()) return true;
          else if (lhs.get_type() != rhs.get_type()) return false;
          else if (rawlhs.buffer == rawrhs.buffer) return true;

          // Fall back on a comparison of the underlying buffers.
          auto lhs_size = dart::detail::find_sizeof<RefCount>(rawlhs);
          auto rhs_size = dart::detail::find_sizeof<RefCount>(rawrhs);
          if (lhs_size == rhs_size) {
            return std::equal(rawlhs.buffer, rawlhs.buffer + lhs_size, rawrhs.buffer);
          } else {
            return false;
          }
        }
      };
      template <template <class> class RefCount>
      struct typed_compare<basic_packet<RefCount>> {
        template <class OtherPacket>
        static bool compare(basic_packet<RefCount> const& lhs, OtherPacket const& rhs) noexcept {
          // Check if we're comparing against ourselves.
          // Cast is necessary to ensure validity if we're comparing
          // against a different refcounter.
          if (static_cast<void const*>(&lhs) == static_cast<void const*>(&rhs)) return true;
          return shim::visit([] (auto& lhs, auto& rhs) { return lhs == rhs; }, lhs.impl, rhs.impl);
        }
      };

      // Function implements the fallback case where we've been asked to compare two
      // Dart types which have been instantiated off of different base templates.
      template <class Lhs, class Rhs>
      bool generic_compare(Lhs const& lhs, Rhs const& rhs) noexcept {
        // Make sure they're at least of the same type.
        if (lhs.get_type() != rhs.get_type()) return false;

        // Perform type specific comparisons.
        switch (lhs.get_type()) {
          case dart::detail::type::object:
            {
              // Bail early if we can.
              if (lhs.size() != rhs.size()) return false;

              // Ouch.
              // Iterates over rhs and looks up into lhs because lhs is the finalized
              // object and lookups should be significantly faster on it.
              typename std::decay_t<Rhs>::iterator k, v;
              std::tie(k, v) = rhs.kvbegin();
              while (v != rhs.end()) {
                if (*v != lhs[*k]) return false;
                ++k, ++v;
              }
              return true;
            }
          case dart::detail::type::array:
            {
              // Bail early if we can.
              if (lhs.size() != rhs.size()) return false;

              // Ouch.
              for (auto i = 0U; i < lhs.size(); ++i) {
                if (lhs[i] != rhs[i]) return false;
              }
              return true;
            }
          case dart::detail::type::string:
            return lhs.strv() == rhs.strv();
          case dart::detail::type::integer:
            return lhs.integer() == rhs.integer();
          case dart::detail::type::decimal:
            return lhs.decimal() == rhs.decimal();
          case dart::detail::type::boolean:
            return lhs.boolean() == rhs.boolean();
          default:
            DART_ASSERT(lhs.is_null());
            return true;
        }
      }

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
            meta::is_string<U>::value
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
      using user_cast_in_t =
          decltype(std::declval<conversion_traits<std::decay_t<T>>>().
                template to_dart<Packet>(std::declval<T>()));

      template <class T, class Packet>
      using user_cast_out_t =
          decltype(std::declval<conversion_traits<std::decay_t<T>>>().from_dart(std::declval<Packet>()));

      // Makes the question of whether a user comparison is defined SFINAEable
      // so that it can be used in meta::is_detected.
      template <class T, class Packet>
      using user_compare_t =
          decltype(std::declval<conversion_traits<std::decay_t<T>>>().compare(std::declval<Packet const&>(), std::declval<T>()));

      template <class T, class Packet>
      using nothrow_user_compare_t = std::enable_if_t<
        noexcept(std::declval<conversion_traits<std::decay_t<T>>>().compare(std::declval<Packet const&>(), std::declval<T>()))
      >;

      // Calculates if two dart types are using the same reference counter
      // implementation, even if the two dart types aren't the same.
      template <class PacketOne, class PacketTwo>
      struct same_refcounter : std::false_type {};
      template <template <class> class RefCount,
               template <template <class> class> class PacketOne,
               template <template <class> class> class PacketTwo>
      struct same_refcounter<PacketOne<RefCount>, PacketTwo<RefCount>> : std::true_type {};

      // Calculates if two dart types are using the same base template,
      // event if the supplied reference counters aren't the same.
      template <class PacketOne, class PacketTwo>
      struct same_packet : std::false_type {};
      template <template <class> class LhsRC,
               template <class> class RhsRC,
               template <template <class> class> class Packet>
      struct same_packet<Packet<LhsRC>, Packet<RhsRC>> : std::true_type {};

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

      // Assuming that BOTH From and To are Dart packet types,
      // calculates whether one type is the view type of the other.
      // If one or both of the two types are NOT Dart packet types,
      // will likely generate a syntax error
      template <class From, class To>
      struct are_view_compatible :
        meta::disjunction<
          meta::conjunction<
            std::is_lvalue_reference<
              From
            >,
            std::is_same<
              typename std::decay_t<From>::view,
              To
            >
          >,
          std::is_same<
            std::decay_t<From>,
            typename To::view
          >
        >
      {};

      // Calculates whether MaybeView is the view type of Base.
      // If Base is not a Dart packet type, will likely generate
      // a syntax error
      template <class MaybeView, class Base>
      struct is_view_of :
        std::is_same<
          typename Base::view,
          MaybeView
        >
      {};

      // Calculates whether the given dart type is a view type.
      template <class MaybeView>
      struct is_view :
        meta::negation<
          typename MaybeView::is_owning_type
        >
      {};

      // Checks if a user defined comparison is guaranteed
      // to never throw an exception
      template <class T, class Packet>
      struct user_compare_is_nothrow :
        meta::is_detected<
          nothrow_user_compare_t,
          T,
          Packet
        >
      {};

      // Checks if a given type and Dart type can be compared
      template <class T, class Packet, class Category>
      struct are_comparable_impl :
        // We're comparable if...
        meta::disjunction<
          // We've been given a user type...
          meta::conjunction<
            std::is_same<
              Category,
              user_tag
            >,

            // And the user has specialized the user equality struct.
            meta::is_detected<
              user_compare_t,
              T,
              Packet
            >
          >,

          // Else, we're comparable if we're a built in or internal type.
          meta::negation<
            std::is_same<
              Category,
              user_tag
            >
          >
        >
      {};

      // Check if a given type and Dart type can be compared,
      // and would be guaranteed not to throw an exception if so
      template <class T, class Packet, class Category>
      struct are_nothrow_comparable_impl :
        // We're nothrow comparable if...
        meta::conjunction<
          // We're comparable at all...
          are_comparable_impl<T, Packet, Category>,

          // And...
          meta::disjunction<
            // We're either a built in, or internal, type...
            meta::negation<
              std::is_same<
                Category,
                user_tag
              >
            >,
            
            // Or the user defined comparison is nothrow.
            user_compare_is_nothrow<T, Packet>
          >
        >
      {};

      // XXX: This has turned into a mess. In need of refactoring
      // This struct enforces conversion constraints when To is known to be
      // a Dart type (we're casting "into" the Dart API).
      // From: Can be ANY type, including cv-qualified reference types (must be decayed)
      // To: Known to be a canonical Dart type (doesn't need to be decayed)
      // Category: Calculated type category for From.
      template <class From, class To, class Category>
      struct is_incoming_castable :
        // The given type can be converted into a Dart type if...
        meta::disjunction<
          // It is a built in type...
          meta::conjunction<
            meta::contained<
              Category,
              string_tag,
              decimal_tag,
              integer_tag,
              boolean_tag,
              null_tag
            >,

            // And our chosen Dart type is capable of
            // representing it.
            typename To::is_mutable_type
          >,

          // It is a dart type...
          meta::conjunction<
            std::is_same<
              Category,
              dart_tag
            >,

            meta::disjunction<
              meta::conjunction<
                // A view type...
                is_view<
                  std::decay_t<From>
                >,

                // And the refcounters are the same.
                same_refcounter<
                  std::decay_t<From>,
                  typename To::view
                >
              >,

              // The refcounters are the same...
              same_refcounter<
                std::decay_t<From>,
                To
              >,

              // Or one is the view type for the other.
              are_view_compatible<
                From,
                To
              >
            >
          >,

          // It is a dart wrapper type...
          meta::conjunction<
            std::is_same<
              Category,
              wrapper_tag
            >,

            // And the reference counters are the same.
            same_wrapped_refcounter<
              To,
              std::decay_t<From>
            >
          >,

          // It is a user type...
          meta::conjunction<
            std::is_same<
              Category,
              user_tag
            >,

            // And the user has specialized the user conversion struct.
            meta::is_detected<
              user_cast_in_t,
              From,
              To
            >
          >
        >
      {};

      // This struct enforces conversion constraints when To is known to not be
      // a Dart type (we're casting "out of" the Dart API).
      // From: Known to be a Dart type, may be a cv-qualified reference to Dart (must be decayed)
      // To: Known NOT to be a Dart type. Assumed to be a canonical type (doesn't need to be decayed)
      // Category: Calculated type category for To.
      template <class From, class To, class Category>
      struct is_outgoing_castable :
        // We can convert from a Dart type to the given type if...
        meta::disjunction<
          // It is a primitive type other than a string.
          meta::negation<
            meta::contained<
              Category,
              user_tag,
              dart_tag,
              wrapper_tag,
              string_tag
            >
          >,

          // The target type is a string...
          meta::conjunction<
            std::is_same<
              Category,
              string_tag
            >,

            // And it is either NOT a character pointer,
            // or it is a CONST character pointer.
            meta::disjunction<
              meta::negation<
                std::is_pointer<To>
              >,
              meta::is_ptr_to_const<To>
            >
          >,

          // The target type is a user type...
          meta::conjunction<
            std::is_same<
              Category,
              user_tag
            >,

            // And the user has specialized the user conversion struct.
            meta::is_detected<
              user_cast_out_t,
              To,
              From
            >
          >
        >
      {};

      // The dart::convert::are_comparable type trait can receive
      // arguments in either order, so this dispatch struct takes care
      // of figuring out what order the arguments were passed in so that
      // the implementation struct can make concrete assumptions about
      // what its arguments represent
      template <template <class, class, class> class Impl,
               class LhsCat, class RhsCat, class Lhs, class Rhs>
      struct are_comparable_dispatch;

      // Case: Dart type and Dart type. Must exist to avoid ambiguity
      template <template <class, class, class> class Impl, class Lhs, class Rhs>
      struct are_comparable_dispatch<Impl, dart_tag, dart_tag, Lhs, Rhs> :
        Impl<Lhs, Rhs, dart_tag>
      {};

      // Case: Dart type and wrapper type. Must exist to avoid ambiguity
      template <template <class, class, class> class Impl, class Lhs, class Rhs>
      struct are_comparable_dispatch<Impl, dart_tag, wrapper_tag, Lhs, Rhs> :
        Impl<Rhs, Lhs, wrapper_tag>
      {};

      // Case: Wrapper type and Dart type. Must exist to avoid ambiguity.
      template <template <class, class, class> class Impl, class Lhs, class Rhs>
      struct are_comparable_dispatch<Impl, wrapper_tag, dart_tag, Lhs, Rhs> :
        Impl<Lhs, Rhs, wrapper_tag>
      {};

      // Case: Wrapper type and wrapper type. Must exist to avoid ambiguity.
      template <template <class, class, class> class Impl, class Lhs, class Rhs>
      struct are_comparable_dispatch<Impl, wrapper_tag, wrapper_tag, Lhs, Rhs> :
        Impl<Lhs, typename std::decay_t<Rhs>::value_type, wrapper_tag>
      {};

      // Case: Dart type and user type.
      template <template <class, class, class> class Impl, class FreeCat, class Lhs, class Rhs>
      struct are_comparable_dispatch<Impl, dart_tag, FreeCat, Lhs, Rhs> :
        Impl<Rhs, Lhs, FreeCat>
      {};

      // Case: Wrapper type and user type.
      template <template <class, class, class> class Impl, class FreeCat, class Lhs, class Rhs>
      struct are_comparable_dispatch<Impl, wrapper_tag, FreeCat, Lhs, Rhs> :
        Impl<Rhs, typename std::decay_t<Lhs>::value_type, FreeCat>
      {};

      // Case: User type and Dart type.
      template <template <class, class, class> class Impl, class FreeCat, class Lhs, class Rhs>
      struct are_comparable_dispatch<Impl, FreeCat, dart_tag, Lhs, Rhs> :
        Impl<Lhs, Rhs, FreeCat>
      {};

      // Case: User type and wrapper type.
      template <template <class, class, class> class Impl, class FreeCat, class Lhs, class Rhs>
      struct are_comparable_dispatch<Impl, FreeCat, wrapper_tag, Lhs, Rhs> :
        Impl<Lhs, typename std::decay_t<Rhs>::value_type, FreeCat>
      {};

      template <class FromCat, class ToCat, class From, class To>
      struct is_castable_dispatch;

      template <class From, class To>
      struct is_castable_dispatch<dart_tag, dart_tag, From, To> :
        is_incoming_castable<From, To, dart_tag>
      {};
      template <class From, class To>
      struct is_castable_dispatch<dart_tag, wrapper_tag, From, To> :
        is_incoming_castable<From, typename To::value_type, dart_tag>
      {};
      template <class From, class To>
      struct is_castable_dispatch<wrapper_tag, dart_tag, From, To> :
        is_incoming_castable<From, To, wrapper_tag>
      {};
      template <class From, class To>
      struct is_castable_dispatch<wrapper_tag, wrapper_tag, From, To> :
        is_incoming_castable<From, typename To::value_type, wrapper_tag>
      {};

      template <class FreeCat, class From, class To>
      struct is_castable_dispatch<dart_tag, FreeCat, From, To> :
        is_outgoing_castable<From, To, FreeCat>
      {};
      template <class FreeCat, class From, class To>
      struct is_castable_dispatch<wrapper_tag, FreeCat, From, To> :
        is_outgoing_castable<typename std::decay_t<From>::value_type, To, FreeCat>
      {};
      template <class FreeCat, class From, class To>
      struct is_castable_dispatch<FreeCat, dart_tag, From, To> :
        is_incoming_castable<From, To, FreeCat>
      {};
      template <class FreeCat, class From, class To>
      struct is_castable_dispatch<FreeCat, wrapper_tag, From, To> :
        is_incoming_castable<From, typename To::value_type, FreeCat>
      {};

      // Switch table implementation for type conversions.
      template <class T>
      struct incoming_caster;
      template <>
      struct incoming_caster<null_tag> {
        // This handles the case where we were given nullptr
        template <class Packet>
        static Packet cast(std::nullptr_t) {
          return Packet::make_null();
        }
      };
      template <>
      struct incoming_caster<boolean_tag> {
        // This handles the case where we were given bool.
        template <class Packet>
        static Packet cast(bool val) {
          return Packet::make_boolean(val);
        }
      };
      template <>
      struct incoming_caster<integer_tag> {
        // This handles the case where we were given int anything.
        template <class Packet>
        static Packet cast(int64_t val) {
          return Packet::make_integer(val);
        }
      };
      template <>
      struct incoming_caster<decimal_tag> {
        // This handles the case where we were given float/double anything.
        template <class Packet>
        static Packet cast(double val) {
          return Packet::make_decimal(val);
        }
      };
      template <>
      struct incoming_caster<string_tag> {
        // This handles the case where we given anything convertible to
        // std::string_view
        template <class Packet>
        static Packet cast(shim::string_view val) {
          return Packet::make_string(val);
        }
      };
      template <>
      struct incoming_caster<dart_tag> {
        // This handles the case that we were given
        // the right type to start with.
        // Forwards whatever it was given back out.
        template <class TargetPacket, class Packet, class =
          std::enable_if_t<
            std::is_same<
              TargetPacket,
              std::decay_t<Packet>
            >::value
          >
        >
        static Packet&& cast_impl(Packet&& pkt, meta::priority_tag<3>) {
          return std::forward<Packet>(pkt);
        }

        // This handles the case where we were given two packet types
        // where one is the view type of the other
        template <class TargetPacket, class Packet, class =
          std::enable_if_t<
            are_view_compatible<
              Packet,
              TargetPacket
            >::value
          >
        >
        static TargetPacket cast_impl(Packet&& pkt, meta::priority_tag<2>) {
          // Calculates whether Packet is the view type
          // of TargetPacket
          using view_check = is_view_of<
            std::decay_t<Packet>,
            TargetPacket
          >;
          return view_convert<TargetPacket>(std::forward<Packet>(pkt), view_check {});
        }

        // This handles the case where we're converting FROM a view type
        // to a NON-view type of a disparate base template
        template <class TargetPacket, class Packet, class =
          std::enable_if_t<
            is_view<std::decay_t<Packet>>::value
          >
        >
        static TargetPacket cast_impl(Packet&& pkt, meta::priority_tag<1>) {
          return TargetPacket {pkt.as_owner()};
        }

        // This handles the generic case where we
        // were given two otherwise unrelated Dart types that are not views
        template <class TargetPacket, class Packet>
        static TargetPacket cast_impl(Packet&& pkt, meta::priority_tag<0>) {
          return api_converter<std::decay_t<Packet>, TargetPacket>::convert(std::forward<Packet>(pkt));
        }

        // Have to take manual control of overload resolution for a bit here
        template <class TargetPacket, class Packet>
        static decltype(auto) cast(Packet&& pkt) {
          return cast_impl<TargetPacket>(std::forward<Packet>(pkt), meta::priority_tag<3> {});
        }
      };
      template <>
      struct incoming_caster<wrapper_tag> {
        // Handles the case where we were given a Dart wrapper
        // type like basic_object or basic_array.
        // Hands off to the conversion logic for the underlying implementation type.
        template <class Packet, class Wrapper>
        static Packet cast(Wrapper&& wrapper) {
          return incoming_caster<dart_tag>::cast<Packet>(std::forward<Wrapper>(wrapper).dynamic());
        }
      };
      template <>
      struct incoming_caster<user_tag> {
        // Handles the case where we were given a user type
        // that has a defined conversion.
        template <class Packet, class T>
        static Packet cast(T&& val) {
          return conversion_traits<std::decay_t<T>> {}.template to_dart<Packet>(std::forward<T>(val));
        }
      };

      template <class T>
      struct outgoing_caster;
      template <>
      struct outgoing_caster<null_tag> {
        template <class, class Packet>
        static std::nullptr_t cast(Packet const& pkt) {
          if (!pkt.is_null()) {
            report_type_mismatch(dart::detail::type::null, pkt.get_type());
          }
          return nullptr;
        }
      };
      template <>
      struct outgoing_caster<boolean_tag> {
        template <class, class Packet>
        static bool cast(Packet const& pkt) {
          if (pkt.is_boolean()) {
            return pkt.boolean();
          } else {
            return !pkt.is_null();
          }
        }
      };
      template <>
      struct outgoing_caster<integer_tag> {
        template <class T, class Packet>
        static T cast(Packet const& pkt) {
          if (!pkt.is_integer()) {
            report_type_mismatch(dart::detail::type::integer, pkt.get_type());
          }
          return static_cast<T>(pkt.integer());
        }
      };
      template <>
      struct outgoing_caster<decimal_tag> {
        template <class T, class Packet>
        static T cast(Packet const& pkt) {
          if (!pkt.is_decimal()) {
            report_type_mismatch(dart::detail::type::decimal, pkt.get_type());
          }
          return static_cast<T>(pkt.decimal());
        }
      };
      template <>
      struct outgoing_caster<string_tag> {
        template <class T, class Packet,
          std::enable_if_t<
            meta::contained<
              T,
              std::string,
              shim::string_view
            >::value
          >* = nullptr
        >
        static T cast(Packet const& pkt) {
          return static_cast<T>(pkt.strv());
        }
        template <class T, class Packet,
          std::enable_if_t<
            !meta::contained<
              T,
              std::string,
              shim::string_view
            >::value
          >* = nullptr
        >
        static T cast(Packet const& pkt) {
          static_assert(
            meta::disjunction<
              meta::negation<
                std::is_pointer<T>
              >,
              meta::is_ptr_to_const<T>
            >::value,
            "Dart cannot safely discard const for string"
          );
          return pkt.str();
        }
      };
      template <>
      struct outgoing_caster<dart_tag> {
        // This handles the case that we were given
        // the right type to start with.
        // Forwards whatever it was given back out.
        template <class T, class Packet,
          std::enable_if_t<
            std::is_same<
              T,
              std::decay_t<Packet>
            >::value
          >* = nullptr
        >
        static Packet&& cast(Packet&& pkt) {
          return std::forward<Packet>(pkt);
        }
        // This handles the case where we were given
        // another dart type, but not the correct type.
        template <class T, class Packet,
          std::enable_if_t<
            !std::is_same<
              T,
              std::decay_t<Packet>
            >::value
          >* = nullptr
        >
        static T cast(Packet&& pkt) {
          return T {std::forward<Packet>(pkt)};
        }
      };
      template <>
      struct outgoing_caster<wrapper_tag> {
        template <class T, class Packet>
        static T cast(Packet&& pkt) {
          return T {outgoing_caster<dart_tag>::cast<typename T::value_type>(std::forward<Packet>(pkt))};
        }
      };
      template <>
      struct outgoing_caster<user_tag> {
        template <class T, class Packet>
        static T cast(Packet&& pkt) {
          return conversion_traits<std::decay_t<T>> {}.from_dart(std::forward<Packet>(pkt));
        }
      };

      // Switch table implementation for Dart comparisons
      template <class T>
      struct compare_impl;
      template <>
      struct compare_impl<null_tag> {
        template <class Packet>
        static bool compare(Packet const& pkt, std::nullptr_t) noexcept {
          return pkt.is_null();
        }
      };
      template <>
      struct compare_impl<boolean_tag> {
        template <class Packet>
        static bool compare(Packet const& pkt, bool val) noexcept {
          return pkt.is_boolean() && pkt.boolean() == val;
        }
      };
      template <>
      struct compare_impl<integer_tag> {
        template <class Packet>
        static bool compare(Packet const& pkt, int64_t val) noexcept {
          if (pkt.is_integer()) return pkt.integer() == val;
          else if (pkt.is_decimal()) return pkt.decimal() == val;
          else return false;
        }
      };
      template <>
      struct compare_impl<decimal_tag> {
        template <class Packet>
        static bool compare(Packet const& pkt, double val) noexcept {
          if (pkt.is_integer()) return pkt.integer() == val;
          else if (pkt.is_decimal()) return pkt.decimal() == val;
          else return false;
        }
      };
      template <>
      struct compare_impl<string_tag> {
        template <class Packet>
        static bool compare(Packet const& pkt, shim::string_view val) noexcept {
          return pkt.is_str() && pkt.strv() == val;
        }
      };
      template <>
      struct compare_impl<dart_tag> {
        // Handles the case where the packet templates in use
        // are the same, even if the chosen reference counter is not.
        // Since both lhs and rhs are based off of the same template class,
        // we can use internal implementation details to perform comparison
        // faster.
        // MUCH faster in the case of dart::basic_buffer.
        template <class LhsPacket, class RhsPacket,
          std::enable_if_t<
            same_packet<
              LhsPacket,
              RhsPacket
            >::value
          >* = nullptr
        >
        static bool compare(LhsPacket const& lhs, RhsPacket const& rhs) noexcept {
          return typed_compare<LhsPacket>::compare(lhs, rhs);
        }

        // Handles the case where the packet templates in use
        // are NOT the same, regardless of the chosen reference counter.
        // Since lhs and rhs are NOT based off of the same template class,
        // we use the public API to avoid going insane
        template <class LhsPacket, class RhsPacket,
          std::enable_if_t<
            !same_packet<
              LhsPacket,
              RhsPacket
            >::value
          >* = nullptr
        >
        static bool compare(LhsPacket const& lhs, RhsPacket const& rhs) noexcept {
          // Lookups are faster on finalized objects, so dispatch such that
          // we attempt to perform lookups against the finalized object.
          if (lhs.is_finalized()) {
            return generic_compare(lhs, rhs);
          } else {
            return generic_compare(rhs, lhs);
          }
        }
      };
      template <>
      struct compare_impl<wrapper_tag> {
        template <class Packet, class Wrapper>
        static bool compare(Packet const& pkt, Wrapper const& wrap) noexcept {
          return compare_impl<dart_tag>::compare(pkt, wrap.dynamic());
        }
      };
      template <>
      struct compare_impl<user_tag> {
        template <class Packet, class T>
        static bool compare(Packet const& pkt, T const& val) noexcept(user_compare_is_nothrow<Packet, T>::value) {
          return conversion_traits<std::decay_t<T>> {}.compare(pkt, val);
        }
      };

      // These overloads have to exist to avoid overload ambiguity.
      // Argument order is not enormously significant.
      template <class Lhs, class Rhs>
      bool compare_dispatch(dart_tag, dart_tag, Lhs const& lhs, Rhs const& rhs) {
        return detail::compare_impl<dart_tag>::compare(lhs, rhs);
      }
      template <class Lhs, class Rhs>
      bool compare_dispatch(dart_tag, wrapper_tag, Lhs const& lhs, Rhs const& rhs) {
        return detail::compare_impl<wrapper_tag>::compare(lhs, rhs);
      }
      template <class Lhs, class Rhs>
      bool compare_dispatch(wrapper_tag, dart_tag, Lhs const& lhs, Rhs const& rhs) {
        return detail::compare_impl<dart_tag>::compare(rhs, lhs.dynamic());
      }
      template <class Lhs, class Rhs>
      bool compare_dispatch(wrapper_tag, wrapper_tag, Lhs const& lhs, Rhs const& rhs) {
        return detail::compare_impl<wrapper_tag>::compare(lhs.dynamic(), rhs);
      }

      // These are the actual important calls, and argument order is significant as
      // all of the compare_impl functions expect Lhs to be a dart type.
      // The purpose of this whole run-around is so that the user can pass arguments
      // into dart::convert::compare in either order, and pass either a dart packet type,
      // or a wrapper type, without issue.
      template <class Free, class Lhs, class Rhs>
      bool compare_dispatch(dart_tag, Free, Lhs const& lhs, Rhs const& rhs) {
        return detail::compare_impl<Free>::compare(lhs, rhs);
      }
      template <class Free, class Lhs, class Rhs>
      bool compare_dispatch(wrapper_tag, Free, Lhs const& lhs, Rhs const& rhs) {
        return detail::compare_impl<Free>::compare(lhs.dynamic(), rhs);
      }
      template <class Free, class Lhs, class Rhs>
      bool compare_dispatch(Free, dart_tag, Lhs const& lhs, Rhs const& rhs) {
        return detail::compare_impl<Free>::compare(rhs, lhs);
      }
      template <class Free, class Lhs, class Rhs>
      bool compare_dispatch(Free, wrapper_tag, Lhs const& lhs, Rhs const& rhs) {
        return detail::compare_impl<Free>::compare(rhs.dynamic(), lhs);
      }

      // These overloads have to exist to avoid overload ambiguity.
      // Argument order is not enormously significant.
      template <class To, class From>
      decltype(auto) cast_dispatch(dart_tag, dart_tag, From&& val) {
        return detail::incoming_caster<dart_tag>::template cast<To>(std::forward<From>(val));
      }
      template <class To, class From>
      decltype(auto) cast_dispatch(dart_tag, wrapper_tag, From&& val) {
        return To {detail::incoming_caster<dart_tag>::template cast<typename To::value_type>(std::forward<From>(val))};
      }
      template <class To, class From>
      decltype(auto) cast_dispatch(wrapper_tag, dart_tag, From&& val) {
        return detail::incoming_caster<wrapper_tag>::template cast<To>(std::forward<From>(val));
      }
      template <class To, class From>
      decltype(auto) cast_dispatch(wrapper_tag, wrapper_tag, From&& val) {
        return To {detail::incoming_caster<wrapper_tag>::template cast<typename To::value_type>(std::forward<From>(val))};
      }

      template <class To, class From, class Free>
      decltype(auto) cast_dispatch(dart_tag, Free, From&& val) {
        return detail::outgoing_caster<Free>::template cast<To>(std::forward<From>(val));
      }
      template <class To, class From, class Free>
      decltype(auto) cast_dispatch(wrapper_tag, Free, From&& val) {
        return detail::outgoing_caster<Free>::template cast<To>(std::forward<From>(val).dynamic());
      }
      template <class To, class From, class Free>
      decltype(auto) cast_dispatch(Free, dart_tag, From&& val) {
        return detail::incoming_caster<Free>::template cast<To>(std::forward<From>(val));
      }
      template <class To, class From, class Free>
      decltype(auto) cast_dispatch(Free, wrapper_tag, From&& val) {
        return To {detail::incoming_caster<Free>::template cast<typename To::value_type>(std::forward<From>(val))};
      }

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
    template <class From, class To>
    struct is_castable : 
      meta::conjunction<
        meta::negation<
          std::is_reference<To>
        >,
        detail::is_castable_dispatch<
          detail::normalize_t<From>,
          detail::normalize_t<To>,
          From,
          To
        >
      >
    {};

    /**
     *  @brief
     *  Meta-function calculates whether a call to dart::convert::compare will be well-formed
     *  for a particular T and Dart type. Arguments can be passed in either order, at least one
     *  must be known to be a Dart type.
     *
     *  @details
     *  The expression:
     *  ```
     *  static_assert(dart::convert::are_comparable<T, dart::packet>::value);
     *  ```
     *  Will compile successfully for any T that is either a builtin machine type,
     *  is an STL container of builtin machine types, is a user type for which
     *  a comparison has been defined using the comparison API, or is an STL container
     *  of such a user type.
     */
    template <class Lhs, class Rhs>
    struct are_comparable :
      meta::conjunction<
        meta::negation<
          std::is_reference<Lhs>
        >,
        meta::negation<
          std::is_reference<Rhs>
        >,
        detail::are_comparable_dispatch<
          detail::are_comparable_impl,
          detail::normalize_t<Lhs>,
          detail::normalize_t<Rhs>,
          Lhs,
          Rhs
        >
      >
    {};

    /**
     *  @brief
     *  Meta-function calculates whether a call to dart::convert::compare will be well-formed
     *  and is guaranteed not to throw exceptions for a particular T and Dart type.
     *  Arguments can be passed in either order, at least one must be known to be a Dart type.
     *
     *  @details
     *  The expression:
     *  ```
     *  static_assert(dart::convert::are_nothrow_comparable<T, dart::packet>::value);
     *  ```
     *  Will compile successfully for any T that is either a builtin machine type,
     *  is an STL container of builtin machine types, is a user type for which
     *  a comparison has been defined using the comparison API and is marked noexcept,
     *  or is an STL container of such a user type.
     */
    template <class Lhs, class Rhs>
    struct are_nothrow_comparable :
      meta::conjunction<
        meta::negation<
          std::is_reference<Lhs>
        >,
        meta::negation<
          std::is_reference<Rhs>
        >,
        detail::are_comparable_dispatch<
          detail::are_nothrow_comparable_impl,
          detail::normalize_t<Lhs>,
          detail::normalize_t<Rhs>,
          Lhs,
          Rhs
        >
      >
    {};

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
     *    struct conversion_traits<my_string> {
     *      template <class Packet>
     *      Packet to_dart(my_string const& s) {
     *        return Packet::make_string(s.str);
     *      }
     *    };
     *  }
     *  ```
     */
    template <class To = dart::basic_packet<std::shared_ptr>, class From>
    decltype(auto) cast(From&& val) {
      using to_cat = detail::normalize_t<To>;
      using from_cat = detail::normalize_t<From>;
      static_assert(
        !std::is_reference<To>::value,
        "Cannot cast to a reference type"
      );
      return detail::cast_dispatch<To>(from_cat {}, to_cat {}, std::forward<From>(val));
    }

    /**
     *  @brief
     *  Function can be used to compare a Dart type against any registered type.
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
     *  dart::string str {"hello"};
     *  my_string mystr {"hello"};
     *  assert(str == mystr);
     *  ```
     *  You can accomplish this by specializing the struct `dart::convert::to_dart`
     *  with your custom type and implementing a compare function like so:
     *  ```
     *  namespace dart::convert {
     *    template <>
     *    struct conversion_traits<my_string> {
     *      template <class Packet>
     *      bool compare(Packet const& pkt, my_string const& s) {
     *        return pkt.strv() == s.str;
     *      }
     *    };
     *  }
     *  ```
     */
    template <class Lhs, class Rhs>
    bool compare(Lhs const& lhs, Rhs const& rhs) noexcept(are_nothrow_comparable<Lhs, Rhs>::value) {
      return detail::compare_dispatch(detail::normalize_t<Lhs> {}, detail::normalize_t<Rhs> {}, lhs, rhs);
    }

    // Converts a sequence of arguments convertible to Dart types
    // into a gsl::span of Dart types backed by stack-based storage.
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

  }

}

#endif
