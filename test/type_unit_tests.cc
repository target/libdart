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
          REQUIRE(dup == obj);
        }
      }

      DYNAMIC_WHEN("the object is moved", idx) {
        auto moved = std::move(obj);
        DYNAMIC_THEN("the two do NOT compare equal", idx) {
          REQUIRE(moved.empty());
          REQUIRE(moved.is_object());
          REQUIRE(moved != obj);
          REQUIRE(obj != moved);
          REQUIRE(!obj.is_object());
          REQUIRE(!obj.is_aggregate());
          REQUIRE(obj.is_null());
          REQUIRE_FALSE(static_cast<bool>(obj));
        }
      }

      DYNAMIC_WHEN("the object is copied, then moved", idx) {
        auto dup = obj;
        auto moved = std::move(dup);
        DYNAMIC_THEN("the two DO compare equal", idx) {
          REQUIRE(obj.empty());
          REQUIRE(obj.is_object());
          REQUIRE(obj == moved);
          REQUIRE(moved == obj);
          REQUIRE(!dup.is_object());
          REQUIRE(!dup.is_aggregate());
          REQUIRE(dup.is_null());
          REQUIRE_FALSE(static_cast<bool>(dup));
        }
      }

      DYNAMIC_WHEN("the object is compared against an equivalent, disparate, object", idx) {
        object dup;
        DYNAMIC_THEN("the two compare equal", idx) {
          REQUIRE(dup.empty());
          REQUIRE(dup.is_object());
          REQUIRE(dup == obj);
          REQUIRE(obj == dup);
        }
      }

      DYNAMIC_WHEN("the object is compared against an inequivalent object", idx) {
        object nope {"won't", "work"};
        DYNAMIC_THEN("the two do NOT compare equal", idx) {
          REQUIRE(!nope.empty());
          REQUIRE(nope.is_object());
          REQUIRE(nope.size() == 1U);
          REQUIRE(nope != obj);
          REQUIRE(obj != nope);
        }
      }

      DYNAMIC_WHEN("the object decays to a dynamic type", idx) {
        typename object::value_type dynamic = obj;
        DYNAMIC_THEN("the two remain equivalent", idx) {
          REQUIRE(dynamic == obj);
          REQUIRE(obj == dynamic);
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
          REQUIRE("dark side" == dark_side);
          REQUIRE(abbey_road == "abbey road");
          REQUIRE("abbey road" == abbey_road);
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
          REQUIRE(contents == dup);
        }
      }

      DYNAMIC_WHEN("the network buffer is duplicated and reconstructed", idx) {
        // Duplicate the buffer and create a new object.
        object dup {contents.dup_bytes()};

        DYNAMIC_THEN("the two objects are equal", idx) {
          REQUIRE(dup == contents);
          REQUIRE(contents == dup);
        }
      }

      DYNAMIC_WHEN("the network buffer is shared and reconstructed", idx) {
        // Share the buffer and create a new object.
        std::shared_ptr<gsl::byte const> bytes;
        contents.share_bytes(bytes);
        object dup {std::move(bytes)};
        DYNAMIC_THEN("the two objects are equal", idx) {
          REQUIRE(dup == contents);
          REQUIRE(contents == dup);
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

      object obj {"hello", "goodbye", "ruid", 138000709, "half", 0.5};
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
          REQUIRE(string {"goodbye"} == obj[string {"hello"}]);
          REQUIRE(obj.get("ruid"_dart) == number {138000709});
          REQUIRE(number {138000709} == obj.get("ruid"_dart));
          REQUIRE(obj["half"] == number {0.5});
          REQUIRE(number {0.5} == obj["half"]);
          REQUIRE(obj.get(string {""}) == string {"problems?"});
          REQUIRE(string {"problems?"} == obj.get(string {""}));
          REQUIRE(obj["int"_dart] == number {42});
          REQUIRE(number {42} == obj["int"_dart]);
          REQUIRE(obj.get("unsigned") == number {365});
          REQUIRE(number {365} == obj.get("unsigned"));
          REQUIRE(obj[string {"long"}] == number {86400});
          REQUIRE(number {86400} == obj[string {"long"}]);
          REQUIRE(obj.get("unsigned long"_dart) == number {3600});
          REQUIRE(number {3600} == obj.get("unsigned long"_dart));
          REQUIRE(obj["long long"] == number {7200});
          REQUIRE(number {7200} == obj["long long"]);
          REQUIRE(obj.get(string {"unsigned long long"}) == number {93000000});
          REQUIRE(number {93000000} == obj.get(string {"unsigned long long"}));
          REQUIRE(obj["pi"_dart].decimal() == Approx(3.14159));
          REQUIRE(Approx(3.14159) == obj["pi"_dart].decimal());
          REQUIRE(obj.get("c").decimal() == Approx(2.99792));
          REQUIRE(Approx(2.99792) == obj.get("c").decimal());
          REQUIRE(obj[string {"truth"}]);
          REQUIRE_FALSE(obj.get("lies"_dart));
          REQUIRE(obj["absent"].get_type() == dart::packet::type::null);
        }
      }

      DYNAMIC_WHEN("machine types are removed", idx) {
        // Remove everything.
        obj.erase(string {"hello"});
        obj.erase(obj.erase("half"_dart));
        DYNAMIC_THEN("nothing remains", idx) {
          REQUIRE(!obj[string {"hello"}]);
          REQUIRE(!obj["half"_dart]);
          REQUIRE(!obj["ruid"]);
          REQUIRE(obj.empty());
        }
      }

      DYNAMIC_WHEN("other objects are inserted", idx) {
        obj.add_field("other", object {"c", 2.99792}).add_field("another", object {"asdf", "qwerty"});
        DYNAMIC_THEN("everything checks out", idx) {
          REQUIRE(obj["other"]["c"].decimal() == Approx(2.99792));
          REQUIRE(Approx(2.99792) == obj["other"]["c"].decimal());
          REQUIRE(obj["another"]["asdf"] == "qwerty");
          REQUIRE("qwerty" == obj["another"]["asdf"]);
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

      auto obj = pkt::make_object("hello", "goodbye", "ruid", 138000709, "half", 0.5);
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
          REQUIRE(string {"goodbye"} == obj[string {"hello"}]);
          REQUIRE(obj.get("ruid"_dart) == number {138000709});
          REQUIRE(number {138000709} == obj.get("ruid"_dart));
          REQUIRE(obj["half"] == number {0.5});
          REQUIRE(number {0.5} == obj["half"]);
          REQUIRE(obj.get(string {""}) == string {"problems?"});
          REQUIRE(string {"problems?"} == obj.get(string {""}));
          REQUIRE(obj["int"_dart] == number {42});
          REQUIRE(number {42} == obj["int"_dart]);
          REQUIRE(obj.get("unsigned") == number {365});
          REQUIRE(number {365} == obj.get("unsigned"));
          REQUIRE(obj[string {"long"}] == number {86400});
          REQUIRE(number {86400} == obj[string {"long"}]);
          REQUIRE(obj.get("unsigned long"_dart) == number {3600});
          REQUIRE(number {3600} == obj.get("unsigned long"_dart));
          REQUIRE(obj["long long"] == number {7200});
          REQUIRE(number {7200} == obj["long long"]);
          REQUIRE(obj.get(string {"unsigned long long"}) == number {93000000});
          REQUIRE(number {93000000} == obj.get(string {"unsigned long long"}));
          REQUIRE(obj["pi"_dart].decimal() == Approx(3.14159));
          REQUIRE(Approx(3.14159) == obj["pi"_dart].decimal());
          REQUIRE(obj.get("c").decimal() == Approx(2.99792));
          REQUIRE(Approx(2.99792) == obj.get("c").decimal());
          REQUIRE(obj[string {"truth"}]);
          REQUIRE_FALSE(obj.get("lies"_dart));
          REQUIRE(obj["absent"].get_type() == dart::packet::type::null);
        }
      }

      DYNAMIC_WHEN("machine types are removed", idx) {
        // Remove everything.
        obj.erase(string {"hello"});
        obj.erase(obj.erase("half"_dart));
        DYNAMIC_THEN("nothing remains", idx) {
          REQUIRE(!obj[string {"hello"}]);
          REQUIRE(!obj["half"_dart]);
          REQUIRE(!obj["ruid"]);
          REQUIRE(obj.empty());
        }
      }

      DYNAMIC_WHEN("other objects are inserted", idx) {
        obj.add_field("other", object {"c", 2.99792}).add_field("another", object {"asdf", "qwerty"});
        DYNAMIC_THEN("everything checks out", idx) {
          REQUIRE(obj["other"]["c"].decimal() == Approx(2.99792));
          REQUIRE(Approx(2.99792) == obj["other"]["c"].decimal());
          REQUIRE(obj["another"]["asdf"] == "qwerty");
          REQUIRE("qwerty" == obj["another"]["asdf"]);
        }
      }
    });
  }
}

SCENARIO("objects can set individual indices", "[type unit]") {
  GIVEN("a statically typed, mutable object with contents") {
    dart::mutable_object_api_test([] (auto tag, auto idx) {
      using object = typename decltype(tag)::type;
      using string = dart::basic_string<typename object::value_type>;
      using number = dart::basic_number<typename object::value_type>;
      using flag = dart::basic_flag<typename object::value_type>;
      using null = dart::basic_null<typename object::value_type>;

      object obj {"1st", "wrong", "second", "fail", "thiiiird", "error"};
      DYNAMIC_WHEN("values are set", idx) {
        auto set_one = obj.set(string {"1st"}, "correct");
        auto set_two = obj.set("second", string {"pass"});
        auto set_three = obj.set(string {"thiiiird"}, string {"ack"});
        DYNAMIC_THEN("everything is where we expect", idx) {
          REQUIRE(obj[string {"1st"}] == "correct");
          REQUIRE("correct" == *obj.find(string {"1st"}));
          REQUIRE(*obj.find("second") == string {"pass"});
          REQUIRE(obj.get("thiiiird") == "ack");
          REQUIRE(set_one == obj.find("1st"));
          REQUIRE(set_two == obj.find(string {"second"}));
          REQUIRE(set_three == obj.find(string {"thiiiird"}.strv()));
        }
      }

      DYNAMIC_WHEN("iterators are set", idx) {
        auto set_one = obj.set(obj.begin(), "correct");
        auto set_two = obj.set(++obj.begin(), string {"pass"});
        auto set_three = obj.set(--obj.end(), string {"ack"}.strv());
        DYNAMIC_THEN("everything is where we expect", idx) {
          REQUIRE(obj[string {"1st"}] == "correct");
          REQUIRE("correct" == *obj.find(string {"1st"}));
          REQUIRE(*obj.find("second") == string {"pass"});
          REQUIRE(obj.get("thiiiird") == "ack");
          REQUIRE(set_one == obj.find("1st"));
          REQUIRE(set_two == obj.find(string {"second"}));
          REQUIRE(set_three == obj.find(string {"thiiiird"}.strv()));
        }
      }

      DYNAMIC_WHEN("a non-existent assignment is attempted", idx) {
        DYNAMIC_THEN("it refuses the assignment", idx) {
          REQUIRE_THROWS_AS(obj.set("sorry", "nope"), std::out_of_range);
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

SCENARIO("objects can find iterators to keys and values", "[type unit]") {
  GIVEN("an object with some contents") {
    dart::object_api_test([] (auto tag, auto idx) {
      using object = typename decltype(tag)::type;
      using string = dart::basic_string<typename object::value_type>;

      // Get a nice, complicated, object.
      object big {
        "hello", "goodbye",
        "pi", 3.14159,
        "nested", object {
          "nested_key", std::vector<std::string> {"nested", "values"}
        },
        "arr", std::make_tuple(1, 1, 2, 3, 5, 8, 13)
      };

      DYNAMIC_WHEN("the values are accessed", idx) {
        auto hello_it = big.find("hello");
        auto pi_it = big.find("pi");
        auto nested_it = big.find("nested");
        auto arr_it = big.find("arr");
        DYNAMIC_THEN("the returned iterators are correct", idx) {
          REQUIRE(hello_it != big.end());
          REQUIRE(*hello_it == "goodbye");
          REQUIRE(pi_it != big.end());
          REQUIRE(pi_it->decimal() == Approx(3.14159));
          REQUIRE(nested_it != big.end());
          REQUIRE(nested_it->at("nested_key").size() == 2U);
          REQUIRE(arr_it != big.end());
          REQUIRE(arr_it->front() == 1);
          REQUIRE(arr_it->back() == 13);
          REQUIRE(arr_it->size() == 7U);
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
          REQUIRE(dup == arr);
        }
      }

      DYNAMIC_WHEN("the array is moved", idx) {
        auto moved = std::move(arr);
        DYNAMIC_THEN("the two do NOT compare equal", idx) {
          REQUIRE(moved.empty());
          REQUIRE(moved.is_array());
          REQUIRE(moved != arr);
          REQUIRE(arr != moved);
          REQUIRE(!arr.is_array());
          REQUIRE(!arr.is_aggregate());
          REQUIRE(arr.is_null());
          REQUIRE_FALSE(static_cast<bool>(arr));
        }
      }

      DYNAMIC_WHEN("the array is copied, then moved", idx) {
        auto dup = arr;
        auto moved = std::move(dup);
        DYNAMIC_THEN("the two DO compare equal", idx) {
          REQUIRE(arr.empty());
          REQUIRE(arr.is_array());
          REQUIRE(arr == moved);
          REQUIRE(moved == arr);
          REQUIRE(!dup.is_array());
          REQUIRE(!dup.is_aggregate());
          REQUIRE(dup.is_null());
          REQUIRE_FALSE(static_cast<bool>(dup));
        }
      }

      DYNAMIC_WHEN("the array is compared against an equivalent, disparate, array", idx) {
        array dup;
        DYNAMIC_THEN("the two compare equal", idx) {
          REQUIRE(dup.empty());
          REQUIRE(dup.is_array());
          REQUIRE(dup == arr);
          REQUIRE(arr == dup);
        }
      }

      DYNAMIC_WHEN("the object is compared against an inequivalent object", idx) {
        array nope {"won't", "work"};
        DYNAMIC_THEN("the two do NOT compare equal", idx) {
          REQUIRE(!nope.empty());
          REQUIRE(nope.is_array());
          REQUIRE(nope.size() == 2U);
          REQUIRE(nope != arr);
          REQUIRE(arr != nope);
        }
      }

      DYNAMIC_WHEN("the object decays to a dynamic type", idx) {
        typename array::value_type dynamic = arr;
        DYNAMIC_THEN("the two remain equivalent", idx) {
          REQUIRE(dynamic == arr);
          REQUIRE(arr == dynamic);
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
          REQUIRE(0.5 == arr.front());
          REQUIRE(arr[number {1}] == number {2.99792});
          REQUIRE(number {2.99792} == arr[number {1}]);
          REQUIRE(arr.get(2_dart) == 3600UL);
          REQUIRE(3600UL == arr.get(2_dart));
          REQUIRE(arr[3] == number {93000000ULL});
          REQUIRE(number {93000000ULL} == arr[3]);
          REQUIRE(arr.get(number {4}) == 365U);
          REQUIRE(365U == arr.get(number {4}));
          REQUIRE(arr[5_dart] == number {42});
          REQUIRE(number {42} == arr[5_dart]);
          REQUIRE(arr.get(6) == "hello");
          REQUIRE("hello" == arr.get(6));
          REQUIRE(arr[number {7}] == number {1337});
          REQUIRE(number {1337} == arr[number {7}]);
          REQUIRE(arr.get(8_dart) == 3.14159);
          REQUIRE(3.14159 == arr.get(8_dart));
          REQUIRE(arr[9] == flag {false});
          REQUIRE(flag {false} == arr[9]);
          REQUIRE(arr.get(number {10}) == null {nullptr});
          REQUIRE(null {nullptr} == arr.get(number {10}));
          REQUIRE(arr[11_dart] == "");
          REQUIRE("" == arr[11_dart]);
          REQUIRE(arr.get(12) == number {86400});
          REQUIRE(number {86400} == arr.get(12));
          REQUIRE(arr[number {13}] == 7200LL);
          REQUIRE(7200LL == arr[number {13}]);
          REQUIRE(arr.get(14_dart) == number {6.022});
          REQUIRE(number {6.022} == arr.get(14_dart));
          REQUIRE(arr[15] == false);
          REQUIRE(false == arr[15]);
          REQUIRE(arr.get(number {16}) == flag {true});
          REQUIRE(flag {true} == arr.get(number {16}));
          REQUIRE(arr.back() == nullptr);
          REQUIRE(nullptr == arr.back());
          REQUIRE(arr.size() == 18ULL);
          REQUIRE(18ULL == arr.size());
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
          REQUIRE(0.5 == arr.front());
          REQUIRE(arr[number {1}] == number {2.99792});
          REQUIRE(number {2.99792} == arr[number {1}]);
          REQUIRE(arr.get(2_dart) == 3600UL);
          REQUIRE(3600UL == arr.get(2_dart));
          REQUIRE(arr[3] == number {93000000ULL});
          REQUIRE(number {93000000ULL} == arr[3]);
          REQUIRE(arr.get(number {4}) == 365U);
          REQUIRE(365U == arr.get(number {4}));
          REQUIRE(arr[5_dart] == number {42});
          REQUIRE(number {42} == arr[5_dart]);
          REQUIRE(arr.get(6) == "hello");
          REQUIRE("hello" == arr.get(6));
          REQUIRE(arr[number {7}] == number {1337});
          REQUIRE(number {1337} == arr[number {7}]);
          REQUIRE(arr.get(8_dart) == 3.14159);
          REQUIRE(3.14159 == arr.get(8_dart));
          REQUIRE(arr[9] == flag {false});
          REQUIRE(flag {false} == arr[9]);
          REQUIRE(arr.get(number {10}) == null {nullptr});
          REQUIRE(null {nullptr} == arr.get(number {10}));
          REQUIRE(arr[11_dart] == "");
          REQUIRE("" == arr[11_dart]);
          REQUIRE(arr.get(12) == number {86400});
          REQUIRE(number {86400} == arr.get(12));
          REQUIRE(arr[number {13}] == 7200LL);
          REQUIRE(7200LL == arr[number {13}]);
          REQUIRE(arr.get(14_dart) == number {6.022});
          REQUIRE(number {6.022} == arr.get(14_dart));
          REQUIRE(arr[15] == false);
          REQUIRE(false == arr[15]);
          REQUIRE(arr.get(number {16}) == flag {true});
          REQUIRE(flag {true} == arr.get(number {16}));
          REQUIRE(arr.back() == nullptr);
          REQUIRE(nullptr == arr.back());
          REQUIRE(arr.size() == 18ULL);
          REQUIRE(18ULL == arr.size());
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
          REQUIRE("front" == arr.front());
          REQUIRE(arr[0] == "front");
          REQUIRE("front" == arr[0]);
          REQUIRE(arr[1] == "almost_front");
          REQUIRE("almost_front" == arr[1]);
          REQUIRE(arr[2] == "almost_middle");
          REQUIRE("almost_middle" == arr[2]);
          REQUIRE(arr[3] == "middle");
          REQUIRE("middle" == arr[3]);
          REQUIRE(arr.back() == "middle");
          REQUIRE("middle" == arr.back());
        }

        DYNAMIC_WHEN("those values are popped from the front", idx) {
          arr.pop_front();
          arr.erase(0);
          arr.erase(arr.begin());
          DYNAMIC_THEN("only the original content remains", idx) {
            REQUIRE(arr.size() == 1U);
            REQUIRE(arr.front() == arr.back());
            REQUIRE(arr[0] == "middle");
            REQUIRE("middle" == arr[0]);
          }
        }
      }
      
      DYNAMIC_WHEN("a value is inserted at the back", idx) {
        arr.push_back("almost_middle");
        arr.insert(arr.size(), "almost_back");
        arr.insert(arr.end(), "back");
        DYNAMIC_THEN("values are in expected order", idx) {
          REQUIRE(arr.front() == "middle");
          REQUIRE("middle" == arr.front());
          REQUIRE(arr[0] == "middle");
          REQUIRE("middle" == arr[0]);
          REQUIRE(arr[1] == "almost_middle");
          REQUIRE("almost_middle" == arr[1]);
          REQUIRE(arr[2] == "almost_back");
          REQUIRE("almost_back" == arr[2]);
          REQUIRE(arr[3] == "back");
          REQUIRE("back" == arr[3]);
          REQUIRE(arr.back() == "back");
          REQUIRE("back" == arr.back());
        }

        DYNAMIC_WHEN("those values are popped from the back", idx) {
          arr.pop_back();
          arr.erase(arr.size() - 1);
          arr.erase(--arr.end());
          DYNAMIC_THEN("only the original content remains", idx) {
            REQUIRE(arr.size() == 1U);
            REQUIRE(arr.front() == arr.back());
            REQUIRE(arr[0] == "middle");
            REQUIRE("middle" == arr[0]);
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
          REQUIRE("nope" == val);
        }
      }

      DYNAMIC_WHEN("when the back is optionally accessed", idx) {
        auto val = arr.back_or("still nope");
        DYNAMIC_THEN("the optional value is returned", idx) {
          REQUIRE(val == "still nope");
          REQUIRE("still nope" == val);
        }
      }

      DYNAMIC_WHEN("some index is optionally accessed", idx) {
        auto val = arr.get_or(15, "wasn't there");
        DYNAMIC_THEN("the optional value is returned", idx) {
          REQUIRE(val == "wasn't there");
          REQUIRE("wasn't there" == val);
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
          REQUIRE("first" == val);
        }
      }

      DYNAMIC_WHEN("the back is optionally accessed", idx) {
        auto val = arr.back_or("not me either");
        DYNAMIC_THEN("the index is returned", idx) {
          REQUIRE(val == "third");
          REQUIRE("third" == val);
        }
      }

      DYNAMIC_WHEN("the middle is optionally accessed", idx) {
        auto val = arr.get_or(1, "lastly, not me");
        DYNAMIC_THEN("the middle is returned", idx) {
          REQUIRE(val == "second");
          REQUIRE("second" == val);
        }
      }
    });
  }
}

SCENARIO("arrays can be resized dynamically", "[type unit]") {
  GIVEN("a statically typed, mutable array") {
    dart::mutable_array_api_test([] (auto tag, auto idx) {
      using array = typename decltype(tag)::type;
      using object = dart::basic_object<typename array::value_type>;

      array arr;
      REQUIRE(arr.size() == 0U);
      auto cap = arr.capacity();
      DYNAMIC_WHEN("we reserve double the current capacity", idx) {
        // Capacity may start at zero.
        arr.reserve((cap * 2) + 1);

        DYNAMIC_THEN("the capacity grows to meet the reservation", idx) {
          REQUIRE(arr.capacity() >= (cap * 2) + 1);
        }
      }

      DYNAMIC_WHEN("we set the size explicitly", idx) {
        arr.resize(7);

        DYNAMIC_THEN("the size/capacity change as expect", idx) {
          REQUIRE(arr.size() == 7U);
          REQUIRE(arr.capacity() >= 7U);
        }
      }

      DYNAMIC_WHEN("we set the size and supply a default value", idx) {
        arr.resize(7, "will it work?");
        DYNAMIC_THEN("all values added are set to it", idx) {
          for (auto v : arr) REQUIRE(v == "will it work?");
        }

        DYNAMIC_WHEN("we finalize the array", idx) {
          auto buff = object {"arr", arr}.lower()["arr"];
          DYNAMIC_THEN("all values added are still set to it", idx) {
            for (auto v : buff) REQUIRE(v == "will it work?");
          }
        }
      }
    });
  }
}

SCENARIO("arrays can set individual indices", "[type unit]") {
  GIVEN("a statically typed, sized, mutable array") {
    dart::mutable_array_api_test([] (auto tag, auto idx) {
      using array = typename decltype(tag)::type;
      using string = dart::basic_string<typename array::value_type>;
      using number = dart::basic_number<typename array::value_type>;
      using flag = dart::basic_flag<typename array::value_type>;
      using null = dart::basic_null<typename array::value_type>;

      array arr;
      arr.resize(4);
      DYNAMIC_WHEN("values are set", idx) {
        arr.set(0, string {"yes"});
        arr.set(number {1}, "no");
        arr.set(2, "stop");
        arr.set(number {3}, string {"go"});
        DYNAMIC_THEN("everything is where we expect", idx) {
          REQUIRE(arr.front() == string {"yes"});
          REQUIRE(string {"yes"} == arr.front());
          REQUIRE(arr[0] == "yes");
          REQUIRE("yes" == arr[0]);
          REQUIRE(arr[number {1}] == "no");
          REQUIRE("no" == arr[number {1}]);
          REQUIRE(arr[2] == "stop");
          REQUIRE("stop" == arr[2]);
          REQUIRE(arr[3] == "go");
          REQUIRE("go" == arr[3]);
          REQUIRE(arr.back() == "go");
          REQUIRE("go" == arr.back());
        }
      }

      DYNAMIC_WHEN("iterators are set", idx) {
        arr.resize(3);
        arr.set(arr.begin(), string {"yes"});
        arr.set(++arr.begin(), "no");
        arr.set(--arr.end(), "stop and go");
        DYNAMIC_THEN("everything is where we expect", idx) {
          REQUIRE(arr.front() == string {"yes"});
          REQUIRE(string {"yes"} == arr.front());
          REQUIRE(arr[0] == "yes");
          REQUIRE("yes" == arr[0]);
          REQUIRE(arr[number {1}] == "no");
          REQUIRE("no" == arr[number {1}]);
          REQUIRE(arr[2] == "stop and go");
          REQUIRE("stop and go" == arr[2]);
          REQUIRE(arr.back() == "stop and go");
          REQUIRE("stop and go" == arr.back());
        }
      }

      DYNAMIC_WHEN("an out of bounds assignment is attempted", idx) {
        DYNAMIC_THEN("it refuses the assignment", idx) {
          REQUIRE_THROWS_AS(arr.set(4, "nope"), std::out_of_range);
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

SCENARIO("strings are regular types", "[type unit]") {
  GIVEN("a statically typed, mutable string") {
    dart::mutable_string_api_test([] (auto tag, auto idx) {
      using string = typename decltype(tag)::type;

      // Validate basic properties.
      string str;
      REQUIRE(str.empty());
      REQUIRE(str.is_str());
      REQUIRE_FALSE(str.is_aggregate());
      REQUIRE_FALSE(str.is_object());
      REQUIRE_FALSE(str.is_integer());
      REQUIRE_FALSE(str.is_decimal());
      REQUIRE_FALSE(str.is_numeric());
      REQUIRE_FALSE(str.is_boolean());
      REQUIRE_FALSE(str.is_null());
      REQUIRE(str.is_primitive());
      REQUIRE(str.size() == 0U);
      REQUIRE(str.get_type() == dart::packet::type::string);

      DYNAMIC_WHEN("the string is copied", idx) {
        auto dup = str;
        DYNAMIC_THEN("the two compare equal", idx) {
          REQUIRE(dup.empty());
          REQUIRE(dup.is_str());
          REQUIRE(dup == str);
          REQUIRE(str == dup);
        }
      }

      DYNAMIC_WHEN("the string is moved", idx) {
        auto moved = std::move(str);
        DYNAMIC_THEN("the two do NOT compare equal", idx) {
          REQUIRE(moved.empty());
          REQUIRE(moved.is_str());
          REQUIRE(moved != str);
          REQUIRE(str != moved);
          REQUIRE(!str.is_str());
          REQUIRE(str.is_null());
          REQUIRE(str.is_primitive());
          REQUIRE_FALSE(static_cast<bool>(str));
        }
      }

      DYNAMIC_WHEN("the string is copied, then moved", idx) {
        auto dup = str;
        auto moved = std::move(dup);
        DYNAMIC_THEN("the two DO compare equal", idx) {
          REQUIRE(str.empty());
          REQUIRE(str.is_str());
          REQUIRE(moved == str);
          REQUIRE(str == moved);
          REQUIRE(str.is_primitive());
          REQUIRE(!dup.is_str());
          REQUIRE(dup.is_primitive());
          REQUIRE(dup.is_null());
          REQUIRE_FALSE(static_cast<bool>(dup));
        }
      }

      DYNAMIC_WHEN("the string is compared against an equivalent, disparate, string", idx) {
        string dup;
        DYNAMIC_THEN("the two compare equal", idx) {
          REQUIRE(dup.empty());
          REQUIRE(dup.is_str());
          REQUIRE(dup == str);
          REQUIRE(str == dup);
        }
      }

      DYNAMIC_WHEN("the string is compared against an inequivalent string", idx) {
        string nope {"not equal"};
        DYNAMIC_THEN("the two do NOT compare equal", idx) {
          REQUIRE(!nope.empty());
          REQUIRE(nope.is_str());
          REQUIRE(nope.size() == 9U);
          REQUIRE(nope != str);
          REQUIRE(str != nope);
        }
      }

      DYNAMIC_WHEN("the string decays to a dynamic type", idx) {
        typename string::value_type dynamic = str;
        DYNAMIC_THEN("the two remain equivalent", idx) {
          REQUIRE(dynamic == str);
          REQUIRE(str == dynamic);
        }
      }
    });
  }
}

SCENARIO("strings can be unwrapped to different machine types", "[type unit]") {
  GIVEN("a statically typed, mutable string with contents") {
    dart::mutable_string_api_test([] (auto tag, auto idx) {
      using string = typename decltype(tag)::type;

      string str {"the rain in spain falls mainly on the plain"};
      DYNAMIC_WHEN("we cast the string to a std::string_view", idx) {
        dart::shim::string_view view {str};
        DYNAMIC_THEN("it compares equal", idx) {
          REQUIRE(view == str);
          REQUIRE(str == view);
        }
      }

      DYNAMIC_WHEN("we explicitly access the string as a std::string_view", idx) {
        auto view = str.strv();
        DYNAMIC_THEN("it compares equal", idx) {
          REQUIRE(view == str);
          REQUIRE(str == view);
        }
      }

      DYNAMIC_WHEN("we cast the string into a std::string", idx) {
        std::string copy {str};
        DYNAMIC_THEN("it compares equal", idx) {
          REQUIRE(copy == str);
          REQUIRE(str == copy);
        }
      }

      DYNAMIC_WHEN("we explicitly access the string as a std::string", idx) {
        std::string copy = str.str();
        DYNAMIC_THEN("it compares equal", idx) {
          REQUIRE(copy == str);
          REQUIRE(str == copy);
        }
      }

      DYNAMIC_WHEN("we dereference the string as a std::string_view", idx) {
        auto view = *str;
        DYNAMIC_THEN("it compares equal", idx) {
          REQUIRE(view == str);
          REQUIRE(str == view);
        }
      }
    });
  }
}

SCENARIO("numbers are regular types", "[type unit]") {
  GIVEN("a statically typed, mutable number") {
    dart::mutable_number_api_test([] (auto tag, auto idx) {
      using number = typename decltype(tag)::type;

      // Validate basic properties.
      number num;
      REQUIRE(num.is_integer());
      REQUIRE(num.is_numeric());
      REQUIRE_FALSE(num.is_aggregate());
      REQUIRE_FALSE(num.is_object());
      REQUIRE_FALSE(num.is_array());
      REQUIRE_FALSE(num.is_str());
      REQUIRE_FALSE(num.is_decimal());
      REQUIRE_FALSE(num.is_boolean());
      REQUIRE_FALSE(num.is_null());
      REQUIRE(num.get_type() == dart::packet::type::integer);

      DYNAMIC_WHEN("the number is copied", idx) {
        auto dup = num;
        DYNAMIC_THEN("the two compare equal", idx) {
          REQUIRE(dup.is_integer());
          REQUIRE(dup == num);
          REQUIRE(num == dup);
        }
      }

      DYNAMIC_WHEN("the number is moved", idx) {
        auto moved = std::move(num);
        DYNAMIC_THEN("the two do NOT compare equal", idx) {
          REQUIRE(moved.is_integer());
          REQUIRE(moved != num);
          REQUIRE(num != moved);
          REQUIRE(!num.is_integer());
          REQUIRE(!num.is_numeric());
          REQUIRE(num.is_null());
          REQUIRE_FALSE(static_cast<bool>(num));
        }
      }

      DYNAMIC_WHEN("the number is copied, then moved", idx) {
        auto dup = num;
        auto moved = std::move(dup);
        DYNAMIC_THEN("the two DO compare equal", idx) {
          REQUIRE(moved.is_integer());
          REQUIRE(moved == num);
          REQUIRE(num == moved);
          REQUIRE(num.is_primitive());
          REQUIRE(!dup.is_integer());
          REQUIRE(dup.is_null());
          REQUIRE_FALSE(static_cast<bool>(dup));
        }
      }

      DYNAMIC_WHEN("the number is compared against an equivalent, disparate, number", idx) {
        number dup;
        DYNAMIC_THEN("the two compare equal", idx) {
          REQUIRE(dup.is_integer());
          REQUIRE(dup == num);
          REQUIRE(num == dup);
        }
      }

      DYNAMIC_WHEN("the number is compared against an inequivalent number", idx) {
        number nope {5};
        DYNAMIC_THEN("the two do NOT compare equal", idx) {
          REQUIRE(nope.is_integer());
          REQUIRE(nope != num);
          REQUIRE(num != nope);
        }
      }

      DYNAMIC_WHEN("the number decays to a dynamic type", idx) {
        typename number::value_type dynamic = num;
        DYNAMIC_THEN("the two remain equivalent", idx) {
          REQUIRE(dynamic == num);
          REQUIRE(num == dynamic);
        }
      }
    });
  }
}

SCENARIO("numbers can be unwrapped to different machine types", "[type unit]") {
  GIVEN("a statically typed, mutable number with contents") {
    dart::mutable_number_api_test([] (auto tag, auto idx) {
      using number = typename decltype(tag)::type;

      number num {2.99792};
      DYNAMIC_WHEN("we cast the number into an int64_t", idx) {
        int64_t val {num};
        DYNAMIC_THEN("the value has been floored", idx) {
          REQUIRE(val == static_cast<int64_t>(num.decimal()));
        }
      }

      DYNAMIC_WHEN("we explicitly access the number as an int64_t", idx) {
        DYNAMIC_THEN("it throws, as its runtime type is decimal", idx) {
          REQUIRE_THROWS_AS(num.integer(), dart::type_error);
        }
      }

      DYNAMIC_WHEN("we cast the number into a double", idx) {
        double val {num};
        DYNAMIC_THEN("it compares equal", idx) {
          REQUIRE(Approx(val) == num.decimal());
        }
      }

      DYNAMIC_WHEN("we explicitly access the number as a double", idx) {
        auto val = num.decimal();
        DYNAMIC_THEN("it compares equal", idx) {
          REQUIRE(Approx(val) == num.decimal());
        }
      }

      DYNAMIC_WHEN("we dereference the number as a double", idx) {
        auto val = *num;
        DYNAMIC_THEN("it compares equal", idx) {
          REQUIRE(Approx(val) == num.decimal());
        }
      }
    });
  }
}

SCENARIO("flags are regular types", "[type unit]") {
  GIVEN("a statically typed, mutable boolean") {
    dart::mutable_flag_api_test([] (auto tag, auto idx) {
      using flag = typename decltype(tag)::type;

      // Validate basic properties.
      flag cond;
      REQUIRE(cond.is_boolean());
      REQUIRE_FALSE(cond.is_aggregate());
      REQUIRE_FALSE(cond.is_object());
      REQUIRE_FALSE(cond.is_array());
      REQUIRE_FALSE(cond.is_integer());
      REQUIRE_FALSE(cond.is_decimal());
      REQUIRE_FALSE(cond.is_numeric());
      REQUIRE_FALSE(cond.is_null());
      REQUIRE(cond.is_primitive());
      REQUIRE_FALSE(static_cast<bool>(cond));
      REQUIRE(cond.get_type() == dart::packet::type::boolean);

      DYNAMIC_WHEN("the flag is copied", idx) {
        auto dup = cond;
        DYNAMIC_THEN("the two compare equal", idx) {
          REQUIRE(dup.is_boolean());
          REQUIRE(dup == cond);
          REQUIRE(cond == dup);
        }
      }

      DYNAMIC_WHEN("the flag is moved", idx) {
        auto moved = std::move(cond);
        DYNAMIC_THEN("the two do NOT compare equal", idx) {
          REQUIRE(moved.is_boolean());
          REQUIRE(moved != cond);
          REQUIRE(cond != moved);
          REQUIRE(!cond.is_boolean());
          REQUIRE(cond.is_null());
          REQUIRE_FALSE(static_cast<bool>(cond));
          REQUIRE_FALSE(static_cast<bool>(moved));
        }
      }

      DYNAMIC_WHEN("the flag is copied, the moved", idx) {
        auto dup = cond;
        auto moved = std::move(dup);
        DYNAMIC_THEN("the two DO compare equal", idx) {
          REQUIRE(cond.is_boolean());
          REQUIRE(cond == moved);
          REQUIRE(moved == cond);
          REQUIRE(!dup.is_boolean());
          REQUIRE(dup.is_null());
          REQUIRE_FALSE(static_cast<bool>(dup));
          REQUIRE_FALSE(static_cast<bool>(cond));
          REQUIRE_FALSE(static_cast<bool>(moved));
        }
      }

      DYNAMIC_WHEN("the flag is compared against an equivalent, disparate, flag", idx) {
        flag dup;
        DYNAMIC_THEN("the two compare equal", idx) {
          REQUIRE(dup.is_boolean());
          REQUIRE(dup == cond);
          REQUIRE(cond == dup);
        }
      }

      DYNAMIC_WHEN("the flag is compared against an inequivalent flag", idx) {
        flag nope {true};
        DYNAMIC_THEN("the two do NOT compare equal", idx) {
          REQUIRE(nope.is_boolean());
          REQUIRE(nope != cond);
          REQUIRE(cond != nope);
          REQUIRE(static_cast<bool>(nope));
          REQUIRE_FALSE(static_cast<bool>(cond));
        }
      }

      DYNAMIC_WHEN("the flag decays to a dynamic type", idx) {
        typename flag::value_type dynamic = cond;
        DYNAMIC_THEN("the two remain equivalent", idx) {
          REQUIRE(dynamic == cond);
          REQUIRE(cond == dynamic);
        }
      }
    });
  }
}

SCENARIO("flags can be unwrapped to different machine types", "[type unit]") {
  GIVEN("a statically typed, mutable flag with contents") {
    dart::mutable_flag_api_test([] (auto tag, auto idx) {
      using flag = typename decltype(tag)::type;

      flag cond {true};
      DYNAMIC_WHEN("we cast the flag into a bool", idx) {
        bool val {cond};
        DYNAMIC_THEN("it compares equal", idx) {
          REQUIRE(val == cond);
          REQUIRE(cond == val);
        }
      }

      DYNAMIC_WHEN("we explicitly access the flag as a bool", idx) {
        auto val = cond.boolean();
        DYNAMIC_THEN("it compares equal", idx) {
          REQUIRE(val == cond);
          REQUIRE(cond == val);
        }
      }

      DYNAMIC_WHEN("we dereference the flag as a bool", idx) {
        auto val = *cond;
        DYNAMIC_THEN("it compares equal", idx) {
          REQUIRE(val == cond);
          REQUIRE(cond == val);
        }
      }
    });
  }
}

SCENARIO("nulls are regular types", "[type unit]") {
  GIVEN("a statically typed null value") {
    dart::mutable_null_api_test([] (auto tag, auto idx) {
      using null = typename decltype(tag)::type;

      // Validate basic properties.
      null none;
      REQUIRE(none.is_null());
      REQUIRE_FALSE(static_cast<bool>(none));
      REQUIRE_FALSE(none.is_aggregate());
      REQUIRE_FALSE(none.is_object());
      REQUIRE_FALSE(none.is_integer());
      REQUIRE_FALSE(none.is_decimal());
      REQUIRE_FALSE(none.is_numeric());
      REQUIRE_FALSE(none.is_boolean());
      REQUIRE(none.is_primitive());
      REQUIRE(none.get_type() == dart::packet::type::null);

      DYNAMIC_WHEN("the null is copied", idx) {
        auto dup = none;
        DYNAMIC_THEN("the two compare equal", idx) {
          REQUIRE(dup.is_null());
          REQUIRE(dup == none);
          REQUIRE(none == dup);
        }
      }

      DYNAMIC_WHEN("the null is moved", idx) {
        auto moved = std::move(none);
        DYNAMIC_THEN("moving null is a no-op, so the two compare equal", idx) {
          REQUIRE(moved.is_null());
          REQUIRE(moved == none);
          REQUIRE(none == moved);
        }
      }

      DYNAMIC_WHEN("the null is copied, then moved", idx) {
        auto dup = none;
        auto moved = std::move(dup);
        DYNAMIC_THEN("moving null is a no-op, so all three compare equal", idx) {
          REQUIRE(dup.is_null());
          REQUIRE(moved.is_null());
          REQUIRE(dup == moved);
          REQUIRE(moved == dup);
          REQUIRE(none == dup);
          REQUIRE(dup == none);
          REQUIRE(none == moved);
          REQUIRE(moved == none);
        }
      }

      DYNAMIC_WHEN("the null is compared against an equivalent, disparate, null", idx) {
        null dup;
        DYNAMIC_THEN("the two compare equal", idx) {
          REQUIRE(dup.is_null());
          REQUIRE(none == dup);
          REQUIRE(dup == none);
        }
      }

      DYNAMIC_WHEN("the null decays to a dynamic type", idx) {
        typename null::value_type dynamic = none;
        DYNAMIC_THEN("the two remain equivalent", idx) {
          REQUIRE(dynamic == none);
          REQUIRE(none == dynamic);
        }
      }
    });
  }
}
