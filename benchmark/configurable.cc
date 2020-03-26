/*----- System Includes -----*/

#include <fstream>
#include <numeric>
#include <algorithm>
#include <benchmark/benchmark.h>

#ifdef DART_USE_SAJSON
#include <sajson.h>
#endif

#ifdef DART_HAS_YAJL
#include <yajl/yajl_gen.h>
#include <yajl/yajl_tree.h>
#endif

#ifdef DART_HAS_JANSSON
#include <jansson.h>
#endif

#ifdef DART_HAS_NLJSON
#include <nlohmann/json.hpp>
#endif

#if DART_HAS_FLEXBUFFERS
#include <flatbuffers/flexbuffers.h>
#endif

/*----- Local Includes -----*/

#include "../include/dart.h"

/*----- Globals -----*/

auto extract_directory(dart::shim::string_view path) noexcept {
  return path.substr(0, path.find_last_of("/"));
}

static auto base_dir = extract_directory(__FILE__);
static auto byte_counter = [] (auto a, auto& s) { return a + s.size(); };
static std::string const json_input = std::string {base_dir} + "/input.json";

/*----- Type Declarations -----*/

#ifdef DART_HAS_NLJSON
namespace nl = nlohmann;
#endif
namespace rj = rapidjson;

using unsafe_heap = dart::basic_heap<dart::unsafe_ptr>;
using unsafe_buffer = dart::basic_buffer<dart::unsafe_ptr>;
using unsafe_packet = dart::basic_packet<dart::unsafe_ptr>;

#ifdef DART_HAS_YAJL
struct yajl_owner {
  yajl_owner() = default;
  yajl_owner(yajl_val val) noexcept : val(val) {}
  yajl_owner(yajl_owner const&) = delete;
  yajl_owner(yajl_owner&& other) noexcept : val(other.val) {
    other.val = nullptr;
  }
  ~yajl_owner() {
    if (val) yajl_tree_free(val);
  }

  yajl_owner& operator =(yajl_owner const&) = delete;
  yajl_owner& operator =(yajl_owner&& other) noexcept {
    if (this == &other) return *this;
    this->~yajl_owner();
    new(this) yajl_owner(std::move(other));
    return *this;
  }
  yajl_owner& operator =(yajl_val other) noexcept {
    this->~yajl_owner();
    new(this) yajl_owner(other);
    return *this;
  }

  yajl_val val;
};
#endif

#ifdef DART_HAS_JANSSON
struct jansson_owner {
  jansson_owner() = default;
  jansson_owner(json_t* val) : val(val) {}
  jansson_owner(jansson_owner const&) = delete;
  jansson_owner(jansson_owner&& other) noexcept : val(other.val) {
    other.val = nullptr;
  }
  ~jansson_owner() {
    if (val) json_decref(val);
  }

  jansson_owner& operator =(jansson_owner const&) = delete;
  jansson_owner& operator =(jansson_owner&& other) noexcept {
    if (this == &other) return *this;
    this->~jansson_owner();
    new(this) jansson_owner(std::move(other));
    return *this;
  }
  jansson_owner& operator =(json_t* other) noexcept {
    this->~jansson_owner();
    new(this) jansson_owner(other);
    return *this;
  }

  json_t* val;
};
#endif

struct benchmark_helper : benchmark::Fixture {

  /*----- Test Construction/Destruction Functions -----*/

  void SetUp(benchmark::State const&) override;
  void TearDown(benchmark::State const&) override {}

  /*----- Helpers -----*/

  std::vector<std::string> load_input(dart::shim::string_view path) const;
  std::vector<unsafe_buffer> parse_input_dart(gsl::span<std::string const> packets) const;
  std::vector<unsafe_heap> parse_mutable_dart(gsl::span<std::string const> packets) const;
  std::vector<rj::Document> parse_input_rj(gsl::span<std::string const> packets) const;
#ifdef DART_USE_SAJSON
  std::vector<sajson::document> parse_input_sajson(gsl::span<std::string const> packets) const;
#endif
#ifdef DART_HAS_NLJSON
  std::vector<nl::json> parse_input_nljson(gsl::span<std::string const> packets) const;
#endif
#ifdef DART_HAS_YAJL
  std::vector<yajl_owner> parse_input_yajl(gsl::span<std::string const> packets) const;
#endif
#ifdef DART_HAS_JANSSON
  std::vector<jansson_owner> parse_input_jansson(gsl::span<std::string const> packets) const;
#endif
  std::vector<std::vector<std::string>> discover_keys(gsl::span<unsafe_buffer const> packets) const;

  /*----- Members -----*/

  std::vector<std::string> input;
  std::vector<unsafe_buffer> parsed_dart;
  std::vector<unsafe_heap> mutable_dart;
  std::vector<rj::Document> parsed_rj;
#ifdef DART_USE_SAJSON
  std::vector<sajson::document> parsed_sajson;
#endif
#ifdef DART_HAS_NLJSON
  std::vector<nl::json> parsed_nljson;
#endif
#ifdef DART_HAS_YAJL
  std::vector<yajl_owner> parsed_yajl;
#endif
#ifdef DART_HAS_JANSSON
  std::vector<jansson_owner> parsed_jansson;
#endif
  std::vector<std::vector<std::string>> keys;

  benchmark::Counter rate_counter;

};

/*----- Helpers -----*/

template <class Idx>
constexpr bool all_equal(Idx) {
  return true;
}
template <class Lhs, class Rhs, class... Idxs>
constexpr bool all_equal(Lhs lidx, Rhs ridx, Idxs... lens) {
  return lidx == ridx && all_equal(ridx, lens...);
}

template <class... Containers, class Func>
constexpr void for_multi(Func&& cb, Containers&&... cs) {
  // Setup ADL.
  using std::end;
  using std::begin;

  // Validation.
  if (!all_equal(cs.size()...)) {
    throw std::invalid_argument("All containers must be of the same size");
  }

  // Get a tuple of iterators.
  [&] (auto first, auto... rest) {
    // Get an end iterator to use as a sentinel.
    auto& end = std::get<1>(first);
    auto& curr = std::get<0>(first);

    // Walk each iterator along together.
    while (curr != end) {
      // Dereference.
      cb(*curr, *std::get<0>(rest)...);

      // Increment our iterators.
      ++curr;
      (void) std::initializer_list<int> {(++std::get<0>(rest), 0)...};
    }
  }(std::make_tuple(begin(cs), end(cs))...);
}

// This benchmark is getting out of hand.
#ifdef DART_HAS_YAJL
void yajl_serialize(yajl_val curr, yajl_gen handle) {
  yajl_gen_status ret;
  switch (curr->type) {
    case yajl_t_object:
      {
        ret = yajl_gen_map_open(handle);
        assert(ret == yajl_gen_status_ok);
        auto* obj = YAJL_GET_OBJECT(curr);
        for (auto i = 0U; i < obj->len; ++i) {
          auto* key = obj->keys[i];
          ret = yajl_gen_string(handle, reinterpret_cast<unsigned char const*>(key), strlen(key));
          assert(ret == yajl_gen_status_ok);
          yajl_serialize(obj->values[i], handle);
        }
        ret = yajl_gen_map_close(handle);
        assert(ret == yajl_gen_status_ok);
      }
      break;
    case yajl_t_array:
      {
        ret = yajl_gen_array_open(handle);
        assert(ret == yajl_gen_status_ok);
        auto* arr = YAJL_GET_ARRAY(curr);
        for (auto i = 0U; i < arr->len; ++i) yajl_serialize(arr->values[i], handle);
        ret = yajl_gen_array_close(handle);
        assert(ret == yajl_gen_status_ok);
      }
      break;
    case yajl_t_string:
      {
        auto* str = YAJL_GET_STRING(curr);
        ret = yajl_gen_string(handle, reinterpret_cast<unsigned char const*>(str), strlen(str));
      }
      break;
    case yajl_t_number:
      if (YAJL_IS_INTEGER(curr)) {
        ret = yajl_gen_integer(handle, YAJL_GET_INTEGER(curr));
      } else {
        ret = yajl_gen_double(handle, YAJL_GET_DOUBLE(curr));
      }
      break;
    case yajl_t_true:
      ret = yajl_gen_bool(handle, false);
      break;
    case yajl_t_false:
      ret = yajl_gen_bool(handle, false);
      break;
    default:
      assert(curr->type == yajl_t_null);
      ret = yajl_gen_null(handle);
      break;
  }
  assert(ret == yajl_gen_status_ok);
}
#endif

#if DART_HAS_FLEXBUFFERS
void convert_dart_to_fb(unsafe_buffer pkt, flexbuffers::Builder& fbb, char const* currkey = nullptr) {
  switch (pkt.get_type()) {
    case unsafe_buffer::type::object:
      {
        unsafe_buffer::iterator k, v;
        std::tie(k, v) = pkt.kvbegin();
        
        // Flexbuffer builder API is kind of awkward to use
        // in a recursive function like this, but we can get it done
        auto work = [&] {
          while (v != pkt.end()) {
            convert_dart_to_fb(*v, fbb, k->str());
            ++k, ++v;
          }
        };

        // Need to call differently depending on if we're already
        // in the process of building an object
        if (currkey) fbb.Map(currkey, work);
        else fbb.Map(work);
      }
      break;
    case unsafe_buffer::type::array:
      {
        // Flexbuffer builder API is kind of awkward to use
        // in a recursive function like this, but we can get it done
        auto work = [&] {
          for (auto v : pkt) {
            convert_dart_to_fb(v, fbb);
          }
        };

        // Need to call differently depending on if we're already
        // in the process of building an object
        if (currkey) fbb.Vector(currkey, work);
        else fbb.Vector(work);
      }
      break;
    case unsafe_buffer::type::string:
      fbb.String(pkt.str());
      break;
    case unsafe_buffer::type::integer:
      fbb.Int(pkt.integer());
      break;
    case unsafe_buffer::type::decimal:
      fbb.Float(pkt.decimal());
      break;
    case unsafe_buffer::type::boolean:
      fbb.Bool(pkt.boolean());
      break;
    default:
      fbb.Null();
      break;
  }
}
#endif

/*----- Benchmark Definitions -----*/

BENCHMARK_F(benchmark_helper, dart_nontrivial_finalized_json_test) (benchmark::State& state) {
  for (auto _ : state) {
    for (auto const& pkt : input) {
      benchmark::DoNotOptimize(unsafe_buffer::from_json(pkt));
      rate_counter++;
    }
  }

  auto bytes = std::accumulate(std::begin(input), std::end(input), 0, byte_counter);
  state.SetBytesProcessed(bytes * state.iterations());
  state.counters["parsed packets"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, dart_nontrivial_dynamic_json_test) (benchmark::State& state) {
  auto chunk = input.size();
  for (auto _ : state) {
    for (auto const& pkt : input) benchmark::DoNotOptimize(unsafe_heap::from_json(pkt));
    rate_counter += chunk;
  }

  auto bytes = std::accumulate(std::begin(input), std::end(input), 0, byte_counter);
  state.SetBytesProcessed(bytes * state.iterations());
  state.counters["parsed packets"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, dart_nontrivial_finalized_json_generation_test) (benchmark::State& state) {
  auto chunk = input.size();
  for (auto _ : state) {
    for (auto const& pkt : parsed_dart) benchmark::DoNotOptimize(pkt.to_json());
    rate_counter += chunk;
  }
  state.counters["serialized packets"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, dart_nontrivial_dynamic_json_generation_test) (benchmark::State& state) {
  // Convert our finalized packets into dynamic ones for serialization.
  auto chunk = input.size();
  std::vector<unsafe_heap> dynamic(parsed_dart.size());
  std::transform(parsed_dart.begin(), parsed_dart.end(), dynamic.begin(), [&] (auto& pkt) {
    return unsafe_heap {pkt};
  });

  // Run the benchmark.
  for (auto _ : state) {
    for (auto const& pkt : dynamic) benchmark::DoNotOptimize(pkt.to_json());
    rate_counter += chunk;
  }
  state.counters["serialized packets"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, dart_nontrivial_json_key_lookups) (benchmark::State& state) {
  auto chunk = input.size();
  for (auto _ : state) {
    for_multi([&] (auto& pkt, auto& keys) {
      for (auto const& key : keys) benchmark::DoNotOptimize(pkt[key].get_type());
      rate_counter += chunk;
    }, parsed_dart, keys);
  }
  state.counters["parsed key lookups"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, dart_nontrivial_json_finalizing) (benchmark::State& state) {
  int64_t bytes = 0;
  for (auto _ : state) {
    for (auto const& pkt : mutable_dart) {
      auto finalized = pkt.lower();
      auto buf = finalized.get_bytes();
      benchmark::DoNotOptimize(buf.data());
      rate_counter++;
      bytes += buf.size();
    }
  }

  state.SetBytesProcessed(bytes);
  state.counters["parsed packets"] = rate_counter;
}

#if DART_HAS_FLEXBUFFERS
BENCHMARK_F(benchmark_helper, flexbuffer_nontrivial_json_finalizing) (benchmark::State& state) {
  int64_t bytes = 0;
  for (auto _ : state) {
    for (auto const& pkt : parsed_dart) {
      // Lay out the flexbuffer
      flexbuffers::Builder fbb;
      convert_dart_to_fb(pkt, fbb);

      // Finish it.
      fbb.Finish();
      rate_counter++;
      bytes += fbb.GetBuffer().size();
    }
  }

  state.SetBytesProcessed(bytes);
  state.counters["parsed packets"] = rate_counter;
}
#endif

BENCHMARK_F(benchmark_helper, rapidjson_nontrivial_insitu_json_test) (benchmark::State& state) {
  auto chunk = input.size();
  for (auto _ : state) {
    for (auto const& pkt : input) {
      // Copy the string explicitly in case we're running on Redhat with the COW string.
      auto copy = std::make_unique<char[]>(pkt.size() + 1);
      std::copy_n(pkt.data(), pkt.size(), copy.get());
      copy[pkt.size()] = '\0';

      // Fire up the RapidJSON in-situ parser.
      rj::Document doc;
      benchmark::DoNotOptimize(doc.ParseInsitu(copy.get()).HasParseError());
    }
    rate_counter += chunk;
  }

  auto bytes = std::accumulate(std::begin(input), std::end(input), 0, byte_counter);
  state.SetBytesProcessed(bytes * state.iterations());
  state.counters["parsed packets"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, rapidjson_nontrivial_json_test) (benchmark::State& state) {
  auto chunk = input.size();
  for (auto _ : state) {
    for (auto const& pkt : input) {
      // Fire up the RapidJSON in-situ parser.
      rj::Document doc;
      benchmark::DoNotOptimize(doc.Parse(pkt.data()).HasParseError());
    }
    rate_counter += chunk;
  }

  auto bytes = std::accumulate(std::begin(input), std::end(input), 0, byte_counter);
  state.SetBytesProcessed(bytes * state.iterations());
  state.counters["parsed packets"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, rapidjson_nontrivial_json_key_lookups) (benchmark::State& state) {
  auto chunk = input.size();
  for (auto _ : state) {
    for_multi([&] (auto& pkt, auto& keys) {
      for (auto const& key : keys) benchmark::DoNotOptimize(pkt[key.data()].GetType());
      rate_counter += chunk;
    }, parsed_rj, keys);
  }
  state.counters["parsed key lookups"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, rapidjson_nontrivial_json_generation_test) (benchmark::State& state) {
  auto chunk = input.size();
  for (auto _ : state) {
    for (auto const& pkt : parsed_rj) {
      rj::StringBuffer buf;
      rj::Writer<rj::StringBuffer> writer(buf);
      pkt.Accept(writer);
      benchmark::DoNotOptimize(buf.GetString());
    }
    rate_counter += chunk;
  }
  state.counters["serialized packets"] = rate_counter;
}

#ifdef DART_USE_SAJSON
BENCHMARK_F(benchmark_helper, sajson_nontrivial_json_test) (benchmark::State& state) {
  auto chunk = input.size();
  for (auto _ : state) {
    for (auto const& pkt : input) {
      // sajson is ridiculously fast, but its API is genuinely awful
      auto copy = sajson::mutable_string_view {sajson::string {pkt.data(), pkt.size()}};
      auto doc = sajson::parse(sajson::single_allocation {}, std::move(copy));
      benchmark::DoNotOptimize(doc.is_valid());
    }
    rate_counter += chunk;
  }

  auto bytes = std::accumulate(std::begin(input), std::end(input), 0, byte_counter);
  state.SetBytesProcessed(bytes * state.iterations());
  state.counters["parsed packets"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, sajson_nontrivial_json_key_lookups) (benchmark::State& state) {
  auto chunk = input.size();
  for (auto _ : state) {
    for_multi([&] (auto& pkt, auto& keys) {
      auto root = pkt.get_root();
      for (auto const& key : keys) {
        sajson::string str {key.data(), key.size()};
        benchmark::DoNotOptimize(root.get_value_of_key(str).get_type());
      }
      rate_counter += chunk;
    }, parsed_sajson, keys);
  }
  state.counters["parsed key lookups"] = rate_counter;
}
#endif

#ifdef DART_HAS_NLJSON
BENCHMARK_F(benchmark_helper, nlohmann_json_nontrivial_json_test) (benchmark::State& state) {
  auto chunk = input.size();
  for (auto _ : state) {
    for (auto const& pkt : input) benchmark::DoNotOptimize(nl::json::parse(pkt));
    rate_counter += chunk;
  }

  auto bytes = std::accumulate(std::begin(input), std::end(input), 0, byte_counter);
  state.SetBytesProcessed(bytes * state.iterations());
  state.counters["parsed packets"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, nlohmann_json_nontrivial_json_key_lookups) (benchmark::State& state) {
  auto chunk = input.size();
  for (auto _ : state) {
    for_multi([&] (auto& pkt, auto& keys) {
      for (auto const& key : keys) benchmark::DoNotOptimize(pkt[key].type());
      rate_counter += chunk;
    }, parsed_nljson, keys);
  }
  state.counters["parsed key lookups"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, nlohmann_json_nontrivial_json_generation_test) (benchmark::State& state) {
  auto chunk = input.size();
  for (auto _ : state) {
    for (auto const& pkt : parsed_nljson) benchmark::DoNotOptimize(pkt.dump());
    rate_counter += chunk;
  }
  state.counters["serialized packets"] = rate_counter;
}
#endif

#ifdef DART_HAS_YAJL
BENCHMARK_F(benchmark_helper, yajl_nontrivial_json_test) (benchmark::State& state) {
  auto chunk = input.size();
  for (auto _ : state) {
    for (auto const& pkt : input) {
      yajl_owner owner = yajl_tree_parse(pkt.data(), nullptr, 0);
      benchmark::DoNotOptimize(owner.val);
    }
    rate_counter += chunk;
  }

  auto bytes = std::accumulate(std::begin(input), std::end(input), 0, byte_counter);
  state.SetBytesProcessed(bytes * state.iterations());
  state.counters["parsed packets"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, yajl_nontrivial_json_key_lookups) (benchmark::State& state) {
  auto chunk = input.size();
  for (auto _ : state) {
    for_multi([&] (auto& pkt, auto& keys) {
      for (auto const& key : keys) {
        char const* path[] = { key.data(), nullptr };
        auto* found = yajl_tree_get(pkt.val, path, yajl_t_any);
        benchmark::DoNotOptimize(found->type);
      }
      rate_counter += chunk;
    }, parsed_yajl, keys);
  }
  state.counters["parsed key lookups"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, yajl_nontrivial_json_generation_test) (benchmark::State& state) {
  auto chunk = input.size();
  for (auto _ : state) {
    for (auto const& pkt : parsed_yajl) {
      // Have Yajl recursively generate json for this packet.
      auto* gen = yajl_gen_alloc(nullptr);
      yajl_serialize(pkt.val, gen);

      // Get the json.
      size_t len;
      unsigned char const* ptr;
      yajl_gen_get_buf(gen, &ptr, &len);
      benchmark::DoNotOptimize(ptr);

      // Free the generator.
      yajl_gen_free(gen);
    }
    rate_counter += chunk;
  }
  state.counters["serialized packets"] = rate_counter;
}
#endif

#ifdef DART_HAS_JANSSON
BENCHMARK_F(benchmark_helper, jansson_nontrivial_json_test) (benchmark::State& state) {
  auto chunk = input.size();
  for (auto _ : state) {
    for (auto const& pkt : input) {
      jansson_owner owner = json_loads(pkt.data(), 0, nullptr);
      benchmark::DoNotOptimize(owner.val);
    }
    rate_counter += chunk;
  }

  auto bytes = std::accumulate(std::begin(input), std::end(input), 0, byte_counter);
  state.SetBytesProcessed(bytes * state.iterations());
  state.counters["parsed packets"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, jansson_nontrivial_json_key_lookups) (benchmark::State& state) {
  auto chunk = input.size();
  for (auto _ : state) {
    for_multi([&] (auto& pkt, auto& keys) {
      for (auto const& key : keys) {
        auto* found = json_object_get(pkt.val, key.data());
        benchmark::DoNotOptimize(found->type);
      }
      rate_counter += chunk;
    }, parsed_jansson, keys);
  }
  state.counters["parsed key lookups"] = rate_counter;
}

BENCHMARK_F(benchmark_helper, jansson_nontrivial_json_generation_test) (benchmark::State& state) {
  auto chunk = input.size();
  for (auto _ : state) {
    for (auto const& pkt : parsed_jansson) {
      auto* owner = json_dumps(pkt.val, 0);
      benchmark::DoNotOptimize(owner);
      free(owner);
    }
    rate_counter += chunk;
  }
  state.counters["serialized packets"] = rate_counter;
}
#endif

BENCHMARK_MAIN();

/*----- Helper Implementations -----*/

void benchmark_helper::SetUp(benchmark::State const&) {
  input = load_input(json_input);
  parsed_dart = parse_input_dart(input);
  mutable_dart = parse_mutable_dart(input);
  parsed_rj = parse_input_rj(input);
#ifdef DART_USE_SAJSON
  parsed_sajson = parse_input_sajson(input);
#endif
#ifdef DART_HAS_NLJSON
  parsed_nljson = parse_input_nljson(input);
#endif
#ifdef DART_HAS_YAJL
  parsed_yajl = parse_input_yajl(input);
#endif
#ifdef DART_HAS_JANSSON
  parsed_jansson = parse_input_jansson(input);
#endif
  keys = discover_keys(parsed_dart);
  rate_counter = benchmark::Counter(0, benchmark::Counter::kIsRate);
}

std::vector<std::string> benchmark_helper::load_input(dart::shim::string_view path) const {
  // Read our file in.
  std::string line;
  std::ifstream input(std::string {path});
  std::vector<std::string> packets;
  while (std::getline(input, line)) packets.push_back(std::move(line));
  return packets;
}

std::vector<unsafe_buffer> benchmark_helper::parse_input_dart(gsl::span<std::string const> packets) const {
  std::vector<unsafe_buffer> parsed(packets.size());
  std::transform(packets.begin(), packets.end(), parsed.begin(), [] (auto& pkt) {
    return unsafe_buffer::from_json(pkt);
  });
  return parsed;
}

std::vector<unsafe_heap> benchmark_helper::parse_mutable_dart(gsl::span<std::string const> packets) const {
  std::vector<unsafe_heap> parsed(packets.size());
  std::transform(packets.begin(), packets.end(), parsed.begin(), [] (auto& pkt) {
    return unsafe_heap::from_json(pkt);
  });
  return parsed;
}

std::vector<rj::Document> benchmark_helper::parse_input_rj(gsl::span<std::string const> packets) const {
  std::vector<rj::Document> parsed(packets.size());
  std::transform(packets.begin(), packets.end(), parsed.begin(), [] (auto& pkt) {
    rj::Document doc;
    if (doc.Parse(pkt.data()).HasParseError()) {
      throw std::runtime_error("Failed to parse a packet, check the input");
    }
    return doc;
  });
  return parsed;
}

#ifdef DART_USE_SAJSON
std::vector<sajson::document> benchmark_helper::parse_input_sajson(gsl::span<std::string const> packets) const {
  std::vector<sajson::document> parsed;
  parsed.reserve(packets.size());
  for (auto const& pkt : packets) {
    auto copy = sajson::mutable_string_view {sajson::string {pkt.data(), pkt.size()}};
    parsed.push_back(sajson::parse(sajson::dynamic_allocation {}, std::move(copy)));
    if (!parsed.back().is_valid()) throw std::runtime_error("Failed to parse packet, check the input");
  }
  return parsed;
}
#endif

#ifdef DART_HAS_NLJSON
std::vector<nl::json> benchmark_helper::parse_input_nljson(gsl::span<std::string const> packets) const {
  std::vector<nl::json> parsed(packets.size());
  std::transform(packets.begin(), packets.end(), parsed.begin(), [] (auto& pkt) { return nl::json::parse(pkt); });
  return parsed;
}
#endif

#ifdef DART_HAS_YAJL
std::vector<yajl_owner> benchmark_helper::parse_input_yajl(gsl::span<std::string const> packets) const {
  std::vector<yajl_owner> parsed(packets.size());
  std::transform(packets.begin(), packets.end(), parsed.begin(), [] (auto& pkt) {
    auto* val = yajl_tree_parse(pkt.data(), nullptr, 0);
    if (!val) throw std::runtime_error("Failed to parse packet, check the input");
    return val;
  });
  return parsed;
}
#endif

#ifdef DART_HAS_JANSSON
std::vector<jansson_owner> benchmark_helper::parse_input_jansson(gsl::span<std::string const> packets) const {
  std::vector<jansson_owner> parsed(packets.size());
  std::transform(packets.begin(), packets.end(), parsed.begin(), [] (auto& pkt) {
    auto* val = json_loads(pkt.data(), 0, nullptr);
    if (!val) throw std::runtime_error("Failed to parse packet, check the input");
    return val;
  });
  return parsed;
}
#endif

std::vector<std::vector<std::string>> benchmark_helper::discover_keys(gsl::span<unsafe_buffer const> packets) const {
  std::vector<std::vector<std::string>> keys(packets.size());
  std::transform(packets.begin(), packets.end(), keys.begin(), [] (auto& pkt) {
    // Collect the keys as strings.
    std::vector<std::string> keys(pkt.size());
    std::transform(pkt.key_begin(), pkt.key_end(), keys.begin(), [] (auto p) { return p.str(); });
    return keys;
  });
  return keys;
}
