/*----- System Includes -----*/

#include <unordered_set>

/*----- Local Includes -----*/

#include "dart_tests.h"

/*----- Namespace Inclusions -----*/

using namespace dart::literals;

/*----- Tests -----*/

SCENARIO("objects are regular types", "[type unit]") {
  GIVEN("a default-constructed, strongly typed, object") {
    dart::object_api_test([] (auto tag, auto idx) {
      using object = typename decltype(tag)::type;

      // Validate basic properties.
      object obj;
      REQUIRE(obj.empty());
      REQUIRE(obj.is_object());
      REQUIRE(obj.is_aggregate());
      REQUIRE_FALSE(obj.is_array());
      REQUIRE_FALSE(obj.is_str());
      REQUIRE_FALSE(obj.is_integer());
      REQUIRE_FALSE(obj.is_decimal());
      REQUIRE_FALSE(obj.is_numeric());
      REQUIRE_FALSE(obj.is_boolean());
      REQUIRE_FALSE(obj.is_null());
      REQUIRE_FALSE(obj.is_primitive());
      REQUIRE(obj.size() == 0U);
      REQUIRE(static_cast<bool>(obj));
      REQUIRE(obj.get_type() == dart::packet::type::object);

      DYNAMIC_WHEN("the object is copied", idx) {
        auto dup = obj;
        DYNAMIC_THEN("the two compare equal", idx) {
          REQUIRE(obj.empty());
          REQUIRE(obj.is_object());
          REQUIRE(obj == dup);
        }
      }

      DYNAMIC_WHEN("the object is moved", idx) {
        auto moved = std::move(obj);
        DYNAMIC_THEN("the two do NOT compare equal", idx) {
          REQUIRE(moved.empty());
          REQUIRE(moved.is_object());
          REQUIRE(moved != obj);
        }
      }

      DYNAMIC_WHEN("the object is copied, then moved", idx) {
        auto dup = obj;
        auto moved = std::move(dup);
        DYNAMIC_THEN("the two DO compare equal", idx) {
          REQUIRE(obj.empty());
          REQUIRE(obj.is_object());
          REQUIRE(obj == moved);
        }
      }

      DYNAMIC_WHEN("the object is compared against an equivalent, disparate, object", idx) {
        object dup;
        DYNAMIC_THEN("the two compare equal", idx) {
          REQUIRE(dup.empty());
          REQUIRE(dup.is_object());
          REQUIRE(dup == obj);
        }
      }

      DYNAMIC_WHEN("the object is compared against an inequivalent object", idx) {
        object nope {"won't", "work"};
        DYNAMIC_THEN("the two do NOT compare equal", idx) {
          REQUIRE(!nope.empty());
          REQUIRE(nope.is_object());
          REQUIRE(nope.size() == 1U);
          REQUIRE(nope != obj);
        }
      }

      DYNAMIC_WHEN("the object decays to a dynamic type", idx) {
        typename object::value_type dynamic = obj;
        DYNAMIC_THEN("the two remain equivalent", idx) {
          REQUIRE(dynamic == obj);
        }
      }
    });
  }
}

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

      DYNAMIC_WHEN("we request the valuespace", idx) {
        auto vals = obj.values();
        std::unordered_set<std::string> visited;
        for (auto& v : vals) visited.insert(v.str());
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

      DYNAMIC_WHEN("we request the keyspace", idx) {
        auto ks = obj.keys();
        std::unordered_set<std::string> visited;
        for (auto& k : ks) visited.insert(k.str());
        DYNAMIC_THEN("we visit all keys", idx) {
          REQUIRE(visited == keys);
        }
      }
    });
  }
}

SCENARIO("objects can access nested keys in a single step", "[type unit]") {
  GIVEN("an object with nested fields") {
    dart::object_api_test([] (auto tag, auto idx) {
      using object = typename decltype(tag)::type;

      object obj {"songs", object {"time", "dark side", "come_together", "abbey road"}};

      DYNAMIC_WHEN("accessing a valid nested field", idx) {
        auto dark_side = obj.get_nested("songs.time");
        auto abbey_road = obj.get_nested("songs.come_together");
        DYNAMIC_THEN("it returns the correct value", idx) {
          REQUIRE(dark_side == "dark side");
          REQUIRE(abbey_road == "abbey road");
        }
      }

      DYNAMIC_WHEN("accessing an invalid path", idx) {
        auto nested = obj.get_nested("songs.not_here");
        auto bad_nested = obj.get_nested(".songs..definitely_not_here.");
        DYNAMIC_THEN("it returns null", idx) {
          REQUIRE(nested.is_null());
          REQUIRE(bad_nested.is_null());
        }
      }

      DYNAMIC_WHEN("accessing a path prefix", idx) {
        auto nested = obj.get_nested("song");
        DYNAMIC_THEN("it returns null", idx) {
          REQUIRE(nested.is_null());
        }
      }
    });
  }
}

SCENARIO("finalized objects can be sent over the network", "[type unit]") {
  GIVEN("a statically typed, finalized, object with contents") {
    dart::simple_finalized_object_api_test([] (auto tag, auto idx) {
      using object = typename decltype(tag)::type;

      // Get a good spread.
      object contents {"hello", "goodbye", "answer", 42, "pi", 3.14159, "lies", false, "nested", object {}};
      contents.finalize();

      DYNAMIC_WHEN("the network buffer is accessed", idx) {
        auto bytes = contents.get_bytes();
        DYNAMIC_THEN("a non-empty buffer of bytes is returned", idx) {
          REQUIRE(!bytes.empty());
          REQUIRE(bytes.data() != nullptr);
        }
      }

      DYNAMIC_WHEN("the network buffer is duplicated", idx) {
        size_t len;
        auto bytes = contents.dup_bytes(len);
        DYNAMIC_THEN("a non-empty unique_ptr of bytes is returned", idx) {
          REQUIRE(len > 0U);
          REQUIRE(bytes.get() != nullptr);
        }
      }

      DYNAMIC_WHEN("the network buffer is shared", idx) {
        std::shared_ptr<gsl::byte const> bytes;
        auto len = contents.share_bytes(bytes);
        DYNAMIC_THEN("a non-empty reference counter is returned", idx) {
          REQUIRE(len > 0U);
          REQUIRE(bytes.get() != nullptr);
        }
      }

      DYNAMIC_WHEN("the network buffer is reconstructed", idx) {
        // Duplicate the buffer and create a new object.
        object dup {contents.get_bytes()};

        DYNAMIC_THEN("the two objects are equal", idx) {
          REQUIRE(dup == contents);
        }
      }

      DYNAMIC_WHEN("the network buffer is duplicated and reconstructed", idx) {
        // Duplicate the buffer and create a new object.
        object dup {contents.dup_bytes()};

        DYNAMIC_THEN("the two objects are equal", idx) {
          REQUIRE(dup == contents);
        }
      }

      DYNAMIC_WHEN("the network buffer is shared and reconstructed", idx) {
        // Share the buffer and create a new object.
        std::shared_ptr<gsl::byte const> bytes;
        contents.share_bytes(bytes);
        object dup {std::move(bytes)};
        DYNAMIC_THEN("the two objects are equal", idx) {
          REQUIRE(dup == contents);
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

SCENARIO("object keys must be strings", "[type unit]") {
  GIVEN("a statically typed, mutable object") {
    dart::mutable_object_api_test([] (auto tag, auto idx) {
      using object = typename decltype(tag)::type;
      using number = dart::basic_number<typename object::value_type>;
      using flag = dart::basic_flag<typename object::value_type>;
      using null = dart::basic_null<typename object::value_type>;

      object obj;
      DYNAMIC_WHEN("key-value pairs are inserted", idx) {
        DYNAMIC_THEN("the keys must be strings", idx) {
          REQUIRE_THROWS_AS(obj.add_field(nullptr, "oops"), std::logic_error);
          REQUIRE_THROWS_AS(obj.add_field(5, "oops"), std::logic_error);
          REQUIRE_THROWS_AS(obj.add_field(false, "oops"), std::logic_error);
          REQUIRE_THROWS_AS(obj.add_field(2.0, "oops"), std::logic_error);
          REQUIRE_THROWS_AS(obj.add_field(number {5}, "oops"), std::logic_error);
          REQUIRE_THROWS_AS(obj.add_field(number {2.0}, "oops"), std::logic_error);
          REQUIRE_THROWS_AS(obj.add_field(flag {true}, "oops"), std::logic_error);
          REQUIRE_THROWS_AS(obj.insert(nullptr, "oops"), std::logic_error);
          REQUIRE_THROWS_AS(obj.insert(5, "oops"), std::logic_error);
          REQUIRE_THROWS_AS(obj.insert(false, "oops"), std::logic_error);
          REQUIRE_THROWS_AS(obj.insert(2.0, "oops"), std::logic_error);
          REQUIRE_THROWS_AS(obj.insert(null {}, "oops"), std::logic_error);
          REQUIRE_THROWS_AS(obj.insert(number {5}, "oops"), std::logic_error);
          REQUIRE_THROWS_AS(obj.insert(number {2.0}, "oops"), std::logic_error);
          REQUIRE_THROWS_AS(obj.insert(flag {true}, "oops"), std::logic_error);
        }
      }
    });
  }
}

SCENARIO("arrays are regular types", "[type unit]") {
  GIVEN("a default-constructed, strongly typed, array") {
    dart::mutable_array_api_test([] (auto tag, auto idx) {
      using array = typename decltype(tag)::type;

      // Validate basic properties.
      array arr;
      REQUIRE(arr.empty());
      REQUIRE(arr.is_array());
      REQUIRE(arr.is_aggregate());
      REQUIRE_FALSE(arr.is_object());
      REQUIRE_FALSE(arr.is_str());
      REQUIRE_FALSE(arr.is_integer());
      REQUIRE_FALSE(arr.is_decimal());
      REQUIRE_FALSE(arr.is_numeric());
      REQUIRE_FALSE(arr.is_boolean());
      REQUIRE_FALSE(arr.is_null());
      REQUIRE_FALSE(arr.is_primitive());
      REQUIRE(arr.size() == 0U);
      REQUIRE(arr.get_type() == dart::packet::type::array);

      DYNAMIC_WHEN("the array is copied", idx) {
        auto dup = arr;
        DYNAMIC_THEN("the two compare equal", idx) {
          REQUIRE(arr.empty());
          REQUIRE(arr.is_array());
          REQUIRE(arr == dup);
        }
      }

      DYNAMIC_WHEN("the array is moved", idx) {
        auto moved = std::move(arr);
        DYNAMIC_THEN("the two do NOT compare equal", idx) {
          REQUIRE(moved.empty());
          REQUIRE(moved.is_array());
          REQUIRE(moved != arr);
        }
      }

      DYNAMIC_WHEN("the array is copied, then moved", idx) {
        auto dup = arr;
        auto moved = std::move(dup);
        DYNAMIC_THEN("the two DO compare equal", idx) {
          REQUIRE(arr.empty());
          REQUIRE(arr.is_array());
          REQUIRE(arr == moved);
        }
      }

      DYNAMIC_WHEN("the array is compared against an equivalent, disparate, array", idx) {
        array dup;
        DYNAMIC_THEN("the two compare equal", idx) {
          REQUIRE(dup.empty());
          REQUIRE(dup.is_array());
          REQUIRE(dup == arr);
        }
      }

      DYNAMIC_WHEN("the object is compared against an inequivalent object", idx) {
        array nope {"won't", "work"};
        DYNAMIC_THEN("the two do NOT compare equal", idx) {
          REQUIRE(!nope.empty());
          REQUIRE(nope.is_array());
          REQUIRE(nope.size() == 2U);
          REQUIRE(nope != arr);
        }
      }

      DYNAMIC_WHEN("the object decays to a dynamic type", idx) {
        typename array::value_type dynamic = arr;
        DYNAMIC_THEN("the two remain equivalent", idx) {
          REQUIRE(dynamic == arr);
        }
      }
    });
  }
}

SCENARIO("arrays accept a variety of different types", "[type unit]") {
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

SCENARIO("arrays can be iterated over", "[type unit]") {
  GIVEN("a statically typed array with contents") {
    dart::mutable_array_api_test([] (auto tag, auto idx) {
      using array = typename decltype(tag)::type;

      // Get some values.
      std::unordered_set<std::string> values {"hello", "goodbye", "yes", "no"};

      // Put them in an array.
      array arr;
      for (auto& v : values) arr.push_back(v);

      DYNAMIC_WHEN("we iterate over the valuespace", idx) {
        std::unordered_set<std::string> visited;
        for (auto v : arr) visited.insert(v.str());
        DYNAMIC_THEN("we visit all values", idx) {
          REQUIRE(visited == values);
        }
      }

      DYNAMIC_WHEN("we request the valuespace", idx) {
        auto vals = arr.values();
        std::unordered_set<std::string> visited;
        for (auto& v : vals) visited.insert(v.str());
        DYNAMIC_THEN("we visit all values", idx) {
          REQUIRE(visited == values);
        }
      }
    });
  }
}

SCENARIO("arrays are ordered containers", "[type unit]") {
  GIVEN("a statically typed, mutable array") {
    dart::mutable_array_api_test([] (auto tag, auto idx) {
      using array = typename decltype(tag)::type;

      array arr {"middle"};
      DYNAMIC_WHEN("a value is inserted at the front", idx) {
        arr.push_front("almost_middle");
        arr.insert(0, "almost_front");
        arr.insert(arr.begin(), "front");
        DYNAMIC_THEN("values are in expected order", idx) {
          REQUIRE(arr.size() == 4U);
          REQUIRE(arr.front() == "front");
          REQUIRE(arr[0] == "front");
          REQUIRE(arr[1] == "almost_front");
          REQUIRE(arr[2] == "almost_middle");
          REQUIRE(arr[3] == "middle");
          REQUIRE(arr.back() == "middle");
        }

        DYNAMIC_WHEN("those values are popped from the front", idx) {
          arr.pop_front();
          arr.erase(0);
          arr.erase(arr.begin());
          DYNAMIC_THEN("only the original content remains", idx) {
            REQUIRE(arr.size() == 1U);
            REQUIRE(arr.front() == arr.back());
            REQUIRE(arr[0] == "middle");
          }
        }
      }
      
      DYNAMIC_WHEN("a value is inserted at the back", idx) {
        arr.push_back("almost_middle");
        arr.insert(arr.size(), "almost_back");
        arr.insert(arr.end(), "back");
        DYNAMIC_THEN("values are in expected order", idx) {
          REQUIRE(arr.front() == "middle");
          REQUIRE(arr[0] == "middle");
          REQUIRE(arr[1] == "almost_middle");
          REQUIRE(arr[2] == "almost_back");
          REQUIRE(arr[3] == "back");
          REQUIRE(arr.back() == "back");
        }

        DYNAMIC_WHEN("those values are popped from the back", idx) {
          arr.pop_back();
          arr.erase(arr.size() - 1);
          arr.erase(--arr.end());
          DYNAMIC_THEN("only the original content remains", idx) {
            REQUIRE(arr.size() == 1U);
            REQUIRE(arr.front() == arr.back());
            REQUIRE(arr[0] == "middle");
          }
        }
      }
    });
  }
}

SCENARIO("arrays can be accessed optionally", "[type unit]") {
  GIVEN("an empty statically typed, mutable array") {
    dart::mutable_array_api_test([] (auto tag, auto idx) {
      using array = typename decltype(tag)::type;

      array arr;
      DYNAMIC_WHEN("when the front is optionally accessed", idx) {
        auto val = arr.front_or("nope");
        DYNAMIC_THEN("the optional value is returned", idx) {
          REQUIRE(val == "nope");
        }
      }

      DYNAMIC_WHEN("when the back is optionally accessed", idx) {
        auto val = arr.back_or("still nope");
        DYNAMIC_THEN("the optional value is returned", idx) {
          REQUIRE(val == "still nope");
        }
      }

      DYNAMIC_WHEN("some index is optionally accessed", idx) {
        auto val = arr.get_or(15, "wasn't there");
        DYNAMIC_THEN("the optional value is returned", idx) {
          REQUIRE(val == "wasn't there");
        }
      }
    });
  }

  GIVEN("a statically typed, mutable array with contents") {
    dart::mutable_array_api_test([] (auto tag, auto idx) {
      using array = typename decltype(tag)::type;

      array arr {"first", "second", "third"};
      DYNAMIC_WHEN("the front is optionally accessed", idx) {
        auto val = arr.front_or("not me");
        DYNAMIC_THEN("the index is returned", idx) {
          REQUIRE(val == "first");
        }
      }

      DYNAMIC_WHEN("the back is optionally accessed", idx) {
        auto val = arr.back_or("not me either");
        DYNAMIC_THEN("the index is returned", idx) {
          REQUIRE(val == "third");
        }
      }

      DYNAMIC_WHEN("the middle is optionally accessed", idx) {
        auto val = arr.get_or(1, "lastly, not me");
        DYNAMIC_THEN("the middle is returned", idx) {
          REQUIRE(val == "second");
        }
      }
    });
  }
}

SCENARIO("arrays indices must be integers", "[type unit]") {
  GIVEN("a statically typed, mutable array") {
    dart::mutable_array_api_test([] (auto tag, auto idx) {
      using array = typename decltype(tag)::type;
      using string = dart::basic_string<typename array::value_type>;
      using number = dart::basic_number<typename array::value_type>;
      using flag = dart::basic_flag<typename array::value_type>;
      using null = dart::basic_null<typename array::value_type>;

      array arr;
      DYNAMIC_WHEN("values are inserted", idx) {
        DYNAMIC_THEN("the indices must be integers", idx) {
          REQUIRE_THROWS_AS(arr.insert("asdf", "oops"), std::logic_error);
          REQUIRE_THROWS_AS(arr.insert(false, "oops"), std::logic_error);
          REQUIRE_THROWS_AS(arr.insert(2.5, "oops"), std::logic_error);
          REQUIRE_THROWS_AS(arr.insert(nullptr, "oops"), std::logic_error);
          REQUIRE_THROWS_AS(arr.insert(string {"asdf"}, "oops"), std::logic_error);
          REQUIRE_THROWS_AS(arr.insert(flag {true}, "oops"), std::logic_error);
          REQUIRE_THROWS_AS(arr.insert(number {2.5}, "oops"), std::logic_error);
          REQUIRE_THROWS_AS(arr.insert(null {}, "oops"), std::logic_error);
        }
      }
    });
  }
}
