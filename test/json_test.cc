/*----- Local Includes -----*/

#include "dart_tests.h"
#include "../include/dart/abi.h"

/*----- System Includes -----*/

#include <vector>
#include <string>
#include <iostream>
#include <istream>
#include <fstream>

#if DART_HAS_RAPIDJSON
# include <rapidjson/document.h>
#endif

/*----- Namespace Inclusions -----*/

#if DART_HAS_RAPIDJSON
namespace rj = rapidjson;
#endif

/*----- Types -----*/

/*----- Local Function Declarations -----*/

template <class Func>
struct scope_guard : Func {
  scope_guard(scope_guard const&) = default;
  scope_guard(scope_guard&&) = default;
  scope_guard(Func const& cb) : Func(cb) {}
  scope_guard(Func&& cb) : Func(std::move(cb)) {}

  ~scope_guard() noexcept {
    try {
      (*this)();
    } catch (...) {
      // Oops...
    }
  }
};

template <class Func>
auto make_scope_guard(Func&& cb) {
  return scope_guard<std::decay_t<Func>> {cb};
}

#if DART_HAS_RAPIDJSON
void compare_rj_dart(rj::Value const& obj, dart::packet const& pkt);
void compare_rj_dart_abi(rj::Value const& obj, dart_packet_t const* pkt);
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
    auto pkt = dart::parse(packets[index], true);

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
          compare_rj_rj(orig_arr[static_cast<rj::SizeType>(index)], dup_arr[static_cast<rj::SizeType>(index)]);
        }
        break;
      }
    default:
      REQUIRE(obj == dup);
  }
}
#endif

#ifdef DART_HAS_ABI
TEST_CASE("dart_packet parses JSON via RapidJSON", "[json unit]") {
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
    auto pkt = dart_from_json(packets[index].data());
    auto guard_one = make_scope_guard([&] { dart_destroy(&pkt); });

    // Validate against the reference.
    compare_rj_dart_abi(packet, &pkt);

    // Validate the underlying buffer.
    auto fin = dart_finalize(&pkt);
    auto dup = dart_take_bytes(dart_dup_bytes(&fin, nullptr));
    auto guard_two = make_scope_guard([&] {
      dart_destroy(&dup);
      dart_destroy(&fin);
    });
    compare_rj_dart_abi(packet, &dup);

    // Generate JSON, reparse it, and validate it's still the same.
    rj::Document rj_dup;
    auto* json = dart_to_json(&pkt, nullptr);
    auto guard_three = make_scope_guard([=] { free(json); });

    rj_dup.Parse(json);
    compare_rj_rj(packet, rj_dup);
  }
}

void compare_rj_dart_abi(rj::Value const& obj, dart_packet_t const* pkt) {
  switch (obj.GetType()) {
    case rj::kObjectType:
      {
        // We're validating an object.
        REQUIRE(dart_is_obj(pkt));
        auto rj_obj = obj.GetObject();

        // Check to make sure the number of fields agree.
        REQUIRE(rj_obj.MemberCount() == dart_size(pkt));

        // Iterate over the keys and recusively check each one.
        for (auto const& member : rj_obj) {
          // Make sure we have this key.
          REQUIRE(dart_obj_has_key(pkt, member.name.GetString()));

          // Recursively check the value.
          auto val = dart_obj_get(pkt, member.name.GetString());
          auto guard = make_scope_guard([&] { dart_destroy(&val); });
          compare_rj_dart_abi(member.value, &val);
        }
        break;
      }
    case rj::kArrayType:
      {
        // We're validating an array.
        REQUIRE(dart_is_arr(pkt));
        auto rj_arr = obj.GetArray();

        // Check to make sure the number of elements agree.
        REQUIRE(rj_arr.Size() == dart_size(pkt));

        // Iterate over the values and recursively check each one.
        for (size_t index = 0; index < obj.Size(); ++index) {
          // Run the check.
          auto val = dart_arr_get(pkt, index);
          auto guard = make_scope_guard([&] { dart_destroy(&val); });
          compare_rj_dart_abi(rj_arr[index], &val);
        }
        break;
      }
    case rj::kStringType:
      // We're validating a string.
      REQUIRE(dart_is_str(pkt));

      // Compare the strings.
      REQUIRE_FALSE(strcmp(obj.GetString(), dart_str_get(pkt)));
      break;
    case rj::kNumberType:
      // We're validating some type of number.
      REQUIRE((dart_is_int(pkt) || dart_is_dcm(pkt)));

      if (dart_is_int(pkt)) REQUIRE(obj.GetInt64() == dart_int_get(pkt));
      else REQUIRE(obj.GetDouble() == dart_dcm_get(pkt));
      break;
    case rj::kTrueType:
    case rj::kFalseType:
      // We're validating a boolean.
      REQUIRE(dart_is_bool(pkt));
      REQUIRE(obj.GetBool() == dart_bool_get(pkt));
      break;
    default:
      // We're validating a null.
      REQUIRE(dart_is_null(pkt));
  }
}

#endif
