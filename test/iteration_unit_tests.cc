/*----- System Includes -----*/

#include <dart.h>
#include <unordered_map>
#include <unordered_set>
#include <extern/catch.h>

/*----- Function Implementations -----*/

SCENARIO("iterators are regular", "[iteration unit]") {
  auto str = dart::shim::string_view {"alongenoughstringtoovercomesso"};
  GIVEN("an empty iterator") {
    dart::packet::iterator it;

    THEN("its initial properties make sense") {
      REQUIRE_FALSE(static_cast<bool>(it));
    }

    WHEN("the iterator is copied") {
      auto copy = it;
      THEN("it produces another empty iterator") {
        REQUIRE_FALSE(static_cast<bool>(copy));
        REQUIRE(it == copy);
        REQUIRE(copy == it);
      }
    }

    WHEN("the iterator is assigned to") {
      auto obj = dart::packet::make_object("hello", str);
      it = obj.begin();
      THEN("it correctly models the new iterator") {
        REQUIRE(static_cast<bool>(it));
        REQUIRE(it->strv() == str);
        REQUIRE(*it == str);
        REQUIRE(it->refcount() == 2ULL);
      }

      WHEN("the iterator is moved from") {
        auto moved = std::move(it);
        THEN("all of its properties transfer") {
          REQUIRE_FALSE(static_cast<bool>(it));
          REQUIRE(static_cast<bool>(moved));
          REQUIRE_FALSE(moved == it);
          REQUIRE_FALSE(it == moved);
          REQUIRE(moved->strv() == str);
          REQUIRE(*moved == str);
          REQUIRE(moved->refcount() == 2ULL);
        }
      }
    }
  }

  GIVEN("an iterator with a non-finalized value") {
    auto obj = dart::packet::make_object("hello", str);
    auto it = obj.begin();

    THEN("its initial properties make sense") {
      REQUIRE(static_cast<bool>(it));
      REQUIRE(it->strv() == str);
      REQUIRE(*it == str);
    }

    WHEN("the iterator is copied") {
      auto copy = it;
      THEN("it produces an identical iterator") {
        REQUIRE(static_cast<bool>(copy));
        REQUIRE(copy == it);
        REQUIRE(it == copy);
        REQUIRE(copy->strv() == str);
        REQUIRE(*copy == str);
        REQUIRE(it->refcount() == 2ULL);
        REQUIRE(copy->refcount() == 2ULL);
      }
    }

    WHEN("the iterator is moved from") {
      auto moved = std::move(it);
      THEN("all of its properties transfer") {
        REQUIRE_FALSE(static_cast<bool>(it));
        REQUIRE(static_cast<bool>(moved));
        REQUIRE_FALSE(it == moved);
        REQUIRE_FALSE(moved == it);
        REQUIRE(moved->strv() == str);
        REQUIRE(*moved == str);
        REQUIRE(moved->refcount() == 2ULL);
      }
    }

    auto another = dart::shim::string_view {"anotherstringlongenoughtoovercomesso"};
    WHEN("the iterator is assigned to") {
      auto second = dart::packet::make_object("hello", another);
      it = second.begin();
      THEN("all of its properties are updated") {
        REQUIRE(static_cast<bool>(it));
        REQUIRE(it->strv() == another);
        REQUIRE(*it == another);
        REQUIRE(it->refcount() == 2ULL);
      }
    }

    WHEN("the iterator is assigned to from a finalized iterator") {
      auto second = dart::packet::make_object("hello", another).finalize();
      it = second.begin();
      THEN("nothing explodes") {
        REQUIRE(static_cast<bool>(it));
        REQUIRE(it->strv() == another);
        REQUIRE(*it == another);
        REQUIRE(it->refcount() == 3ULL);
      }
    }
  }

  GIVEN("an iterator with a finalized value") {
    auto obj = dart::packet::make_object("hello", str).finalize();
    auto it = obj.begin();

    THEN("its initial properties make sense") {
      REQUIRE(static_cast<bool>(it));
      REQUIRE(it->strv() == str);
      REQUIRE(*it == str);
    }

    WHEN("the iterator is copied") {
      auto copy = it;
      THEN("it produces an identical iterator") {
        REQUIRE(static_cast<bool>(copy));
        REQUIRE(copy == it);
        REQUIRE(it == copy);
        REQUIRE(copy->strv() == str);
        REQUIRE(*copy == str);
        REQUIRE(copy->refcount() == 4ULL);
        REQUIRE(it->refcount() == 4ULL);
      }
    }

    WHEN("the iterator is moved from") {
      auto moved = std::move(it);
      THEN("all of its properties transfer") {
        REQUIRE_FALSE(static_cast<bool>(it));
        REQUIRE(static_cast<bool>(moved));
        REQUIRE_FALSE(it == moved);
        REQUIRE_FALSE(moved == it);
        REQUIRE(moved->strv() == str);
        REQUIRE(*moved == str);
        REQUIRE(moved->refcount() == 3ULL);
      }
    }

    auto another = dart::shim::string_view {"anotherstringlongenoughtoovercomesso"};
    WHEN("the iterator is assigned to") {
      auto second = dart::packet::make_object("hello", another).finalize();
      it = second.begin();
      THEN("all of its properties are updated") {
        REQUIRE(static_cast<bool>(it));
        REQUIRE(it->strv() == another);
        REQUIRE(*it == another);
        REQUIRE(it->refcount() == 3ULL);
      }
    }

    WHEN("the iterator is assigned to from a non-finalized iterator") {
      auto second = dart::packet::make_object("hello", another);
      it = second.begin();
      THEN("nothing explodes") {
        REQUIRE(static_cast<bool>(it));
        REQUIRE(it->strv() == another);
        REQUIRE(*it == another);
        REQUIRE(it->refcount() == 2ULL);
      }
    }
  }
}

SCENARIO("iterators can iterate over objects", "[iteration unit]") {
  GIVEN("a packet with some contents") {
    // Create a base object.
    auto obj = dart::packet::make_object();

    // Define some key-value pairs and add them to our object.
    std::unordered_map<std::string, std::string> fields = {
      {"hello", "goodbye"},
      {"yes", "no"},
      {"stop", "go"}
    };

    WHEN("iterating over the object's keys") {
      for (auto& field : fields) obj.add_field(field.first, field.second);
      for (auto it = obj.key_begin(); it != obj.key_end(); ++it) fields.erase(it->str());
      THEN("all keys are visited") {
        REQUIRE(fields.size() == 0ULL);
        REQUIRE(fields.empty());
      }
    }

    WHEN("iterating over the object's values") {
      for (auto& field : fields) obj.add_field(field.second, field.first);
      for (auto v : obj) fields.erase(v.str());
      THEN("all values are visited") {
        REQUIRE(fields.size() == 0ULL);
        REQUIRE(fields.empty());
      }
    }

    WHEN("simultaneously iterating over keys and values") {
      for (auto& field : fields) obj.add_field(field.first, field.second);
      THEN("all pairs check out") {
        dart::packet::iterator k, v;
        std::tie(k, v) = obj.kvbegin();
        while (v != obj.end()) {
          REQUIRE(fields[k->str()] == *v);
          ++k, ++v;
        }
      }
    }
  }

  GIVEN("a finalized packet with some contents") {
    // Create a base object.
    auto obj = dart::packet::make_object();

    // Define some key-value pairs and add them to our object.
    std::unordered_map<std::string, std::string> fields = {
      {"hello", "goodbye"},
      {"yes", "no"},
      {"stop", "go"}
    };

    WHEN("iterating over the object's keys") {
      for (auto& field : fields) obj.add_field(field.first, field.second);
      obj.finalize();
      for (auto it = obj.key_begin(); it != obj.key_end(); ++it) fields.erase(it->str());
      THEN("all keys are visited") {
        REQUIRE(fields.size() == 0ULL);
        REQUIRE(fields.empty());
      }
    }

    WHEN("iterating over the object's values") {
      for (auto& field : fields) obj.add_field(field.second, field.first);
      obj.finalize();
      for (auto v : obj) fields.erase(v.str());
      THEN("all values are visited") {
        REQUIRE(fields.size() == 0ULL);
        REQUIRE(fields.empty());
      }
    }

    WHEN("simultaneously iterating over keys and values") {
      for (auto& field : fields) obj.add_field(field.first, field.second);
      obj.finalize();
      THEN("all pairs check out") {
        dart::packet::iterator k, v;
        std::tie(k, v) = obj.kvbegin();
        while (v != obj.end()) {
          REQUIRE(fields[k->str()] == *v);
          ++k, ++v;
        }
      }
    }
  }
}

SCENARIO("iterators can iterate over array elements", "[iteration unit]") {
  GIVEN("an array with some contents") {
    // Define some keys and add them to our object.
    auto arr = dart::packet::make_array();
    std::unordered_set<std::string> elems = {"hello", "yes", "stop"};
    for (auto const& elem : elems) arr.push_back(elem);

    WHEN("iterating over the array's values") {
      THEN("an iterator visits all elements") {
        for (auto const& elem : arr) elems.erase(elem.str());
        REQUIRE(elems.size() == 0ULL);
      }
    }

    WHEN("that packet is finalized") {
      arr = dart::packet::make_object("arr", arr).finalize()["arr"];
      THEN("an iterator still visits all elements") {
        for (auto const& elem : arr) elems.erase(elem.str());
        REQUIRE(elems.size() == 0ULL);
      }
    }
  }
}
