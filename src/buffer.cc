/*----- Local Includes -----*/

#include "helpers.h"

/*----- Helpers -----*/

namespace {

  // XXX: Thank microsoft for this whole mess.
  // MSVC can't do generic lambdas inside of extern C functions.
  // It ends up improperly propagating the function linkage to the lambda,
  // and it explodes since a template can't have C language linkage.
  // Apparently some previews of Visual Studio 2019 have this fixed, but we'll just
  // hack around it for now.
  // So like half of the functions in the ABI have to call through an implementation
  // function with internal linkage.
  int dart_buffer_obj_has_key_len_impl(dart_buffer_t const* src, char const* key, size_t len) {
    bool val = false;
    auto err = buffer_access(
      [&val, key, len] (auto& src) { val = src.has_key(string_view {key, len}); },
      src
    );
    if (err) return false;
    else return val;
  }

  char const* dart_buffer_str_get_len_impl(dart_buffer_t const* src, size_t* len) {
    char const* str;
    auto get_str = [&] (auto& src) {
      auto view = src.strv();
      str = view.data();
      *len = view.size();
    };
    auto err = buffer_access(
      compose(
        [get_str] (dart::buffer const& src) { get_str(src); },
        [get_str] (dart::unsafe_buffer const& src) { get_str(src); }
      ),
      src
    );
    if (err) return nullptr;
    else return str;
  }

  dart_err_t dart_buffer_int_get_err_impl(dart_buffer_t const* src, int64_t* val) {
    auto get_int = [=] (auto& src) { *val = src.integer(); };
    return buffer_access(
      compose(
        [get_int] (dart::buffer const& src) { get_int(src); },
        [get_int] (dart::unsafe_buffer const& src) { get_int(src); }
      ),
      src
    );
  }

  dart_err_t dart_buffer_dcm_get_err_impl(dart_buffer_t const* src, double* val) {
    auto get_dcm = [=] (auto& src) { *val = src.decimal(); };
    return buffer_access(
      compose(
        [get_dcm] (dart::buffer const& src) { get_dcm(src); },
        [get_dcm] (dart::unsafe_buffer const& src) { get_dcm(src); }
      ),
      src
    );
  }

  dart_err_t dart_buffer_bool_get_err_impl(dart_buffer_t const* src, int* val) {
    auto get_bool = [=] (auto& src) { *val = src.boolean(); };
    return buffer_access(
      compose(
        [get_bool] (dart::buffer const& src) { get_bool(src); },
        [get_bool] (dart::unsafe_buffer const& src) { get_bool(src); }
      ),
      src
    );
  }

  size_t dart_buffer_size_impl(dart_buffer_t const* src) {
    size_t val = 0;
    auto err = buffer_access([&val] (auto& src) { val = src.size(); }, src);
    if (err) return DART_FAILURE;
    else return val;
  }

  int dart_buffer_equal_impl(dart_buffer_t const* lhs, dart_buffer_t const* rhs) {
    bool equal = false;
    auto check = [&] (auto& lhs, auto& rhs) { equal = (lhs == rhs); };
    auto err = buffer_access(
      compose(
        [check, rhs] (dart::buffer const& lhs) {
          buffer_access([check, lhs] (dart::buffer const& rhs) { check(lhs, rhs); }, rhs);
        },
        [check, rhs] (dart::unsafe_buffer const& lhs) {
          buffer_access([check, lhs] (dart::unsafe_buffer const& rhs) { check(lhs, rhs); }, rhs);
        }
      ),
      lhs
    );
    if (err) return false;
    else return equal;
  }

  dart_type_t dart_buffer_get_type_impl(dart_buffer_t const* src) {
    dart_type_t type;
    auto get_type = [&] (auto& pkt) { type = abi_type(pkt.get_type()); };
    auto err = buffer_access(
      compose(
        [=] (dart::buffer const& pkt) { get_type(pkt); },
        [=] (dart::unsafe_buffer const& pkt) { get_type(pkt); }
      ),
      src
    );
    if (err) return DART_INVALID;
    else return type;
  }

  char* dart_buffer_to_json_impl(dart_buffer_t const* pkt, size_t* len) {
    // How long has it been since I've called a raw malloc like this...
    char* outstr;
    auto print = [&] (auto& pkt) {
      // Call these first so they throw before allocation.
      auto instr = pkt.to_json();
      auto inlen = instr.size();
      if (len) *len = inlen;
      outstr = reinterpret_cast<char*>(malloc(inlen + 1));
      memcpy(outstr, instr.data(), inlen + 1);
    };
    auto ret = buffer_access(
      compose(
        [=] (dart::buffer const& pkt) { print(pkt); },
        [=] (dart::unsafe_buffer const& pkt) { print(pkt); }
      ),
      pkt
    );
    if (ret) return nullptr;
    return outstr;
  }

  void const* dart_buffer_get_bytes_impl(dart_buffer_t const* src, size_t* len) {
    void const* ptr = nullptr;
    auto err = buffer_access(
      [&ptr, len] (auto& src) {
        auto bytes = src.get_bytes();
        ptr = bytes.data();
        if (len) *len = bytes.size();
      },
      src
    );
    if (err) return nullptr;
    else return ptr;
  }

  void* dart_buffer_dup_bytes_impl(dart_buffer_t const* src, size_t* len) {
    void* ptr = nullptr;
    auto err = buffer_access(
      [&ptr, len] (auto& src) {
        auto bytes = [&] {
          if (len) return src.dup_bytes(*len);
          else return src.dup_bytes();
        }();

        // FIXME: This is pretty not great, but it SHOULD be ok at the
        // moment because the underlying buffer isn't ACTUALLY const.
        // It's returned with a const pointer because it makes the type
        // conversion logic for then passing that buffer into a packet
        // constructor much simpler, but logically speaking the buffer
        // is, and must be, mutable as it's owned by the client, and
        // free doesn't take a const pointer.
        // If you check the implementation of dart::buffer::dup_bytes
        // you'll see that it also has to use a const_cast internally
        // to be able to destroy the buffer.
        ptr = const_cast<gsl::byte*>(bytes.release());
      },
      src
    );
    if (err) return nullptr;
    else return ptr;
  }

}

/*----- Function Implementations -----*/

extern "C" {

  dart_buffer_t dart_buffer_init() {
    // Cannot meaningfully fail.
    dart_buffer_t dst;
    dart_buffer_init_rc_err(&dst, DART_RC_SAFE);
    return dst;
  }

  dart_err_t dart_buffer_init_err(dart_buffer_t* dst) {
    return dart_buffer_init_rc_err(dst, DART_RC_SAFE);
  }

  dart_buffer_t dart_buffer_init_rc(dart_rc_type_t rc) {
    // Cannot meaningfully fail.
    dart_buffer_t dst;
    dart_buffer_init_rc_err(&dst, rc);
    return dst;
  }

  dart_err_t dart_buffer_init_rc_err(dart_buffer_t* dst, dart_rc_type_t rc) {
    // Initialize.
    dst->rtti = {DART_BUFFER, rc};
    return buffer_constructor_access(
      compose(
        [] (dart::buffer* ptr) { new(ptr) dart::buffer(); },
        [] (dart::unsafe_buffer* ptr) { new(ptr) dart::unsafe_buffer(); }
      ),
      dst
    );
  }

  dart_buffer_t dart_buffer_copy(dart_buffer_t const* src) {
    dart_buffer_t dst;
    auto err = dart_buffer_copy_err(&dst, src);
    if (err) return dart_buffer_init();
    else return dst;
  }

  dart_err_t dart_buffer_copy_err(dart_buffer_t* dst, dart_buffer_t const* src) {
    // Initialize.
    dst->rtti = src->rtti;
    return buffer_access(
      compose(
        [dst] (dart::buffer const& src) {
          return buffer_construct([&src] (dart::buffer* dst) { new(dst) dart::buffer(src); }, dst);
        },
        [dst] (dart::unsafe_buffer const& src) {
          return buffer_construct([&src] (dart::unsafe_buffer* dst) { new(dst) dart::unsafe_buffer(src); }, dst);
        }
      ),
      src
    );
  }

  dart_buffer_t dart_buffer_move(dart_buffer_t* src) {
    dart_buffer_t dst;
    auto err = dart_buffer_move_err(&dst, src);
    if (err) return dart_buffer_init();
    else return dst;
  }

  dart_err_t dart_buffer_move_err(dart_buffer_t* dst, dart_buffer_t* src) {
    // Initialize.
    dst->rtti = src->rtti;
    return buffer_access(
      compose(
        [dst] (dart::buffer& src) {
          return buffer_construct([&src] (dart::buffer* dst) { new(dst) dart::buffer(std::move(src)); }, dst);
        },
        [dst] (dart::unsafe_buffer& src) {
          return buffer_construct([&src] (dart::unsafe_buffer* dst) { new(dst) dart::unsafe_buffer(std::move(src)); }, dst);
        }
      ),
      src
    );
  }

  dart_err_t dart_buffer_destroy(dart_buffer_t* dst) {
    // Destroy.
    return buffer_access(
      compose(
        [] (dart::buffer& dst) { dst.~basic_buffer(); },
        [] (dart::unsafe_buffer& dst) { dst.~basic_buffer(); }
      ),
      dst
    );
  }

  int dart_buffer_obj_has_key(dart_buffer_t const* src, char const* key) {
    return dart_buffer_obj_has_key_len(src, key, strlen(key));
  }

  int dart_buffer_obj_has_key_len(dart_buffer_t const* src, char const* key, size_t len) {
    return dart_buffer_obj_has_key_len_impl(src, key, len);
  }

  dart_buffer_t dart_buffer_obj_get(dart_buffer_t const* src, char const* key) {
    dart_buffer_t dst;
    auto err = dart_buffer_obj_get_err(&dst, src, key);
    if (err) return dart_buffer_init();
    else return dst;
  }

  dart_err_t dart_buffer_obj_get_err(dart_buffer_t* dst, dart_buffer_t const* src, char const* key) {
    return dart_buffer_obj_get_len_err(dst, src, key, strlen(key));
  }

  dart_buffer_t dart_buffer_obj_get_len(dart_buffer_t const* src, char const* key, size_t len) {
    dart_buffer_t dst;
    auto err = dart_buffer_obj_get_len_err(&dst, src, key, len);
    if (err) return dart_buffer_init();
    else return dst;
  }

  dart_err_t dart_buffer_obj_get_len_err(dart_buffer_t* dst, dart_buffer_t const* src, char const* key, size_t len) {
    // Initialize.
    dst->rtti = src->rtti;
    return buffer_access(
      compose(
        [=] (dart::buffer const& src) {
          return buffer_construct([&] (dart::buffer* dst) {
            new(dst) dart::buffer(src[{key, len}]);
          }, dst);
        },
        [=] (dart::unsafe_buffer const& src) {
          return buffer_construct([&] (dart::unsafe_buffer* dst) {
            new(dst) dart::unsafe_buffer(src[{key, len}]);
          }, dst);
        }
      ),
      src
    );
  }

  dart_buffer_t dart_buffer_arr_get(dart_buffer_t const* src, size_t idx) {
    dart_buffer_t dst;
    auto err = dart_buffer_arr_get_err(&dst, src, idx);
    if (err) return dart_buffer_init();
    else return dst;
  }

  dart_err_t dart_buffer_arr_get_err(dart_buffer_t* dst, dart_buffer_t const* src, size_t idx) {
    // Initialize.
    dst->rtti = src->rtti;
    return buffer_access(
      compose(
        [=] (dart::buffer const& src) {
          return buffer_construct([&] (dart::buffer* dst) {
            new(dst) dart::buffer(src[idx]);
          }, dst);
        },
        [=] (dart::unsafe_buffer const& src) {
          return buffer_construct([&] (dart::unsafe_buffer* dst) {
            new(dst) dart::unsafe_buffer(src[idx]);
          }, dst);
        }
      ),
      src
    );
  }

  char const* dart_buffer_str_get(dart_buffer_t const* src) {
    size_t dummy;
    return dart_buffer_str_get_len(src, &dummy);
  }

  char const* dart_buffer_str_get_len(dart_buffer_t const* src, size_t* len) {
    return dart_buffer_str_get_len_impl(src, len);
  }

  int64_t dart_buffer_int_get(dart_buffer_t const* src) {
    // No way unique way to signal failure here,
    // the user needs to know this will succeed.
    int64_t val = 0;
    dart_buffer_int_get_err(src, &val);
    return val;
  }

  dart_err_t dart_buffer_int_get_err(dart_buffer_t const* src, int64_t* val) {
    return dart_buffer_int_get_err_impl(src, val);
  }

  double dart_buffer_dcm_get(dart_buffer_t const* src) {
    double val = std::numeric_limits<double>::quiet_NaN();
    dart_buffer_dcm_get_err(src, &val);
    return val;
  }

  dart_err_t dart_buffer_dcm_get_err(dart_buffer_t const* src, double* val) {
    return dart_buffer_dcm_get_err_impl(src, val);
  }

  int dart_buffer_bool_get(dart_buffer_t const* src) {
    // No way unique way to signal failure here,
    // the user needs to know this will succeed.
    int val = 0;
    dart_buffer_bool_get_err(src, &val);
    return val;
  }

  dart_err_t dart_buffer_bool_get_err(dart_buffer_t const* src, int* val) {
    return dart_buffer_bool_get_err_impl(src, val);
  }

  size_t dart_buffer_size(dart_buffer_t const* src) {
    return dart_buffer_size_impl(src);
  }

  int dart_buffer_equal(dart_buffer_t const* lhs, dart_buffer_t const* rhs) {
    return dart_buffer_equal_impl(lhs, rhs);
  }

  int dart_buffer_is_obj(dart_buffer_t const* src) {
    return dart_buffer_get_type(src) == DART_OBJECT;
  }

  int dart_buffer_is_arr(dart_buffer_t const* src) {
    return dart_buffer_get_type(src) == DART_ARRAY;
  }

  int dart_buffer_is_str(dart_buffer_t const* src) {
    return dart_buffer_get_type(src) == DART_STRING;
  }

  int dart_buffer_is_int(dart_buffer_t const* src) {
    return dart_buffer_get_type(src) == DART_INTEGER;
  }

  int dart_buffer_is_dcm(dart_buffer_t const* src) {
    return dart_buffer_get_type(src) == DART_DECIMAL;
  }

  int dart_buffer_is_bool(dart_buffer_t const* src) {
    return dart_buffer_get_type(src) == DART_BOOLEAN;
  }

  int dart_buffer_is_null(dart_buffer_t const* src) {
    return dart_buffer_get_type(src) == DART_NULL;
  }

  dart_type_t dart_buffer_get_type(dart_buffer_t const* src) {
    return dart_buffer_get_type_impl(src);
  }

  dart_buffer_t dart_buffer_from_json(char const* str) {
    dart_buffer_t dst;
    auto err = dart_buffer_from_json_err(&dst, str);
    if (err) return dart_buffer_init();
    else return dst;
  }

  dart_err_t dart_buffer_from_json_err(dart_buffer_t* dst, char const* str) {
    return dart_buffer_from_json_len_rc_err(dst, DART_RC_SAFE, str, strlen(str));
  }

  dart_buffer_t dart_buffer_from_json_rc(dart_rc_type_t rc, char const* str) {
    dart_buffer_t dst;
    auto err = dart_buffer_from_json_rc_err(&dst, rc, str);
    if (err) return dart_buffer_init();
    else return dst;
  }

  dart_err_t dart_buffer_from_json_rc_err(dart_buffer_t* dst, dart_rc_type_t rc, char const* str) {
    return dart_buffer_from_json_len_rc_err(dst, rc, str, strlen(str));
  }

  dart_buffer_t dart_buffer_from_json_len(char const* str, size_t len) {
    dart_buffer_t dst;
    auto err = dart_buffer_from_json_len_err(&dst, str, len);
    if (err) return dart_buffer_init();
    else return dst;
  }

  dart_err_t dart_buffer_from_json_len_err(dart_buffer_t* dst, char const* str, size_t len) {
    return dart_buffer_from_json_len_rc_err(dst, DART_RC_SAFE, str, len);
  }

  dart_buffer_t dart_buffer_from_json_len_rc(dart_rc_type_t rc, char const* str, size_t len) {
    dart_buffer_t dst;
    auto err = dart_buffer_from_json_len_rc_err(&dst, rc, str, len);
    if (err) return dart_buffer_init();
    else return dst;
  }

  dart_err_t dart_buffer_from_json_len_rc_err(dart_buffer_t* dst, dart_rc_type_t rc, char const* str, size_t len) {
    auto err = dart_buffer_init_rc_err(dst, rc);
    if (err) return err;

    // Assign to it.
    return err_handler([=] {
      return buffer_unwrap(
        compose(
          [=] (dart::buffer& dst) { dst = dart::buffer::from_json({str, len}); },
          [=] (dart::unsafe_buffer& dst) { dst = dart::unsafe_buffer::from_json({str, len}); }
        ),
        dst
      );
    });
  }

  char* dart_buffer_to_json(dart_buffer_t const* pkt, size_t* len) {
    return dart_buffer_to_json_impl(pkt, len);
  }

  dart_heap_t dart_buffer_lift(dart_buffer_t const* src) {
    dart_heap_t dst;
    auto err = dart_buffer_lift_err(&dst, src);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_buffer_lift_err(dart_heap_t* dst, dart_buffer_t const* src) {
    // Initialize.
    dst->rtti = {DART_HEAP, src->rtti.rc_id};
    return buffer_access(
      compose(
        [dst] (dart::buffer const& src) {
          heap_construct([&src] (dart::heap* dst) { new(dst) dart::heap(src.lift()); }, dst);
        },
        [dst] (dart::unsafe_buffer const& src) {
          heap_construct([&src] (dart::unsafe_heap* dst) { new(dst) dart::unsafe_heap(src.lift()); }, dst);
        }
      ),
      src
    );
  }

  dart_heap_t dart_buffer_definalize(dart_buffer_t const* src) {
    dart_heap_t dst;
    auto err = dart_buffer_definalize_err(&dst, src);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_buffer_definalize_err(dart_heap_t* dst, dart_buffer_t const* src) {
    return dart_buffer_lift_err(dst, src);
  }

  void const* dart_buffer_get_bytes(dart_buffer_t const* src, size_t* len) {
    return dart_buffer_get_bytes_impl(src, len);
  }

  void* dart_buffer_dup_bytes(dart_buffer_t const* src, size_t* len) {
    return dart_buffer_dup_bytes_impl(src, len);
  }

  dart_buffer_t dart_buffer_from_bytes(void const* bytes, size_t len) {
    dart_buffer_t dst;
    auto err = dart_buffer_from_bytes_err(&dst, bytes, len);
    if (err) return dart_buffer_init();
    else return dst;
  }

  dart_err_t dart_buffer_from_bytes_err(dart_buffer_t* dst, void const* bytes, size_t len) {
    return dart_buffer_from_bytes_rc_err(dst, DART_RC_SAFE, bytes, len);
  }

  dart_buffer_t dart_buffer_from_bytes_rc(void const* bytes, dart_rc_type_t rc, size_t len) {
    dart_buffer_t dst;
    auto err = dart_buffer_from_bytes_rc_err(&dst, rc, bytes, len);
    if (err) return dart_buffer_init();
    else return dst;
  }

  dart_err_t dart_buffer_from_bytes_rc_err(dart_buffer_t* dst, dart_rc_type_t rc, void const* bytes, size_t len) {
    auto err = dart_buffer_init_rc_err(dst, rc);
    if (err) return err;

    // Assign to it.
    auto* punned = reinterpret_cast<gsl::byte const*>(bytes);
    return err_handler([=] {
      return buffer_unwrap(
        compose(
          [punned, len] (dart::buffer& dst) { dst = dart::buffer {gsl::make_span(punned, len)}; },
          [punned, len] (dart::unsafe_buffer& dst) { dst = dart::unsafe_buffer {gsl::make_span(punned, len)}; }
        ),
        dst
      );
    });
  }

  dart_buffer_t dart_buffer_take_bytes(void* bytes) {
    dart_buffer_t dst;
    auto err = dart_buffer_take_bytes_err(&dst, bytes);
    if (err) return dart_buffer_init();
    else return dst;
  }

  dart_err_t dart_buffer_take_bytes_err(dart_buffer_t* dst, void* bytes) {
    return dart_buffer_take_bytes_rc_err(dst, DART_RC_SAFE, bytes);
  }

  dart_buffer_t dart_buffer_take_bytes_rc(void* bytes, dart_rc_type_t rc) {
    dart_buffer_t dst;
    auto err = dart_buffer_take_bytes_rc_err(&dst, rc, bytes);
    if (err) return dart_buffer_init();
    else return dst;
  }

  dart_err_t dart_buffer_take_bytes_rc_err(dart_buffer_t* dst, dart_rc_type_t rc, void* bytes) {
    using owner_type = std::unique_ptr<gsl::byte const[], void (*) (gsl::byte const*)>;

    auto err = dart_buffer_init_rc_err(dst, rc);
    if (err) return err;

    // Assign to it.
    auto* del = +[] (gsl::byte const* ptr) { free(const_cast<gsl::byte*>(ptr)); };
    owner_type owner {reinterpret_cast<gsl::byte const*>(bytes), del};
    return err_handler([&] {
      return buffer_unwrap(
        compose(
          [&] (dart::buffer& dst) { dst = dart::buffer {std::move(owner)}; },
          [&] (dart::unsafe_buffer& dst) { dst = dart::unsafe_buffer {std::move(owner)}; }
        ),
        dst
      );
    });
  }
  
}
