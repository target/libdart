/*----- System Includes -----*/

#include <random>
#include <cassert>
#include <algorithm>
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

/*---- Helpers -----*/

std::string rand_string(size_t len);

/*----- Benchmark Definitions -----*/

#if DART_HAS_RAPIDJSON
BENCHMARK_F(benchmark_helper, parse_flat_packet) (benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(unsafe_heap::from_json(flat_json));
    ++rate_counter;
  }
  state.counters["parsed flat packets"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, parse_nested_packet) (benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(unsafe_heap::from_json(nested_json));
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
  unsafe_buffer::object data {pkt};
  for (auto _ : state) {
    for (auto const& key : keys) benchmark::DoNotOptimize(data[key]);
    rate_counter += data.size();
  }
  state.counters["finalized random field lookups"] = rate_counter;
}

BENCHMARK_REGISTER_F(benchmark_helper, lookup_finalized_random_fields)->Ranges({{1 << 0, 1 << 8}, {1 << 2, 1 << 8}});

#ifdef DART_HAS_FLEXBUFFERS
BENCHMARK_DEFINE_F(benchmark_helper, flexbuffer_lookup_finalized_random_fields) (benchmark::State& state) {
  // Generate some random strings.
  std::vector<std::string> keys(state.range(0));
  std::generate(keys.begin(), keys.end(), [&] { return rand_string(state.range(1)); });

  // Build a flexbuffer.
  flexbuffers::Builder fbb;
  fbb.Map([&] {
    for (auto& key : keys) fbb.String(key.data(), key.data());
  });
  fbb.Finish();

  // Expand it.
  auto buffer = fbb.GetBuffer();
  auto pkt = flexbuffers::GetRoot(buffer).AsMap();

  // Run the test.
  for (auto _ : state) {
    for (auto& key : keys) benchmark::DoNotOptimize(pkt[key.data()]);
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
  for (auto& key : keys) tmp.add_field(key, key);
  auto json = tmp.to_json();
  auto doc = sajson::parse(sajson::single_allocation(), sajson::string {json.data(), json.size()});

  // Run the test.
  auto obj = doc.get_root();
  for (auto _ : state) {
    for (auto& key : keys) {
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

BENCHMARK_F(benchmark_helper, insert_into_exclusive_dynamic_packet) (benchmark::State& state) {
  // Generate a huge number of exclusively owned packets in one go
  // so that we can focus on timing what we're interested in.
  auto gen = [&] {
    std::vector<unsafe_heap> pkts(1 << 10);
    for (auto& p : pkts) p = unsafe_heap::transmogrify<dart::unsafe_ptr>(unsafe_heap {flat});
    return pkts;
  };

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
  state.counters["exclusive dynamic packet field modifications"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, insert_into_shared_dynamic_packet) (benchmark::State& state) {
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
  state.counters["shared dynamic packet field modifications"] = rate_counter;
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

std::string rand_string(size_t len) {
  static std::mt19937 engine(std::random_device{}());
  static std::vector<char> alpha {
    'a', 'b', 'c', 'd', 'e', 'f', 'g',
    'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u',
    'v', 'w', 'x', 'y', 'z'
  };

  std::string retval(len, 0);
  std::uniform_int_distribution<> dist(0, alpha.size() - 1);
  std::generate(retval.begin(), retval.end(), [&] { return alpha[dist(engine)]; });
  return retval;
}

std::tuple<std::string, unsafe_packet> benchmark_helper::generate_finalized_packet(unsafe_packet base) const {
#if DART_HAS_RAPIDJSON
  return {base.to_json(), base.finalize()};
#else
  return {"", base.finalize()};
#endif
}
