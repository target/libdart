/*----- Local Includes -----*/

#include "dart_tests.h"

/*----- System Includes -----*/

#include <vector>
#include <string>
#include <fstream>
#include <sstream>

// YAML test validates against a known JSON parser and file,
// so figure out which one to use.
#if DART_HAS_RAPIDJSON
#include <rapidjson/document.h>
#else
static_assert(false, "dart::packet YAML tests require RapidJSON to be installed");
#endif

/*----- Namespace Inclusions -----*/

#if DART_HAS_RAPIDJSON
namespace rj = rapidjson;
#endif

/*----- Local Function Declarations -----*/

#if DART_HAS_RAPIDJSON
void compare_rj_dart(rj::Value const& obj, dart::packet const& pkt);
#else
void compare_jansson_dart(json_t* obj, dart::packet const& pkt);
#endif

/*----- Function Implementations -----*/

#if DART_HAS_YAML
TEST_CASE("dart::packet parses YAML via libyaml", "[yaml unit]") {
  // Read in the YAML file from disk.
  std::ostringstream stream;
  std::ifstream input("test.yml");
  stream << input.rdbuf();

  // Parse it.
  auto dart = dart::packet::from_yaml(stream.str())["tests"];

  // Read in our json file line by line.
  std::string json;
#if DART_HAS_RAPIDJSON
  std::vector<rj::Document> parsed;
#else
  std::vector<json_t*> parsed;
#endif
  input = std::ifstream("test.json");
  while (std::getline(input, json)) {
#if DART_HAS_RAPIDJSON
    parsed.emplace_back();
    rj::ParseResult success = parsed.back().Parse(json.data());
    REQUIRE(success);
#else
    json_error_t err;
    json_t* obj = json_loads(json.c_str(), 0, &err);
    REQUIRE_FALSE(obj == nullptr);
    parsed.push_back(obj);
#endif
  }

  // Iterate over our objects and validate.
  auto it = dart.begin();
  for (auto const& obj : parsed) {
    // Validate the original packet.
#if DART_HAS_RAPIDJSON
    compare_rj_dart(obj, *it++);
#else
    compare_jansson_dart(obj, *it++);
    json_decref(obj);
#endif
  }
}
#endif

#if DART_HAS_RAPIDJSON
void compare_rj_dart(rj::Value const& obj, dart::packet const& pkt) {
  switch (obj.GetType()) {
    case rj::kObjectType:
      {
        // We're validating an object.
        REQUIRE(pkt.is_object());
        auto rj_obj = obj.GetObject();

        // Check to make sure the number of fields agree.
        REQUIRE(rj_obj.MemberCount() == pkt.size());

        // Iterate over the keys and recusively check each one.
        for (auto const& member : rj_obj) {
          // Make sure we have this key.
          REQUIRE(pkt.has_key(member.name.GetString()));

          // Recursively check the value.
          compare_rj_dart(member.value, pkt[member.name.GetString()]);
        }
        break;
      }
    case rj::kArrayType:
      {
        // We're validating an array.
        REQUIRE(pkt.is_array());
        auto rj_arr = obj.GetArray();

        // Check to make sure the number of elements agree.
        REQUIRE(rj_arr.Size() == pkt.size());

        // Iterate over the values and recursively check each one.
        for (size_t index = 0; index < obj.Size(); ++index) {
          // Run the check.
          compare_rj_dart(rj_arr[static_cast<rj::SizeType>(index)], pkt[index]);
        }
        break;
      }
    case rj::kStringType:
      // We're validating a string.
      REQUIRE(pkt.is_str());

      // Compare the strings.
      REQUIRE_FALSE(strcmp(obj.GetString(), pkt.str()));
      break;
    case rj::kNumberType:
      // We're validating some type of number.
      REQUIRE((pkt.is_integer() || pkt.is_decimal()));

      if (pkt.is_integer()) REQUIRE(obj.GetInt64() == pkt.integer());
      else REQUIRE(obj.GetDouble() == pkt.decimal());
      break;
    case rj::kTrueType:
    case rj::kFalseType:
      // We're validating a boolean.
      REQUIRE(pkt.is_boolean());
      REQUIRE(obj.GetBool() == pkt.boolean());
      break;
    default:
      // We're validating a null.
      REQUIRE(pkt.is_null());
  }
}
#else
void compare_jansson_dart(json_t* obj, dart::packet const& pkt) {
  // Validate whatever type of object this is.
  switch (json_typeof(obj)) {
    case JSON_OBJECT:
      // We're validating an object.
      REQUIRE(pkt.is_object());

      // Check to make sure the number of fields agree.
      REQUIRE(json_object_size(obj) == pkt.size());

      // Iterate over the keys and recursively check each one.
      json_t* value;
      const char* key;
      json_object_foreach(obj, key, value) {
        // Check to make sure the dart::packet has the same key.
        REQUIRE(pkt.has_key(key));

        // Get the value.
        dart::packet value_obj = pkt[key];

        // Recursively check it.
        compare_jansson_dart(value, value_obj);
      }
      break;
    case JSON_ARRAY:
      // We're validating an array.
      REQUIRE(pkt.is_array());

      // Check to make sure the number of elements agree.
      REQUIRE(json_array_size(obj) == pkt.size());

      // Iterate over the elements and recursively check each one.
      for (size_t i = 0; i < json_array_size(obj); i++) {
        // Get the Jansson object.
        json_t* elem = json_array_get(obj, i);

        // Get the dart::packet.
        dart::packet elem_obj = pkt[i];

        // Recusively check it.
        compare_jansson_dart(elem, elem_obj);
      }
      break;
    case JSON_STRING:
      // We're validating a string.
      REQUIRE(pkt.is_str());

      // Compare the strings.
      REQUIRE_FALSE(strcmp(json_string_value(obj), pkt.str()));
      break;
    case JSON_INTEGER:
      // We're validating an integer.
      REQUIRE(pkt.is_integer());

      // Compare the integers.
      REQUIRE(json_integer_value(obj) == pkt.integer());
      break;
    case JSON_REAL:
      // We're validating a decimal.
      REQUIRE(pkt.is_decimal());

      // Compare the decimals.
      REQUIRE(json_real_value(obj) == pkt.decimal());
      break;
    case JSON_TRUE:
    case JSON_FALSE:
      // We're validating a boolean.
      REQUIRE(pkt.is_boolean());

      // Compare the booleans.
      REQUIRE(static_cast<bool>(json_is_true(obj)) == pkt.boolean());
      break;
    case JSON_NULL:
      // We're validating a null.
      REQUIRE(pkt.is_null());
      break;
    default:
      // Shouldn't happen.
      REQUIRE(false);
  }
}
#endif
