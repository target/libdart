/*----- System Includes -----*/

#include <cstdio>

/*----- Local Includes -----*/

#include "dart_tests.h"

/*----- Globals -----*/

// This global exists to try to force the compiler to actually
// run the explore function, as otherwise it doesn't really have
// any side-effects
constexpr size_t input_len = 128;
constexpr size_t output_len = 1024;
thread_local char volatile* dummy_output;

/*----- Function Implementations -----*/

// Function runs DFS across the given packet, reaching all leaf values,
// to ensure that validated buffers are actually usable.
void explore(dart::buffer::view pkt) {
  switch (pkt.get_type()) {
    case dart::buffer::type::object:
      {
        dart::buffer::view::iterator kit, vit;
        std::tie(kit, vit) = pkt.kvbegin();
        while (vit != pkt.end()) {
          explore(*kit);
          explore(*vit);
          ++kit, ++vit;
        }
      }
      break;
    case dart::buffer::type::array:
      for (auto val : pkt) explore(val);
      break;
    case dart::buffer::type::string:
      {
        dart::buffer::string::view sv {pkt};
        for (auto i = 0; i < std::min(sv.size(), output_len); ++i) {
          dummy_output[i] = sv[i];
        }
      }
      break;
    case dart::buffer::type::integer:
      *reinterpret_cast<int64_t volatile*>(dummy_output) = pkt.integer();
      break;
    case dart::buffer::type::decimal:
      *reinterpret_cast<double volatile*>(dummy_output) = pkt.decimal();
      break;
    case dart::buffer::type::boolean:
      *reinterpret_cast<bool volatile*>(dummy_output) = pkt.boolean();
      break;
    default:
      DART_ASSERT(pkt.get_type() == dart::buffer::type::null);
  }
}

extern "C" {

  int LLVMFuzzerTestOneInput(gsl::byte const* data, size_t size) {
    dummy_output = new char volatile[output_len];

    auto bytes = gsl::make_span(data, size);
    if (dart::is_valid(bytes)) {
      dart::buffer buff {bytes};
      explore(buff);
    }

    delete[] dummy_output;
    return 0;
  }

}

#ifdef DART_USING_AFL

int main() {
  // Reopen stdin in binary mode
  std::freopen(nullptr, "rb", stdin);

  // Read from stdin until we hit EOF
  size_t len;
  std::vector<char> storage;
  std::array<char, input_len> buff;
  while ((len = std::fread(buff.data(), sizeof(char), buff.size(), stdin)) > 0) {
    storage.insert(storage.end(), buff.data(), buff.data() + len);
  }

  // Pass to our fuzzer function.
  LLVMFuzzerTestOneInput(reinterpret_cast<gsl::byte const*>(storage.data()), storage.size());
}

#endif
