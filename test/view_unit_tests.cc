/*----- System Includes -----*/

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>

/*----- Local Includes -----*/

#include "dart_tests.h"

/*----- Function Implementations -----*/

SCENARIO("views can be created", "[view unit]") {
  GIVEN("an object") {
    dart::api_test([] (auto tag, auto) {
      using pkt = typename decltype(tag)::type;
      using view = typename pkt::view;

      // Get an object.
      auto obj = pkt::make_object();
      view obj_view = obj;

      // Check to make sure the type agrees.
      REQUIRE(obj_view.is_object());
      REQUIRE(obj_view.get_type() == dart::packet::type::object);

      // Check to make sure the object is empty.
      REQUIRE(obj_view.size() == 0ULL);
    });
  }
}

SCENARIO("views can be copied", "[view unit]") {
  GIVEN("an object with some fields") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      using view = typename pkt::view;

      // Get an object.
      auto obj = pkt::make_object("nested", pkt::make_object("hello", "world"));
      view obj_view = obj;

      // Check the initial refcount.
      REQUIRE(obj.refcount() == 1);
      REQUIRE(obj_view.refcount() == 1);

      DYNAMIC_WHEN("the view is copied", idx) {
        auto copy = obj_view;
        DYNAMIC_THEN("its reference count does not change", idx) {
          REQUIRE(obj_view.refcount() == 1U);
          REQUIRE(copy.refcount() == 1U);
        }
      }

      DYNAMIC_WHEN("a field is copied", idx) {
        auto nested = obj_view["nested"];
        DYNAMIC_THEN("reference counts do not change", idx) {
          REQUIRE(obj_view.refcount() == 1U);
          REQUIRE(nested.refcount() == 1U);
          REQUIRE(nested["hello"].refcount() == 1U);
        }
      }

      DYNAMIC_WHEN("a field is copied from the copy", idx) {
        auto copy = obj_view;
        auto nested = copy["nested"];
        DYNAMIC_THEN("reference counts do not change", idx) {
          REQUIRE(copy["nested"].refcount() == 1U);
          REQUIRE(nested.refcount() == 1U);
        }
      }
    });
  }
}

SCENARIO("views can be moved", "[view unit]") {
  GIVEN("an object with some fields") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      using view = typename pkt::view;

      // Get an object.
      auto obj = pkt::make_object("nested", pkt::make_object("hello", "world"));
      view obj_view = obj;

      // Check the initial refcount.
      REQUIRE(obj.refcount() == 1ULL);
      REQUIRE(obj_view.refcount() == 1ULL);

      DYNAMIC_WHEN("the object is moved", idx) {
        auto new_obj = std::move(obj_view);
        DYNAMIC_THEN("its reference count does not change", idx) {
          REQUIRE(obj_view.refcount() == 0ULL);
          REQUIRE(new_obj.refcount() == 1ULL);
          REQUIRE(obj_view.get_type() == pkt::type::null);
          REQUIRE(new_obj.get_type() == pkt::type::object);
        }
      }

      DYNAMIC_WHEN("a field is moved", idx) {
        auto nested = obj_view["nested"];
        auto new_nested = std::move(nested);
        DYNAMIC_THEN("the reference count for the field does not change", idx) {
          REQUIRE(nested.refcount() == 0ULL);
          REQUIRE(new_nested.refcount() == 1ULL);
          REQUIRE(nested.get_type() == pkt::type::null);
          REQUIRE(new_nested.get_type() == pkt::type::object);
        }
      }

    });
  }
}

SCENARIO("finalized views can be deep copied", "[view unit]") {
  GIVEN("a finalized object with some contents") {
    dart::finalized_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      using view = typename pkt::view;

      auto obj = pkt::make_object("hello", "world!").finalize();
      view obj_view = obj;
      DYNAMIC_WHEN("the underlying buffer is copied", idx) {
        auto buf = obj_view.dup_bytes();
        DYNAMIC_THEN("a new packet can be initialized from it", idx) {
          pkt copy(std::move(buf));
        }
      }
    });
  }
}

SCENARIO("views can be compared for equality", "[view unit]") {
  GIVEN("two empty objects") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      using view = typename pkt::view;

      auto obj_one = pkt::make_object(), obj_two = pkt::make_object();
      view view_one = obj_one, view_two = obj_two;
      DYNAMIC_WHEN("an object is compared against itself", idx) {
        DYNAMIC_THEN("it compares equal", idx) {
          REQUIRE(view_one == view_one);
        }
      }

      DYNAMIC_WHEN("two disparate objects are compared", idx) {
        DYNAMIC_THEN("they still compare equal", idx) {
          REQUIRE(view_one == view_two);
        }
      }

      DYNAMIC_WHEN("one object is assigned to the other", idx) {
        view_two = view_one;
        DYNAMIC_THEN("they compare equal", idx) {
          REQUIRE(obj_one == obj_two);
        }
      }
    });
  }

  GIVEN("two objects with simple, but identical contents") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      using view = typename pkt::view;

      auto obj_one = pkt::make_object("hello", "world", "one", 1, "two", 2.0, "true", true);
      auto obj_two = pkt::make_object("hello", "world", "one", 1, "two", 2.0, "true", true);
      view view_one = obj_one, view_two = obj_two;
      DYNAMIC_WHEN("they are compared", idx) {
        DYNAMIC_THEN("they compare equal", idx) {
          REQUIRE(view_one == view_two);
        }
      }
    });
  }

  GIVEN("two objects with simple, but different contents") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      using view = typename pkt::view;

      auto obj_one = pkt::make_object("hello", "life", "one", 1, "two", 2.0, "true", true);
      auto obj_two = pkt::make_object("hello", "world", "one", 1, "two", 2.0, "true", true);
      view view_one = obj_one, view_two = obj_two;

      DYNAMIC_WHEN("they are compared", idx) {
        DYNAMIC_THEN("they do not compare equal", idx) {
          REQUIRE(view_one != view_two);
        }
      }
    });
  }

  GIVEN("two objects with nested objects") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      using view = typename pkt::view;

      auto obj_one = pkt::make_object("obj", pkt::make_object("yes", "no"), "pi", 3.14159);
      auto obj_two = pkt::make_object("obj", pkt::make_object("yes", "no"), "pi", 3.14159);
      view view_one = obj_one, view_two = obj_two;

      DYNAMIC_WHEN("they are compared", idx) {
        DYNAMIC_THEN("they compare equal", idx) {
          REQUIRE(view_one == view_two);
        }
      }
    });
  }
}

SCENARIO("views contextually convert to true", "[view unit]") {
  GIVEN("an object with some contents") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      using view = typename pkt::view;

      auto obj = pkt::make_object("hello", "goodbye");
      view obj_view = obj;
      DYNAMIC_WHEN("the object is converted to a boolean", idx) {
        auto valid = static_cast<bool>(obj_view);
        DYNAMIC_THEN("it converts to true", idx) {
          REQUIRE(valid);
        }
      }

      DYNAMIC_WHEN("a field is converted to a boolean", idx) {
        auto valid = static_cast<bool>(obj_view["hello"]);
        DYNAMIC_THEN("it converts to true", idx) {
          REQUIRE(valid);
        }
      }

      DYNAMIC_WHEN("a non-existent field is converted to a boolean", idx) {
        auto valid = static_cast<bool>(obj_view["nope"]);
        DYNAMIC_THEN("it converts to false", idx) {
          REQUIRE_FALSE(valid);
        }
      }
    });
  }
}

SCENARIO("finalized views always return buffers for the current object", "[view unit]") {
  GIVEN("an object with some contents") {
    dart::finalized_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      using view = typename pkt::view;

      auto obj = pkt::make_object("nested", pkt::make_object("data", "value")).finalize();
      view obj_view = obj;
      DYNAMIC_WHEN("a nested object is accessed", idx) {
        auto nested = obj_view["nested"];
        DYNAMIC_THEN("it returns its own network buffer", idx) {
          pkt dup {nested.get_bytes()};
          view dup_view = dup;
          REQUIRE(dup_view == nested);
          REQUIRE(dup_view["data"] == nested["data"]);
        }
      }
    });
  }
}

SCENARIO("object views cannot be used as an array", "[view unit]") {
  GIVEN("an object") {
    dart::mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      using view = typename pkt::view;

      auto obj = pkt::make_object();
      view obj_view = obj;
      DYNAMIC_WHEN("using that object as an array", idx) {
        DYNAMIC_THEN("it refuses to do so", idx) {
          REQUIRE_THROWS_AS(obj_view.back(), std::logic_error);
          REQUIRE_THROWS_AS(obj_view[0], std::logic_error);
        }
      }
    });
  }
}

SCENARIO("object views can access nested keys in one step", "[view unit]") {
  GIVEN("an object with nested fields") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      using view = typename pkt::view;

      // Get some data to work on.
      auto nested = pkt::make_object("time", "dark side", "come_together", "abbey road");
      auto obj = pkt::make_object("songs", std::move(nested));
      view obj_view = obj;

      DYNAMIC_WHEN("accessing a valid nested field", idx) {
        auto dark_side = obj_view.get_nested("songs.time");
        auto abbey_road = obj_view.get_nested("songs.come_together");
        DYNAMIC_THEN("it returns the correct value", idx) {
          REQUIRE(dark_side == "dark side");
          REQUIRE(abbey_road == "abbey road");
        }
      }

      DYNAMIC_WHEN("accessing an invalid path", idx) {
        auto nested = obj_view.get_nested("songs.not_here");
        auto bad_nested = obj_view.get_nested(".songs..definitely_not_here.");
        DYNAMIC_THEN("it returns null", idx) {
          REQUIRE(nested.is_null());
          REQUIRE(bad_nested.is_null());
        }
      }

      DYNAMIC_WHEN("accessing a path prefix", idx) {
        auto nested = obj_view.get_nested("song");
        DYNAMIC_THEN("it returns null", idx) {
          REQUIRE(nested.is_null());
        }
      }
    });
  }
}

SCENARIO("object views can check membership for keys", "[view unit]") {
  GIVEN("a set of keys and an object with those keys") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      using view = typename pkt::view;

      auto tmp = dart::heap::make_object();
      std::vector<std::string> keys = {"pi", "e", "avogadro", "c"};
      std::vector<double> values = {3.14159, 2.71828, 6.02214, 2.99792};
      for (size_t i = 0; i < keys.size(); i++) tmp.add_field(keys[i], values[i]);

      auto obj = dart::conversion_helper<pkt>(tmp);
      view obj_view = obj;
      DYNAMIC_WHEN("checking for keys known to exist", idx) {
        DYNAMIC_THEN("they're reported as being present", idx) {
          for (auto& key : keys) REQUIRE(obj_view.has_key(key));
        }
      }

      DYNAMIC_WHEN("checking for keys that don't exist", idx) {
        DYNAMIC_THEN("they're reported as absent", idx) {
          REQUIRE_FALSE(obj_view.has_key("nope"));
        }
      }

      DYNAMIC_WHEN("asking directly for the keys the object maintains", idx) {
        auto direct_keys = obj_view.keys();
        DYNAMIC_THEN("they're all reported as present", idx) {
          for (auto& key : direct_keys) REQUIRE(obj_view.has_key(key));
        }
      }
    });
  }
}

SCENARIO("object views can export all current values", "[view unit]") {
  GIVEN("an object with some values") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      using view = typename pkt::view;

      std::vector<std::string> orig_keys = {"hello", "goodbye", "yes", "no"};
      std::vector<std::string> orig_vals = {"stop", "go", "yellow", "submarine"};
      auto tmp = dart::heap::make_object("boolean", true, "null", dart::heap::null());
      for (size_t i = 0; i < orig_keys.size(); i++) tmp.add_field(orig_keys[i], orig_vals[i]);

      auto obj = dart::conversion_helper<pkt>(tmp);
      view obj_view = obj;
      DYNAMIC_WHEN("requesting all currently held values", idx) {
        auto values = obj_view.values();
        DYNAMIC_THEN("it returns the full set", idx) {
          REQUIRE(values.size() == orig_vals.size() + 2);

          for (auto const& val : values) {
            if (val.is_str()) {
              std::string value(val.str());
              REQUIRE(std::find(orig_vals.begin(), orig_vals.end(), value) != orig_vals.end());
            } else if (val.is_boolean()) {
              REQUIRE(val.boolean());
            } else {
              REQUIRE(val.get_type() == dart::packet::type::null);
            }
          }
        }
      }
    });
  }
}
