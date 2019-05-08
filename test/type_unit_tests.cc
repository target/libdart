/*----- System Includes -----*/

#include <unordered_set>

/*----- Local Includes -----*/

#include "dart_tests.h"

/*----- Namespace Inclusions -----*/

using namespace dart::literals;

/*----- Tests -----*/

SCENARIO("objects can be iterated over", "[type unit]") {
  GIVEN("a statically typed object with contents") {
    dart::object_api_test([] (auto tag, auto idx) {
      using object = typename decltype(tag)::type;
      using string = dart::basic_string<typename object::value_type>;
      using number = dart::basic_number<typename object::value_type>;
      using flag = dart::basic_flag<typename object::value_type>;
      using null = dart::basic_null<typename object::value_type>;

      // Get some keys and values.
      std::unordered_set<std::string> keys {"hello", "yes", "stop"};
      std::unordered_set<std::string> values {"goodbye", "no", "go"};

      // Put them into an object.
      auto kit = keys.begin(), vit = values.begin();
      object obj {*kit++, *vit++, *kit++, *vit++, *kit++, *vit++};
      DYNAMIC_WHEN("we iterate over the valuespace", idx) {
        std::unordered_set<std::string> visited;
        for (auto v : obj) visited.insert(v.str());
        DYNAMIC_THEN("we visit all values", idx) {
          REQUIRE(visited == values);
        }
      }

      DYNAMIC_WHEN("we iterate over the keyspace", idx) {
        std::unordered_set<std::string> visited;
        for (auto k = obj.key_begin(); k != obj.key_end(); ++k) {
          visited.insert(k->str());
        }
        DYNAMIC_THEN("we visit all keys", idx) {
          REQUIRE(visited == keys);
        }
      }
    });
  }
}

SCENARIO("objects accept a variety of different types", "[type unit]") {
  GIVEN("a statically typed, mutable object") {
    dart::mutable_object_api_test([] (auto tag, auto idx) {
      using object = typename decltype(tag)::type;
      using string = dart::basic_string<typename object::value_type>;
      using number = dart::basic_number<typename object::value_type>;
      using flag = dart::basic_flag<typename object::value_type>;
      using null = dart::basic_null<typename object::value_type>;

      object obj {"hello", "goodbye", "ruid", 138000709, "halfway", 0.5};
      DYNAMIC_WHEN("machine types are inserted", idx) {
        // Run the gamut to ensure our overloads behave.
        obj.add_field("", string {"problems?"});
        obj.insert(string {"int"}, number {42});
        obj.add_field("unsigned", number {365U});
        obj.insert(string {"long"}, number {86400L});
        obj.add_field("unsigned long", number {3600UL});
        obj.insert(string {"long long"}, number {7200LL});
        obj.add_field("unsigned long long", number {93000000ULL});
        obj.insert(string {"pi"}, number {3.14159});
        obj.add_field("c", number {2.99792f});
        obj.insert(string {"truth"}, flag {true});
        obj.add_field("lies", flag {false});
        obj.insert(string {"absent"}, null {nullptr});

        DYNAMIC_THEN("it all checks out", idx) {
          REQUIRE(obj[string {"hello"}] == string {"goodbye"});
          REQUIRE(obj.get("ruid"_dart) == number {138000709});
          REQUIRE(obj["halfway"] == number {0.5});
          REQUIRE(obj.get(string {""}) == string {"problems?"});
          REQUIRE(obj["int"_dart] == number {42});
          REQUIRE(obj.get("unsigned") == number {365});
          REQUIRE(obj[string {"long"}] == number {86400});
          REQUIRE(obj.get("unsigned long"_dart) == number {3600});
          REQUIRE(obj["long long"] == number {7200});
          REQUIRE(obj.get(string {"unsigned long long"}) == number {93000000});
          REQUIRE(obj["pi"_dart].decimal() == Approx(3.14159));
          REQUIRE(obj.get("c").decimal() == Approx(2.99792));
          REQUIRE(obj[string {"truth"}]);
          REQUIRE_FALSE(obj.get("lies"_dart));
          REQUIRE(obj["absent"].get_type() == dart::packet::type::null);
        }
      }

      DYNAMIC_WHEN("machine types are removed", idx) {
        // Remove everything.
        obj.erase(string {"hello"});
        obj.erase(obj.erase("halfway"_dart));
        DYNAMIC_THEN("nothing remains", idx) {
          REQUIRE(!obj[string {"hello"}]);
          REQUIRE(!obj["halfway"_dart]);
          REQUIRE(!obj["ruid"]);
          REQUIRE(obj.empty());
        }
      }

      DYNAMIC_WHEN("other objects are inserted", idx) {
        obj.add_field("other", object {"c", 2.99792}).add_field("another", object {"asdf", "qwerty"});
        DYNAMIC_THEN("everything checks out", idx) {
          REQUIRE(obj["other"]["c"].decimal() == Approx(2.99792));
          REQUIRE(obj["another"]["asdf"] == "qwerty");
        }
      }
    });
  }

  GIVEN("a dynamically typed mutable object") {
    dart::simple_mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      using object = dart::basic_object<pkt>;
      using string = dart::basic_string<pkt>;
      using number = dart::basic_number<pkt>;
      using flag = dart::basic_flag<pkt>;
      using null = dart::basic_null<pkt>;

      auto obj = pkt::make_object("hello", "goodbye", "ruid", 138000709, "halfway", 0.5);
      DYNAMIC_WHEN("machine types are inserted", idx) {
        // Run the gamut to ensure our overloads behave.
        obj.add_field("", string {"problems?"});
        obj.insert(string {"int"}, number {42});
        obj.add_field("unsigned", number {365U});
        obj.insert(string {"long"}, number {86400L});
        obj.add_field("unsigned long", number {3600UL});
        obj.insert(string {"long long"}, number {7200LL});
        obj.add_field("unsigned long long", number {93000000ULL});
        obj.insert(string {"pi"}, number {3.14159});
        obj.add_field("c", number {2.99792});
        obj.insert(string {"truth"}, flag {true});
        obj.add_field("lies", flag {false});
        obj.insert(string {"absent"}, null {nullptr});

        DYNAMIC_THEN("it all checks out", idx) {
          REQUIRE(obj[string {"hello"}] == string {"goodbye"});
          REQUIRE(obj.get("ruid"_dart) == number {138000709});
          REQUIRE(obj["halfway"] == number {0.5});
          REQUIRE(obj.get(string {""}) == string {"problems?"});
          REQUIRE(obj["int"_dart] == number {42});
          REQUIRE(obj.get("unsigned") == number {365});
          REQUIRE(obj[string {"long"}] == number {86400});
          REQUIRE(obj.get("unsigned long"_dart) == number {3600});
          REQUIRE(obj["long long"] == number {7200});
          REQUIRE(obj.get(string {"unsigned long long"}) == number {93000000});
          REQUIRE(obj["pi"_dart].decimal() == Approx(3.14159));
          REQUIRE(obj.get("c").decimal() == Approx(2.99792));
          REQUIRE(obj[string {"truth"}]);
          REQUIRE_FALSE(obj.get("lies"_dart));
          REQUIRE(obj["absent"].get_type() == dart::packet::type::null);
        }
      }

      DYNAMIC_WHEN("machine types are removed", idx) {
        // Remove everything.
        obj.erase(string {"hello"});
        obj.erase(obj.erase("halfway"_dart));
        DYNAMIC_THEN("nothing remains", idx) {
          REQUIRE(!obj[string {"hello"}]);
          REQUIRE(!obj["halfway"_dart]);
          REQUIRE(!obj["ruid"]);
          REQUIRE(obj.empty());
        }
      }

      DYNAMIC_WHEN("other objects are inserted", idx) {
        obj.add_field("other", object {"c", 2.99792}).add_field("another", object {"asdf", "qwerty"});
        DYNAMIC_THEN("everything checks out", idx) {
          REQUIRE(obj["other"]["c"].decimal() == Approx(2.99792));
          REQUIRE(obj["another"]["asdf"] == "qwerty");
        }
      }
    });
  }
}

SCENARIO("arrays accept a variety of different types", "[overload unit]") {
  GIVEN("a statically typed, mutable array") {
    dart::mutable_array_api_test([] (auto tag, auto idx) {
      using array = typename decltype(tag)::type;
      using string = dart::basic_string<typename array::value_type>;
      using number = dart::basic_number<typename array::value_type>;
      using flag = dart::basic_flag<typename array::value_type>;
      using null = dart::basic_null<typename array::value_type>;

      array arr {"hello", 1337, 3.14159, false, nullptr};
      DYNAMIC_WHEN("machine types are inserted", idx) {
        arr.push_back(string {""});
        arr.push_front(number {42});
        arr.insert(arr.begin(), number {365U});
        arr.insert(arr.end(), number {86400L});
        arr.insert(0, number {3600UL});
        arr.insert(arr.size(), number {7200LL});
        arr.insert(number {1}, number {93000000ULL});
        arr.push_back(number {6.022});
        arr.push_front(number {2.99792});
        arr.push_front(number {0.5f});
        arr.push_back(false);
        arr.push_back(flag {true});
        arr.push_back(null {nullptr});

        DYNAMIC_THEN("everything winds up where we expect", idx) {
          REQUIRE(arr.front() == 0.5);
          REQUIRE(arr[number {1}] == number {2.99792});
          REQUIRE(arr.get(2_dart) == 3600UL);
          REQUIRE(arr[3] == number {93000000ULL});
          REQUIRE(arr.get(number {4}) == 365U);
          REQUIRE(arr[5_dart] == number {42});
          REQUIRE(arr.get(6) == "hello");
          REQUIRE(arr[number {7}] == number {1337});
          REQUIRE(arr.get(8_dart) == 3.14159);
          REQUIRE(arr[9] == flag {false});
          REQUIRE(arr.get(number {10}) == null {nullptr});
          REQUIRE(arr[11_dart] == "");
          REQUIRE(arr.get(12) == number {86400});
          REQUIRE(arr[number {13}] == 7200LL);
          REQUIRE(arr.get(14_dart) == number {6.022});
          REQUIRE(arr[15] == false);
          REQUIRE(arr.get(number {16}) == flag {true});
          REQUIRE(arr.back() == nullptr);
          REQUIRE(arr.size() == 18ULL);
        }
      }
    });
  }

  GIVEN("a dynamically typed mutable array") {
    dart::simple_mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      using object = dart::basic_object<pkt>;
      using string = dart::basic_string<pkt>;
      using number = dart::basic_number<pkt>;
      using flag = dart::basic_flag<pkt>;
      using null = dart::basic_null<pkt>;

      auto arr = pkt::make_array("hello", 1337, 3.14159, false, nullptr);
      DYNAMIC_WHEN("machine types are inserted", idx) {
        arr.push_back(string {""});
        arr.push_front(number {42});
        arr.insert(arr.begin(), number {365U});
        arr.insert(arr.end(), number {86400L});
        arr.insert(0, number {3600UL});
        arr.insert(arr.size(), number {7200LL});
        arr.insert(number {1}, number {93000000ULL});
        arr.push_back(number {6.022});
        arr.push_front(number {2.99792});
        arr.push_front(number {0.5f});
        arr.push_back(false);
        arr.push_back(flag {true});
        arr.push_back(null {nullptr});

        DYNAMIC_THEN("everything winds up where we expect", idx) {
          REQUIRE(arr.front() == 0.5);
          REQUIRE(arr[number {1}] == number {2.99792});
          REQUIRE(arr.get(2_dart) == 3600UL);
          REQUIRE(arr[3] == number {93000000ULL});
          REQUIRE(arr.get(number {4}) == 365U);
          REQUIRE(arr[5_dart] == number {42});
          REQUIRE(arr.get(6) == "hello");
          REQUIRE(arr[number {7}] == number {1337});
          REQUIRE(arr.get(8_dart) == 3.14159);
          REQUIRE(arr[9] == flag {false});
          REQUIRE(arr.get(number {10}) == null {nullptr});
          REQUIRE(arr[11_dart] == "");
          REQUIRE(arr.get(12) == number {86400});
          REQUIRE(arr[number {13}] == 7200LL);
          REQUIRE(arr.get(14_dart) == number {6.022});
          REQUIRE(arr[15] == false);
          REQUIRE(arr.get(number {16}) == flag {true});
          REQUIRE(arr.back() == nullptr);
          REQUIRE(arr.size() == 18ULL);
        }
      }
    });
  }
}
