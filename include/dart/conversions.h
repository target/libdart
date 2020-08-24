#ifndef DART_CONVERSIONS_H
#define DART_CONVERSIONS_H

/*----- System Includes -----*/

#include <set>
#include <map>
#include <list>
#include <ctime>
#include <deque>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <forward_list>
#include <unordered_set>
#include <unordered_map>

/*----- Local Includes -----*/

#include "conversion_traits.h"

/*----- Type Declarations -----*/

namespace dart {

  namespace convert {

    namespace detail {
      // This is fairly barbaric, but it lets us hack std::is_permutation
      // to work when the two iterators don't have the same value type.
      struct multimap_comparator {
        template <class T>
        struct is_pair : std::false_type {};
        template <class First, class Second>
        struct is_pair<std::pair<First, Second>> : std::true_type {};

        template <class T, class =
          std::enable_if_t<
            !is_pair<T>::value
          >
        >
        bool operator ()(T const& lhs, T const& rhs) const {
          return lhs == rhs;
        }
        template <class First, class Second, class T, class =
          std::enable_if_t<
            !is_pair<T>::value
          >
        >
        bool operator ()(std::pair<First, Second> const& lhs, T const& rhs) const {
          return lhs.second == rhs;
        }
        template <class First, class Second, class T, class =
          std::enable_if_t<
            !is_pair<T>::value
          >
        >
        bool operator ()(T const& lhs, std::pair<First, Second> const& rhs) const {
          return lhs == rhs.second;
        }
        template <class First, class Second>
        bool operator ()(std::pair<First, Second> const& lhs, std::pair<First, Second> const& rhs) const {
          return lhs.second == rhs.second;
        }
      };

      template <class Packet, class Map, class Callback>
      bool generic_map_compare_impl(Packet const& pkt, Map const& map, Callback&& cb) {
        // Lookup into the packet will be much faster if it's finalized,
        // and the same if not, so iterate over the map.
        typename Packet::view v = pkt;
        for (auto& pair : map) {
          auto val = cb(v, pair.first);
          if (val != pair.second) return false;
        }
        return true;
      }

      // General case of comparing a Dart type against a std map.
      template <class Packet, class Map>
      bool generic_map_compare(Packet const& pkt, Map const& map, std::true_type) {
        return generic_map_compare_impl(pkt, map, [] (auto& v, auto& k) { return v[k]; });
      }

      template <class Packet, class Map>
      bool generic_map_compare(Packet const& pkt, Map const& map, std::false_type) {
        // This is truly unfortunate.
        // The user gave us a map implementation with
        // a key type that IS convertible into a Dart type,
        // but which is NOT convertible into std::string_view
        // which means our only recourse is to create a temporary
        // packet for each key with which to perform the lookup.
        return generic_map_compare_impl(pkt, map, [] (auto& v, auto& k) { return v[convert::cast(k)]; });
      }

      template <class Packet, class Map, class Extract, class Next>
      bool generic_multimap_compare_impl(Packet const& pkt, Map const& map, Extract&& extract, Next&& next) {
        auto end = map.end();
        auto it = map.begin();
        typename Packet::view v = pkt;
        while (it != end) {
          // Find the end iterator for this key
          auto edge = next(it, end);
          
          // Get the Dart equivalent.
          auto val = extract(v, it->first);

          // Attempt to short-circuit
          if (!val.is_array()) return false;
          else if (static_cast<ssize_t>(val.size()) != std::distance(it, edge)) return false;

          // Perform the comparison.
          auto check = std::is_permutation(it, edge, val.begin(), multimap_comparator {});
          if (!check) return false;

          // Skip past all records with this key.
          it = edge;
        }
        return true;
      }

      template <class Packet, class Key, class Value, class Comp, class Alloc>
      bool generic_multimap_compare(Packet const& pkt,
          std::multimap<Key, Value, Comp, Alloc> const& map, std::true_type) {
        auto extract = [] (auto& v, auto& k) {
          return v[k];
        };
        auto next = [&] (auto& it, auto& end) {
          return std::upper_bound(it, end, *it, map.value_comp());
        };
        return generic_multimap_compare_impl(pkt, map, extract, next);
      }

      template <class Packet, class Key, class Value, class Comp, class Alloc>
      bool generic_multimap_compare(Packet const& pkt,
          std::multimap<Key, Value, Comp, Alloc> const& map, std::false_type) {
        auto extract = [] (auto& v, auto& k) {
          return v[convert::cast(k)];
        };
        auto next = [&] (auto& it, auto& end) {
          return std::upper_bound(it, end, *it, map.value_comp());
        };
        return generic_multimap_compare_impl(pkt, map, extract, next);
      }

      template <class Packet, class Key, class Value, class Hash, class Equal, class Alloc>
      bool generic_multimap_compare(Packet const& pkt,
          std::unordered_multimap<Key, Value, Hash, Equal, Alloc> const& map, std::true_type) {
        auto extract = [] (auto& v, auto& k) {
          return v[k];
        };
        auto next = [] (auto& it, auto& end) {
          return std::find_if(it, end, [&] (auto& e) { return e.first != it->first; });
        };
        return generic_multimap_compare_impl(pkt, map, extract, next);
      }

      template <class Packet, class Key, class Value, class Hash, class Equal, class Alloc>
      bool generic_multimap_compare(Packet const& pkt,
          std::unordered_multimap<Key, Value, Hash, Equal, Alloc> const& map, std::false_type) {
        auto extract = [] (auto& v, auto& k) {
          return v[convert::cast(k)];
        };
        auto next = [] (auto& it, auto& end) {
          return std::find_if(it, end, [&] (auto& e) { return e.first != it->first; });
        };
        return generic_multimap_compare_impl(pkt, map, extract, next);
      }

    }

    // Specialization for interoperability with gsl::span
#ifdef DART_USING_GSL_LITE
    template <class T>
    struct conversion_traits<gsl::span<T>> {
#else
    template <class T, std::size_t extent>
    struct conversion_traits<gsl::span<T, extent>> {
#endif
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      Packet to_dart(gsl::span<T const> span) {
        auto pkt = Packet::make_array();
        pkt.reserve(span.size());
        for (auto& val : span) pkt.push_back(val);
        return pkt;
      }

      // Equality
      // Span is taken by const& here because for whatever reason
      // the gsl-lite span copy/move constructor isn't marked
      // noexcept on specific versions of gcc, which confuses our
      // noexcept calculation logic
      template <class Packet, class =
        std::enable_if_t<
          convert::are_comparable<T, Packet>::value
        >
      >
      bool compare(Packet const& pkt, gsl::span<T const> const& span)
        noexcept(convert::are_nothrow_comparable<T, Packet>::value)
      {
        if (!pkt.is_array()) return false;
        else if (pkt.size() != static_cast<size_t>(span.size())) return false;

        auto it = span.begin();
        typename Packet::view v = pkt;
        for (auto val : v) {
          if (val != *it) return false;
          ++it;
        }
        return true;
      }
    };

    // Specialization for interoperability with std::vector
    template <class T, class Alloc>
    struct conversion_traits<std::vector<T, Alloc>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      Packet to_dart(std::vector<T, Alloc> const& vec) {
        auto pkt = Packet::make_array();
        pkt.reserve(vec.size());
        for (auto& val : vec) pkt.push_back(val);
        return pkt;
      }
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Packet, T>::value
        >
      >
      std::vector<T, Alloc> from_dart(Packet const& pkt) {
        // Check to ensure we have an array.
        if (!pkt.is_array()) {
          detail::report_type_mismatch(dart::detail::type::array, pkt.get_type());
        }

        // typename Packet::view is idempotent
        // to anyone who understands why this is necessary and useful,
        // you're welcome
        typename Packet::view v = pkt;
        std::vector<T, Alloc> vec;
        vec.reserve(pkt.size());
        for (auto val : v) {
          vec.emplace_back(convert::cast<T>(std::move(val)));
        }
        return vec;
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      Packet to_dart(std::vector<T, Alloc>&& vec) {
        auto pkt = Packet::make_array();
        pkt.reserve(vec.size());
        for (auto& val : vec) pkt.push_back(std::move(val));
        return pkt;
      }

      // Equality
      template <class Packet, class =
        std::enable_if_t<
          convert::are_comparable<T, Packet>::value
        >
      >
      bool compare(Packet const& pkt, std::vector<T, Alloc> const& vec)
        noexcept(convert::are_nothrow_comparable<T, Packet>::value)
      {
        // Try to short-circuit.
        if (!pkt.is_array()) return false;
        else if (pkt.size() != vec.size()) return false;

        // Iterate and check.
        auto it = vec.cbegin();
        typename Packet::view v = pkt;
        for (auto val : v) {
          if (val != *it) return false;
          ++it;
        }
        return true;
      }
    };

    // Specialization for interoperability with std::array
    template <class T, size_t len>
    struct conversion_traits<std::array<T, len>> {
      // All this to avoid imposing default-constructibility
      template <class... Elems, class It>
      std::array<T, len> build_array_impl(It&, Elems&&... elems) {
        return std::array<T, len> {{convert::cast<T>(std::forward<Elems>(elems))...}};
      }

      template <size_t, size_t... idxs, class... Elems, class It>
      std::array<T, len> build_array_impl(It& it, Elems&&... elems) {
        auto curr = it++;
        return build_array_impl<idxs...>(it, *curr, std::forward<Elems>(elems)...);
      }

      template <class It, size_t... idxs>
      std::array<T, len> build_array(It&& it, std::index_sequence<idxs...>) {
        auto curr = it++;
        return build_array_impl<idxs...>(it, *curr);
      }

      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      Packet to_dart(std::array<T, len> const& arr) {
        auto pkt = Packet::make_array();
        pkt.reserve(len);
        for (auto& val : arr) pkt.push_back(val);
        return pkt;
      }
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Packet, T>::value
        >
      >
      std::array<T, len> from_dart(Packet const& pkt) {
        // Check to make sure we have an array of the proper length
        if (!pkt.is_array()) {
          detail::report_type_mismatch(dart::detail::type::array, pkt.get_type());
        } else if (pkt.size() != len) {
          throw type_error("Encountered array of unexpected length during serialization");
        }
        return build_array(pkt.rbegin(), std::make_index_sequence<len - 1> {});
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      Packet to_dart(std::array<T, len>&& arr) {
        auto pkt = Packet::make_array();
        pkt.reserve(len);
        for (auto& val : arr) pkt.push_back(std::move(val));
        return pkt;
      }

      // Equality
      template <class Packet, class =
        std::enable_if_t<
          convert::are_comparable<T, Packet>::value
        >
      >
      bool compare(Packet const& pkt, std::array<T, len> const& arr)
        noexcept(convert::are_nothrow_comparable<T, Packet>::value)
      {
        // Try to short-circuit.
        if (!pkt.is_array()) return false;
        else if (pkt.size() != len) return false;

        // Iterate and check.
        auto it = arr.cbegin();
        typename Packet::view v = pkt;
        for (auto val : v) {
          if (val != *it) return false;
          ++it;
        }
        return true;
      }
    };

    // Specialization for interoperability with std::deque
    template <class T, class Alloc>
    struct conversion_traits<std::deque<T, Alloc>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      Packet to_dart(std::deque<T, Alloc> const& deq) {
        auto pkt = Packet::make_array();
        pkt.reserve(deq.size());
        for (auto& val : deq) pkt.push_back(val);
        return pkt;
      }
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Packet, T>::value
        >
      >
      std::deque<T, Alloc> from_dart(Packet const& pkt) {
        // Check to ensure we have an array.
        if (!pkt.is_array()) {
          detail::report_type_mismatch(dart::detail::type::array, pkt.get_type());
        }

        // typename Packet::view is idempotent
        // to anyone who understands why this is necessary and useful,
        // you're welcome
        typename Packet::view v = pkt;
        std::deque<T, Alloc> deq;
        for (auto val : v) {
          deq.emplace_back(convert::cast<T>(std::move(val)));
        }
        return deq;
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      Packet to_dart(std::deque<T, Alloc>&& deq) {
        auto pkt = Packet::make_array();
        pkt.reserve(deq.size());
        for (auto& val : deq) pkt.push_back(std::move(val));
        return pkt;
      }

      // Equality
      template <class Packet, class =
        std::enable_if_t<
          convert::are_comparable<T, Packet>::value
        >
      >
      bool compare(Packet const& pkt, std::deque<T, Alloc> const& deq)
        noexcept(convert::are_nothrow_comparable<T, Packet>::value)
      {
        // Try to short-circuit.
        if (!pkt.is_array()) return false;
        else if (pkt.size() != deq.size()) return false;

        // Iterate and check.
        auto it = deq.cbegin();
        typename Packet::view v = pkt;
        for (auto val : v) {
          if (val != *it) return false;
          ++it;
        }
        return true;
      }
    };

    // Specialization for interoperability with std::list
    template <class T, class Alloc>
    struct conversion_traits<std::list<T, Alloc>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      Packet to_dart(std::list<T, Alloc> const& lst) {
        auto pkt = Packet::make_array();
        pkt.reserve(lst.size());
        for (auto& val : lst) pkt.push_back(val);
        return pkt;
      }
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Packet, T>::value
        >
      >
      std::list<T, Alloc> from_dart(Packet const& pkt) {
        // Check to ensure we have an array.
        if (!pkt.is_array()) {
          detail::report_type_mismatch(dart::detail::type::array, pkt.get_type());
        }

        // typename Packet::view is idempotent
        // to anyone who understands why this is necessary and useful,
        // you're welcome
        typename Packet::view v = pkt;
        std::list<T, Alloc> lst;
        for (auto val : v) {
          lst.emplace_back(convert::cast<T>(std::move(val)));
        }
        return lst;
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      Packet to_dart(std::list<T, Alloc>&& lst) {
        auto pkt = Packet::make_array();
        pkt.reserve(lst.size());
        for (auto& val : lst) pkt.push_back(std::move(val));
        return pkt;
      }

      // Equality
      template <class Packet, class =
        std::enable_if_t<
          convert::are_comparable<T, Packet>::value
        >
      >
      bool compare(Packet const& pkt, std::list<T, Alloc> const& lst)
        noexcept(convert::are_nothrow_comparable<T, Packet>::value)
      {
        // Try to short-circuit.
        if (!pkt.is_array()) return false;
        else if (pkt.size() != lst.size()) return false;

        // Iterate and check.
        auto it = lst.cbegin();
        typename Packet::view v = pkt;
        for (auto val : v) {
          if (val != *it) return false;
          ++it;
        }
        return true;
      }
    };

    // Specialization for interoperability with std::forward_list
    template <class T, class Alloc>
    struct conversion_traits<std::forward_list<T, Alloc>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      Packet to_dart(std::forward_list<T, Alloc> const& lst) {
        auto pkt = Packet::make_array();
        for (auto& val : lst) pkt.push_back(val);
        return pkt;
      }
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Packet, T>::value
        >
      >
      std::forward_list<T, Alloc> from_dart(Packet const& pkt) {
        // Check to ensure we have an array.
        if (!pkt.is_array()) {
          detail::report_type_mismatch(dart::detail::type::array, pkt.get_type());
        }

        // typename Packet::view is idempotent
        // to anyone who understands why this is necessary and useful,
        // you're welcome
        typename Packet::view v = pkt;
        std::forward_list<T, Alloc> flst;
        for (auto it = pkt.rbegin(); it != pkt.rend(); ++it) {
          flst.emplace_front(convert::cast<T>(*it));
        }
        return flst;
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      Packet to_dart(std::forward_list<T, Alloc>&& lst) {
        auto pkt = Packet::make_array();
        for (auto& val : lst) pkt.push_back(std::move(val));
        return pkt;
      }

      // Equality
      // Template parameter U is unfortunately necessary here to delay
      // instantiation of the std::enable_if_t to SFINAE properly
      template <class Packet, class =
        std::enable_if_t<
          convert::are_comparable<T, Packet>::value
        >
      >
      bool compare(Packet const& pkt, std::forward_list<T, Alloc> const& lst)
        noexcept(convert::are_nothrow_comparable<T, Packet>::value)
      {
        // Try to short-circuit.
        if (!pkt.is_array()) return false;

        // Iterate and check.
        auto it = lst.cbegin();
        typename Packet::view v = pkt;
        for (auto val : v) {
          if (val != *it) return false;
          ++it;
        }
        return it == lst.cend();
      }
    };

    // Specialization for interoperability with std::map
    template <class Key, class Value, class Comp, class Alloc>
    struct conversion_traits<std::map<Key, Value, Comp, Alloc>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
          &&
          convert::is_castable<Value, Packet>::value
        >
      >
      Packet to_dart(std::map<Key, Value, Comp, Alloc> const& map) {
        auto obj = Packet::make_object();
        for (auto& pair : map) obj.add_field(pair.first, pair.second);
        return obj;
      }
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Packet, Key>::value
          &&
          convert::is_castable<Packet, Value>::value
        >
      >
      std::map<Key, Value, Comp, Alloc> from_dart(Packet const& pkt) {
        // Check to ensure we have an object
        if (!pkt.is_object()) {
          detail::report_type_mismatch(dart::detail::type::object, pkt.get_type());
        }

        typename Packet::view vw = pkt;
        typename Packet::view::iterator k, v;
        std::map<Key, Value, Comp, Alloc> map;
        std::tie(k, v) = vw.kvbegin();
        while (v != vw.end()) {
          map.emplace(convert::cast<Key>(*k++), convert::cast<Value>(*v++));
        }
        return map;
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
          &&
          convert::is_castable<Value, Packet>::value
        >
      >
      Packet to_dart(std::map<Key, Value, Comp, Alloc>&& map) {
        auto obj = Packet::make_object();
        for (auto& pair : map) obj.add_field(std::move(pair).first, std::move(pair).second);
        return obj;
      }

      // Equality
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
          &&
          convert::are_comparable<Key, Packet>::value
          &&
          convert::are_comparable<Value, Packet>::value
        >
      >
      bool compare(Packet const& pkt, std::map<Key, Value, Comp, Alloc> const& map)
        noexcept(
          convert::are_nothrow_comparable<Key, Packet>::value
          &&
          convert::are_nothrow_comparable<Value, Packet>::value
        )
      {
        // Try to short-circuit.
        if (!pkt.is_object()) return false;
        else if (pkt.size() != map.size()) return false;

        // We have to indirect through an implementation function here to handle an edge
        // case where the user passed is a map with a key type that IS convertible to a
        // Dart type, but which is NOT convertible to shim::string_view.
        return detail::generic_map_compare(pkt, map, std::is_convertible<Key, shim::string_view> {});
      }
    };

    // Specialization for interoperability with std::unordered_map
    template <class Key, class Value, class Hash, class Equal, class Alloc>
    struct conversion_traits<std::unordered_map<Key, Value, Hash, Equal, Alloc>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
          &&
          convert::is_castable<Value, Packet>::value
        >
      >
      Packet to_dart(std::unordered_map<Key, Value, Hash, Equal, Alloc> const& map) {
        auto obj = Packet::make_object();
        for (auto& pair : map) obj.add_field(pair.first, pair.second);
        return obj;
      }
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Packet, Key>::value
          &&
          convert::is_castable<Packet, Value>::value
        >
      >
      std::unordered_map<Key, Value, Hash, Equal, Alloc> from_dart(Packet const& pkt) {
        // Check to ensure we have an object
        if (!pkt.is_object()) {
          detail::report_type_mismatch(dart::detail::type::object, pkt.get_type());
        }

        typename Packet::view vw = pkt;
        typename Packet::view::iterator k, v;
        std::unordered_map<Key, Value, Hash, Equal, Alloc> map;
        std::tie(k, v) = vw.kvbegin();
        while (v != vw.end()) {
          map.emplace(convert::cast<Key>(*k++), convert::cast<Value>(*v++));
        }
        return map;
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
          &&
          convert::is_castable<Value, Packet>::value
        >
      >
      Packet to_dart(std::unordered_map<Key, Value, Hash, Equal, Alloc>&& map) {
        auto obj = Packet::make_object();
        for (auto& pair : map) obj.add_field(std::move(pair).first, std::move(pair).second);
        return obj;
      }

      // Equality
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
          &&
          convert::are_comparable<Key, Packet>::value
          &&
          convert::are_comparable<Value, Packet>::value
        >
      >
      bool compare(Packet const& pkt, std::unordered_map<Key, Value, Hash, Equal, Alloc> const& map)
        noexcept(
          convert::are_nothrow_comparable<Key, Packet>::value
          &&
          convert::are_nothrow_comparable<Value, Packet>::value
        )
      {
        // Try to short-circuit.
        if (!pkt.is_object()) return false;
        else if (pkt.size() != map.size()) return false;

        // We have to indirect through an implementation function here to handle an edge
        // case where the user passed is a map with a key type that IS convertible to a
        // Dart type, but which is NOT convertible to shim::string_view.
        return detail::generic_map_compare(pkt, map, std::is_convertible<Key, shim::string_view> {});
      }
    };

    // Specialization for interoperability with std::map
    template <class Key, class Value, class Comp, class Alloc>
    struct conversion_traits<std::multimap<Key, Value, Comp, Alloc>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
          &&
          convert::is_castable<Value, Packet>::value
        >
      >
      Packet to_dart(std::multimap<Key, Value, Comp, Alloc> const& map) {
        std::map<Key, typename Packet::array, Comp> accum;
        for (auto& pair : map) accum[pair.first].push_back(pair.second);
        return convert::cast<Packet>(std::move(accum));
      }
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Packet, Key>::value
          &&
          convert::is_castable<Packet, Value>::value
        >
      >
      std::multimap<Key, Value, Comp, Alloc> from_dart(Packet const& pkt) {
        // Check to ensure we have an object
        if (!pkt.is_object()) {
          detail::report_type_mismatch(dart::detail::type::object, pkt.get_type());
        }

        typename Packet::view vw = pkt;
        typename Packet::view::iterator k, v;
        std::multimap<Key, Value, Comp, Alloc> map;
        std::tie(k, v) = vw.kvbegin();
        while (v != vw.end()) {
          auto key = convert::cast<Key>(*k);
          if (v->is_array()) {
            for (auto val : *v) map.emplace(key, convert::cast<Value>(std::move(val)));
          } else {
            map.emplace(std::move(key), convert::cast<Value>(*v));
          }
          ++k, ++v;
        }
        return map;
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
          &&
          convert::is_castable<Value, Packet>::value
        >
      >
      Packet to_dart(std::multimap<Key, Value, Comp, Alloc>&& map) {
        std::map<Key, typename Packet::array, Comp> accum;
        for (auto& pair : map) accum[pair.first].push_back(std::move(pair.second));
        return convert::cast<Packet>(std::move(accum));
      }

      // Equality
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
          &&
          convert::are_comparable<Key, Packet>::value
          &&
          convert::are_comparable<Value, Packet>::value
        >
      >
      bool compare(Packet const& pkt, std::multimap<Key, Value, Comp, Alloc> const& map)
        noexcept(
          convert::are_nothrow_comparable<Key, Packet>::value
          &&
          convert::are_nothrow_comparable<Value, Packet>::value
        )
      {
        // Try to short-circuit.
        if (!pkt.is_object()) return false;
        return detail::generic_multimap_compare(pkt, map, std::is_convertible<Key, shim::string_view> {});
      }
    };

    // Specialization for interoperability with std::map
    template <class Key, class Value, class Hash, class Equal, class Alloc>
    struct conversion_traits<std::unordered_multimap<Key, Value, Hash, Equal, Alloc>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
          &&
          convert::is_castable<Value, Packet>::value
        >
      >
      Packet to_dart(std::unordered_multimap<Key, Value, Hash, Equal, Alloc> const& map) {
        std::unordered_map<Key, typename Packet::array, Hash, Equal> accum;
        for (auto& pair : map) accum[pair.first].push_back(pair.second);
        return convert::cast<Packet>(std::move(accum));
      }
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Packet, Key>::value
          &&
          convert::is_castable<Packet, Value>::value
        >
      >
      std::unordered_multimap<Key, Value, Hash, Equal, Alloc> from_dart(Packet const& pkt) {
        // Check to ensure we have an object
        if (!pkt.is_object()) {
          detail::report_type_mismatch(dart::detail::type::object, pkt.get_type());
        }

        typename Packet::view vw = pkt;
        typename Packet::view::iterator k, v;
        std::unordered_multimap<Key, Value, Hash, Equal, Alloc> map;
        std::tie(k, v) = vw.kvbegin();
        while (v != vw.end()) {
          auto key = convert::cast<Key>(*k);
          if (v->is_array()) {
            for (auto val : *v) map.emplace(key, convert::cast<Value>(std::move(val)));
          } else {
            map.emplace(std::move(key), convert::cast<Value>(*v));
          }
          ++k, ++v;
        }
        return map;
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
          &&
          convert::is_castable<Value, Packet>::value
        >
      >
      Packet to_dart(std::unordered_multimap<Key, Value, Hash, Equal, Alloc>&& map) {
        std::unordered_map<Key, typename Packet::array, Hash, Equal> accum;
        for (auto& pair : map) accum[pair.first].push_back(std::move(pair.second));
        return convert::cast<Packet>(std::move(accum));
      }

      // Equality
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
          &&
          convert::are_comparable<Key, Packet>::value
          &&
          convert::are_comparable<Value, Packet>::value
        >
      >
      bool compare(Packet const& pkt, std::unordered_multimap<Key, Value, Hash, Equal, Alloc> const& map)
        noexcept(
          convert::are_nothrow_comparable<Key, Packet>::value
          &&
          convert::are_nothrow_comparable<Value, Packet>::value
        )
      {
        // Try to short-circuit.
        if (!pkt.is_object()) return false;
        return detail::generic_multimap_compare(pkt, map, std::is_convertible<Key, shim::string_view> {});
      }
    };

    template <class Key, class Compare, class Alloc>
    struct conversion_traits<std::set<Key, Compare, Alloc>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
        >
      >
      Packet to_dart(std::set<Key, Compare, Alloc> const& set) {
        auto pkt = Packet::make_array();
        pkt.reserve(set.size());
        for (auto& val : set) pkt.push_back(val);
        return pkt;
      }
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Packet, Key>::value
        >
      >
      std::set<Key, Compare, Alloc> from_dart(Packet const& pkt) {
        if (!pkt.is_array()) {
          detail::report_type_mismatch(dart::detail::type::array, pkt.get_type());
        }

        typename Packet::view v = pkt;
        std::set<Key, Compare, Alloc> set;
        for (auto val : v) {
          set.emplace(convert::cast<Key>(std::move(val)));
        }
        return set;
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
        >
      >
      Packet to_dart(std::set<Key, Compare, Alloc>&& set) {
        auto pkt = Packet::make_array();
        pkt.reserve(set.size());
        for (auto& val : set) pkt.push_back(std::move(val));
        return pkt;
      }

      // Equality
      // Template parameter K is unfortunately necessary here to delay
      // instantiation of the std::enable_if_t to SFINAE properly
      template <class Packet, class =
        std::enable_if_t<
          convert::are_comparable<Key, Packet>::value
        >
      >
      bool compare(Packet const& pkt, std::set<Key, Compare, Alloc> const& set)
        noexcept(convert::are_nothrow_comparable<Key, Packet>::value)
      {
        // Try to short-circuit
        if (!pkt.is_array()) return false;
        else if (pkt.size() != set.size()) return false;

        // Unfortunate, but we have no idea how the given set's comparator
        // behaves in comparison to the Dart comparator, and we're simulating
        // sets as arrays, so we don't have any efficient way to look up set
        // members in constant time.
        // Thus, this currently hands off to std::is_permutation.
        return std::is_permutation(pkt.begin(), pkt.end(), set.begin());
      }
    };

    template <class Key, class Hash, class KeyEq, class Alloc>
    struct conversion_traits<std::unordered_set<Key, Hash, KeyEq, Alloc>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
        >
      >
      Packet to_dart(std::unordered_set<Key, Hash, KeyEq, Alloc> const& set) {
        auto pkt = Packet::make_array();
        pkt.reserve(set.size());
        for (auto& val : set) pkt.push_back(val);
        return pkt;
      }
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Packet, Key>::value
        >
      >
      std::unordered_set<Key, Hash, KeyEq, Alloc> from_dart(Packet const& pkt) {
        if (!pkt.is_array()) {
          detail::report_type_mismatch(dart::detail::type::array, pkt.get_type());
        }

        typename Packet::view v = pkt;
        std::unordered_set<Key, Hash, KeyEq, Alloc> set;
        for (auto val : v) {
          set.emplace(convert::cast<Key>(std::move(val)));
        }
        return set;
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
        >
      >
      Packet to_dart(std::unordered_set<Key, Hash, KeyEq, Alloc>&& set) {
        auto pkt = Packet::make_array();
        pkt.reserve(set.size());
        for (auto& val : set) pkt.push_back(std::move(val));
        return pkt;
      }

      // Equality
      // Template parameter K is unfortunately necessary here to delay
      // instantiation of the std::enable_if_t to SFINAE properly
      template <class Packet, class =
        std::enable_if_t<
          convert::are_comparable<Key, Packet>::value
        >
      >
      bool compare(Packet const& pkt, std::unordered_set<Key, Hash, KeyEq, Alloc> const& set)
        noexcept(convert::are_nothrow_comparable<Key, Packet>::value)
      {
        // Try to short-circuit
        if (!pkt.is_array()) return false;
        else if (pkt.size() != set.size()) return false;

        // Unfortunate, but we have no idea how the given set's comparator
        // behaves in comparison to the Dart comparator, and we're simulating
        // sets as arrays, so we don't have any efficient way to look up set
        // members in constant time.
        // Thus, this currently hands off to std::is_permutation.
        return std::is_permutation(pkt.begin(), pkt.end(), set.begin());
      }
    };

    template <class Key, class Compare, class Alloc>
    struct conversion_traits<std::multiset<Key, Compare, Alloc>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
        >
      >
      Packet to_dart(std::multiset<Key, Compare, Alloc> const& set) {
        auto pkt = Packet::make_array();
        pkt.reserve(set.size());
        for (auto& val : set) pkt.push_back(val);
        return pkt;
      }
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Packet, Key>::value
        >
      >
      std::multiset<Key, Compare, Alloc> from_dart(Packet const& pkt) {
        if (!pkt.is_array()) {
          detail::report_type_mismatch(dart::detail::type::array, pkt.get_type());
        }

        typename Packet::view v = pkt;
        std::multiset<Key, Compare, Alloc> set;
        for (auto val : v) {
          set.emplace(convert::cast<Key>(std::move(val)));
        }
        return set;
      }


      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
        >
      >
      Packet to_dart(std::multiset<Key, Compare, Alloc>&& set) {
        auto pkt = Packet::make_array();
        pkt.reserve(set.size());
        for (auto& val : set) pkt.push_back(std::move(val));
        return pkt;
      }

      // Equality
      // Template parameter K is unfortunately necessary here to delay
      // instantiation of the std::enable_if_t to SFINAE properly
      template <class Packet, class =
        std::enable_if_t<
          convert::are_comparable<Key, Packet>::value
        >
      >
      bool compare(Packet const& pkt, std::multiset<Key, Compare, Alloc> const& set)
        noexcept(convert::are_nothrow_comparable<Key, Packet>::value)
      {
        // Try to short-circuit
        if (!pkt.is_array()) return false;
        else if (pkt.size() != set.size()) return false;

        // FIXME: Need to benchmark this and add a case where it falls back
        // and constructs a temporary set to perform the comparison.
        // Unfortunate, but we have no idea how the given set's comparator
        // behaves in comparison to the Dart comparator, and we're simulating
        // sets as arrays, so we don't have any efficient way to look up set
        // members in constant time.
        // Thus, this currently hands off to std::is_permutation.
        return std::is_permutation(pkt.begin(), pkt.end(), set.begin());
      }
    };

    template <class Key, class Hash, class KeyEq, class Alloc>
    struct conversion_traits<std::unordered_multiset<Key, Hash, KeyEq, Alloc>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
        >
      >
      Packet to_dart(std::unordered_multiset<Key, Hash, KeyEq, Alloc> const& set) {
        auto pkt = Packet::make_array();
        pkt.reserve(set.size());
        for (auto& val : set) pkt.push_back(val);
        return pkt;
      }
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Packet, Key>::value
        >
      >
      std::unordered_multiset<Key, Hash, KeyEq, Alloc> from_dart(Packet const& pkt) {
        if (!pkt.is_array()) {
          detail::report_type_mismatch(dart::detail::type::array, pkt.get_type());
        }

        typename Packet::view v = pkt;
        std::unordered_multiset<Key, Hash, KeyEq, Alloc> set;
        for (auto val : v) {
          set.emplace(convert::cast<Key>(std::move(val)));
        }
        return set;
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
        >
      >
      Packet to_dart(std::unordered_multiset<Key, Hash, KeyEq, Alloc>&& set) {
        auto pkt = Packet::make_array();
        pkt.reserve(set.size());
        for (auto& val : set) pkt.push_back(std::move(val));
        return pkt;
      }

      // Equality
      // Template parameter K is unfortunately necessary here to delay
      // instantiation of the std::enable_if_t to SFINAE properly
      template <class Packet, class =
        std::enable_if_t<
          convert::are_comparable<Key, Packet>::value
        >
      >
      bool compare(Packet const& pkt, std::unordered_multiset<Key, Hash, KeyEq, Alloc> const& set)
        noexcept(convert::are_nothrow_comparable<Key, Packet>::value)
      {
        // Try to short-circuit
        if (!pkt.is_array()) return false;
        else if (pkt.size() != set.size()) return false;

        // Unfortunate, but we have no idea how the given set's comparator
        // behaves in comparison to the Dart comparator, and we're simulating
        // sets as arrays, so we don't have any efficient way to look up set
        // members in constant time.
        // Thus, this currently hands off to std::is_permutation.
        return std::is_permutation(pkt.begin(), pkt.end(), set.begin());
      }
    };

    // Specialization for interoperability with std::optional.
    template <class T>
    struct conversion_traits<shim::optional<T>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      Packet to_dart(shim::optional<T> const& opt) {
        if (opt) return convert::cast<Packet>(*opt);
        else return Packet::make_null();
      }
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Packet, T>::value
        >
      >
      shim::optional<T> from_dart(Packet const& pkt) {
        if (pkt.is_null()) return shim::nullopt;
        else return convert::cast<T>(pkt);
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      Packet to_dart(shim::optional<T>&& opt) {
        if (opt) return convert::cast<Packet>(std::move(*opt));
        else return Packet::make_null();
      }

      // Equality
      template <class Packet, class =
        std::enable_if_t<
          convert::are_comparable<T, Packet>::value
        >
      >
      bool compare(Packet const& pkt, shim::optional<T> const& opt)
        noexcept(convert::are_nothrow_comparable<T, Packet>::value)
      {
        if (opt) return pkt == *opt;
        else return pkt.is_null();
      }
    };

    // Specialization for interoperability with std::variant.
    template <class... Ts>
    struct conversion_traits<shim::variant<Ts...>> {
      template <class Packet>
      shim::variant<Ts...> build_variant(Packet const& pkt) {
        std::string errmsg = "Unable to convert type \"";
        errmsg += detail::type_to_string(pkt.get_type());
        errmsg += "\" during serialization after trying ";
        errmsg += std::to_string(sizeof...(Ts));
        errmsg += " different conversions";
        throw type_error(errmsg.c_str());
      }
      template <class Curr, class... Rest, class Packet>
      shim::variant<Ts...> build_variant(Packet const& pkt) {
        auto opt = [&] () -> shim::optional<Curr> {
          try {
            return convert::cast<Curr>(pkt);
          } catch (...) {
            return shim::nullopt;
          }
        }();
        if (opt) return std::move(*opt);
        else return build_variant<Rest...>(pkt);
      }

      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          meta::conjunction<
            convert::is_castable<Ts, Packet>...
          >::value
        >
      >
      Packet to_dart(shim::variant<Ts...> const& var) {
        return shim::visit([] (auto& val) { return convert::cast<Packet>(val); }, var);
      }
      template <class Packet, class =
        std::enable_if_t<
          meta::conjunction<
            convert::is_castable<Packet, Ts>...
          >::value
        >
      >
      shim::variant<Ts...> from_dart(Packet const& pkt) {
        typename Packet::view v = pkt;
        return build_variant<Ts...>(v);
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          meta::conjunction<
            convert::is_castable<Ts, Packet>...
          >::value
        >
      >
      Packet to_dart(shim::variant<Ts...>&& var) {
        return shim::visit([] (auto&& val) { return convert::cast<Packet>(std::move(val)); }, std::move(var));
      }

      // Equality
      template <class Packet, class =
        std::enable_if_t<
          meta::conjunction<
            convert::are_comparable<Ts, Packet>...
          >::value
        >
      >
      bool compare(Packet const& pkt, shim::variant<Ts...> const& var)
        noexcept(
          meta::conjunction<
            convert::are_nothrow_comparable<Ts, Packet>...
          >::value
        )
      {
        return shim::visit([&] (auto& val) { return val == pkt; }, var);
      }
    };

    // Specialization for interoperability with std::tuple.
    template <class... Ts>
    struct conversion_traits<std::tuple<Ts...>> {
      template <class Packet, class Tuple, size_t... idxs>
      Packet unpack_convert(Tuple&& tup, std::index_sequence<idxs...>) {
        return Packet::make_array(std::get<idxs>(std::forward<Tuple>(tup))...);
      }

      template <class... Elems, class It>
      std::tuple<Ts...> build_tuple(It&&, Elems&&... elems) {
        return std::tuple<Ts...> {convert::cast<Ts>(std::forward<Elems>(elems))...};
      }
      template <class Curr, class... Rest, class... Elems, class It>
      std::tuple<Ts...> build_tuple(It&& it, Elems&&... elems) {
        auto curr = it++;
        return build_tuple<Rest...>(it, *curr, std::forward<Elems>(elems)...);
      }

      template <class PackIt, class Tuple>
      bool unpack_compare_impl(PackIt&&, Tuple const&) {
        return true;
      }
      template <size_t idx, size_t... idxs, class PackIt, class Tuple>
      bool unpack_compare_impl(PackIt&& curr, Tuple const& tup) {
        return (*curr == std::get<idx>(tup)) && unpack_compare_impl<idxs...>(++curr, tup);
      }
      template <class Packet, class Tuple, size_t... idxs>
      bool unpack_compare(Packet const& pkt, Tuple const& tup, std::index_sequence<idxs...>) {
        return unpack_compare_impl<idxs...>(pkt.begin(), tup);
      }

      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          meta::conjunction<
            convert::is_castable<Ts, Packet>...
          >::value
        >
      >
      Packet to_dart(std::tuple<Ts...> const& tup) {
        return unpack_convert<Packet>(tup, std::index_sequence_for<Ts...> {});
      }
      template <class Packet, class =
        std::enable_if_t<
          meta::conjunction<
            convert::is_castable<Packet, Ts>...
          >::value
        >
      >
      std::tuple<Ts...> from_dart(Packet const& pkt) {
        if (!pkt.is_array()) {
          detail::report_type_mismatch(dart::detail::type::array, pkt.get_type());
        } else if (pkt.size() != sizeof...(Ts)) {
          throw type_error("Encountered array of unexpected length during serialization");
        }
        typename Packet::view v = pkt;
        return build_tuple<Ts...>(v.rbegin());
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          meta::conjunction<
            convert::is_castable<Ts, Packet>...
          >::value
        >
      >
      Packet to_dart(std::tuple<Ts...>&& tup) {
        return unpack_convert<Packet>(std::move(tup), std::index_sequence_for<Ts...> {});
      }

      // Equality
      template <class Packet, class =
        std::enable_if_t<
          meta::conjunction<
            convert::are_comparable<Ts, Packet>...
          >::value
        >
      >
      bool compare(Packet const& pkt, std::tuple<Ts...> const& tup)
        noexcept(
          meta::conjunction<
            convert::are_nothrow_comparable<Ts, Packet>...
          >::value
        )
      {
        if (!pkt.is_array()) return false;
        else if (pkt.size() != sizeof...(Ts)) return false;
        return unpack_compare(pkt, tup, std::index_sequence_for<Ts...> {});
      }
    };

    // Specialization for interoperability with std::pair.
    template <class First, class Second>
    struct conversion_traits<std::pair<First, Second>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<First, Packet>::value
          &&
          convert::is_castable<Second, Packet>::value
        >
      >
      Packet to_dart(std::pair<First, Second> const& pair) {
        return Packet::make_array(pair.first, pair.second);
      }
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Packet, First>::value
          &&
          convert::is_castable<Packet, Second>::value
        >
      >
      std::pair<First, Second> from_dart(Packet const& pkt) {
        if (!pkt.is_array()) {
          detail::report_type_mismatch(dart::detail::type::array, pkt.get_type());
        } else if (pkt.size() != 2U) {
          throw type_error("Encountered array of unexpected length during serialization");
        }
        typename Packet::view v = pkt;
        return std::pair<First, Second> {convert::cast<First>(v[0]), convert::cast<Second>(v[1])};
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<First, Packet>::value
          &&
          convert::is_castable<Second, Packet>::value
        >
      >
      Packet to_dart(std::pair<First, Second>&& pair) {
        return Packet::make_array(std::move(pair.first), std::move(pair.second));
      }

      // Equality
      // Template parameter F is unfortunately necessary here to delay
      // instantiation of the std::enable_if_t to SFINAE properly
      template <class Packet, class =
        std::enable_if_t<
          convert::are_comparable<First, Packet>::value
          &&
          convert::are_comparable<Second, Packet>::value
        >
      >
      bool compare(Packet const& pkt, std::pair<First, Second> const& pair)
        noexcept(
          convert::are_nothrow_comparable<First, Packet>::value
          &&
          convert::are_nothrow_comparable<Second, Packet>::value
        )
      {
        // Try to short-circuit
        if (!pkt.is_array()) return false;
        else if (pkt.size() != 2U) return false;

        // Check the values.
        typename Packet::view v = pkt;
        return v[0] == pair.first && v[1] == pair.second;
      }
    };

    template <class Duration>
    struct conversion_traits<std::chrono::time_point<std::chrono::system_clock, Duration>> {
      using sys_time_point = std::chrono::time_point<std::chrono::system_clock, Duration>;

      std::string convert(sys_time_point tp) {
        // Convert into the second worst type in the world
        auto const t = std::chrono::system_clock::to_time_t(tp);

        // Convert into the actual worst type in the world
        std::tm time;
        shim::gmtime(&t, &time);

        // Put this in a lambda so we can easily precalculate
        // how much space we'll need.
        auto mk_time = [time] (size_t n, char* buf) {
          return snprintf(buf, n,
              "%04d-%02d-%02dT%02d:%02d:%02d.000Z", time.tm_year + 1900,
              time.tm_mon + 1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
        };

        // Check how much we'll need, then do it.
        auto len = mk_time(0, nullptr);
        std::string date(len + 1, ' ');
        mk_time(date.size(), &date[0]);
        date.resize(len);
        return date;
      }

      sys_time_point convert(char const* str) {
        // It's the year 2020
        float s;
        int y, M, d, h, m;
        int ret = sscanf(str, "%d-%d-%dT%d:%d:%fZ", &y, &M, &d, &h, &m, &s);
        if (ret != 6) {
          throw type_error("Unable to parse date string during serialization");
        }

        // And I'm still parsing my dates
        std::tm time;
        time.tm_year = y - 1900;              // Year since 1900
        time.tm_mon = M - 1;                  // 0-11
        time.tm_mday = d;                     // 1-31
        time.tm_hour = h;                     // 0-23
        time.tm_min = m;                      // 0-59
        time.tm_sec = static_cast<int>(s);    // 0-59

        // With sscanf
        // Unbelievable
        auto tp = std::chrono::system_clock::from_time_t(shim::timegm(&time));
        return std::chrono::time_point_cast<Duration>(tp);
      }

      // Copy conversion
      template <class Packet>
      Packet to_dart(sys_time_point tp) {
        return Packet::make_string(convert(tp));
      }
      template <class Packet>
      sys_time_point from_dart(Packet const& pkt) {
        if (!pkt.is_str()) {
          detail::report_type_mismatch(dart::detail::type::string, pkt.get_type());
        }
        return convert(pkt.str());
      }

      // Equality
      template <class Packet>
      bool compare(Packet const& pkt, sys_time_point tp) {
        return pkt == convert(tp);
      }
    };

    // Bizzarely useful in some meta-programming situations.
    template <class T, T val>
    struct conversion_traits<std::integral_constant<T, val>> {
      // Conversion
      template <class Packet>
      Packet to_dart(std::integral_constant<T, val>) {
        return convert::cast<Packet>(val);
      }
      template <class Packet>
      std::integral_constant<T, val> from_dart(Packet const& pkt) {
        if (pkt == val) {
          return std::integral_constant<T, val> {};
        } else {
          throw type_error("Encountered integral constant of unexpected value during serialization");
        }
      }

      // Equality
      template <class Packet>
      bool compare(Packet const& pkt, std::integral_constant<T, val>) {
        return pkt == convert::cast<Packet>(val);
      }
    };

  }

}

#endif
