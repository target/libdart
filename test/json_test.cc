/*----- System Includes -----*/

#include <dart.h>
#include <vector>
#include <string>
#include <iostream>
#include <istream>
#include <fstream>
#include <extern/catch.h>

#if DART_HAS_RAPIDJSON
# include <rapidjson/document.h>
#endif

/*----- Namespace Inclusions -----*/

#if DART_HAS_RAPIDJSON
namespace rj = rapidjson;
#endif

/*----- Local Function Declarations -----*/

#if DART_HAS_RAPIDJSON
void compare_rj_dart(rj::Value const& obj, dart::packet const& pkt);
void compare_rj_rj(rj::Value const& obj, rj::Value const& dup);
#endif

/*----- Function Implementations -----*/

#if DART_HAS_RAPIDJSON
TEST_CASE("dart::packet parses JSON via RapidJSON", "[json unit]") {
  // Read in the packets from disk.
  std::string json;
  std::ifstream file("test.json");
  std::vector<std::string> packets;
  std::vector<rj::Document> parsed;
  while (std::getline(file, json)) {
    parsed.emplace_back();
    rj::ParseResult success = parsed.back().Parse(json.data());
    REQUIRE(success);
    packets.push_back(std::move(json));
  }

  // Iterate over the packets and validate.
  for (size_t index = 0; index < parsed.size(); ++index) {
    // Construct the packet.
    auto& packet = parsed[index];
    auto pkt = dart::packet::from_json(packets[index]);

    // Validate against the reference.
    compare_rj_dart(packet, pkt);

    // Validate the underlying buffer.
    dart::packet dup(pkt.dup_bytes());
    compare_rj_dart(packet, dup);

    // Generate JSON, reparse it, and validate it's still the same.
    rj::Document rj_dup;
    auto json = pkt.to_json();
    rj_dup.Parse(json.data());
    compare_rj_rj(packet, rj_dup);
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
          compare_rj_dart(rj_arr[index], pkt[index]);
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

void compare_rj_rj(rj::Value const& obj, rj::Value const& dup) {
  switch (obj.GetType()) {
    case rj::kObjectType:
      {
        // We're validating an object.
        REQUIRE(dup.IsObject());
        auto orig_obj = obj.GetObject(), dup_obj = dup.GetObject();

        // Make sure the number of fields agree.
        REQUIRE(orig_obj.MemberCount() == dup_obj.MemberCount());

        // Iterate over the keys and recursively check each one.
        for (auto const& member : orig_obj) {
          // Make sure we have this key.
          REQUIRE(dup_obj.HasMember(member.name.GetString()));

          // Recursively check the value.
          compare_rj_rj(member.value, dup_obj[member.name.GetString()]);
        }
        break;
      }
    case rj::kArrayType:
      {
        // We're validating an array.
        REQUIRE(dup.IsArray());
        auto orig_arr = obj.GetArray(), dup_arr = dup.GetArray();

        // Make sure the number of elements agree.
        REQUIRE(orig_arr.Size() == dup_arr.Size());

        // Iterate over the values and recursively check each one.
        for (size_t index = 0; index < orig_arr.Size(); ++index) {
          // Run the check.
          compare_rj_rj(orig_arr[index], dup_arr[index]);
        }
        break;
      }
    default:
      REQUIRE(obj == dup);
  }
}
#endif
