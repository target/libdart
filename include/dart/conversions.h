#ifndef DART_CONVERSIONS_H
#define DART_CONVERSIONS_H

/*----- System Includes -----*/

#include <set>
#include <map>
#include <list>
#include <deque>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <forward_list>
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
    template <class T, std::ptrdiff_t extent>
    struct to_dart<gsl::span<T, extent>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      Packet cast(gsl::span<T const> span) {
        auto pkt = Packet::make_array();
        pkt.reserve(span.size());
        for (auto& val : span) pkt.push_back(val);
        return pkt;
      }

      // Equality
      template <class Packet, class =
        std::enable_if_t<
          convert::are_comparable<T, Packet>::value
        >
      >
      bool compare(Packet const& pkt, gsl::span<T const> span)
        noexcept(convert::are_nothrow_comparable<T, Packet>::value)
      {
        if (!pkt.is_array()) return false;
        else if (pkt.size() != static_cast<size_t>(span.size())) return false;

        auto it = span.cbegin();
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
    struct to_dart<std::vector<T, Alloc>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      Packet cast(std::vector<T, Alloc> const& vec) {
        auto pkt = Packet::make_array();
        pkt.reserve(vec.size());
        for (auto& val : vec) pkt.push_back(val);
        return pkt;
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      Packet cast(std::vector<T, Alloc>&& vec) {
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
    struct to_dart<std::array<T, len>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      Packet cast(std::array<T, len> const& arr) {
        auto pkt = Packet::make_array();
        pkt.reserve(len);
        for (auto& val : arr) pkt.push_back(val);
        return pkt;
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      Packet cast(std::array<T, len>&& arr) {
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
    struct to_dart<std::deque<T, Alloc>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      Packet cast(std::deque<T, Alloc> const& deq) {
        auto pkt = Packet::make_array();
        pkt.reserve(deq.size());
        for (auto& val : deq) pkt.push_back(val);
        return pkt;
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      Packet cast(std::deque<T, Alloc>&& deq) {
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
    struct to_dart<std::list<T, Alloc>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      Packet cast(std::list<T, Alloc> const& lst) {
        auto pkt = Packet::make_array();
        pkt.reserve(lst.size());
        for (auto& val : lst) pkt.push_back(val);
        return pkt;
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      Packet cast(std::list<T, Alloc>&& lst) {
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
    struct to_dart<std::forward_list<T, Alloc>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      Packet cast(std::forward_list<T, Alloc> const& lst) {
        auto pkt = Packet::make_array();
        for (auto& val : lst) pkt.push_back(val);
        return pkt;
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      Packet cast(std::forward_list<T, Alloc>&& lst) {
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
    struct to_dart<std::map<Key, Value, Comp, Alloc>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
          &&
          convert::is_castable<Value, Packet>::value
        >
      >
      Packet cast(std::map<Key, Value, Comp, Alloc> const& map) {
        auto obj = Packet::make_object();
        for (auto& pair : map) obj.add_field(pair.first, pair.second);
        return obj;
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
          &&
          convert::is_castable<Value, Packet>::value
        >
      >
      Packet cast(std::map<Key, Value, Comp, Alloc>&& map) {
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
    struct to_dart<std::unordered_map<Key, Value, Hash, Equal, Alloc>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
          &&
          convert::is_castable<Value, Packet>::value
        >
      >
      Packet cast(std::unordered_map<Key, Value, Hash, Equal, Alloc> const& map) {
        auto obj = Packet::make_object();
        for (auto& pair : map) obj.add_field(pair.first, pair.second);
        return obj;
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
          &&
          convert::is_castable<Value, Packet>::value
        >
      >
      Packet cast(std::unordered_map<Key, Value, Hash, Equal, Alloc>&& map) {
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
    struct to_dart<std::multimap<Key, Value, Comp, Alloc>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
          &&
          convert::is_castable<Value, Packet>::value
        >
      >
      Packet cast(std::multimap<Key, Value, Comp, Alloc> const& map) {
        std::map<Key, typename Packet::array, Comp> accum;
        for (auto& pair : map) accum[pair.first].push_back(pair.second);
        return convert::cast<Packet>(std::move(accum));
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
          &&
          convert::is_castable<Value, Packet>::value
        >
      >
      Packet cast(std::multimap<Key, Value, Comp, Alloc>&& map) {
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
    struct to_dart<std::unordered_multimap<Key, Value, Hash, Equal, Alloc>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
          &&
          convert::is_castable<Value, Packet>::value
        >
      >
      Packet cast(std::unordered_multimap<Key, Value, Hash, Equal, Alloc> const& map) {
        std::unordered_map<Key, typename Packet::array, Hash, Equal> accum;
        for (auto& pair : map) accum[pair.first].push_back(pair.second);
        return convert::cast<Packet>(std::move(accum));
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
          &&
          convert::is_castable<Value, Packet>::value
        >
      >
      Packet cast(std::unordered_multimap<Key, Value, Hash, Equal, Alloc>&& map) {
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
    struct to_dart<std::set<Key, Compare, Alloc>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
        >
      >
      Packet cast(std::set<Key, Compare, Alloc> const& set) {
        auto pkt = Packet::make_array();
        pkt.reserve(set.size());
        for (auto& val : set) pkt.push_back(val);
        return pkt;
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
        >
      >
      Packet cast(std::set<Key, Compare, Alloc>&& set) {
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

    template <class Key, class Compare, class Alloc>
    struct to_dart<std::multiset<Key, Compare, Alloc>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
        >
      >
      Packet cast(std::multiset<Key, Compare, Alloc> const& set) {
        auto pkt = Packet::make_array();
        pkt.reserve(set.size());
        for (auto& val : set) pkt.push_back(val);
        return pkt;
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<Key, Packet>::value
        >
      >
      Packet cast(std::multiset<Key, Compare, Alloc>&& set) {
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

    // Specialization for interoperability with std::optional.
    template <class T>
    struct to_dart<shim::optional<T>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      Packet cast(shim::optional<T> const& opt) {
        if (opt) return convert::cast<Packet>(*opt);
        else return Packet::make_null();
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<T, Packet>::value
        >
      >
      Packet cast(shim::optional<T>&& opt) {
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
    struct to_dart<shim::variant<Ts...>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          meta::conjunction<
            convert::is_castable<Ts, Packet>...
          >::value
        >
      >
      Packet cast(shim::variant<Ts...> const& var) {
        return shim::visit([] (auto& val) { return convert::cast<Packet>(val); }, var);
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          meta::conjunction<
            convert::is_castable<Ts, Packet>...
          >::value
        >
      >
      Packet cast(shim::variant<Ts...>&& var) {
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
    struct to_dart<std::tuple<Ts...>> {
      template <class Packet, class Tuple, size_t... idxs>
      Packet unpack_convert(Tuple&& tup, std::index_sequence<idxs...>) {
        return Packet::make_array(std::get<idxs>(std::forward<Tuple>(tup))...);
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
      Packet cast(std::tuple<Ts...> const& tup) {
        return unpack_convert<Packet>(tup, std::index_sequence_for<Ts...> {});
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          meta::conjunction<
            convert::is_castable<Ts, Packet>...
          >::value
        >
      >
      Packet cast(std::tuple<Ts...>&& tup) {
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
    struct to_dart<std::pair<First, Second>> {
      // Copy conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<First, Packet>::value
          &&
          convert::is_castable<Second, Packet>::value
        >
      >
      Packet cast(std::pair<First, Second> const& pair) {
        return Packet::make_array(pair.first, pair.second);
      }

      // Move conversion
      template <class Packet, class =
        std::enable_if_t<
          convert::is_castable<First, Packet>::value
          &&
          convert::is_castable<Second, Packet>::value
        >
      >
      Packet cast(std::pair<First, Second>&& pair) {
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
    struct to_dart<std::chrono::time_point<std::chrono::system_clock, Duration>> {
      using sys_time_point = std::chrono::time_point<std::chrono::system_clock, Duration>;

      std::string convert(sys_time_point tp) {
        std::ostringstream oss;
        std::time_t gmt = std::chrono::system_clock::to_time_t(tp);
        oss << std::put_time(std::localtime(&gmt), "%FT%TZ");
        return oss.str();
      }

      // Copy conversion
      template <class Packet>
      Packet cast(sys_time_point tp) {
        return Packet::make_string(convert(tp));
      }

      // Equality
      template <class Packet>
      bool compare(Packet const& pkt, sys_time_point tp) {
        return pkt == convert(tp);
      }
    };

    // Bizzarely useful in some meta-programming situations.
    template <class T, T val>
    struct to_dart<std::integral_constant<T, val>> {
      // Conversion
      template <class Packet>
      Packet cast(std::integral_constant<T, val>) {
        return convert::cast<Packet>(val);
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
