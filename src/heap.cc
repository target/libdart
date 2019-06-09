
/*----- Local Includes -----*/

#include "helpers.h"

/*----- Function Implementations -----*/

extern "C" {

  dart_heap_t dart_heap_init() {
    // Cannot meaningfully fail.
    dart_heap_t dst;
    dart_heap_init_rc_err(&dst, DART_RC_SAFE);
    return dst;
  }

  dart_err_t dart_heap_init_err(dart_heap_t* dst) {
    return dart_heap_init_rc_err(dst, DART_RC_SAFE);
  }

  dart_heap_t dart_heap_init_rc(dart_rc_type_t rc) {
    // Cannot meaningfully fail.
    dart_heap_t dst;
    dart_heap_init_rc_err(&dst, rc);
    return dst;
  }

  dart_err_t dart_heap_init_rc_err(dart_heap_t* dst, dart_rc_type_t rc) {
    // Initialize.
    dst->rtti = {DART_HEAP, rc};
    return heap_constructor_access(
      compose(
        [] (dart::heap* ptr) { new(ptr) dart::heap(); },
        [] (dart::unsafe_heap* ptr) { new(ptr) dart::unsafe_heap(); }
      ),
      dst
    );
  }

  dart_heap_t dart_heap_copy(dart_heap_t const* src) {
    dart_heap_t dst;
    auto err = dart_heap_copy_err(&dst, src);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_copy_err(dart_heap_t* dst, dart_heap_t const* src) {
    // Initialize.
    dst->rtti = src->rtti;
    return heap_access(
      compose(
        [dst] (dart::heap const& src) {
          return heap_construct([&src] (dart::heap* dst) { new(dst) dart::heap(src); }, dst);
        },
        [dst] (dart::unsafe_heap const& src) {
          return heap_construct([&src] (dart::unsafe_heap* dst) { new(dst) dart::unsafe_heap(src); }, dst);
        }
      ),
      src
    );
  }

  dart_heap_t dart_heap_move(dart_heap_t* src) {
    dart_heap_t dst;
    auto err = dart_heap_move_err(&dst, src);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_move_err(dart_heap_t* dst, dart_heap_t* src) {
    // Initialize.
    dst->rtti = src->rtti;
    return heap_access(
      compose(
        [dst] (dart::heap& src) {
          return heap_construct([&src] (dart::heap* dst) { new(dst) dart::heap(std::move(src)); }, dst);
        },
        [dst] (dart::unsafe_heap& src) {
          return heap_construct([&src] (dart::unsafe_heap* dst) { new(dst) dart::unsafe_heap(std::move(src)); }, dst);
        }
      ),
      src
    );
  }

  dart_err_t dart_heap_destroy(dart_heap_t* dst) {
    // Destroy.
    return heap_access(
      compose(
        [] (dart::heap& dst) { dst.~basic_heap(); },
        [] (dart::unsafe_heap& dst) { dst.~basic_heap(); }
      ),
      dst
    );
  }

  dart_heap_t dart_heap_obj_init() {
    dart_heap_t dst;
    auto err = dart_heap_obj_init_err(&dst);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_obj_init_err(dart_heap_t* dst) {
    return dart_heap_obj_init_rc_err(dst, DART_RC_SAFE);
  }

  dart_heap_t dart_heap_obj_init_rc(dart_rc_type_t rc) {
    dart_heap_t dst;
    auto err = dart_heap_obj_init_rc_err(&dst, rc);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_obj_init_rc_err(dart_heap_t* dst, dart_rc_type_t rc) {
    // Default initialize, then assign.
    return heap_typed_constructor_access(
      compose(
        [] (dart::heap& dst) { dst = dart::heap::make_object(); },
        [] (dart::unsafe_heap& dst) { dst = dart::unsafe_heap::make_object(); }
      ),
      dst,
      rc
    );
  }

  static dart_err_t dart_heap_obj_init_va_impl(dart_heap_t* dst, dart_rc_type_t rc, char const* format, va_list args) {
    return heap_typed_constructor_access(
      compose(
        [format, args] (dart::heap& dst) mutable {
          dst = dart::heap::make_object();
          parse_pairs(dst, format, args);
        },
        [format, args] (dart::unsafe_heap& dst) mutable {
          dst = dart::unsafe_heap::make_object();
          parse_pairs(dst, format, args);
        }
      ),
      dst,
      rc
    );
  }

  dart_heap_t dart_heap_obj_init_va(char const* format, ...) {
    va_list args;
    dart_heap_t dst;
    va_start(args, format);
    auto ret = dart_heap_obj_init_va_impl(&dst, DART_RC_SAFE, format, args);
    va_end(args);
    if (ret) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_obj_init_va_err(dart_heap_t* dst, char const* format, ...) {
    va_list args;
    va_start(args, format);
    auto ret = dart_heap_obj_init_va_impl(dst, DART_RC_SAFE, format, args);
    va_end(args);
    return ret;
  }

  dart_heap_t dart_heap_obj_init_va_rc(dart_rc_type_t rc, char const* format, ...) {
    va_list args;
    dart_heap_t dst;
    va_start(args, format);
    auto ret = dart_heap_obj_init_va_impl(&dst, rc, format, args);
    va_end(args);
    if (ret) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_obj_init_va_rc_err(dart_heap_t* dst, dart_rc_type_t rc, char const* format, ...) {
    va_list args;
    va_start(args, format);
    auto ret = dart_heap_obj_init_va_impl(dst, rc, format, args);
    va_end(args);
    return ret;
  }

  dart_heap_t dart_heap_arr_init() {
    dart_heap_t dst;
    auto err = dart_heap_arr_init_err(&dst);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_arr_init_err(dart_heap_t* dst) {
    return dart_heap_arr_init_rc_err(dst, DART_RC_SAFE);
  }

  dart_heap_t dart_heap_arr_init_rc(dart_rc_type_t rc) {
    dart_heap_t dst;
    auto err = dart_heap_arr_init_rc_err(&dst, rc);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_arr_init_rc_err(dart_heap_t* dst, dart_rc_type_t rc) {
    // Default initialize, then assign.
    return heap_typed_constructor_access(
      compose(
        [] (dart::heap& dst) { dst = dart::heap::make_array(); },
        [] (dart::unsafe_heap& dst) { dst = dart::unsafe_heap::make_array(); }
      ),
      dst,
      rc
    );
  }

  static dart_err_t dart_heap_arr_init_va_impl(dart_heap_t* dst, dart_rc_type_t rc, char const* format, va_list args) {
    return heap_typed_constructor_access(
      compose(
        [format, args] (dart::heap& dst) mutable {
          dst = dart::heap::make_array();
          parse_vals(dst, format, args);
        },
        [format, args] (dart::unsafe_heap& dst) mutable {
          dst = dart::unsafe_heap::make_array();
          parse_vals(dst, format, args);
        }
      ),
      dst,
      rc
    );
  }

  dart_heap_t dart_heap_arr_init_va(char const* format, ...) {
    va_list args;
    dart_heap_t dst;
    va_start(args, format);
    auto ret = dart_heap_arr_init_va_impl(&dst, DART_RC_SAFE, format, args);
    if (ret) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_arr_init_va_err(dart_heap_t* dst, char const* format, ...) {
    va_list args;
    va_start(args, format);
    auto ret = dart_heap_arr_init_va_impl(dst, DART_RC_SAFE, format, args);
    va_end(args);
    return ret;
  }

  dart_heap_t dart_heap_arr_init_va_rc(dart_rc_type_t rc, char const* format, ...) {
    va_list args;
    dart_heap_t dst;
    va_start(args, format);
    auto ret = dart_heap_arr_init_va_impl(&dst, rc, format, args);
    if (ret) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_arr_init_va_rc_err(dart_heap_t* dst, dart_rc_type_t rc, char const* format, ...) {
    va_list args;
    va_start(args, format);
    auto ret = dart_heap_arr_init_va_impl(dst, rc, format, args);
    va_end(args);
    return ret;
  }

  dart_heap_t dart_heap_str_init(char const* str) {
    dart_heap_t dst;
    auto err = dart_heap_str_init_err(&dst, str);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_str_init_err(dart_heap_t* dst, char const* str) {
    return dart_heap_str_init_rc_len_err(dst, DART_RC_SAFE, str, strlen(str));
  }

  dart_heap_t dart_heap_str_init_len(char const* str, size_t len) {
    dart_heap_t dst;
    auto err = dart_heap_str_init_len_err(&dst, str, len);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_str_init_len_err(dart_heap_t* dst, char const* str, size_t len) {
    return dart_heap_str_init_rc_len_err(dst, DART_RC_SAFE, str, len);
  }

  dart_heap_t dart_heap_str_init_rc(dart_rc_type_t rc, char const* str) {
    dart_heap_t dst;
    auto err = dart_heap_str_init_rc_err(&dst, rc, str);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_str_init_rc_err(dart_heap_t* dst, dart_rc_type_t rc, char const* str) {
    return dart_heap_str_init_rc_len_err(dst, rc, str, strlen(str));
  }

  dart_heap_t dart_heap_str_init_rc_len(dart_rc_type_t rc, char const* str, size_t len) {
    dart_heap_t dst;
    auto err = dart_heap_str_init_rc_len_err(&dst, rc, str, len);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_str_init_rc_len_err(dart_heap_t* dst, dart_rc_type_t rc, char const* str, size_t len) {
    // Default initialize, then assign
    return heap_typed_constructor_access(
      compose(
        [str, len] (dart::heap& dst) { dst = dart::heap::make_string({str, len}); },
        [str, len] (dart::unsafe_heap& dst) { dst = dart::unsafe_heap::make_string({str, len}); }
      ),
      dst,
      rc
    );
  }

  dart_heap_t dart_heap_int_init(int64_t val) {
    dart_heap_t dst;
    auto err = dart_heap_int_init_err(&dst, val);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_int_init_err(dart_heap_t* dst, int64_t val) {
    return dart_heap_int_init_rc_err(dst, DART_RC_SAFE, val);
  }

  dart_heap_t dart_heap_int_init_rc(dart_rc_type_t rc, int64_t val) {
    dart_heap_t dst;
    auto err = dart_heap_int_init_rc_err(&dst, rc, val);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_int_init_rc_err(dart_heap_t* dst, dart_rc_type_t rc, int64_t val) {
    // Default initialize, then assign
    return heap_typed_constructor_access(
      compose(
        [val] (dart::heap& dst) { dst = dart::heap::make_integer(val); },
        [val] (dart::unsafe_heap& dst) { dst = dart::unsafe_heap::make_integer(val); }
      ),
      dst,
      rc
    );
  }

  dart_heap_t dart_heap_dcm_init(double val) {
    dart_heap_t dst;
    auto err = dart_heap_dcm_init_err(&dst, val);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_dcm_init_err(dart_heap_t* dst, double val) {
    return dart_heap_dcm_init_rc_err(dst, DART_RC_SAFE, val);
  }

  dart_heap_t dart_heap_dcm_init_rc(dart_rc_type_t rc, double val) {
    dart_heap_t dst;
    auto err = dart_heap_dcm_init_rc_err(&dst, rc, val);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_dcm_init_rc_err(dart_heap_t* dst, dart_rc_type_t rc, double val) {
    // Default initialize, then assign
    return heap_typed_constructor_access(
      compose(
        [val] (dart::heap& dst) { dst = dart::heap::make_decimal(val); },
        [val] (dart::unsafe_heap& dst) { dst = dart::unsafe_heap::make_decimal(val); }
      ),
      dst,
      rc
    );
  }

  dart_heap_t dart_heap_bool_init(int val) {
    dart_heap_t dst;
    auto err = dart_heap_bool_init_err(&dst, val);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_bool_init_err(dart_heap_t* dst, int val) {
    return dart_heap_bool_init_rc_err(dst, DART_RC_SAFE, val);
  }

  dart_heap_t dart_heap_bool_init_rc(dart_rc_type_t rc, int val) {
    dart_heap_t dst;
    auto err = dart_heap_bool_init_rc_err(&dst, rc, val);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_bool_init_rc_err(dart_heap_t* dst, dart_rc_type_t rc, int val) {
    // Default initialize, then assign
    return heap_typed_constructor_access(
      compose(
        [val] (dart::heap& dst) { dst = dart::heap::make_boolean(val); },
        [val] (dart::unsafe_heap& dst) { dst = dart::unsafe_heap::make_boolean(val); }
      ),
      dst,
      rc
    );
  }

  dart_heap_t dart_heap_null_init() {
    dart_heap_t dst;
    auto err = dart_heap_null_init_err(&dst);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_null_init_err(dart_heap_t* dst) {
    return dart_heap_null_init_rc_err(dst, DART_RC_SAFE);
  }

  dart_heap_t dart_heap_null_init_rc(dart_rc_type_t rc) {
    dart_heap_t dst;
    auto err = dart_heap_null_init_rc_err(&dst, rc);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_null_init_rc_err(dart_heap_t* dst, dart_rc_type_t rc) {
    // Default initialize, then assign.
    // Unnecessary, but done for consistency of code formatting.
    return heap_typed_constructor_access(
      compose(
        [] (dart::heap& dst) { dst = dart::heap::make_null(); },
        [] (dart::unsafe_heap& dst) { dst = dart::unsafe_heap::make_null(); }
      ),
      dst,
      rc
    );
  }

  dart_err_t dart_heap_obj_insert_heap(dart_heap_t* dst, char const* key, dart_heap_t const* val) {
    return dart_heap_obj_insert_heap_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_heap_obj_insert_heap_len(dart_heap_t* dst, char const* key, size_t len, dart_heap_t const* val) {
    auto insert = [=] (auto& dst, auto& val) { dst.insert(string_view {key, len}, val); };
    return heap_access(
      compose(
        [=] (dart::heap& dst) {
          return heap_access([=, &dst] (dart::heap const& val) { insert(dst, val); }, val);
        },
        [=] (dart::unsafe_heap& dst) {
          return heap_access([=, &dst] (dart::unsafe_heap const& val) { insert(dst, val); }, val);
        }
      ),
      dst
    );
  }

  dart_err_t dart_heap_obj_insert_take_heap(dart_heap_t* dst, char const* key, dart_heap_t* val) {
    return dart_heap_obj_insert_take_heap_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_heap_obj_insert_take_heap_len(dart_heap_t* dst, char const* key, size_t len, dart_heap_t* val) {
    auto insert = [=] (auto& dst, auto& val) { dst.insert(string_view {key, len}, std::move(val)); };
    return heap_access(
      compose(
        [=] (dart::heap& dst) {
          return heap_access([=, &dst] (dart::heap& val) { insert(dst, val); }, val);
        },
        [=] (dart::unsafe_heap& dst) {
          return heap_access([=, &dst] (dart::unsafe_heap& val) { insert(dst, val); }, val);
        }
      ),
      dst
    );
  }

  dart_err_t dart_heap_obj_insert_str(dart_heap_t* dst, char const* key, char const* val) {
    return dart_heap_obj_insert_str_len(dst, key, strlen(key), val, strlen(val));
  }

  dart_err_t dart_heap_obj_insert_str_len(dart_heap_t* dst, char const* key, size_t len, char const* val, size_t val_len) {
    auto insert = [=] (auto& dst) { dst.insert(string_view {key, len}, string_view {val, val_len}); };
    return heap_access(
      compose(
        [insert] (dart::heap& dst) { insert(dst); },
        [insert] (dart::unsafe_heap& dst) { insert(dst); }
      ),
      dst
    );
  }

  dart_err_t dart_heap_obj_insert_int(dart_heap_t* dst, char const* key, int64_t val) {
    return dart_heap_obj_insert_int_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_heap_obj_insert_int_len(dart_heap_t* dst, char const* key, size_t len, int64_t val) {
    auto insert = [=] (auto& dst) { dst.insert(string_view {key, len}, val); };
    return heap_access(
      compose(
        [insert] (dart::heap& dst) { insert(dst); },
        [insert] (dart::unsafe_heap& dst) { insert(dst); }
      ),
      dst
    );
  }

  dart_err_t dart_heap_obj_insert_dcm(dart_heap_t* dst, char const* key, double val) {
    return dart_heap_obj_insert_dcm_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_heap_obj_insert_dcm_len(dart_heap_t* dst, char const* key, size_t len, double val) {
    auto insert = [=] (auto& dst) { dst.insert(string_view {key, len}, val); };
    return heap_access(
      compose(
        [insert] (dart::heap& dst) { insert(dst); },
        [insert] (dart::unsafe_heap& dst) { insert(dst); }
      ),
      dst
    );
  }

  dart_err_t dart_heap_obj_insert_bool(dart_heap_t* dst, char const* key, int val) {
    return dart_heap_obj_insert_bool_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_heap_obj_insert_bool_len(dart_heap_t* dst, char const* key, size_t len, int val) {
    auto insert = [=] (auto& dst) { dst.insert(string_view {key, len}, static_cast<bool>(val)); };
    return heap_access(
      compose(
        [insert] (dart::heap& dst) { insert(dst); },
        [insert] (dart::unsafe_heap& dst) { insert(dst); }
      ),
      dst
    );
  }

  dart_err_t dart_heap_obj_insert_null(dart_heap_t* dst, char const* key) {
    return dart_heap_obj_insert_null_len(dst, key, strlen(key));
  }

  dart_err_t dart_heap_obj_insert_null_len(dart_heap_t* dst, char const* key, size_t len) {
    auto insert = [=] (auto& dst) { dst.insert(string_view {key, len}, nullptr); };
    return heap_access(
      compose(
        [insert] (dart::heap& dst) { insert(dst); },
        [insert] (dart::unsafe_heap& dst) { insert(dst); }
      ),
      dst
    );
  }

  dart_err_t dart_heap_obj_set_heap(dart_heap_t* dst, char const* key, dart_heap_t const* val) {
    return dart_heap_obj_set_heap_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_heap_obj_set_heap_len(dart_heap_t* dst, char const* key, size_t len, dart_heap_t const* val) {
    auto set = [=] (auto& dst, auto& val) { dst.set(string_view {key, len}, val); };
    return heap_access(
      compose(
        [=] (dart::heap& dst) {
          return heap_access([=, &dst] (dart::heap const& val) { set(dst, val); }, val);
        },
        [=] (dart::unsafe_heap& dst) {
          return heap_access([=, &dst] (dart::unsafe_heap const& val) { set(dst, val); }, val);
        }
      ),
      dst
    );
  }

  dart_err_t dart_heap_obj_set_take_heap(dart_heap_t* dst, char const* key, dart_heap_t* val) {
    return dart_heap_obj_set_take_heap_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_heap_obj_set_take_heap_len(dart_heap_t* dst, char const* key, size_t len, dart_heap_t* val) {
    auto set = [=] (auto& dst, auto& val) { dst.set(string_view {key, len}, std::move(val)); };
    return heap_access(
      compose(
        [=] (dart::heap& dst) {
          return heap_access([=, &dst] (dart::heap& val) { set(dst, val); }, val);
        },
        [=] (dart::unsafe_heap& dst) {
          return heap_access([=, &dst] (dart::unsafe_heap& val) { set(dst, val); }, val);
        }
      ),
      dst
    );
  }

  dart_err_t dart_heap_obj_set_str(dart_heap_t* dst, char const* key, char const* val) {
    return dart_heap_obj_set_str_len(dst, key, strlen(key), val, strlen(val));
  }

  dart_err_t dart_heap_obj_set_str_len(dart_heap_t* dst, char const* key, size_t len, char const* val, size_t val_len) {
    auto set = [=] (auto& dst) { dst.set(string_view {key, len}, string_view {val, val_len}); };
    return heap_access(
      compose(
        [set] (dart::heap& dst) { set(dst); },
        [set] (dart::unsafe_heap& dst) { set(dst); }
      ),
      dst
    );
  }

  dart_err_t dart_heap_obj_set_int(dart_heap_t* dst, char const* key, int64_t val) {
    return dart_heap_obj_set_int_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_heap_obj_set_int_len(dart_heap_t* dst, char const* key, size_t len, int64_t val) {
    auto set = [=] (auto& dst) { dst.set(string_view {key, len}, val); };
    return heap_access(
      compose(
        [set] (dart::heap& dst) { set(dst); },
        [set] (dart::unsafe_heap& dst) { set(dst); }
      ),
      dst
    );
  }

  dart_err_t dart_heap_obj_set_dcm(dart_heap_t* dst, char const* key, double val) {
    return dart_heap_obj_set_dcm_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_heap_obj_set_dcm_len(dart_heap_t* dst, char const* key, size_t len, double val) {
    auto set = [=] (auto& dst) { dst.set(string_view {key, len}, val); };
    return heap_access(
      compose(
        [set] (dart::heap& dst) { set(dst); },
        [set] (dart::unsafe_heap& dst) { set(dst); }
      ),
      dst
    );
  }

  dart_err_t dart_heap_obj_set_bool(dart_heap_t* dst, char const* key, int val) {
    return dart_heap_obj_set_bool_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_heap_obj_set_bool_len(dart_heap_t* dst, char const* key, size_t len, int val) {
    auto set = [=] (auto& dst) { dst.set(string_view {key, len}, static_cast<bool>(val)); };
    return heap_access(
      compose(
        [set] (dart::heap& dst) { set(dst); },
        [set] (dart::unsafe_heap& dst) { set(dst); }
      ),
      dst
    );
  }

  dart_err_t dart_heap_obj_set_null(dart_heap_t* dst, char const* key) {
    return dart_heap_obj_set_null_len(dst, key, strlen(key));
  }

  dart_err_t dart_heap_obj_set_null_len(dart_heap_t* dst, char const* key, size_t len) {
    auto set = [=] (auto& dst) { dst.set(string_view {key, len}, nullptr); };
    return heap_access(
      compose(
        [set] (dart::heap& dst) { set(dst); },
        [set] (dart::unsafe_heap& dst) { set(dst); }
      ),
      dst
    );
  }

  dart_err_t dart_heap_obj_erase(dart_heap_t* dst, char const* key) {
    return dart_heap_obj_erase_len(dst, key, strlen(key));
  }

  dart_err_t dart_heap_obj_erase_len(dart_heap_t* dst, char const* key, size_t len) {
    auto erase = [=] (auto& dst) { dst.erase(string_view {key, len}); };
    return heap_access(
      compose(
        [erase] (dart::heap& dst) { erase(dst); },
        [erase] (dart::unsafe_heap& dst) { erase(dst); }
      ),
      dst
    );
  }

  dart_err_t dart_heap_arr_insert_heap(dart_heap_t* dst, size_t idx, dart_heap_t const* val) {
    auto insert = [=] (auto& dst, auto& val) { dst.insert(idx, val); };
    return heap_access(
      compose(
        [=] (dart::heap& dst) {
          return heap_access([=, &dst] (dart::heap const& val) { insert(dst, val); }, val);
        },
        [=] (dart::unsafe_heap& dst) {
          return heap_access([=, &dst] (dart::unsafe_heap const& val) { insert(dst, val); }, val);
        }
      ),
      dst
    );
  }

  dart_err_t dart_heap_arr_insert_take_heap(dart_heap_t* dst, size_t idx, dart_heap_t* val) {
    auto insert = [=] (auto& dst, auto& val) { dst.insert(idx, std::move(val)); };
    return heap_access(
      compose(
        [=] (dart::heap& dst) {
          return heap_access([=, &dst] (dart::heap& val) { insert(dst, val); }, val);
        },
        [=] (dart::unsafe_heap& dst) {
          return heap_access([=, &dst] (dart::unsafe_heap& val) { insert(dst, val); }, val);
        }
      ),
      dst
    );
  }

  dart_err_t dart_heap_arr_insert_str(dart_heap_t* dst, size_t idx, char const* val) {
    return dart_heap_arr_insert_str_len(dst, idx, val, strlen(val));
  }

  dart_err_t dart_heap_arr_insert_str_len(dart_heap_t* dst, size_t idx, char const* val, size_t val_len) {
    auto insert = [=] (auto& dst) { dst.insert(idx, string_view {val, val_len}); };
    return heap_access(
      compose(
        [insert] (dart::heap& dst) { insert(dst); },
        [insert] (dart::unsafe_heap& dst) { insert(dst); }
      ),
      dst
    );
  }

  dart_err_t dart_heap_arr_insert_int(dart_heap_t* dst, size_t idx, int64_t val) {
    auto insert = [=] (auto& dst) { dst.insert(idx, val); };
    return heap_access(
      compose(
        [insert] (dart::heap& dst) { insert(dst); },
        [insert] (dart::unsafe_heap& dst) { insert(dst); }
      ),
      dst
    );
  }

  dart_err_t dart_heap_arr_insert_dcm(dart_heap_t* dst, size_t idx, double val) {
    auto insert = [=] (auto& dst) { dst.insert(idx, val); };
    return heap_access(
      compose(
        [insert] (dart::heap& dst) { insert(dst); },
        [insert] (dart::unsafe_heap& dst) { insert(dst); }
      ),
      dst
    );
  }

  dart_err_t dart_heap_arr_insert_bool(dart_heap_t* dst, size_t idx, int val) {
    auto insert = [=] (auto& dst) { dst.insert(idx, static_cast<bool>(val)); };
    return heap_access(
      compose(
        [insert] (dart::heap& dst) { insert(dst); },
        [insert] (dart::unsafe_heap& dst) { insert(dst); }
      ),
      dst
    );
  }

  dart_err_t dart_heap_arr_insert_null(dart_heap_t* dst, size_t idx) {
    auto insert = [=] (auto& dst) { dst.insert(idx, nullptr); };
    return heap_access(
      compose(
        [=] (dart::heap& dst) { insert(dst); },
        [=] (dart::unsafe_heap& dst) { insert(dst); }
      ),
      dst
    );
  }

  dart_err_t dart_heap_arr_set_heap(dart_heap_t* dst, size_t idx, dart_heap_t const* val) {
    auto set = [=] (auto& dst, auto& val) { dst.set(idx, val); };
    return heap_access(
      compose(
        [=] (dart::heap& dst) {
          return heap_access([=, &dst] (dart::heap const& val) { set(dst, val); }, val);
        },
        [=] (dart::unsafe_heap& dst) {
          return heap_access([=, &dst] (dart::unsafe_heap const& val) { set(dst, val); }, val);
        }
      ),
      dst
    );
  }

  dart_err_t dart_heap_arr_set_take_heap(dart_heap_t* dst, size_t idx, dart_heap_t* val) {
    auto set = [=] (auto& dst, auto& val) { dst.set(idx, std::move(val)); };
    return heap_access(
      compose(
        [=] (dart::heap& dst) {
          return heap_access([=, &dst] (dart::heap& val) { set(dst, val); }, val);
        },
        [=] (dart::unsafe_heap& dst) {
          return heap_access([=, &dst] (dart::unsafe_heap& val) { set(dst, val); }, val);
        }
      ),
      dst
    );
  }

  dart_err_t dart_heap_arr_set_str(dart_heap_t* dst, size_t idx, char const* val) {
    return dart_heap_arr_set_str_len(dst, idx, val, strlen(val));
  }

  dart_err_t dart_heap_arr_set_str_len(dart_heap_t* dst, size_t idx, char const* val, size_t val_len) {
    auto set = [=] (auto& dst) { dst.set(idx, string_view {val, val_len}); };
    return heap_access(
      compose(
        [set] (dart::heap& dst) { set(dst); },
        [set] (dart::unsafe_heap& dst) { set(dst); }
      ),
      dst
    );
  }

  dart_err_t dart_heap_arr_set_int(dart_heap_t* dst, size_t idx, int64_t val) {
    auto set = [=] (auto& dst) { dst.set(idx, val); };
    return heap_access(
      compose(
        [set] (dart::heap& dst) { set(dst); },
        [set] (dart::unsafe_heap& dst) { set(dst); }
      ),
      dst
    );
  }

  dart_err_t dart_heap_arr_set_dcm(dart_heap_t* dst, size_t idx, double val) {
    auto set = [=] (auto& dst) { dst.set(idx, val); };
    return heap_access(
      compose(
        [set] (dart::heap& dst) { set(dst); },
        [set] (dart::unsafe_heap& dst) { set(dst); }
      ),
      dst
    );
  }

  dart_err_t dart_heap_arr_set_bool(dart_heap_t* dst, size_t idx, int val) {
    auto set = [=] (auto& dst) { dst.set(idx, static_cast<bool>(val)); };
    return heap_access(
      compose(
        [set] (dart::heap& dst) { set(dst); },
        [set] (dart::unsafe_heap& dst) { set(dst); }
      ),
      dst
    );
  }

  dart_err_t dart_heap_arr_set_null(dart_heap_t* dst, size_t idx) {
    auto set = [=] (auto& dst) { dst.set(idx, nullptr); };
    return heap_access(
      compose(
        [=] (dart::heap& dst) { set(dst); },
        [=] (dart::unsafe_heap& dst) { set(dst); }
      ),
      dst
    );
  }

  dart_err_t dart_heap_arr_erase(dart_heap_t* dst, size_t idx) {
    auto erase = [=] (auto& dst) { dst.erase(idx); };
    return heap_access(
      compose(
        [erase] (dart::heap& dst) { erase(dst); },
        [erase] (dart::unsafe_heap& dst) { erase(dst); }
      ),
      dst
    );
  }

  dart_err_t dart_heap_arr_resize(dart_heap_t* dst, size_t len) {
    auto resize = [=] (auto& dst) { dst.resize(len); };
    return heap_access(
      compose(
        [resize] (dart::heap& dst) { resize(dst); },
        [resize] (dart::unsafe_heap& dst) { resize(dst); }
      ),
      dst
    );
  }

  dart_err_t dart_heap_arr_reserve(dart_heap_t* dst, size_t len) {
    auto reserve = [=] (auto& dst) { dst.reserve(len); };
    return heap_access(
      compose(
        [reserve] (dart::heap& dst) { reserve(dst); },
        [reserve] (dart::unsafe_heap& dst) { reserve(dst); }
      ),
      dst
    );
  }

  int dart_heap_obj_has_key(dart_heap_t const* src, char const* key) {
    return dart_heap_obj_has_key_len(src, key, strlen(key));
  }

  int dart_heap_obj_has_key_len(dart_heap_t const* src, char const* key, size_t len) {
    bool val = false;
    auto err = heap_access(
      [&val, key, len] (auto& src) { val = src.has_key(string_view {key, len}); },
      src
    );
    if (err) return false;
    else return val;
  }

  dart_heap_t dart_heap_obj_get(dart_heap_t const* src, char const* key) {
    dart_heap_t dst;
    auto err = dart_heap_obj_get_err(&dst, src, key);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_obj_get_err(dart_heap_t* dst, dart_heap_t const* src, char const* key) {
    return dart_heap_obj_get_len_err(dst, src, key, strlen(key));
  }

  dart_heap_t dart_heap_obj_get_len(dart_heap_t const* src, char const* key, size_t len) {
    dart_heap_t dst;
    auto err = dart_heap_obj_get_len_err(&dst, src, key, len);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_obj_get_len_err(dart_heap_t* dst, dart_heap_t const* src, char const* key, size_t len) {
    // Initialize.
    dst->rtti = src->rtti;
    return heap_access(
      compose(
        [=] (dart::heap const& src) {
          return heap_construct([&] (dart::heap* dst) {
            new(dst) dart::heap(src[string_view {key, len}]);
          }, dst);
        },
        [=] (dart::unsafe_heap const& src) {
          return heap_construct([&] (dart::unsafe_heap* dst) {
            new(dst) dart::unsafe_heap(src[string_view {key, len}]);
          }, dst);
        }
      ),
      src
    );
  }

  dart_heap_t dart_heap_arr_get(dart_heap_t const* src, size_t idx) {
    dart_heap_t dst;
    auto err = dart_heap_arr_get_err(&dst, src, idx);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_arr_get_err(dart_heap_t* dst, dart_heap_t const* src, size_t idx) {
    // Initialize.
    dst->rtti = src->rtti;
    return heap_access(
      compose(
        [=] (dart::heap const& src) {
          return heap_construct([&] (dart::heap* dst) {
            new(dst) dart::heap(src[idx]);
          }, dst);
        },
        [=] (dart::unsafe_heap const& src) {
          return heap_construct([&] (dart::unsafe_heap* dst) {
            new(dst) dart::unsafe_heap(src[idx]);
          }, dst);
        }
      ),
      src
    );
  }

  char const* dart_heap_str_get(dart_heap_t const* src) {
    size_t dummy;
    return dart_heap_str_get_len(src, &dummy);
  }

  char const* dart_heap_str_get_len(dart_heap_t const* src, size_t* len) {
    char const* str;
    auto get_str = [&] (auto& src) {
      auto view = src.strv();
      str = view.data();
      *len = view.size();
    };
    auto err = heap_access(
      compose(
        [get_str] (dart::heap const& src) { get_str(src); },
        [get_str] (dart::unsafe_heap const& src) { get_str(src); }
      ),
      src
    );
    if (err) return nullptr;
    else return str;
  }

  int64_t dart_heap_int_get(dart_heap_t const* src) {
    // No way unique way to signal failure here,
    // the user needs to know this will succeed.
    int64_t val = 0;
    dart_heap_int_get_err(src, &val);
    return val;
  }

  dart_err_t dart_heap_int_get_err(dart_heap_t const* src, int64_t* val) {
    auto get_int = [=] (auto& src) { *val = src.integer(); };
    return heap_access(
      compose(
        [get_int] (dart::heap const& src) { get_int(src); },
        [get_int] (dart::unsafe_heap const& src) { get_int(src); }
      ),
      src
    );
  }

  double dart_heap_dcm_get(dart_heap_t const* src) {
    double val = std::numeric_limits<double>::quiet_NaN();
    dart_heap_dcm_get_err(src, &val);
    return val;
  }

  dart_err_t dart_heap_dcm_get_err(dart_heap_t const* src, double* val) {
    auto get_dcm = [=] (auto& src) { *val = src.decimal(); };
    return heap_access(
      compose(
        [get_dcm] (dart::heap const& src) { get_dcm(src); },
        [get_dcm] (dart::unsafe_heap const& src) { get_dcm(src); }
      ),
      src
    );
  }

  int dart_heap_bool_get(dart_heap_t const* src) {
    // No way unique way to signal failure here,
    // the user needs to know this will succeed.
    int val = 0;
    dart_heap_bool_get_err(src, &val);
    return val;
  }

  dart_err_t dart_heap_bool_get_err(dart_heap_t const* src, int* val) {
    auto get_bool = [=] (auto& src) { *val = src.boolean(); };
    return heap_access(
      compose(
        [get_bool] (dart::heap const& src) { get_bool(src); },
        [get_bool] (dart::unsafe_heap const& src) { get_bool(src); }
      ),
      src
    );
  }

  size_t dart_heap_size(dart_heap_t const* src) {
    size_t val = 0;
    auto err = heap_access([&val] (auto& src) { val = src.size(); }, src);
    if (err) return DART_FAILURE;
    else return val;
  }

  int dart_heap_equal(dart_heap_t const* lhs, dart_heap_t const* rhs) {
    bool equal = false;
    auto check = [&] (auto& lhs, auto& rhs) { equal = (lhs == rhs); };
    auto err = heap_access(
      compose(
        [check, rhs] (dart::heap const& lhs) {
          heap_access([check, lhs] (dart::heap const& rhs) { check(lhs, rhs); }, rhs);
        },
        [check, rhs] (dart::unsafe_heap const& lhs) {
          heap_access([check, lhs] (dart::unsafe_heap const& rhs) { check(lhs, rhs); }, rhs);
        }
      ),
      lhs
    );
    if (err) return false;
    else return equal;
  }

  int dart_heap_is_obj(dart_heap_t const* src) {
    return dart_heap_get_type(src) == DART_OBJECT;
  }

  int dart_heap_is_arr(dart_heap_t const* src) {
    return dart_heap_get_type(src) == DART_ARRAY;
  }

  int dart_heap_is_str(dart_heap_t const* src) {
    return dart_heap_get_type(src) == DART_STRING;
  }

  int dart_heap_is_int(dart_heap_t const* src) {
    return dart_heap_get_type(src) == DART_INTEGER;
  }

  int dart_heap_is_dcm(dart_heap_t const* src) {
    return dart_heap_get_type(src) == DART_DECIMAL;
  }

  int dart_heap_is_bool(dart_heap_t const* src) {
    return dart_heap_get_type(src) == DART_BOOLEAN;
  }

  int dart_heap_is_null(dart_heap_t const* src) {
    return dart_heap_get_type(src) == DART_NULL;
  }

  dart_type_t dart_heap_get_type(dart_heap_t const* src) {
    dart_type_t type;
    auto get_type = [&] (auto& pkt) { type = abi_type(pkt.get_type()); };
    auto err = heap_access(
      compose(
        [=] (dart::heap const& pkt) { get_type(pkt); },
        [=] (dart::unsafe_heap const& pkt) { get_type(pkt); }
      ),
      src
    );
    if (err) return DART_INVALID;
    else return type;
  }

  dart_heap_t dart_heap_from_json(char const* str) {
    dart_heap_t dst;
    auto err = dart_heap_from_json_err(&dst, str);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_from_json_err(dart_heap_t* dst, char const* str) {
    return dart_heap_from_json_len_rc_err(dst, DART_RC_SAFE, str, strlen(str));
  }

  dart_heap_t dart_heap_from_json_rc(dart_rc_type_t rc, char const* str) {
    dart_heap_t dst;
    auto err = dart_heap_from_json_rc_err(&dst, rc, str);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_from_json_rc_err(dart_heap_t* dst, dart_rc_type_t rc, char const* str) {
    return dart_heap_from_json_len_rc_err(dst, rc, str, strlen(str));
  }

  dart_heap_t dart_heap_from_json_len(char const* str, size_t len) {
    dart_heap_t dst;
    auto err = dart_heap_from_json_len_err(&dst, str, len);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_from_json_len_err(dart_heap_t* dst, char const* str, size_t len) {
    return dart_heap_from_json_len_rc_err(dst, DART_RC_SAFE, str, len);
  }

  dart_heap_t dart_heap_from_json_len_rc(dart_rc_type_t rc, char const* str, size_t len) {
    dart_heap_t dst;
    auto err = dart_heap_from_json_len_rc_err(&dst, rc, str, len);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_from_json_len_rc_err(dart_heap_t* dst, dart_rc_type_t rc, char const* str, size_t len) {
    return heap_typed_constructor_access(
      compose(
        [=] (dart::heap& dst) {
          dst = dart::heap::from_json({str, len});
        },
        [=] (dart::unsafe_heap& dst) {
          dst = dart::unsafe_heap::from_json({str, len});
        }
      ),
      dst,
      rc
    );
  }

  char* dart_heap_to_json(dart_heap_t const* pkt, size_t* len) {
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
    auto ret = heap_access(
      compose(
        [=] (dart::heap const& pkt) { print(pkt); },
        [=] (dart::unsafe_heap const& pkt) { print(pkt); }
      ),
      pkt
    );
    if (ret) return nullptr;
    return outstr;
  }

  dart_buffer_t dart_heap_lower(dart_heap_t const* src) {
    dart_buffer_t dst;
    auto err = dart_heap_lower_err(&dst, src);
    if (err) return dart_buffer_init();
    else return dst;
  }

  dart_buffer_t dart_heap_finalize(dart_heap_t const* src) {
    dart_buffer_t dst;
    auto err = dart_heap_finalize_err(&dst, src);
    if (err) return dart_buffer_init();
    else return dst;
  }

  dart_err_t dart_heap_lower_err(dart_buffer_t* dst, dart_heap_t const* src) {
    // Initialize.
    dst->rtti = {DART_BUFFER, src->rtti.rc_id};
    return heap_access(
      compose(
        [dst] (dart::heap const& src) {
          buffer_construct([&src] (dart::buffer* dst) { new(dst) dart::buffer(src.lower()); }, dst);
        },
        [dst] (dart::unsafe_heap const& src) {
          buffer_construct([&src] (dart::unsafe_buffer* dst) { new(dst) dart::unsafe_buffer(src.lower()); }, dst);
        }
      ),
      src
    );
  }

  dart_err_t dart_heap_finalize_err(dart_buffer_t* dst, dart_heap_t const* src) {
    return dart_heap_lower_err(dst, src);
  }

}
