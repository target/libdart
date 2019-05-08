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

SCENARIO("objects can be created", "[object unit]") {
  GIVEN("an object") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      // Get an object.
      auto obj = pkt::make_object();

      // Check to make sure the type agrees.
      REQUIRE(obj.is_object());
      REQUIRE(obj.get_type() == dart::packet::type::object);

      // Check to make sure the object is empty.
      REQUIRE(obj.size() == 0ULL);

      DYNAMIC_WHEN("the object is finalized", idx) {
        auto immutable = obj.finalize();
        DYNAMIC_THEN("basic properties remain the same", idx) {
          // Check to make sure the type agrees.
          REQUIRE(immutable.is_object());
          REQUIRE(immutable.get_type() == dart::packet::type::object);

          // Check to make sure the object is empty.
          REQUIRE(immutable.size() == 0ULL);
        }
      }
    });
  }
}

SCENARIO("objects can be copied", "[object unit]") {
  GIVEN("an object with some fields") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      // Get an object.
      auto obj = pkt::make_object("nested", pkt::make_object("hello", "world"));

      // Different implementations have slightly different reference counting.
      auto counts = dart::shim::compose_together(
        [] (dart::meta::types<dart::buffer, dart::unsafe_buffer, dart::obtuse_buffer>) {
          return std::vector<unsigned> {2, 2, 2, 2, 3, 4, 3};
        },
        [] (dart::meta::not_types<dart::buffer, dart::unsafe_buffer, dart::obtuse_buffer>) {
          return std::vector<unsigned> {2, 2, 1, 2, 1, 3, 2};
        }
      )(obj);

      // Check the initial refcount.
      REQUIRE(obj.refcount() == 1);

      DYNAMIC_WHEN("the object is copied", idx) {
        auto copy = obj;
        DYNAMIC_THEN("its reference count goes up", idx) {
          REQUIRE(obj.refcount() == counts[0]);
          REQUIRE(copy.refcount() == counts[1]);
        }
      }

      DYNAMIC_WHEN("a field is copied", idx) {
        auto nested = obj["nested"];
        DYNAMIC_THEN("the reference count for the field goes up", idx) {
          REQUIRE(obj.refcount() == counts[2]);
          REQUIRE(nested.refcount() == counts[3]);

          // Small string optimization makes this so.
          REQUIRE(nested["hello"].refcount() == counts[4]);
        }
      }

      DYNAMIC_WHEN("a field is copied from the copy", idx) {
        auto copy = obj;
        auto nested = copy["nested"];
        DYNAMIC_THEN("reference counts increase together", idx) {
          REQUIRE(copy["nested"].refcount() == counts[5]);
          REQUIRE(nested.refcount() == counts[6]);
        }
      }

      if (dart::meta::is_packet<pkt>::value) {
        DYNAMIC_WHEN("the object is finalized", idx) {
          auto copy = obj;
          obj.finalize();
          DYNAMIC_THEN("previous copies become independent", idx) {
            REQUIRE(obj.refcount() == 1ULL);
            REQUIRE(copy.refcount() == 1ULL);
          }
        }

        DYNAMIC_WHEN("the object is finalized and then copied", idx) {
          obj.finalize();
          auto copy = obj;
          DYNAMIC_THEN("its reference count goes up", idx) {
            REQUIRE(obj.refcount() == 2ULL);
            REQUIRE(copy.refcount() == 2ULL);
          }
        }
      }
    });
  }
}

SCENARIO("objects can be moved", "[object unit]") {
  GIVEN("an object with some fields") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      // Get an object.
      auto obj = pkt::make_object("nested", pkt::make_object("hello", "world"));

      // Check the initial refcount.
      REQUIRE(obj.refcount() == 1ULL);

      DYNAMIC_WHEN("the object is moved", idx) {
        auto new_obj = std::move(obj);
        DYNAMIC_THEN("its reference count does not change", idx) {
          REQUIRE(obj.refcount() == 0ULL);
          REQUIRE(new_obj.refcount() == 1ULL);
          REQUIRE(obj.get_type() == pkt::type::null);
          REQUIRE(new_obj.get_type() == pkt::type::object);
        }
      }

      DYNAMIC_WHEN("a field is moved", idx) {
        auto nested = obj["nested"];
        auto new_nested = std::move(nested);
        DYNAMIC_THEN("the reference count for the field does not change", idx) {
          REQUIRE(nested.refcount() == 0ULL);
          REQUIRE(new_nested.refcount() == 2ULL);
          REQUIRE(nested.get_type() == pkt::type::null);
          REQUIRE(new_nested.get_type() == pkt::type::object);
        }
      }

      if (dart::meta::is_packet<pkt>::value) {
        DYNAMIC_WHEN("the object is finalized and then moved", idx) {
          auto new_obj = std::move(obj.finalize());
          DYNAMIC_THEN("its reference count does not change", idx) {
            REQUIRE(obj.refcount() == 0ULL);
            REQUIRE(new_obj.refcount() == 1ULL);
            REQUIRE(obj.get_type() == pkt::type::null);
            REQUIRE(new_obj.get_type() == pkt::type::object);
          }
        }

        DYNAMIC_WHEN("the object is moved and then finalized", idx) {
          auto new_obj = std::move(obj).finalize();
          DYNAMIC_THEN("its reference count does not change", idx) {
            REQUIRE(obj.refcount() == 0ULL);
            REQUIRE(new_obj.refcount() == 1ULL);
            REQUIRE(obj.get_type() == pkt::type::null);
            REQUIRE(new_obj.get_type() == pkt::type::object);
          }
        }
      }
    });
  }
}

SCENARIO("finalized objects can be deep copied", "[object unit]") {
  GIVEN("a finalized object with some contents") {
    dart::finalized_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto obj = pkt::make_object("hello", "world!").finalize();
      DYNAMIC_WHEN("the underlying buffer is copied", idx) {
        auto buf = obj.dup_bytes();
        DYNAMIC_THEN("a new packet can be initialized from it", idx) {
          pkt copy(std::move(obj));
        }
      }
    });
  }
}

SCENARIO("aliased objects lazily copy data when mutated", "[object unit]") {
  GIVEN("an object") {
    dart::mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto obj = pkt::make_object();

      DYNAMIC_WHEN("an object is nested inside itself", idx) {
        auto copy = obj;
        obj.add_field("stupid", copy.add_field("stupider", obj));
        DYNAMIC_THEN("it automatically breaks the cycle", idx) {
          REQUIRE(obj.refcount() == 1ULL);
          REQUIRE(copy.refcount() == 2ULL);
        }

        if (dart::meta::is_packet<pkt>::value) {
          DYNAMIC_WHEN("that object is finalized", idx) {
            obj.finalize();
            DYNAMIC_THEN("it behaves as expected", idx) {
              auto nested = obj["stupid"];
              auto doubly_nested = nested["stupider"];
              REQUIRE(obj.refcount() == 3ULL);
              REQUIRE(nested.get_type() == dart::packet::type::object);
              REQUIRE(nested.size() == 1ULL);
              REQUIRE(nested.refcount() == 3ULL);
              REQUIRE(doubly_nested.get_type() == dart::packet::type::object);
              REQUIRE(doubly_nested.size() == 0ULL);
              REQUIRE(doubly_nested.refcount() == 3ULL);
            }
          }
        }
      }
    });
  }
}

SCENARIO("objects can be initialized with contents", "[object unit]") {
  using namespace dart::literals;

  GIVEN("a set of values") {
    dart::simple_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      int val_five = 1, val_six = 2;
      bool val_seven = true, val_eight = false;
      double val_three = 3.14159, val_four = 2.99792;
      std::string val_one = "goodbye", val_two = "no";

      std::string key_three = "pi", key_four = "c";
      std::string key_five = "one", key_six = "two";
      std::string key_one = "hello", key_two = "yes";
      std::string key_seven = "true", key_eight = "false";

      DYNAMIC_WHEN("objects are created from them", idx) {
        auto obj_one = pkt::make_object(key_one, val_one, key_two, val_two);
        auto obj_two = pkt::make_object(key_three, val_three, key_four, val_four);
        auto obj_three = pkt::make_object(key_five, val_five, key_six, val_six);
        auto obj_four = pkt::make_object(key_seven, val_seven, key_eight, val_eight);

        DYNAMIC_THEN("they check out", idx) {
          REQUIRE(obj_one["hello"] == "goodbye");
          REQUIRE(obj_one["yes"_dart] == "no");
          REQUIRE(obj_two["pi"].decimal() == Approx(3.14159));
          REQUIRE(obj_two["c"_dart].decimal() == Approx(2.99792));
          REQUIRE(obj_three["one"] == 1);
          REQUIRE(obj_three["two"_dart] == 2);
          REQUIRE(obj_four["true"]);
          REQUIRE_FALSE(obj_four["false"_dart]);
        }

        if (dart::meta::is_packet<pkt>::value || dart::meta::is_buffer<pkt>::value) {
          DYNAMIC_WHEN("they're finalized", idx) {
            obj_one.finalize();
            obj_two.finalize();
            obj_three.finalize();
            obj_four.finalize();

            DYNAMIC_THEN("they still check out", idx) {
              REQUIRE(obj_one["hello"] == "goodbye");
              REQUIRE(obj_one["yes"_dart] == "no");
              REQUIRE(obj_two["pi"].decimal() == Approx(3.14159));
              REQUIRE(obj_two["c"_dart].decimal() == Approx(2.99792));
              REQUIRE(obj_three["one"] == 1);
              REQUIRE(obj_three["two"_dart] == 2);
              REQUIRE(obj_four["true"]);
              REQUIRE_FALSE(obj_four["false"_dart]);
            }
          }
        }
      }
    });
  }
}

SCENARIO("objects can add all types of values", "[object unit]") {
  using namespace dart::literals;

  GIVEN("a base object") {
    dart::simple_mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto obj = pkt::make_object();

      DYNAMIC_WHEN("we add basically every type of value under the sun", idx) {
        // Try some strings.
        std::string key("hello");
        obj.add_field(key, "goodbye");
        obj.add_field("yes", dart::packet::string("no"));
        obj.add_field(dart::packet::string("stop"), "go");

        // Run the gamut to ensure our overloads behave.
        obj.add_field("", "problems?");
        obj.add_field("int", 42);
        obj.add_field("unsigned", 365U);
        obj.add_field("long", 86400L);
        obj.add_field("unsigned long", 3600UL);
        obj.add_field("long long", 7200LL);
        obj.add_field("unsigned long long", 93000000ULL);
        obj.add_field("pi", 3.14159);
        obj.add_field("c", 2.99792);
        obj.add_field("truth", true);
        obj.add_field("lies", false);
        obj.add_field("absent", nullptr);

        DYNAMIC_THEN("it all checks out", idx) {
          REQUIRE(obj["hello"] == "goodbye");
          REQUIRE(obj["yes"_dart] == "no");
          REQUIRE(obj["stop"] == "go");
          REQUIRE(obj[""_dart] == "problems?");
          REQUIRE(obj["int"] == 42);
          REQUIRE(obj["unsigned"_dart] == 365);
          REQUIRE(obj["long"] == 86400);
          REQUIRE(obj["unsigned long"_dart] == 3600);
          REQUIRE(obj["long long"] == 7200);
          REQUIRE(obj["unsigned long long"_dart] == 93000000);
          REQUIRE(obj["pi"].decimal() == Approx(3.14159));
          REQUIRE(obj["c"_dart].decimal() == Approx(2.99792));
          REQUIRE(obj["truth"]);
          REQUIRE_FALSE(obj["lies"_dart]);
          REQUIRE(obj["absent"].get_type() == dart::packet::type::null);
        }

        if (dart::meta::is_packet<pkt>::value) {
          DYNAMIC_WHEN("the packet is finalized", idx) {
            obj.finalize();

            DYNAMIC_THEN("things still check out", idx) {
              REQUIRE_FALSE(strcmp(obj["hello"].str(), "goodbye"));
              REQUIRE_FALSE(strcmp(obj["yes"_dart].str(), "no"));
              REQUIRE_FALSE(strcmp(obj["stop"].str(), "go"));
              REQUIRE(obj[""_dart] == "problems?");
              REQUIRE(obj["int"].integer() == 42);
              REQUIRE(obj["unsigned"_dart].integer() == 365);
              REQUIRE(obj["long"].integer() == 86400);
              REQUIRE(obj["unsigned long"_dart].integer() == 3600);
              REQUIRE(obj["long long"].integer() == 7200);
              REQUIRE(obj["unsigned long long"_dart].integer() == 93000000);
              REQUIRE(obj["pi"].decimal() == Approx(3.14159));
              REQUIRE(obj["c"_dart].decimal() == Approx(2.99792));
              REQUIRE(obj["truth"].boolean());
              REQUIRE_FALSE(obj["lies"_dart].boolean());
              REQUIRE(obj["absent"].get_type() == dart::packet::type::null);
            }
          }
        }
      }
    });
  }
}

SCENARIO("objects can remove keys", "[object unit]") {
  GIVEN("an object with some keys") {
    dart::mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto obj = pkt::make_object("hello", "world");
      DYNAMIC_WHEN("a key mapping is removed", idx) {
        obj.remove_field("hello");
        DYNAMIC_THEN("it shows as removed", idx) {
          REQUIRE_FALSE(obj.has_key("hello"));
          REQUIRE(obj["hello"].get_type() == dart::packet::type::null);
        }

        DYNAMIC_WHEN("the object is finalized", idx) {
          obj.finalize();
          DYNAMIC_THEN("it still shows as removed", idx) {
            REQUIRE_FALSE(obj.has_key("hello"));
            REQUIRE(obj["hello"].get_type() == dart::packet::type::null);
          }
        }
      }
    });
  }
}

SCENARIO("objects can replace keys", "[object unit]") {
  GIVEN("an object with some keys") {
    dart::mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto obj = pkt::make_object("replaceme", "shouldn't see this");
      DYNAMIC_WHEN("the same key is re-added", idx) {
        obj.add_field("replaceme", "hooray!");
        DYNAMIC_THEN("the newer mapping takes precedence", idx) {
          REQUIRE(obj["replaceme"] == "hooray!");
        }

        DYNAMIC_WHEN("the object is finalized", idx) {
          obj.finalize();
          DYNAMIC_THEN("the changes persist", idx) {
            REQUIRE(obj["replaceme"] == "hooray!");
          }
        }
      }
    });
  }
}

SCENARIO("objects can be compared for equality", "[object unit]") {
  GIVEN("two empty objects") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto obj_one = pkt::make_object(), obj_two = pkt::make_object();
      DYNAMIC_WHEN("an object is compared against itself", idx) {
        DYNAMIC_THEN("it compares equal", idx) {
          REQUIRE(obj_one == obj_one);
        }

        DYNAMIC_WHEN("that object is finalized", idx) {
          obj_one.finalize();
          DYNAMIC_THEN("it still compares equal to itself", idx) {
            REQUIRE(obj_one == obj_one);
          }
        }
      }

      DYNAMIC_WHEN("two disparate objects are compared", idx) {
        DYNAMIC_THEN("they still compare equal", idx) {
          REQUIRE(obj_one == obj_two);
        }

        DYNAMIC_WHEN("they are finalized", idx) {
          obj_one.finalize();
          obj_two.finalize();
          DYNAMIC_THEN("they STILL compare equal", idx) {
            REQUIRE(obj_one == obj_two);
          }
        }
      }

      DYNAMIC_WHEN("one object is assigned to the other", idx) {
        obj_two = obj_one;
        DYNAMIC_THEN("they compare equal", idx) {
          REQUIRE(obj_one == obj_two);
        }

        dart::meta::if_constexpr<
          dart::meta::is_packet<pkt>::value
          ||
          dart::meta::is_heap<pkt>::value,
          true
        >(
          [=] (auto& oo) {
            DYNAMIC_WHEN("one of the objects is modified", idx) {
              oo.add_field("hello", "goodbye");
              DYNAMIC_THEN("they no longer compare equal", idx) {
                REQUIRE(oo != obj_two);
              }
            }
          },
          [] (auto&) {},
          obj_one
        );
      }
    });
  }

  GIVEN("two objects with simple, but identical contents") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto obj_one = pkt::make_object("hello", "world", "one", 1, "two", 2.0, "true", true);
      auto obj_two = pkt::make_object("hello", "world", "one", 1, "two", 2.0, "true", true);
      DYNAMIC_WHEN("they are compared", idx) {
        DYNAMIC_THEN("they compare equal", idx) {
          REQUIRE(obj_one == obj_two);
        }
      }

      DYNAMIC_WHEN("one object is finalized", idx) {
        obj_one.finalize();
        DYNAMIC_THEN("they still compare equal", idx) {
          REQUIRE(obj_one == obj_two);
        }
      }

      DYNAMIC_WHEN("both objects are finalized", idx) {
        obj_one.finalize();
        obj_two.finalize();
        DYNAMIC_THEN("they still compare equal", idx) {
          REQUIRE(obj_one == obj_two);
        }
      }
    });
  }

  GIVEN("two objects with simple, but different contents") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto obj_one = pkt::make_object("hello", "life", "one", 1, "two", 2.0, "true", true);
      auto obj_two = pkt::make_object("hello", "world", "one", 1, "two", 2.0, "true", true);

      DYNAMIC_WHEN("they are compared", idx) {
        DYNAMIC_THEN("they do not compare equal", idx) {
          REQUIRE(obj_one != obj_two);
        }
      }

      DYNAMIC_WHEN("one object is finalized", idx) {
        obj_one.finalize();
        DYNAMIC_THEN("they still do not compare equal", idx) {
          REQUIRE(obj_one != obj_two);
        }
      }

      DYNAMIC_WHEN("both objects are finalized", idx) {
        obj_one.finalize();
        obj_two.finalize();
        DYNAMIC_THEN("they still do not compare equal", idx) {
          REQUIRE(obj_one != obj_two);
        }
      }
    });
  }

  GIVEN("two objects with nested objects") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto obj_one = pkt::make_object("obj", pkt::make_object("yes", "no"), "pi", 3.14159);
      auto obj_two = pkt::make_object("obj", pkt::make_object("yes", "no"), "pi", 3.14159);

      DYNAMIC_WHEN("they are compared", idx) {
        DYNAMIC_THEN("they compare equal", idx) {
          REQUIRE(obj_one == obj_two);
        }
      }

      DYNAMIC_WHEN("one object is finalized", idx) {
        obj_one.finalize();
        DYNAMIC_THEN("they still compare equal", idx) {
          REQUIRE(obj_one == obj_two);
        }
      }

      DYNAMIC_WHEN("both objects are finalized", idx) {
        obj_one.finalize();
        obj_two.finalize();
        DYNAMIC_THEN("they still compare equal", idx) {
          REQUIRE(obj_one == obj_two);
        }
      }
    });
  }

  GIVEN("two objects with nested arrays") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_object("obj", dart::heap::array("yes", "no"), "pi", 3.14159);
      auto obj_one = dart::conversion_helper<pkt>(tmp);
      auto obj_two = dart::conversion_helper<pkt>(tmp);
      DYNAMIC_WHEN("they are compared", idx) {
        DYNAMIC_THEN("they compare equal", idx) {
          REQUIRE(obj_one == obj_two);
        }
      }

      DYNAMIC_WHEN("one object is finalized", idx) {
        obj_one.finalize();
        DYNAMIC_THEN("they still compare equal", idx) {
          REQUIRE(obj_one == obj_two);
        }
      }

      DYNAMIC_WHEN("both objects are finalized", idx) {
        obj_one.finalize();
        obj_two.finalize();
        DYNAMIC_THEN("they still compare equal", idx) {
          REQUIRE(obj_one == obj_two);
        }
      }
    });
  }
}

SCENARIO("objects contextually convert to true", "[object unit]") {
  GIVEN("an object with some contents") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto obj = pkt::make_object("hello", "goodbye");
      DYNAMIC_WHEN("the object is converted to a boolean", idx) {
        auto valid = static_cast<bool>(obj);
        DYNAMIC_THEN("it converts to true", idx) {
          REQUIRE(valid);
        }
      }

      DYNAMIC_WHEN("a field is converted to a boolean", idx) {
        auto valid = static_cast<bool>(obj["hello"]);
        DYNAMIC_THEN("it converts to true", idx) {
          REQUIRE(valid);
        }
      }

      DYNAMIC_WHEN("a non-existent field is converted to a boolean", idx) {
        auto valid = static_cast<bool>(obj["nope"]);
        DYNAMIC_THEN("it converts to false", idx) {
          REQUIRE_FALSE(valid);
        }
      }

      DYNAMIC_WHEN("the object is finalized", idx) {
        obj.finalize();
        DYNAMIC_THEN("it still converts to true", idx) {
          REQUIRE(obj);
        }
      }
    });
  }
}

SCENARIO("objects protect scope of shared resources", "[object unit]") {
  GIVEN("some objects at an initial scope") {
    dart::packet_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      
      pkt fin_out_of_scope, dyn_out_of_scope;
      DYNAMIC_WHEN("those objects are assigned to another that goes out of scope", idx) {
        {
          auto obj = pkt::make_object(), nested = pkt::make_object();
          nested.add_field("nested_key", 1337);
          dyn_out_of_scope = nested;
          obj.add_field("nested_object", nested);

          // Finalize the packet to test the code below.
          obj.finalize();

          // Keep a copy outside of this scope.
          fin_out_of_scope = obj["nested_object"]["nested_key"];
        }

        DYNAMIC_THEN("the objects protect shared resources", idx) {
          REQUIRE(fin_out_of_scope.refcount() == 1ULL);
          REQUIRE(dyn_out_of_scope.refcount() == 1ULL);
        }
      }
    });
  }
}

/*
SCENARIO("objects can only be constructed from aligned pointers", "[object unit]") {
  GIVEN("an unaligned pointer") {
    dart::finalized_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      // Get a smart pointer from our type.
      auto unaligned = pkt::make_object().finalize().share_bytes();

      // Allocate an unaligned region of memory and copy the data in.
      gsl::byte* buf = reinterpret_cast<gsl::byte*>(malloc(2));
      uintptr_t addr = reinterpret_cast<uintptr_t>(buf);
      if (!(addr & sizeof(gsl::byte*))) {
        auto deleter = [] (auto* ptr) { free(ptr - 1); };
        unaligned = std::unique_ptr<gsl::byte, std::function<void (gsl::byte*)>>(buf + 1, deleter);
      } else {
        auto deleter = [] (auto* ptr) { free(ptr); };
        unaligned = std::unique_ptr<gsl::byte, std::function<void (gsl::byte*)>>(buf, deleter);
      }

      DYNAMIC_WHEN("a packet is constructed from that pointer", idx) {
        DYNAMIC_THEN("it refuses construction", idx) {
          REQUIRE_THROWS_AS(pkt(std::move(unaligned)), std::invalid_argument);
        }
      }
    });
  }
}
*/

SCENARIO("objects optimize temporary objects", "[object unit]") {
  GIVEN("an object with some contents") {
    dart::finalized_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto obj = pkt::make_object("hello", "goodbye");
      DYNAMIC_WHEN("accessing that object as a temporary", idx) {
        std::move(obj)["hello"];
        DYNAMIC_THEN("the object is mutated in-place", idx) {
          REQUIRE(obj.is_str());
          REQUIRE(obj == "goodbye");
        }
      }
    });
  }
}

SCENARIO("objects cannot be used as an array", "[object unit]") {
  GIVEN("an object") {
    dart::mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto obj = pkt::make_object();
      DYNAMIC_WHEN("using that object as an array", idx) {
        DYNAMIC_THEN("it refuses to do so", idx) {
          REQUIRE_THROWS_AS(obj.push_back(5), std::logic_error);
          REQUIRE_THROWS_AS(obj[0], std::logic_error);
        }
      }
    });
  }
}

SCENARIO("objects can access nested keys in one step", "[object unit]") {
  GIVEN("an object with nested fields") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      // Get some data to work on.
      auto nested = pkt::make_object("time", "dark side", "come_together", "abbey road");
      auto obj = pkt::make_object("songs", std::move(nested));

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

SCENARIO("objects can check membership for keys", "[object unit]") {
  GIVEN("a set of keys and an object with those keys") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_object();
      std::vector<std::string> keys = {"pi", "e", "avogadro", "c"};
      std::vector<double> values = {3.14159, 2.71828, 6.02214, 2.99792};
      for (size_t i = 0; i < keys.size(); i++) tmp.add_field(keys[i], values[i]);

      auto obj = dart::conversion_helper<pkt>(tmp);
      DYNAMIC_WHEN("checking for keys known to exist", idx) {
        DYNAMIC_THEN("they're reported as being present", idx) {
          for (auto& key : keys) REQUIRE(obj.has_key(key));
        }
      }

      DYNAMIC_WHEN("checking for keys that don't exist", idx) {
        DYNAMIC_THEN("they're reported as absent", idx) {
          REQUIRE_FALSE(obj.has_key("nope"));
        }
      }

      DYNAMIC_WHEN("asking directly for the keys the object maintains", idx) {
        auto direct_keys = obj.keys();
        DYNAMIC_THEN("they're all reported as present", idx) {
          for (auto& key : direct_keys) REQUIRE(obj.has_key(key));
        }
      }
    });
  }
}

SCENARIO("objects have limits on key sizes", "[object unit]") {
  GIVEN("a very long string") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      std::string very_long(1 << 20, '!');
      DYNAMIC_WHEN("that string is used as an object key", idx) {
        DYNAMIC_THEN("it is disallowed", idx) {
          REQUIRE_THROWS_AS(pkt::make_object(very_long, "nope"), std::invalid_argument);
        }
      }
    });
  }
}

SCENARIO("objects can export all current values", "[object unit]") {
  GIVEN("an object with some values") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      std::vector<std::string> orig_keys = {"hello", "goodbye", "yes", "no"};
      std::vector<std::string> orig_vals = {"stop", "go", "yellow", "submarine"};
      auto tmp = dart::heap::make_object("boolean", true, "null", dart::heap::null());
      for (size_t i = 0; i < orig_keys.size(); i++) tmp.add_field(orig_keys[i], orig_vals[i]);

      auto obj = dart::conversion_helper<pkt>(tmp);
      DYNAMIC_WHEN("requesting all currently held values", idx) {
        auto values = obj.values();
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

SCENARIO("objects can be embedded inside each other", "[object unit]") {
  GIVEN("a base object") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto obj = pkt::make_object("key", pkt::make_object());
      DYNAMIC_WHEN("checking the integity of the object heap", idx) {
        DYNAMIC_THEN("it checks out", idx) {
          REQUIRE(obj.is_object());
          REQUIRE(obj.get_type() == dart::packet::type::object);
          REQUIRE(obj.size() == 1ULL);
          REQUIRE(obj.has_key("key"));

          auto embedded = obj["key"];
          REQUIRE(embedded.is_object());
          REQUIRE(embedded.get_type() == dart::packet::type::object);
          REQUIRE(embedded.size() == 0ULL);
        }
      }

      DYNAMIC_WHEN("the object is finalized", idx) {
        obj.finalize();
        DYNAMIC_THEN("it still checks out", idx) {
          REQUIRE(obj.is_object());
          REQUIRE(obj.get_type() == dart::packet::type::object);
          REQUIRE(obj.size() == 1ULL);
          REQUIRE(obj.has_key("key"));

          auto embedded = obj["key"];
          REQUIRE(embedded.is_object());
          REQUIRE(embedded.get_type() == dart::packet::type::object);
          REQUIRE(embedded.size() == 0ULL);
        }
      }
    });
  }
}

SCENARIO("objects can contain arrays", "[object unit]") {
  GIVEN("an object containing an array") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      // dart::buffer doesn't implement dart::buffer::array,
      // so build the packet using dart::heap as an intermediary
      // then convert it into the target packet type.
      auto obj = dart::conversion_helper<pkt>(dart::heap::make_object("key", dart::heap::array()));

      DYNAMIC_WHEN("we check the contents", idx) {
        DYNAMIC_THEN("they check out", idx) {
          REQUIRE(obj.is_object());
          REQUIRE(obj.get_type() == dart::packet::type::object);
          REQUIRE(obj.size() == 1ULL);
          REQUIRE(obj.has_key("key"));

          auto embedded = obj["key"];
          REQUIRE(embedded.is_array());
          REQUIRE(embedded.get_type() == dart::packet::type::array);
          REQUIRE(embedded.size() == 0ULL);
        }
      }

      DYNAMIC_WHEN("the object is finalized", idx) {
        obj.finalize();
        DYNAMIC_THEN("it still checks out", idx) {
          REQUIRE(obj.is_object());
          REQUIRE(obj.get_type() == dart::packet::type::object);
          REQUIRE(obj.size() == 1ULL);
          REQUIRE(obj.has_key("key"));

          auto embedded = obj["key"];
          REQUIRE(embedded.is_array());
          REQUIRE(embedded.get_type() == dart::packet::type::array);
          REQUIRE(embedded.size() == 0ULL);
        }
      }
    });
  }
}

SCENARIO("objects can contain strings", "[object unit]") {
  GIVEN("an object containing a string") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto obj = pkt::make_object("key", "value");
      DYNAMIC_WHEN("we check the contents", idx) {
        DYNAMIC_THEN("they check out", idx) {
          REQUIRE(obj.is_object());
          REQUIRE(obj.get_type() == dart::packet::type::object);
          REQUIRE(obj.size() == 1ULL);
          REQUIRE(obj.has_key("key"));

          auto embedded = obj["key"];
          REQUIRE(embedded.is_str());
          REQUIRE(embedded.get_type() == dart::packet::type::string);
          REQUIRE_FALSE(strcmp(embedded.str(), "value"));
          REQUIRE(embedded.size() == 5ULL);
        }
      }

      DYNAMIC_WHEN("the object is finalized", idx) {
        obj.finalize();
        DYNAMIC_THEN("it still checks out", idx) {
          REQUIRE(obj.is_object());
          REQUIRE(obj.get_type() == dart::packet::type::object);
          REQUIRE(obj.size() == 1ULL);
          REQUIRE(obj.has_key("key"));

          auto embedded = obj["key"];
          REQUIRE(embedded.is_str());
          REQUIRE(embedded.get_type() == dart::packet::type::string);
          REQUIRE_FALSE(strcmp(embedded.str(), "value"));
          REQUIRE(embedded.size() == 5ULL);
        }
      }
    });
  }
}

SCENARIO("objects can contain integers", "[object unit]") {
  GIVEN("an object containing an integer") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto obj = pkt::make_object("key", 1337);
      DYNAMIC_WHEN("we check the contents", idx) {
        DYNAMIC_THEN("they check out", idx) {
          REQUIRE(obj.is_object());
          REQUIRE(obj.get_type() == dart::packet::type::object);
          REQUIRE(obj.size() == 1ULL);
          REQUIRE(obj.has_key("key"));

          // Check our integer.
          auto embedded = obj["key"];
          REQUIRE(embedded.is_integer());
          REQUIRE(embedded.get_type() == dart::packet::type::integer);
          REQUIRE(embedded.integer() == 1337);
        }
      }

      DYNAMIC_WHEN("the object is finalized", idx) {
        obj.finalize();
        DYNAMIC_THEN("it still checks out", idx) {
          REQUIRE(obj.is_object());
          REQUIRE(obj.get_type() == dart::packet::type::object);
          REQUIRE(obj.size() == 1ULL);
          REQUIRE(obj.has_key("key"));

          auto embedded = obj["key"];
          REQUIRE(embedded.is_integer());
          REQUIRE(embedded.get_type() == dart::packet::type::integer);
          REQUIRE(embedded.integer() == 1337);
        }
      }
    });
  }
}

SCENARIO("objects can contain floating pointer numbers", "[object unit]") {
  GIVEN("an object containing a float") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto obj = pkt::make_object("key", 3.14159);
      DYNAMIC_WHEN("we check the contents", idx) {
        DYNAMIC_THEN("they check out", idx) {
          REQUIRE(obj.is_object());
          REQUIRE(obj.get_type() == dart::packet::type::object);
          REQUIRE(obj.size() == 1ULL);
          REQUIRE(obj.has_key("key"));

          auto embedded = obj["key"];
          REQUIRE(embedded.is_decimal());
          REQUIRE(embedded.get_type() == dart::packet::type::decimal);
          REQUIRE(embedded.decimal() == 3.14159);
        }
      }

      DYNAMIC_WHEN("the object is finalized", idx) {
        obj.finalize();
        DYNAMIC_THEN("it still checks out", idx) {
          REQUIRE(obj.is_object());
          REQUIRE(obj.get_type() == dart::packet::type::object);
          REQUIRE(obj.size() == 1ULL);
          REQUIRE(obj.has_key("key"));

          auto embedded = obj["key"];
          REQUIRE(embedded.is_decimal());
          REQUIRE(embedded.get_type() == dart::packet::type::decimal);
          REQUIRE(embedded.decimal() == 3.14159);
        }
      }
    });
  }
}

SCENARIO("objects can contain booleans", "[object unit]") {
  GIVEN("an object containing a boolean") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto obj = pkt::make_object("key", true);
      DYNAMIC_WHEN("we check the contents", idx) {
        DYNAMIC_THEN("they check out", idx) {
          REQUIRE(obj.is_object());
          REQUIRE(obj.get_type() == dart::packet::type::object);
          REQUIRE(obj.size() == 1ULL);
          REQUIRE(obj.has_key("key"));

          auto embedded = obj["key"];
          REQUIRE(embedded.is_boolean());
          REQUIRE(embedded.get_type() == dart::packet::type::boolean);
          REQUIRE(embedded.boolean());
        }
      }

      DYNAMIC_WHEN("the object is finalized", idx) {
        obj.finalize();
        DYNAMIC_THEN("it still checks out", idx) {
          REQUIRE(obj.is_object());
          REQUIRE(obj.get_type() == dart::packet::type::object);
          REQUIRE(obj.size() == 1ULL);
          REQUIRE(obj.has_key("key"));

          auto embedded = obj["key"];
          REQUIRE(embedded.is_boolean());
          REQUIRE(embedded.get_type() == dart::packet::type::boolean);
          REQUIRE(embedded.boolean());
        }
      }
    });
  }
}

SCENARIO("objects can contain nulls", "[object unit]") {
  GIVEN("an object containing a null") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto obj = pkt::make_object("key", pkt::make_null());
      DYNAMIC_WHEN("we check the contents", idx) {
        DYNAMIC_THEN("they check out", idx) {
          REQUIRE(obj.is_object());
          REQUIRE(obj.get_type() == dart::packet::type::object);
          REQUIRE(obj.size() == 1ULL);
          REQUIRE(obj.has_key("key"));

          auto embedded = obj["key"];
          REQUIRE(embedded.is_null());
          REQUIRE(embedded.get_type() == dart::packet::type::null);
        }
      }

      DYNAMIC_WHEN("the object is finalized", idx) {
        obj.finalize();
        DYNAMIC_THEN("it still checks out", idx) {
          REQUIRE(obj.is_object());
          REQUIRE(obj.get_type() == dart::packet::type::object);
          REQUIRE(obj.size() == 1ULL);
          REQUIRE(obj.has_key("key"));

          auto embedded = obj["key"];
          REQUIRE(embedded.is_null());
          REQUIRE(embedded.get_type() == dart::packet::type::null);
        }
      }
    });
  }
}

SCENARIO("objects can optionally access non-existent keys with a fallback", "[object unit]") {
  using namespace dart::literals;

  GIVEN("an object without any keys") {
    dart::mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto obj = pkt::make_object();
      DYNAMIC_WHEN("we attempt to optionally access a non-existent key", idx) {
        auto key = dart::conversion_helper<pkt>("nope"_dart);
        auto opt_one = obj.get_or("nope", 1);
        auto opt_two = obj.get_or(key, 1.0);
        auto opt_three = obj.get_or("nope", "not here");
        auto opt_four = obj.get_or(key, false);
        auto opt_five = obj.get_or("nope", pkt::make_object());

        DYNAMIC_THEN("it returns the optional value", idx) {
          REQUIRE(opt_one == 1);
          REQUIRE(opt_two == 1.0);
          REQUIRE(opt_three == "not here");
          REQUIRE(opt_four == false);
          REQUIRE(opt_five == pkt::make_object());
        }
      }

      DYNAMIC_WHEN("we attempt to optionally access a non-existent key on a temporary", idx) {
        auto key = dart::conversion_helper<pkt>("double_nope"_dart);
        auto opt_one = obj["nope"].get_or("double_nope", 1);
        auto opt_two = obj["nope"].get_or(key, 1.0);
        auto opt_three = obj["nope"].get_or("double_nope", "not here");
        auto opt_four = obj["nope"].get_or(key, false);
        auto opt_five = obj["nope"].get_or("double_nope", pkt::make_object());

        DYNAMIC_THEN("it returns the optional value", idx) {
          REQUIRE(opt_one == 1);
          REQUIRE(opt_two == 1.0);
          REQUIRE(opt_three == "not here");
          REQUIRE(opt_four == false);
          REQUIRE(opt_five == pkt::make_object());
        }
      }

      DYNAMIC_WHEN("the object is finalized", idx) {
        obj.finalize();
        auto key = dart::conversion_helper<pkt>("nope"_dart);
        auto opt_one = obj.get_or("nope", 1);
        auto opt_two = obj.get_or(key, 1.0);
        auto opt_three = obj.get_or("nope", "not here");
        auto opt_four = obj.get_or(key, false);
        auto opt_five = obj.get_or("nope", pkt::make_object());

        DYNAMIC_THEN("it still behaves as expected", idx) {
          REQUIRE(opt_one == 1);
          REQUIRE(opt_two == 1.0);
          REQUIRE(opt_three == "not here");
          REQUIRE(opt_four == false);
          REQUIRE(opt_five == pkt::make_object());
        }
      }
    });
  }
}
