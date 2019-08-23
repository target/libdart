/*----- Local Includes -----*/

#include "dart_tests.h"
#include "../include/dart/support/string_view.h"

/*----- System Includes -----*/

#include <string>

#if DART_HAS_CPP17
#include <string_view>
#endif

/*----- Function Implementations -----*/

SCENARIO("string views can be created", "[string view unit]") {
  GIVEN("a default constructed string view") {
    dart::string_view view;

    THEN("its basic properties make sense") {
      REQUIRE(view.empty());
      REQUIRE(view.size() == 0U);
      REQUIRE(view.length() == 0U);
    }
  }

  GIVEN("a string view with a value") {
    auto& msg = "hello world";
    dart::string_view view = msg;

    THEN("its basic properties make sense") {
      REQUIRE(!view.empty());
      REQUIRE(view.size() == (sizeof(msg) - 1));
      REQUIRE(view == msg);
      REQUIRE(msg == view);
      REQUIRE(view.starts_with(msg));
      REQUIRE(view.ends_with(msg));
    }
  }
}

SCENARIO("string views can be copied", "[string view unit]") {
  GIVEN("a string view with an initial value") {
    auto& msg = "testing 1, 2, 3";
    dart::string_view view = msg;

    WHEN("when the view is copied") {
      auto dup = view;
      THEN("all of its properties carry over") {
        REQUIRE(dup == view);
        REQUIRE(view == dup);
        REQUIRE(view == msg);
        REQUIRE(msg == view);
        REQUIRE(dup == msg);
        REQUIRE(msg == dup);
        REQUIRE(dup.starts_with(view));
        REQUIRE(view.starts_with(dup));
        REQUIRE(dup.starts_with(msg));
        REQUIRE(dup.size() == view.size());
      }
    }
  }
}

SCENARIO("string views can be created from a variety of types", "[string view unit]") {
  GIVEN("a raw C character array") {
    auto& str = "the rain in Spain stays mainly on the plains";
    dart::string_view view = str;
    THEN("the view is identical to it") {
      REQUIRE(view == str);
      REQUIRE(str == view);
      REQUIRE(view.size() == (sizeof(str) - 1));
    }
  }

  GIVEN("a standard string") {
    std::string str = "Extraordinary claims require extraordinary evidence";
    dart::string_view view = str;
    THEN("the view is identical to it") {
      REQUIRE(str == view);
      REQUIRE(view == str);
    }
  }

#if DART_HAS_CPP17
  GIVEN("a standard string view") {
    std::string_view str = "Every problem in computer science can be solved with another level of indrection";
    dart::string_view view = str;

    THEN("the view is identical to it") {
      REQUIRE(str == view);
      REQUIRE(view == str);
    }
  }
#endif
}

SCENARIO("string views can return immutable indexes") {
  GIVEN("a string view with some contents") {
    dart::string_view view = "abcdefghijklmnopqrstuvwxyz";

    THEN("we can access, but can't change, its elements") {
      REQUIRE(view[0] == 'a');
      REQUIRE(view.front() == 'a');
      REQUIRE(view.back() == 'z');
      REQUIRE(view[view.size() - 1] == 'z');
      REQUIRE(*(view.data() + 4) == 'e');

      constexpr auto is_const = std::is_const<
        std::remove_reference_t<decltype(view[0])>
      >::value;
      static_assert(is_const, "dart is misconfigured");
    }
  }
}

SCENARIO("string views have a total ordering") {
  GIVEN("some string views with contents") {
    std::vector<dart::string_view> views;
    views.push_back("zebra");
    views.emplace_back("aardvark");
    views.push_back("porcupine");
    views.emplace_back("emu");
    views.push_back("elephant");

    WHEN("we sort the vector") {
      std::sort(views.begin(), views.end());
      THEN("it produces a valid lexicographical ordering") {
        REQUIRE(views.front() == "aardvark");
        REQUIRE(views.back() == "zebra");
        REQUIRE(views[0] == "aardvark");
        REQUIRE(views[1] == "elephant");
        REQUIRE(views[2] == "emu");
        REQUIRE(views[3] == "porcupine");
        REQUIRE(views[4] == "zebra");
      }
    }
  }
}

SCENARIO("string views can check prefix membership") {
  GIVEN("some string views that all share a prefix") {
    std::vector<dart::string_view> views;
    views.push_back("testing one, two, three");
    views.push_back("testing is very necessary");
    views.push_back("testing is also very boring");
    views.push_back("testing, it makes the world go round");
    WHEN("we check their common prefix") {
      THEN("they all return membership") {
        for (auto view : views) REQUIRE(view.starts_with("testing"));
      }
    }

    WHEN("we remove the prefix") {
      for (auto& v : views) v.remove_prefix(7);
      THEN("they no longer return membership") {
        for (auto view : views) REQUIRE(!view.starts_with("testing"));
      }
    }
  }
}

SCENARIO("string views can check suffix membership") {
  GIVEN("some string views that all share a suffix") {
    std::vector<dart::string_view> views;
    views.push_back("how I love the idea of testing");
    views.push_back("how I hate the act of testing");
    views.push_back("where would we be without testing");
    views.push_back("always got time for more testing");
    WHEN("we check their common suffix") {
      THEN("they all return membership") {
        for (auto view : views) REQUIRE(view.ends_with("testing"));
      }
    }

    WHEN("we remove the prefix") {
      for (auto& v : views) v.remove_suffix(7);
      THEN("they no longer return membership") {
        for (auto view : views) REQUIRE(!view.ends_with("testing"));
      }
    }
  }
}

SCENARIO("string views can find an create subviews") {
  GIVEN("a string view with a pattern to look for") {
    dart::string_view base = "we're looking for a needle in a haystack";

    WHEN("we attempt to find the pattern") {
      auto idx = base.find("needle");
      THEN("it returns the base of the pattern") {
        auto sub = base.substr(idx);
        REQUIRE(sub.starts_with("needle"));
        REQUIRE(sub.ends_with("haystack"));
        REQUIRE(sub == "needle in a haystack");
        REQUIRE("needle in a haystack" == sub);
      }
    }
  }
}

SCENARIO("string views can find contained sets of characters") {
  GIVEN("a string view with interesting contents") {
    dart::string_view base = "fjdgiblhmnrcepkwtuovzqsyxa";

    WHEN("we search for the first vowel") {
      auto idx = base.find_first_of("aeiou(y)");
      THEN("it retuns the first vowel") {
        REQUIRE(base[idx] == 'i');
      }
    }

    WHEN("we search for the last vowel") {
      auto idx = base.find_last_of("aeiou(y)");
      THEN("it returns the last vowel") {
        REQUIRE(base[idx] == 'a');
      }
    }
  }
}
