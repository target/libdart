/*----- System Includes -----*/

#include <random>
#include <cassert>
#include <algorithm>
#include <unordered_set>
#include <benchmark/benchmark.h>

#ifdef DART_HAS_FLEXBUFFERS
#include <flatbuffers/flexbuffers.h>
#endif

#ifdef DART_HAS_SAJSON
#include <sajson.h>
#endif

/*----- Local Includes -----*/

#include "../include/dart.h"

/*----- Type Declarations -----*/

using unsafe_heap = dart::basic_heap<dart::unsafe_ptr>;
using unsafe_buffer = dart::basic_buffer<dart::unsafe_ptr>;
using unsafe_packet = dart::basic_packet<dart::unsafe_ptr>;

struct benchmark_helper : benchmark::Fixture {

  /*---- Test Construction/Destruction Functions -----*/

  void SetUp(benchmark::State const&) override;
  void TearDown(benchmark::State const&) override;

  /*----- Helpers -----*/

  unsafe_packet generate_dynamic_flat_packet() const;
  unsafe_packet generate_dynamic_nested_packet() const;
  std::tuple<std::string, unsafe_packet> generate_finalized_packet(unsafe_packet base) const;

  /*----- Members -----*/

  std::string flat_json, nested_json;
  std::vector<std::string> flat_keys, nested_keys;
  unsafe_packet flat, nested, flat_fin, nested_fin;

  benchmark::Counter rate_counter;

};

/*----- Globals -----*/

constexpr auto static_array_size = 64;
constexpr auto static_string_size = 8;

/*----- Helpers -----*/

template <int64_t low = std::numeric_limits<int64_t>::min(),
         int64_t high = std::numeric_limits<int64_t>::max()>
int64_t rand_int() {
  static std::mt19937 engine(std::random_device {}());
  static std::uniform_int_distribution<int64_t> dist(low, high);
  return dist(engine);
}

std::string rand_string(size_t len, dart::shim::string_view prefix = "") {
  constexpr int low = 0, high = 25;
  static std::vector<char> alpha {
    'a', 'b', 'c', 'd', 'e', 'f', 'g',
    'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u',
    'v', 'w', 'x', 'y', 'z'
  };

  std::string retval {prefix};
  retval.resize(len, '\0');
  std::generate(retval.begin() + prefix.size(), retval.end(), [&] {
    return alpha[rand_int<low, high>()];
  });
  return retval;
}

template <class Container>
typename Container::value_type rand_pick(Container const& cont) {
  static std::mt19937 engine(std::random_device {}());

  typename Container::value_type chosen;
  std::sample(std::begin(cont), std::end(cont), &chosen, 1, engine);
  return chosen;
}

/*----- Benchmark Definitions -----*/

#if DART_HAS_RAPIDJSON
BENCHMARK_F(benchmark_helper, parse_dynamic_flat_packet) (benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(unsafe_heap::from_json(flat_json));
    ++rate_counter;
  }
  state.counters["parsed flat packets"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, parse_dynamic_nested_packet) (benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(unsafe_heap::from_json(nested_json));
    ++rate_counter;
  }
  state.counters["parsed nested packets"] = rate_counter;
}
BENCHMARK_F(benchmark_helper, parse_finalized_flat_packet) (benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(unsafe_buffer::from_json(flat_json));
    ++rate_counter;
  }
  state.counters["parsed flat packets"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, parse_finalized_nested_packet) (benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(unsafe_buffer::from_json(nested_json));
    ++rate_counter;
  }
  state.counters["parsed nested packets"] = rate_counter;
}
#endif

BENCHMARK_F(benchmark_helper, lookup_finalized_fields) (benchmark::State& state) {
  unsafe_buffer data {flat_fin};
  for (auto _ : state) {
    for (auto const& key : flat_keys) benchmark::DoNotOptimize(data[key]);
    rate_counter += flat_keys.size();
  }
  state.counters["finalized field lookups"] = rate_counter;
}

BENCHMARK_DEFINE_F(benchmark_helper, lookup_finalized_random_fields) (benchmark::State& state) {
  // Generate some random strings.
  std::vector<std::string> keys(state.range(0));
  std::generate(keys.begin(), keys.end(), [&] { return rand_string(state.range(1)); });

  // Generate a packet.
  unsafe_packet::object pkt;
  for (auto const& key : keys) pkt.add_field(key, key);

  // Run the test.
  auto size = pkt.size();
  unsafe_buffer::object data {pkt};
  for (auto _ : state) {
    for (auto const& key : keys) benchmark::DoNotOptimize(data[key]);
    rate_counter += size;
  }
  state.counters["finalized random field lookups"] = rate_counter;
}

BENCHMARK_REGISTER_F(benchmark_helper, lookup_finalized_random_fields)->Ranges({{1 << 0, 1 << 8}, {1 << 2, 1 << 8}});

BENCHMARK_DEFINE_F(benchmark_helper, lookup_finalized_colliding_fields) (benchmark::State& state) {
  struct hasher {
    auto operator ()(std::string const& str) const {
      // Inefficient, but it'll work.
      return std::hash<std::string> {}(str.substr(0, 2));
    }
  };
  struct eq {
    auto operator ()(std::string const& lhs, std::string const& rhs) const {
      return lhs[0] == rhs[0] && lhs[1] == rhs[1];
    }
  };

  // Generate a unique set of random strings without collisions.
  std::unordered_set<std::string, hasher, eq> unique;
  auto collision_percent = state.range(0) / 100.0;
  size_t num_keys = state.range(1), key_len = state.range(2), num_collisions = std::ceil(num_keys * collision_percent);
  while (unique.size() != num_keys - num_collisions) unique.insert(rand_string(key_len));

  // Inject collisions.
  std::unordered_set<std::string> keys {unique.begin(), unique.end()};
  while (keys.size() != num_keys) {
    auto collision = rand_pick(keys);
    keys.insert(rand_string(key_len, collision.substr(0, 2)));
  }

  // Generate the packet.
  unsafe_packet::object pkt;
  for (auto const& key : keys) pkt.add_field(key, key);

  // Run the test.
  unsafe_buffer::object data {pkt};
  for (auto _ : state) {
    for (auto const& key : keys) benchmark::DoNotOptimize(data[key]);
    rate_counter += data.size();
  }
  state.counters["field collisions"] = num_collisions;
  state.counters["finalized random field lookups"] = rate_counter;
}

BENCHMARK_REGISTER_F(benchmark_helper, lookup_finalized_colliding_fields)
  ->Args({0, 16, 8})
  ->Args({8, 16, 8})
  ->Args({32, 16, 8})
  ->Args({64, 16, 8})
  ->Args({100, 16, 8})
  ->Args({0, 64, 8})
  ->Args({8, 64, 8})
  ->Args({32, 64, 8})
  ->Args({64, 64, 8})
  ->Args({100, 64, 8})
  ->Args({0, 256, 8})
  ->Args({8, 256, 8})
  ->Args({32, 256, 8})
  ->Args({64, 256, 8})
  ->Args({100, 256, 8});

#ifdef DART_HAS_FLEXBUFFERS
BENCHMARK_DEFINE_F(benchmark_helper, flexbuffer_lookup_finalized_random_fields) (benchmark::State& state) {
  // Generate some random strings.
  std::vector<std::string> keys(state.range(0));
  std::generate(keys.begin(), keys.end(), [&] { return rand_string(state.range(1)); });

  // Build a flexbuffer.
  flexbuffers::Builder fbb;
  fbb.Map([&] {
    for (auto const& key : keys) fbb.String(key.data(), key.data());
  });
  fbb.Finish();

  // Expand it.
  auto buffer = fbb.GetBuffer();
  auto pkt = flexbuffers::GetRoot(buffer).AsMap();

  // Run the test.
  for (auto _ : state) {
    for (auto const& key : keys) benchmark::DoNotOptimize(pkt[key.data()]);
    rate_counter += pkt.size();
  }
  state.counters["finalized random field lookups"] = rate_counter;
}

BENCHMARK_REGISTER_F(benchmark_helper, flexbuffer_lookup_finalized_random_fields)->Ranges({{1 << 0, 1 << 8}, {1 << 2, 1 << 8}});
#endif

#ifdef DART_HAS_SAJSON
BENCHMARK_DEFINE_F(benchmark_helper, sajson_lookup_finalized_random_fields) (benchmark::State& state) {
  // Generate some random strings.
  std::vector<std::string> keys(state.range(0));
  std::generate(keys.begin(), keys.end(), [&] { return rand_string(state.range(1)); });

  // sajson only parses json, so we need to go through dart as an intermediary.
  auto tmp = dart::packet::object();
  for (auto const& key : keys) tmp.add_field(key, key);
  auto json = tmp.to_json();
  auto doc = sajson::parse(sajson::single_allocation(), sajson::string {json.data(), json.size()});

  // Run the test.
  auto obj = doc.get_root();
  for (auto _ : state) {
    for (auto const& key : keys) {
      sajson::string str {key.data(), key.size()};
      benchmark::DoNotOptimize(obj.get_value_of_key(str));
    }
    rate_counter += obj.get_length();
  }
  state.counters["finalized random field lookups"] = rate_counter;
}

BENCHMARK_REGISTER_F(benchmark_helper, sajson_lookup_finalized_random_fields)->Ranges({{1 << 0, 1 << 8}, {1 << 2, 1 << 8}});
#endif

BENCHMARK_F(benchmark_helper, lookup_dynamic_fields) (benchmark::State& state) {
  unsafe_heap flatter {flat};
  for (auto _ : state) {
    for (auto const& key : flat_keys) benchmark::DoNotOptimize(flatter[key]);
    rate_counter += flat_keys.size();
  }
  state.counters["dynamic field lookups"] = rate_counter;
}

BENCHMARK_DEFINE_F(benchmark_helper, lookup_dynamic_random_fields) (benchmark::State& state) {
  // Generate some random strings.
  std::vector<std::string> keys(state.range(0));
  std::generate(keys.begin(), keys.end(), [&] { return rand_string(state.range(1)); });

  // Generate a packet.
  unsafe_packet::object pkt;
  for (auto const& key : keys) pkt.add_field(key, key);

  // Run the test.
  unsafe_heap::object data {pkt};
  for (auto _ : state) {
    for (auto const& key : keys) benchmark::DoNotOptimize(data[key]);
    rate_counter += data.size();
  }
  state.counters["dynamic random field lookups"] = rate_counter;
}

BENCHMARK_REGISTER_F(benchmark_helper, lookup_dynamic_random_fields)->Ranges({{1 << 0, 1 << 8}, {1 << 2, 1 << 8}});

BENCHMARK_DEFINE_F(benchmark_helper, iterate_finalized_random_fields) (benchmark::State& state) {
  // Generate some random strings.
  std::vector<std::string> keys(state.range(0));
  std::generate(keys.begin(), keys.end(), [&] { return rand_string(static_string_size); });

  // Generate a packet.
  unsafe_packet::object pkt;
  for (auto const& key : keys) pkt.add_field(key, key);

  // Run the test.
  auto size = pkt.size();
  unsafe_buffer::object data {pkt};
  for (auto _ : state) {
    for (auto val : data) benchmark::DoNotOptimize(val);
    rate_counter += size;
  }
  state.counters["finalized random field iterations"] = rate_counter;
}

BENCHMARK_REGISTER_F(benchmark_helper, iterate_finalized_random_fields)->Ranges({{1 << 0, 1 << 8}});

BENCHMARK_DEFINE_F(benchmark_helper, iterate_finalized_random_elements) (benchmark::State& state) {
  // Generate some random strings.
  std::vector<std::string> strs(state.range(0));
  std::generate(strs.begin(), strs.end(), [&] { return rand_string(static_string_size); });

  // Generate a packet.
  unsafe_packet::array pkt;
  pkt.reserve(strs.size());
  for (auto const& str : strs) pkt.push_back(str);

  // Run the test.
  auto size = pkt.size();
  auto data = unsafe_buffer::object {"arr", std::move(pkt)}["arr"];
  for (auto _ : state) {
    for (auto val : data) benchmark::DoNotOptimize(val);
    rate_counter += size;
  }
  state.counters["finalized random element iterations"] = rate_counter;
}

BENCHMARK_REGISTER_F(benchmark_helper, iterate_finalized_random_elements)->Ranges({{1 << 0, 1 << 8}});

BENCHMARK_DEFINE_F(benchmark_helper, iterate_dynamic_random_fields) (benchmark::State& state) {
  // Generate some random strings.
  std::vector<std::string> keys(state.range(0));
  std::generate(keys.begin(), keys.end(), [&] { return rand_string(static_string_size); });

  // Generate a packet.
  unsafe_packet::object pkt;
  for (auto const& key : keys) pkt.add_field(key, key);

  // Run the test.
  auto size = pkt.size();
  unsafe_heap::object data {pkt};
  for (auto _ : state) {
    for (auto val : data) benchmark::DoNotOptimize(val);
    rate_counter += size;
  }
  state.counters["dynamic random field iterations"] = rate_counter;
}

BENCHMARK_REGISTER_F(benchmark_helper, iterate_dynamic_random_fields)->Ranges({{1 << 0, 1 << 8}});

BENCHMARK_DEFINE_F(benchmark_helper, iterate_dynamic_random_elements) (benchmark::State& state) {
  // Generate some random strings.
  std::vector<std::string> strs(state.range(0));
  std::generate(strs.begin(), strs.end(), [&] { return rand_string(static_string_size); });

  // Generate a packet.
  unsafe_packet::array pkt;
  pkt.reserve(strs.size());
  for (auto const& str : strs) pkt.push_back(str);

  // Run the test.
  auto size = pkt.size();
  unsafe_heap::array data {std::move(pkt)};
  for (auto _ : state) {
    for (auto val : data) benchmark::DoNotOptimize(val);
    rate_counter += size;
  }
  state.counters["dynamic random element iterations"] = rate_counter;
}

BENCHMARK_REGISTER_F(benchmark_helper, iterate_dynamic_random_elements)->Ranges({{1 << 0, 1 << 8}});

BENCHMARK_F(benchmark_helper, access_sequential_finalized_strings) (benchmark::State& state) {
  // Generate some random strings.
  std::vector<std::string> strs(static_array_size);
  std::generate(strs.begin(), strs.end(), [&] { return rand_string(static_string_size); });

  // Generate a packet.
  unsafe_packet::array arr;
  arr.reserve(strs.size());
  for (auto const& str : strs) arr.push_back(str);

  // Run the test.
  auto elems = strs.size();
  auto data = unsafe_buffer::object {"arr", std::move(arr)}["arr"];
  for (auto _ : state) {
    for (auto i = 0U; i < elems; ++i) {
      benchmark::DoNotOptimize(data[i]);
    }
    rate_counter += elems;
  }
  state.counters["finalized sequential element accesses"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, access_sequential_dynamic_strings) (benchmark::State& state) {
  // Generate some random strings.
  std::vector<std::string> strs(static_array_size);
  std::generate(strs.begin(), strs.end(), [&] { return rand_string(static_string_size); });

  // Generate a packet.
  unsafe_packet::array arr;
  arr.reserve(strs.size());
  for (auto const& str : strs) arr.push_back(str);

  // Run the test.
  auto elems = strs.size();
  unsafe_heap::array data {std::move(arr)};
  for (auto _ : state) {
    for (auto i = 0U; i < elems; ++i) {
      benchmark::DoNotOptimize(data[i]);
    }
    rate_counter += elems;
  }
  state.counters["dynamic sequential element accesses"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, access_random_finalized_strings) (benchmark::State& state) {
  // Generate some random strings.
  std::vector<std::string> strs(static_array_size);
  std::generate(strs.begin(), strs.end(), [&] { return rand_string(static_string_size); });

  // Generate a packet.
  unsafe_packet::array arr;
  arr.reserve(strs.size());
  for (auto const& str : strs) arr.push_back(str);

  // Generate a random set of in-range indexes to pull from.
  std::vector<int64_t> idxs(1 << 10);
  for (auto& idx : idxs) idx = rand_int<0, static_array_size - 1>();

  // Run the test.
  auto const elems = idxs.size();
  auto data = unsafe_buffer::object {"arr", std::move(arr)}["arr"];
  for (auto _ : state) {
    for (auto idx : idxs) benchmark::DoNotOptimize(data[idx]);
    rate_counter += elems;
  }
  state.counters["finalized random element accesses"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, access_random_dynamic_strings) (benchmark::State& state) {
  // Generate some random strings.
  std::vector<std::string> strs(static_array_size);
  std::generate(strs.begin(), strs.end(), [&] { return rand_string(static_string_size); });

  // Generate a packet.
  unsafe_packet::array arr;
  arr.reserve(strs.size());
  for (auto const& str : strs) arr.push_back(str);

  // Generate a random set of in-range indexes to pull from.
  std::vector<int64_t> idxs(1 << 10);
  for (auto& idx : idxs) idx = rand_int<0, static_array_size - 1>();

  // Run the test.
  auto const elems = idxs.size();
  unsafe_heap::array data {std::move(arr)};
  for (auto _ : state) {
    for (auto idx : idxs) benchmark::DoNotOptimize(data[idx]);
    rate_counter += elems;
  }
  state.counters["finalized random element accesses"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, insert_into_exclusive_dynamic_object) (benchmark::State& state) {
  // Generate a huge number of exclusively owned packets in one go
  // so that we can focus on timing what we're interested in.
  auto gen = [&] {
    std::vector<unsafe_heap> pkts(1 << 10);
    for (auto& p : pkts) p = unsafe_heap::transmogrify<dart::unsafe_ptr>(unsafe_heap {flat});
    return pkts;
  };

  auto spins = 0ULL;
  auto pkts = gen();
  auto const pkt_count = pkts.size();
  benchmark::DoNotOptimize(pkts.data());
  for (auto _ : state) {
    if (spins < pkt_count) {
      pkts[spins++].add_field("the thin ice", "the wall");
      ++rate_counter;
    } else {
      state.PauseTiming();
      pkts = gen();
      spins = 0;
      benchmark::DoNotOptimize(pkts.data());
      state.ResumeTiming();
    }
    benchmark::ClobberMemory();
  }
  state.counters["exclusive dynamic object field modifications"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, insert_into_exclusive_dynamic_array) (benchmark::State& state) {
  // Generate a prototype array.
  unsafe_heap::array proto;
  auto vals = flat.values();
  proto.reserve(vals.size());
  for (auto const& val : vals) proto.push_back(val);

  // Generate a huge number of exclusively owned packets in one go
  // so that we can focus on timing what we're interested in.
  auto gen = [&] {
    std::vector<unsafe_heap> pkts(1 << 10);
    for (auto& p : pkts) p = unsafe_heap::transmogrify<dart::unsafe_ptr>(proto);
    return pkts;
  };

  auto spins = 0ULL;
  auto pkts = gen();
  auto const pkt_count = pkts.size();
  benchmark::DoNotOptimize(pkts.data());
  for (auto _ : state) {
    if (spins < pkt_count) {
      pkts[spins++].push_back("the wall");
      ++rate_counter;
    } else {
      state.PauseTiming();
      pkts = gen();
      spins = 0;
      benchmark::DoNotOptimize(pkts.data());
      state.ResumeTiming();
    }
    benchmark::ClobberMemory();
  }
  state.counters["exclusive dynamic array element modifications"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, insert_into_shared_dynamic_object) (benchmark::State& state) {
  auto gen = [&] { return std::vector<unsafe_heap>(1 << 10, unsafe_heap {flat}); };
  auto spins = 0ULL;
  auto pkts = gen();
  benchmark::DoNotOptimize(pkts.data());
  for (auto _ : state) {
    if (spins < pkts.size()) {
      pkts[spins++].add_field("the thin ice", "the wall");
    } else {
      state.PauseTiming();
      pkts = gen();
      spins = 0;
      benchmark::DoNotOptimize(pkts.data());
      state.ResumeTiming();
    }
    ++rate_counter;
    benchmark::ClobberMemory();
  }
  state.counters["shared dynamic object field modifications"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, insert_into_shared_dynamic_array) (benchmark::State& state) {
  // Generate a prototype array.
  unsafe_heap::array proto;
  auto vals = flat.values();
  proto.reserve(vals.size());
  for (auto const& val : vals) proto.push_back(val);

  // Generate a large number of shared packets in one go so we can
  // focus on timing what we're interested in.
  auto gen = [&] { return std::vector<unsafe_heap>(1 << 10, proto); };
  auto spins = 0ULL;
  auto pkts = gen();
  benchmark::DoNotOptimize(pkts.data());
  for (auto _ : state) {
    if (spins < pkts.size()) {
      pkts[spins++].push_back("the wall");
    } else {
      state.PauseTiming();
      pkts = gen();
      spins = 0;
      benchmark::DoNotOptimize(pkts.data());
      state.ResumeTiming();
    }
    ++rate_counter;
    benchmark::ClobberMemory();
  }
  state.counters["shared dynamic array element modifications"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, finalize_dynamic_packet) (benchmark::State& state) {
  unsafe_heap flatter {flat};
  for (auto _ : state) {
    auto copy = flatter;
    auto buf = copy.finalize().get_bytes();
    benchmark::DoNotOptimize(buf.data());
    ++rate_counter;
  }
  state.counters["finalized packets"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, serialize_finalized_packet_into_json) (benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(flat_fin.to_json().data());
    ++rate_counter;
  }
  state.counters["serialized finalized packets"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, serialize_finalized_nested_packet_into_json) (benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(nested_fin.to_json().data());
    ++rate_counter;
  }
  state.counters["serialized finalized packets"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, serialize_dynamic_packet_into_json) (benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(flat.to_json().data());
    ++rate_counter;
  }
  state.counters["serialized dynamic packets"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, serialize_dynamic_nested_packet_into_json) (benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(nested.to_json().data());
    ++rate_counter;
  }
  state.counters["serialized dynamic nested packets"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, unwrap_finalized_string) (benchmark::State& state) {
  // Generate some random strings.
  std::vector<unsafe_buffer::string> strs;
  for (auto i = 0; i < 1024; ++i) {
    strs.emplace_back(unsafe_buffer::object {"str", rand_string(static_string_size)}["str"]);
  }

  // Benchmark unwrapping the string into a machine type.
  auto const pkts = strs.size();
  for (auto _ : state) {
    for (auto const& str : strs) benchmark::DoNotOptimize(*str);
    rate_counter += pkts;
  }
  state.counters["finalized string value accesses"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, unwrap_dynamic_string) (benchmark::State& state) {
  // Generate some random strings.
  std::vector<unsafe_heap::string> strs(1024);
  std::generate(strs.begin(), strs.end(), [&] {
    return unsafe_heap::string {rand_string(static_string_size)};
  });

  // Benchmark unwrapping the string into a machine type.
  auto const pkts = strs.size();
  for (auto _ : state) {
    for (auto const& str : strs) benchmark::DoNotOptimize(*str);
    rate_counter += pkts;
  }
  state.counters["dynamic string value accesses"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, unwrap_finalized_integer) (benchmark::State& state) {
  // Generate some random strings.
  std::vector<unsafe_buffer::number> ints;
  for (auto i = 0; i < 1024; ++i) {
    ints.emplace_back(unsafe_buffer::object {"int", rand_int()}["int"]);
  }

  // Benchmark unwrapping the integer into a machine type.
  auto const pkts = ints.size();
  for (auto _ : state) {
    for (auto const& integer : ints) benchmark::DoNotOptimize(integer.integer());
    rate_counter += pkts;
  }
  state.counters["finalized integer value accesses"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, unwrap_dynamic_integer) (benchmark::State& state) {
  // Generate some random strings.
  std::vector<unsafe_heap::number> ints(1024);
  std::generate(ints.begin(), ints.end(), [&] {
    return unsafe_heap::number {rand_int()};
  });

  // Benchmark unwrapping the integer into a machine type.
  auto const pkts = ints.size();
  for (auto _ : state) {
    for (auto const& integer : ints) benchmark::DoNotOptimize(integer.integer());
    rate_counter += pkts;
  }
  state.counters["dynamic integer value accesses"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, unwrap_finalized_decimal) (benchmark::State& state) {
  // Generate some random strings.
  std::vector<unsafe_buffer::number> dbls;
  for (auto i = 0; i < 1024; ++i) {
    dbls.emplace_back(unsafe_buffer::object {"dbl", static_cast<double>(rand_int())}["dbl"]);
  }

  // Benchmark unwrapping the double into a machine type.
  auto const pkts = dbls.size();
  for (auto _ : state) {
    for (auto const& dbl : dbls) benchmark::DoNotOptimize(dbl.decimal());
    rate_counter += pkts;
  }
  state.counters["finalized decimal value accesses"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, unwrap_dynamic_decimal) (benchmark::State& state) {
  // Generate some random strings.
  std::vector<unsafe_heap::number> dbls(1024);
  std::generate(dbls.begin(), dbls.end(), [&] {
    return unsafe_heap::number {static_cast<double>(rand_int())};
  });

  // Benchmark unwrapping the double into a machine type.
  auto const pkts = dbls.size();
  for (auto _ : state) {
    for (auto const& dbl : dbls) benchmark::DoNotOptimize(dbl.decimal());
    rate_counter += pkts;
  }
  state.counters["dynamic decimal value accesses"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, unwrap_finalized_boolean) (benchmark::State& state) {
  // Generate some random strings.
  std::vector<unsafe_buffer::flag> flags;
  for (auto i = 0; i < 1024; ++i) {
    flags.emplace_back(unsafe_buffer::object {"flag", rand_int() % 2 == 0}["flag"]);
  }

  // Benchmark unwrapping the double into a machine type.
  auto const pkts = flags.size();
  for (auto _ : state) {
    for (auto const& flag : flags) benchmark::DoNotOptimize(flag.boolean());
    rate_counter += pkts;
  }
  state.counters["finalized boolean value accesses"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, unwrap_dynamic_boolean) (benchmark::State& state) {
  // Generate some random strings.
  std::vector<unsafe_heap::flag> flags(1024);
  std::generate(flags.begin(), flags.end(), [&] {
    return unsafe_heap::flag {rand_int() % 2 == 0};
  });

  // Benchmark unwrapping the double into a machine type.
  auto const pkts = flags.size();
  for (auto _ : state) {
    for (auto const& flag : flags) benchmark::DoNotOptimize(flag.boolean());
    rate_counter += pkts;
  }
  state.counters["dynamic boolean value accesses"] = rate_counter;
}

BENCHMARK_MAIN();

/*----- Helper Implementations -----*/

void benchmark_helper::SetUp(benchmark::State const&) {
  // Initialize all of the various types of packets we're maintaining.
  flat = generate_dynamic_flat_packet();
  nested = generate_dynamic_nested_packet();
  std::tie(flat_json, flat_fin) = generate_finalized_packet(flat);
  std::tie(nested_json, nested_fin) = generate_finalized_packet(nested);

  // Generate a vector of keys for the flat packet.
  std::vector<std::string> tmp;
  auto keys = flat.keys();
  for (auto const& key : keys) tmp.push_back(key.str());
  flat_keys = std::move(tmp);
  
  // Do the same for the nested packet.
  keys = nested.keys();
  for (auto const& key : keys) tmp.push_back(key.str());
  nested_keys = std::move(tmp);

  // Initialize our counter.
  rate_counter = benchmark::Counter(0, benchmark::Counter::kIsRate);
}

void benchmark_helper::TearDown(benchmark::State const&) {

}

unsafe_packet benchmark_helper::generate_dynamic_flat_packet() const {
  std::string album = "dark side of the moon";
  auto base = unsafe_packet::make_object("speak to me", album, "breathe", album, "on the run", album, "time", album);
  base.add_field("the great gig in the sky", album).add_field("money", album).add_field("us and them", album);
  base.add_field("any colour you like", album).add_field("brain damage", album).add_field("eclipse", album);
  album = "wish you were here";
  base.add_field("shine on you crazy diamond 1-5", album).add_field("welcome to the machine", album).add_field("have a cigar", album);
  return base.add_field(album, album).add_field("shine on you crazy diamond 6-9", album);
}

unsafe_packet benchmark_helper::generate_dynamic_nested_packet() const {
  auto base = unsafe_packet::make_object();

  // Construct our first album.
  auto album = unsafe_packet::make_array("speak to me", "breathe", "on the run", "time", "the great gig in the sky");
  album.push_back("money").push_back("us and them").push_back("any colour you like").push_back("brain damage").push_back("eclipse");
  base.add_field("dark side of the moon", std::move(album));

  // Construct our second album.
  album = unsafe_packet::make_array("shine on you crazy diamond 1-5", "welcome to the machine", "have a cigar");
  album.push_back("wish you were here").push_back("shine on you crazy diamond 6-9");
  base.add_field("wish you were here", std::move(album));
  return base;
}

std::tuple<std::string, unsafe_packet> benchmark_helper::generate_finalized_packet(unsafe_packet base) const {
#if DART_HAS_RAPIDJSON
  return {base.to_json(), base.finalize()};
#else
  return {"", base.finalize()};
#endif
}
