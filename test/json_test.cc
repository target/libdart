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
template <class Packet>
void compare_rj_dart(rj::Value const& obj, typename Packet::view pkt);
template <class AbiType>
void compare_rj_dart_abi(rj::Value const& obj, AbiType const* pkt);
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
    auto pktone = dart::packet::from_json(packets[index], false);
    auto pkttwo = dart::packet::parse(packets[index], true);
    auto pktthree = dart::heap::from_json(packets[index]);
    auto pktfour = dart::heap::parse(packets[index]);
    auto pktfive = dart::buffer::from_json(packets[index]);
    auto pktsix = dart::buffer::parse(packets[index]);

    // Validate against the reference.
    // Function is generic, but declares ones of its parameters
    // as a dependent name, and is therefore non-deducible
    // Ugly, but a simple way to increase coverage
    compare_rj_dart<decltype(pktone)>(packet, pktone);
    compare_rj_dart<decltype(pkttwo)>(packet, pkttwo);
    compare_rj_dart<decltype(pktthree)>(packet, pktthree);
    compare_rj_dart<decltype(pktfour)>(packet, pktfour);
    compare_rj_dart<decltype(pktfive)>(packet, pktfive);
    compare_rj_dart<decltype(pktsix)>(packet, pktsix);

    // Validate the underlying buffer.
    dart::packet duptwo(pkttwo.dup_bytes());
    dart::buffer dupfive(pktfive.dup_bytes());
    dart::buffer dupsix(pktsix.dup_bytes());
    compare_rj_dart<decltype(duptwo)>(packet, duptwo);
    compare_rj_dart<decltype(dupfive)>(packet, dupfive);
    compare_rj_dart<decltype(dupsix)>(packet, dupsix);

    // Generate JSON, reparse it, and validate it's still the same.
    rj::Document rj_dupone, rj_duptwo, rj_dupthree, rj_dupfour, rj_dupfive, rj_dupsix;
    auto jsonone = pktone.to_json();
    auto jsontwo = pkttwo.to_json();
    auto jsonthree = pktthree.to_json();
    auto jsonfour = pktfour.to_json();
    auto jsonfive = pktfive.to_json();
    auto jsonsix = pktsix.to_json();
    rj_dupone.Parse(jsonone.data());
    rj_duptwo.Parse(jsontwo.data());
    rj_dupthree.Parse(jsonthree.data());
    rj_dupfour.Parse(jsonfour.data());
    rj_dupfive.Parse(jsonfive.data());
    rj_dupsix.Parse(jsonsix.data());
    compare_rj_rj(packet, rj_dupone);
    compare_rj_rj(packet, rj_duptwo);
    compare_rj_rj(packet, rj_dupthree);
    compare_rj_rj(packet, rj_dupfour);
    compare_rj_rj(packet, rj_dupfive);
    compare_rj_rj(packet, rj_dupsix);
  }
}

template <class Packet>
void compare_rj_dart(rj::Value const& obj, typename Packet::view pkt) {
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
          compare_rj_dart<Packet>(member.value, pkt[member.name.GetString()]);
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
          compare_rj_dart<Packet>(rj_arr[static_cast<rj::SizeType>(index)], pkt[index]);
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
    auto pktone = dart_from_json(packets[index].data());
    auto pkttwo = dart_from_json_rc(DART_RC_SAFE, packets[index].data());
    auto pktthree = dart_from_json_len(packets[index].data(), packets[index].size());
    auto pktfour = dart_from_json_len_rc(DART_RC_SAFE, packets[index].data(), packets[index].size());
    auto bufferone = dart_buffer_from_json(packets[index].data());
    auto buffertwo = dart_buffer_from_json_rc(DART_RC_SAFE, packets[index].data());
    auto bufferthree = dart_buffer_from_json_len(packets[index].data(), packets[index].size());
    auto bufferfour = dart_buffer_from_json_len_rc(DART_RC_SAFE, packets[index].data(), packets[index].size());
    auto heapone = dart_heap_from_json(packets[index].data());
    auto heaptwo = dart_heap_from_json_rc(DART_RC_SAFE, packets[index].data());
    auto heapthree = dart_heap_from_json_len(packets[index].data(), packets[index].size());
    auto heapfour = dart_heap_from_json_len_rc(DART_RC_SAFE, packets[index].data(), packets[index].size());
    auto guard_one = make_scope_guard([&] {
      dart_destroy(&heapfour);
      dart_destroy(&heapthree);
      dart_destroy(&heaptwo);
      dart_destroy(&heapone);
      dart_destroy(&bufferfour);
      dart_destroy(&bufferthree);
      dart_destroy(&buffertwo);
      dart_destroy(&bufferone);
      dart_destroy(&pktfour);
      dart_destroy(&pktthree);
      dart_destroy(&pkttwo);
      dart_destroy(&pktone);
    });

    // Validate against the reference.
    compare_rj_dart_abi(packet, &pktone);
    compare_rj_dart_abi(packet, &pkttwo);
    compare_rj_dart_abi(packet, &pktthree);
    compare_rj_dart_abi(packet, &pktfour);
    compare_rj_dart_abi(packet, &heapone);
    compare_rj_dart_abi(packet, &heaptwo);
    compare_rj_dart_abi(packet, &heapthree);
    compare_rj_dart_abi(packet, &heapfour);
    compare_rj_dart_abi(packet, &bufferone);
    compare_rj_dart_abi(packet, &buffertwo);
    compare_rj_dart_abi(packet, &bufferthree);
    compare_rj_dart_abi(packet, &bufferfour);
    
    // Validate the underlying buffer.
    auto pktdup = dart_take_bytes(dart_dup_bytes(&bufferone, nullptr));
    auto pktduptwo = dart_take_bytes_rc(dart_dup_bytes(&bufferone, nullptr), DART_RC_SAFE);
    auto bufferdup = dart_buffer_take_bytes(dart_buffer_dup_bytes(&bufferone, nullptr));
    auto bufferduptwo = dart_buffer_take_bytes_rc(dart_buffer_dup_bytes(&bufferone, nullptr), DART_RC_SAFE);
    auto guard_two = make_scope_guard([&] {
      dart_destroy(&bufferduptwo);
      dart_destroy(&bufferdup);
      dart_destroy(&pktduptwo);
      dart_destroy(&pktdup);
    });
    compare_rj_dart_abi(packet, &pktdup);
    compare_rj_dart_abi(packet, &pktduptwo);
    compare_rj_dart_abi(packet, &bufferdup);
    compare_rj_dart_abi(packet, &bufferduptwo);

    // Generate JSON, reparse it, and validate it's still the same.
    rj::Document rjone, rjtwo, rjthree;
    auto* pktjson = dart_to_json(&pktone, nullptr);
    auto* heapjson = dart_heap_to_json(&heapone, nullptr);
    auto* bufferjson = dart_buffer_to_json(&bufferone, nullptr);
    auto guard_three = make_scope_guard([=] {
      free(bufferjson);
      free(heapjson);
      free(pktjson);
    });

    rjone.Parse(pktjson);
    rjtwo.Parse(heapjson);
    rjthree.Parse(bufferjson);
    compare_rj_rj(packet, rjone);
    compare_rj_rj(packet, rjtwo);
    compare_rj_rj(packet, rjthree);
  }
}

template <class AbiType>
void compare_rj_dart_abi(rj::Value const& obj, AbiType const* pkt) {
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
          compare_rj_dart_abi(rj_arr[static_cast<rj::SizeType>(index)], &val);
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
      REQUIRE(static_cast<int>(obj.GetBool()) == dart_bool_get(pkt));
      break;
    default:
      // We're validating a null.
      REQUIRE(dart_is_null(pkt));
  }
}

#endif
