/*----- Local Includes -----*/

#include "helpers.h"

/*----- Globals -----*/

namespace dart {
  namespace detail {
    thread_local std::string errmsg {};
  }
}


/*----- Helpers -----*/

namespace {

  void dart_rtti_propagate(void* dst, void const* src) {
    auto* punned_dst = reinterpret_cast<dart_type_id_t*>(dst);
    auto* punned_src = reinterpret_cast<dart_type_id_t const*>(src);
    *punned_dst = *punned_src;
  }

  void dart_rc_propagate(void* dst, void const* src) {
    auto* punned_dst = reinterpret_cast<dart_type_id_t*>(dst);
    auto* punned_src = reinterpret_cast<dart_type_id_t const*>(src);
    punned_dst->rc_id = punned_src->rc_id;
  }

  // XXX: Thank microsoft for this whole mess.
  // MSVC can't do generic lambdas inside of extern C functions.
  // It ends up improperly propagating the function linkage to the lambda,
  // and it explodes since a template can't have C language linkage.
  // Apparently some previews of Visual Studio 2019 have this fixed, but we'll just
  // hack around it for now.
  // So like half of the functions in the ABI have to call through an implementation
  // function with internal linkage.
  dart_err_t dart_copy_err_impl(void* dst, void const* src) {
    // Initialize.
    dart_rtti_propagate(dst, src);
    return generic_access(
      [=] (auto& src) { return generic_construct([&src] (auto* dst) { safe_construct(dst, src); }, dst); },
      src
    );
  }

  dart_err_t dart_move_err_impl(void* dst, void* src) {
    // Initialize.
    dart_rtti_propagate(dst, src);
    return generic_access(
      [=] (auto& src) { return generic_construct([&src] (auto* dst) { safe_construct(dst, std::move(src)); }, dst); },
      src
    );
  }

  dart_err_t dart_obj_insert_dart_len_impl(void* dst, char const* key, size_t len, void const* val) {
    return generic_access(
      mutable_visitor(
        [key, len, val] (auto& dst) {
          return generic_access([&dst, key, len] (auto& val) {
            safe_insert(dst, string_view {key, len}, val);
          }, val);
        }
      ),
      dst
    );
  }

  dart_err_t dart_obj_insert_take_dart_len_impl(void* dst, char const* key, size_t len, void* val) {
    return generic_access(
      mutable_visitor(
        [key, len, val] (auto& dst) {
          return generic_access([&dst, key, len] (auto& val) {
            safe_insert(dst, string_view {key, len}, std::move(val));
          }, val);
        }
      ),
      dst
    );
  }

  dart_err_t dart_obj_insert_str_len_impl(void* dst, char const* key, size_t len, char const* val, size_t val_len) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.insert(string_view {key, len}, string_view {val, val_len}); }),
      dst
    );
  }

  dart_err_t dart_obj_insert_int_len_impl(void* dst, char const* key, size_t len, int64_t val) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.insert(string_view {key, len}, val); }),
      dst
    );
  }

  dart_err_t dart_obj_insert_dcm_len_impl(void* dst, char const* key, size_t len, double val) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.insert(string_view {key, len}, val); }),
      dst
    );
  }

  dart_err_t dart_obj_insert_bool_len_impl(void* dst, char const* key, size_t len, int val) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.insert(string_view {key, len}, static_cast<bool>(val)); }),
      dst
    );
  }

  dart_err_t dart_obj_insert_null_len_impl(void* dst, char const* key, size_t len) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.insert(string_view {key, len}, nullptr); }),
      dst
    );
  }

  dart_err_t dart_obj_set_dart_len_impl(void* dst, char const* key, size_t len, void const* val) {
    return generic_access(
      mutable_visitor(
        [key, len, val] (auto& dst) {
          return generic_access([&dst, key, len] (auto& val) {
            safe_set(dst, string_view {key, len}, val);
          }, val);
        }
      ),
      dst
    );
  }

  dart_err_t dart_obj_set_take_dart_len_impl(void* dst, char const* key, size_t len, void* val) {
    return generic_access(
      mutable_visitor(
        [key, len, val] (auto& dst) {
          return generic_access([&dst, key, len] (auto& val) {
            safe_set(dst, string_view {key, len}, std::move(val));
          }, val);
        }
      ),
      dst
    );
  }

  dart_err_t dart_obj_set_str_len_impl(void* dst, char const* key, size_t len, char const* val, size_t val_len) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.set(string_view {key, len}, string_view {val, val_len}); }),
      dst
    );
  }

  dart_err_t dart_obj_set_int_len_impl(void* dst, char const* key, size_t len, int64_t val) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.set(string_view {key, len}, val); }),
      dst
    );
  }

  dart_err_t dart_obj_set_dcm_len_impl(void* dst, char const* key, size_t len, double val) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.set(string_view {key, len}, val); }),
      dst
    );
  }

  dart_err_t dart_obj_set_bool_len_impl(void* dst, char const* key, size_t len, int val) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.set(string_view {key, len}, static_cast<bool>(val)); }),
      dst
    );
  }

  dart_err_t dart_obj_set_null_len_impl(void* dst, char const* key, size_t len) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.set(string_view {key, len}, nullptr); }),
      dst
    );
  }

  dart_err_t dart_obj_clear_impl(void* dst) {
    return generic_access(mutable_visitor([] (auto& dst) { dst.clear(); }), dst);
  }

  dart_err_t dart_obj_erase_len_impl(void* dst, char const* key, size_t len) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.erase(string_view {key, len}); }),
      dst
    );
  }

  dart_err_t dart_arr_insert_dart_impl(void* dst, size_t idx, void const* val) {
    return generic_access(
      mutable_visitor(
        [idx, val] (auto& dst) {
          return generic_access([&dst, idx] (auto& val) { safe_insert(dst, idx, val); }, val);
        }
      ),
      dst
    );
  }

  dart_err_t dart_arr_insert_take_dart_impl(void* dst, size_t idx, void* val) {
    return generic_access(
      mutable_visitor(
        [idx, val] (auto& dst) {
          return generic_access([&dst, idx] (auto& val) { safe_insert(dst, idx, std::move(val)); }, val);
        }
      ),
      dst
    );
  }

  dart_err_t dart_arr_insert_str_len_impl(void* dst, size_t idx, char const* val, size_t val_len) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.insert(idx, string_view {val, val_len}); }),
      dst
    );
  }
  
  dart_err_t dart_arr_insert_int_impl(void* dst, size_t idx, int64_t val) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.insert(idx, val); }),
      dst
    );
  }

  dart_err_t dart_arr_insert_dcm_impl(void* dst, size_t idx, double val) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.insert(idx, val); }),
      dst
    );
  }

  dart_err_t dart_arr_insert_bool_impl(void* dst, size_t idx, int val) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.insert(idx, static_cast<bool>(val)); }),
      dst
    );
  }

  dart_err_t dart_arr_insert_null_impl(void* dst, size_t idx) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.insert(idx, nullptr); }),
      dst
    );
  }

  dart_err_t dart_arr_set_dart_impl(void* dst, size_t idx, void const* val) {
    return generic_access(
      mutable_visitor(
        [idx, val] (auto& dst) {
          return generic_access([&dst, idx] (auto& val) { safe_set(dst, idx, val); }, val);
        }
      ),
      dst
    );
  }

  dart_err_t dart_arr_set_take_dart_impl(void* dst, size_t idx, void* val) {
    return generic_access(
      mutable_visitor(
        [idx, val] (auto& dst) {
          return generic_access([&dst, idx] (auto& val) { safe_set(dst, idx, std::move(val)); }, val);
        }
      ),
      dst
    );
  }

  dart_err_t dart_arr_set_str_len_impl(void* dst, size_t idx, char const* val, size_t val_len) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.set(idx, string_view {val, val_len}); }),
      dst
    );
  }
  
  dart_err_t dart_arr_set_int_impl(void* dst, size_t idx, int64_t val) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.set(idx, val); }),
      dst
    );
  }

  dart_err_t dart_arr_set_dcm_impl(void* dst, size_t idx, double val) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.set(idx, val); }),
      dst
    );
  }

  dart_err_t dart_arr_set_bool_impl(void* dst, size_t idx, int val) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.set(idx, static_cast<bool>(val)); }),
      dst
    );
  }

  dart_err_t dart_arr_set_null_impl(void* dst, size_t idx) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.set(idx, nullptr); }),
      dst
    );
  }

  dart_err_t dart_arr_clear_impl(void* dst) {
    return generic_access(mutable_visitor([] (auto& dst) { dst.clear(); }), dst);
  }

  dart_err_t dart_arr_erase_impl(void* dst, size_t idx) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.erase(idx); }),
      dst
    );
  }

  dart_err_t dart_arr_resize_impl(void* dst, size_t len) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.resize(len); }),
      dst
    );
  }

  dart_err_t dart_arr_reserve_impl(void* dst, size_t len) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.reserve(len); }),
      dst
    );
  }

  int dart_obj_has_key_len_impl(void const* src, char const* key, size_t len) {
    bool val = false;
    auto err = generic_access(
      [&val, key, len] (auto& src) { val = src.has_key(string_view {key, len}); },
      src
    );
    if (err) return false;
    else return val;
  }

  dart_err_t dart_obj_get_len_err_impl(dart_packet_t* dst, void const* src, char const* key, size_t len) {
    // Initialize.
    dart_rc_propagate(dst, src);
    dst->rtti.p_id = DART_PACKET;
    return generic_access(
      [=] (auto& src) {
        return packet_construct([=, &src] (auto* dst) {
          safe_construct(dst, src.get({key, len}));
        }, dst);
      },
      src
    );
  }

  dart_err_t dart_arr_get_err_impl(dart_packet_t* dst, void const* src, size_t idx) {
    // Initialize.
    dart_rc_propagate(dst, src);
    dst->rtti.p_id = DART_PACKET;
    return generic_access(
      [=] (auto& src) {
        return packet_construct([=, &src] (auto* dst) {
          safe_construct(dst, src.get(idx));
        }, dst);
      },
      src
    );
  }

  char const* dart_str_get_len_impl(void const* src, size_t* len) {
    char const* ptr;
    auto ret = generic_access([&] (auto& src) {
      auto strv = src.strv();
      ptr = strv.data();
      *len = strv.size();
    }, src);
    if (ret) return nullptr;
    else return ptr;
  }

  dart_err_t dart_int_get_err_impl(void const* src, int64_t* val) {
    return generic_access([=] (auto& str) { *val = str.integer(); }, src);
  }

  dart_err_t dart_dcm_get_err_impl(void const* src, double* val) {
    return generic_access([=] (auto& str) { *val = str.decimal(); }, src);
  }

  dart_err_t dart_bool_get_err_impl(void const* src, int* val) {
    return generic_access([=] (auto& src) { *val = src.boolean(); }, src);
  }

  size_t dart_size_impl(void const* src) {
    size_t val = 0;
    auto err = generic_access([&val] (auto& src) { val = src.size(); }, src);
    if (err) return DART_FAILURE;
    else return val;
  }

  int dart_equal_impl(void const* lhs, void const* rhs) {
    bool equal = false;
    dart::detail::typeless_comparator comp {};
    auto err = generic_access(
      [&] (auto& lhs) { generic_access([&] (auto& rhs) { equal = comp(lhs, rhs); }, rhs); },
      lhs
    );
    if (err) return false;
    else return equal;
  }

  int dart_is_finalized_impl(void const* src) {
    bool finalized = false;
    auto err = generic_access(
      [&] (auto& src) { finalized = src.is_finalized(); },
      src
    );
    if (err) return false;
    else return finalized;
  }

  dart_type_t dart_get_type_impl(void const* src) {
    dart_type_t type;
    auto err = generic_access([&] (auto& str) { type = abi_type(str.get_type()); }, src);
    if (err) return DART_INVALID;
    else return type;
  }

  size_t dart_refcount_impl(void const* src) {
    size_t rc = 0;
    auto err = generic_access([&rc] (auto& src) { rc = src.refcount(); }, src);
    if (err) return DART_FAILURE;
    else return rc;
  }

  char* dart_to_json_impl(void const* src, size_t* len) {
    char* outstr;
    auto ret = generic_access(
      [&] (auto& pkt) {
        // Call these first so they throw before allocation.
        auto instr = pkt.to_json();
        auto inlen = instr.size();
        if (len) *len = inlen;
        outstr = reinterpret_cast<char*>(malloc(inlen + 1));
        memcpy(outstr, instr.data(), inlen + 1);
      },
      src
    );
    if (ret) return nullptr;
    return outstr;
  }

  dart_err_t dart_to_heap_err_impl(dart_heap_t* dst, void const* src) {
    dart_rc_propagate(dst, src);
    dst->rtti.p_id = DART_HEAP;
    return generic_access(
      [=] (auto& src) {
        auto tmp {src};
        return heap_construct([&tmp] (auto* dst) { safe_construct(dst, std::move(tmp).lift()); }, dst);
      },
      src
    );
  }

  dart_err_t dart_to_buffer_err_impl(dart_buffer_t* dst, void const* src) {
    dart_rc_propagate(dst, src);
    dst->rtti.p_id = DART_BUFFER;
    return generic_access(
      [=] (auto& src) {
        auto tmp {src};
        return buffer_construct([&tmp] (auto* dst) { safe_construct(dst, std::move(tmp).lift()); }, dst);
      },
      src
    );
  }

  dart_err_t dart_lower_err_impl(dart_packet_t* dst, void const* src) {
    dart_rc_propagate(dst, src);
    dst->rtti.p_id = DART_PACKET;
    return generic_access(
      [=] (auto& src) {
        auto tmp {src};
        return generic_construct([&tmp] (auto* dst) { safe_construct(dst, std::move(tmp).lower()); }, dst);
      },
      src
    );
  }

  dart_err_t dart_lift_err_impl(dart_packet_t* dst, void const* src) {
    dart_rc_propagate(dst, src);
    dst->rtti.p_id = DART_PACKET;
    return generic_access(
      [=] (auto& src) {
        auto tmp {src};
        return generic_construct([&tmp] (auto* dst) { safe_construct(dst, std::move(tmp).lift()); }, dst);
      },
      src
    );
  }

  void const* dart_get_bytes_impl(void const* src, size_t* len) {
    void const* ptr = nullptr;
    auto err = generic_access(
      immutable_visitor(
        [&ptr, len] (auto& src) {
          auto bytes = src.get_bytes();
          ptr = bytes.data();
          if (len) *len = bytes.size();
        }
      ),
      src
    );
    if (err) return nullptr;
    else return ptr;
  }

  void* dart_dup_bytes_impl(void const* src, size_t* len) {
    void* ptr = nullptr;
    auto err = generic_access(
      immutable_visitor(
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
        }
      ),
      src
    );
    if (err) return nullptr;
    else return ptr;
  }

  dart_err_t dart_from_bytes_rc_err_impl(dart_packet_t* dst, dart_rc_type_t rc, void const* bytes, size_t len) {
    auto* punned = reinterpret_cast<gsl::byte const*>(bytes);
    return packet_typed_constructor_access(
      [punned, len] (auto& dst) {
        using packet_type = std::decay_t<decltype(dst)>;
        dst = packet_type {gsl::make_span(punned, len)};
      },
      dst,
      rc
    );
  }

  dart_err_t dart_take_bytes_rc_err_impl(dart_packet_t* dst, dart_rc_type_t rc, void* bytes) {
    using owner_type = std::unique_ptr<gsl::byte const[], void (*) (gsl::byte const*)>;

    // We need to use dart_aligned_free here for the destructor as on windows, aligned alloc
    // must be paired with a call to aligned free. Linux/Mac are just fine with normal free
    auto* del = +[] (gsl::byte const* ptr) { dart_aligned_free(const_cast<gsl::byte*>(ptr)); };
    owner_type owner {reinterpret_cast<gsl::byte const*>(bytes), del};
    return packet_typed_constructor_access(
      [&] (auto& dst) {
        using packet_type = std::decay_t<decltype(dst)>;
        dst = packet_type {std::move(owner)};
      },
      dst,
      rc
    );
  }

  dart_err_t dart_iterator_init_from_err_impl(dart_iterator_t* dst, void const* src) {
    // Initialize.
    dart_rtti_propagate(dst, src);
    return generic_access(
      [dst] (auto& src) {
        using iterator = typename std::decay_t<decltype(src)>::iterator;
        return iterator_construct([&src] (iterator* begin, iterator* end) {
          new(begin) iterator(src.begin());
          new(end) iterator(src.end());
        }, dst);
      },
      src
    );
  }

  dart_err_t dart_iterator_init_key_from_err_impl(dart_iterator_t* dst, void const* src) {
    // Initialize.
    dart_rtti_propagate(dst, src);
    return generic_access(
      [dst] (auto& src) {
        using iterator = typename std::decay_t<decltype(src)>::iterator;
        return iterator_construct([&src] (iterator* begin, iterator* end) {
          new(begin) iterator(src.key_begin());
          new(end) iterator(src.key_end());
        }, dst);
      },
      src
    );
  }

  dart_err_t dart_iterator_copy_err_impl(dart_iterator_t* dst, dart_iterator_t const* src) {
    // Initialize.
    dart_rtti_propagate(dst, src);
    return iterator_access(
      [dst] (auto& src_curr, auto& src_end) {
        using type = std::decay_t<decltype(src_curr)>;
        return iterator_construct([&src_curr, &src_end] (type* dst_curr, type* dst_end) {
          new(dst_curr) type(src_curr);
          new(dst_end) type(src_end);
        }, dst);
      },
      src
    );
  }

  dart_err_t dart_iterator_move_err_impl(dart_iterator_t* dst, dart_iterator_t* src) {
    // Initialize.
    dart_rtti_propagate(dst, src);
    return iterator_access(
      [dst] (auto& src_curr, auto& src_end) {
        using type = std::decay_t<decltype(src_curr)>;
        return iterator_construct([&src_curr, &src_end] (type* dst_curr, type* dst_end) {
          new(dst_curr) type(std::move(src_curr));
          new(dst_end) type(std::move(src_end));
        }, dst);
      },
      src
    );
  }

  dart_err_t dart_iterator_destroy_impl(dart_iterator_t* dst) {
    return iterator_access([] (auto& start, auto& end) {
      using type = std::decay_t<decltype(start)>;
      start.~type();
      end.~type();
    }, dst);
  }

  dart_err_t dart_iterator_get_err_impl(dart_packet_t* dst, dart_iterator_t const* src) {
    // Initialize.
    dst->rtti.p_id = DART_PACKET;
    dart_rc_propagate(dst, src);
    return iterator_access(
      [dst] (auto& src_curr, auto& src_end) {
        // XXX: Yikes.
        // For the sake of simplicity (this API is already huge), the ABI iterator API always
        // hands out dart_packet_t instances, but the underlying iterator type will depend on what
        // type it was initialized from.
        // We need to safely dereference and convert whatever iterator type we currently have
        // into a dart::packet instance with the same reference counter type as the incoming iterator.
        // Thankfully, all of the Dart types have a builtin type member, generic_type, which will
        // compute the type we actually need to initialize here.
        using type = typename std::decay_t<decltype(src_curr)>::value_type::generic_type;
        if (src_curr == src_end) throw std::runtime_error("dart_iterator has been exhausted");
        return packet_construct([&src_curr] (type* dst) { new(dst) type(*src_curr); }, dst);
      },
      src
    );
  }

  dart_err_t dart_iterator_next_impl(dart_iterator_t* dst) {
    return iterator_access([] (auto& curr, auto& end) { if (curr != end) curr++; }, dst);
  }

  int dart_iterator_done_impl(dart_iterator_t const* src) {
    bool ended = false;
    auto err = iterator_access([&] (auto& curr, auto& end) { ended = (curr == end); }, src);
    if (err) return true;
    else return ended;
  }

}

/*----- Function Implementations -----*/

extern "C" {

  dart_packet_t dart_init() {
    // Cannot meaningfully fail.
    dart_packet_t dst;
    dart_init_err(&dst);
    return dst;
  }

  dart_err_t dart_init_err(dart_packet_t* dst) {
    return dart_init_rc_err(dst, DART_RC_SAFE);
  }

  dart_packet_t dart_init_rc(dart_rc_type_t rc) {
    // Cannot meaningfully fail.
    dart_packet_t dst;
    dart_init_rc_err(&dst, rc);
    return dst;
  }

  dart_err_t dart_init_rc_err(dart_packet_t* dst, dart_rc_type_t rc) {
    // Initialize.
    dst->rtti = {DART_PACKET, rc};
    return packet_constructor_access(
      compose(
        [] (dart::packet* ptr) { new(ptr) dart::packet(); },
        [] (dart::unsafe_packet* ptr) { new(ptr) dart::unsafe_packet(); }
      ),
      dst
    );
  }

  dart_packet_t dart_copy(void const* src) {
    dart_packet_t dst;
    auto err = dart_copy_err(&dst, src);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_copy_err(void* dst, void const* src) {
    return dart_copy_err_impl(dst, src);
  }

  dart_packet_t dart_move(void* src) {
    dart_packet_t dst;
    auto err = dart_move_err(&dst, src);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_move_err(void* dst, void* src) {
    return dart_move_err_impl(dst, src);
  }

  dart_err_t dart_destroy(void* pkt) {
    // Generic destroy function.
    // Get rid of it, whatever it is.
    return generic_access(
      compose(
        [] (dart::heap& pkt) { pkt.~basic_heap(); },
        [] (dart::unsafe_heap& pkt) { pkt.~basic_heap(); },
        [] (dart::buffer& pkt) { pkt.~basic_buffer(); },
        [] (dart::unsafe_buffer& pkt) { pkt.~basic_buffer(); },
        [] (dart::packet& pkt) { pkt.~basic_packet(); },
        [] (dart::unsafe_packet& pkt) { pkt.~basic_packet(); }
      ),
      pkt
    );
  }

  dart_packet_t dart_obj_init() {
    dart_packet_t dst;
    auto err = dart_obj_init_err(&dst);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_obj_init_err(dart_packet_t* dst) {
    return dart_obj_init_rc_err(dst, DART_RC_SAFE);
  }

  dart_packet_t dart_obj_init_rc(dart_rc_type_t rc) {
    dart_packet_t dst;
    auto err = dart_obj_init_rc_err(&dst, rc);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_obj_init_rc_err(dart_packet_t* dst, dart_rc_type_t rc) {
    // Default initialize, then assign.
    return packet_typed_constructor_access(
      compose(
        [] (dart::packet& dst) { dst = dart::packet::make_object(); },
        [] (dart::unsafe_packet& dst) { dst = dart::unsafe_packet::make_object(); }
      ),
      dst,
      rc
    );
  }

  static dart_err_t dart_obj_init_va_impl(dart_packet_t* dst, dart_rc_type_t rc, char const* format, va_list args) {
    return packet_typed_constructor_access(
      compose(
        [format, args] (dart::packet& dst) mutable {
          dst = dart::packet::make_object();
          parse_pairs(dst, format, args);
        },
        [format, args] (dart::unsafe_packet& dst) mutable {
          dst = dart::unsafe_packet::make_object();
          parse_pairs(dst, format, args);
        }
      ),
      dst,
      rc
    );
  }

  dart_packet_t dart_obj_init_va(char const* format, ...) {
    va_list args;
    dart_packet_t dst;
    va_start(args, format);
    auto ret = dart_obj_init_va_impl(&dst, DART_RC_SAFE, format, args);
    va_end(args);
    if (ret) return dart_init();
    else return dst;
  }

  dart_err_t dart_obj_init_va_err(dart_packet_t* dst, char const* format, ...) {
    va_list args;
    va_start(args, format);
    auto ret = dart_obj_init_va_impl(dst, DART_RC_SAFE, format, args);
    va_end(args);
    return ret;
  }

  dart_packet_t dart_obj_init_va_rc(dart_rc_type_t rc, char const* format, ...) {
    va_list args;
    dart_packet_t dst;
    va_start(args, format);
    auto ret = dart_obj_init_va_impl(&dst, rc, format, args);
    va_end(args);
    if (ret) return dart_init();
    else return dst;
  }

  dart_err_t dart_obj_init_va_rc_err(dart_packet_t* dst, dart_rc_type_t rc, char const* format, ...) {
    va_list args;
    va_start(args, format);
    auto ret = dart_obj_init_va_impl(dst, rc, format, args);
    va_end(args);
    return ret;
  }

  dart_packet_t dart_arr_init() {
    dart_packet_t dst;
    auto err = dart_arr_init_err(&dst);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_arr_init_err(dart_packet_t* dst) {
    return dart_arr_init_rc_err(dst, DART_RC_SAFE);
  }

  dart_packet_t dart_arr_init_rc(dart_rc_type_t rc) {
    dart_packet_t dst;
    auto err = dart_arr_init_rc_err(&dst, rc);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_arr_init_rc_err(dart_packet_t* dst, dart_rc_type_t rc) {
    // Default initialize, then assign.
    return packet_typed_constructor_access(
      compose(
        [] (dart::packet& dst) { dst = dart::packet::make_array(); },
        [] (dart::unsafe_packet& dst) { dst = dart::unsafe_packet::make_array(); }
      ),
      dst,
      rc
    );
  }

  static dart_err_t dart_arr_init_va_impl(dart_packet_t* dst, dart_rc_type_t rc, char const* format, va_list args) {
    return packet_typed_constructor_access(
      compose(
        [format, args] (dart::packet& dst) mutable {
          dst = dart::packet::make_array();
          parse_vals(dst, format, args);
        },
        [format, args] (dart::unsafe_packet& dst) mutable {
          dst = dart::unsafe_packet::make_array();
          parse_vals(dst, format, args);
        }
      ),
      dst,
      rc
    );
  }

  dart_packet_t dart_arr_init_va(char const* format, ...) {
    va_list args;
    dart_packet_t dst;
    va_start(args, format);
    auto ret = dart_arr_init_va_impl(&dst, DART_RC_SAFE, format, args);
    if (ret) return dart_init();
    else return dst;
  }

  dart_err_t dart_arr_init_va_err(dart_packet_t* dst, char const* format, ...) {
    va_list args;
    va_start(args, format);
    auto ret = dart_arr_init_va_impl(dst, DART_RC_SAFE, format, args);
    va_end(args);
    return ret;
  }

  dart_packet_t dart_arr_init_va_rc(dart_rc_type_t rc, char const* format, ...) {
    va_list args;
    dart_packet_t dst;
    va_start(args, format);
    auto ret = dart_arr_init_va_impl(&dst, rc, format, args);
    if (ret) return dart_init();
    else return dst;
  }

  dart_err_t dart_arr_init_va_rc_err(dart_packet_t* dst, dart_rc_type_t rc, char const* format, ...) {
    va_list args;
    va_start(args, format);
    auto ret = dart_arr_init_va_impl(dst, rc, format, args);
    va_end(args);
    return ret;
  }

  dart_packet_t dart_str_init(char const* str) {
    dart_packet_t dst;
    auto err = dart_str_init_err(&dst, str);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_str_init_err(dart_packet_t* dst, char const* str) {
    return dart_str_init_len_err(dst, str, strlen(str));
  }

  dart_packet_t dart_str_init_len(char const* str, size_t len) {
    dart_packet_t dst;
    auto err = dart_str_init_len_err(&dst, str, len);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_str_init_len_err(dart_packet_t* dst, char const* str, size_t len) {
    return dart_str_init_rc_len_err(dst, DART_RC_SAFE, str, len);
  }

  dart_packet_t dart_str_init_rc(dart_rc_type_t rc, char const* str) {
    dart_packet_t dst;
    auto err = dart_str_init_rc_err(&dst, rc, str);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_str_init_rc_err(dart_packet_t* dst, dart_rc_type_t rc, char const* str) {
    return dart_str_init_rc_len_err(dst, rc, str, strlen(str));
  }

  dart_packet_t dart_str_init_rc_len(dart_rc_type_t rc, char const* str, size_t len) {
    dart_packet_t dst;
    auto err = dart_str_init_rc_len_err(&dst, rc, str, len);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_str_init_rc_len_err(dart_packet_t* dst, dart_rc_type_t rc, char const* str, size_t len) {
    // Default initialize, then assign
    return packet_typed_constructor_access(
      compose(
        [str, len] (dart::packet& dst) { dst = dart::packet::make_string({str, len}); },
        [str, len] (dart::unsafe_packet& dst) { dst = dart::unsafe_packet::make_string({str, len}); }
      ),
      dst,
      rc
    );
  }

  dart_packet_t dart_int_init(int64_t val) {
    dart_packet_t dst;
    auto err = dart_int_init_err(&dst, val);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_int_init_err(dart_packet_t* dst, int64_t val) {
    return dart_int_init_rc_err(dst, DART_RC_SAFE, val);
  }

  dart_packet_t dart_int_init_rc(dart_rc_type_t rc, int64_t val) {
    dart_packet_t dst;
    auto err = dart_int_init_rc_err(&dst, rc, val);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_int_init_rc_err(dart_packet_t* dst, dart_rc_type_t rc, int64_t val) {
    // Default initialize, then assign
    return packet_typed_constructor_access(
      compose(
        [val] (dart::packet& dst) { dst = dart::packet::make_integer(val); },
        [val] (dart::unsafe_packet& dst) { dst = dart::unsafe_packet::make_integer(val); }
      ),
      dst,
      rc
    );
  }

  dart_packet_t dart_dcm_init(double val) {
    dart_packet_t dst;
    auto err = dart_dcm_init_err(&dst, val);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_dcm_init_err(dart_packet_t* dst, double val) {
    return dart_dcm_init_rc_err(dst, DART_RC_SAFE, val);
  }

  dart_packet_t dart_dcm_init_rc(dart_rc_type_t rc, double val) {
    dart_packet_t dst;
    auto err = dart_dcm_init_rc_err(&dst, rc, val);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_dcm_init_rc_err(dart_packet_t* dst, dart_rc_type_t rc, double val) {
    // Default initialize, then assign
    return packet_typed_constructor_access(
      compose(
        [val] (dart::packet& dst) { dst = dart::packet::make_decimal(val); },
        [val] (dart::unsafe_packet& dst) { dst = dart::unsafe_packet::make_decimal(val); }
      ),
      dst,
      rc
    );
  }

  dart_packet_t dart_bool_init(int val) {
    dart_packet_t dst;
    auto err = dart_bool_init_err(&dst, val);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_bool_init_err(dart_packet_t* dst, int val) {
    return dart_bool_init_rc_err(dst, DART_RC_SAFE, val);
  }

  dart_packet_t dart_bool_init_rc(dart_rc_type_t rc, int val) {
    dart_packet_t dst;
    auto err = dart_bool_init_rc_err(&dst, rc, val);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_bool_init_rc_err(dart_packet_t* dst, dart_rc_type_t rc, int val) {
    // Default initialize, then assign
    return packet_typed_constructor_access(
      compose(
        [val] (dart::packet& dst) { dst = dart::packet::make_boolean(val); },
        [val] (dart::unsafe_packet& dst) { dst = dart::unsafe_packet::make_boolean(val); }
      ),
      dst,
      rc
    );
  }

  dart_packet_t dart_null_init() {
    dart_packet_t dst;
    auto err = dart_null_init_err(&dst);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_null_init_err(dart_packet_t* dst) {
    return dart_null_init_rc_err(dst, DART_RC_SAFE);
  }

  dart_packet_t dart_null_init_rc(dart_rc_type_t rc) {
    dart_packet_t dst;
    auto err = dart_null_init_rc_err(&dst, rc);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_null_init_rc_err(dart_packet_t* dst, dart_rc_type_t rc) {
    // Default initialize, then assign.
    // Unnecessary, but done for consistency of code formatting.
    return packet_typed_constructor_access(
      compose(
        [] (dart::packet& dst) { dst = dart::packet::make_null(); },
        [] (dart::unsafe_packet& dst) { dst = dart::unsafe_packet::make_null(); }
      ),
      dst,
      rc
    );
  }

  dart_err_t dart_obj_insert_dart(void* dst, char const* key, void const* val) {
    return dart_obj_insert_dart_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_obj_insert_dart_len(void* dst, char const* key, size_t len, void const* val) {
    return dart_obj_insert_dart_len_impl(dst, key, len, val);
  }

  dart_err_t dart_obj_insert_take_dart(void* dst, char const* key, void* val) {
    return dart_obj_insert_take_dart_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_obj_insert_take_dart_len(void* dst, char const* key, size_t len, void* val) {
    return dart_obj_insert_take_dart_len_impl(dst, key, len, val);
  }

  dart_err_t dart_obj_insert_str(void* dst, char const* key, char const* val) {
    return dart_obj_insert_str_len(dst, key, strlen(key), val, strlen(val));
  }

  dart_err_t dart_obj_insert_str_len(void* dst, char const* key, size_t len, char const* val, size_t val_len) {
    return dart_obj_insert_str_len_impl(dst, key, len, val, val_len);
  }

  dart_err_t dart_obj_insert_int(void* dst, char const* key, int64_t val) {
    return dart_obj_insert_int_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_obj_insert_int_len(void* dst, char const* key, size_t len, int64_t val) {
    return dart_obj_insert_int_len_impl(dst, key, len, val);
  }

  dart_err_t dart_obj_insert_dcm(void* dst, char const* key, double val) {
    return dart_obj_insert_dcm_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_obj_insert_dcm_len(void* dst, char const* key, size_t len, double val) {
    return dart_obj_insert_dcm_len_impl(dst, key, len, val);
  }

  dart_err_t dart_obj_insert_bool(void* dst, char const* key, int val) {
    return dart_obj_insert_bool_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_obj_insert_bool_len(void* dst, char const* key, size_t len, int val) {
    return dart_obj_insert_bool_len_impl(dst, key, len, val);
  }

  dart_err_t dart_obj_insert_null(void* dst, char const* key) {
    return dart_obj_insert_null_len(dst, key, strlen(key));
  }

  dart_err_t dart_obj_insert_null_len(void* dst, char const* key, size_t len) {
    return dart_obj_insert_null_len_impl(dst, key, len);
  }

  dart_err_t dart_obj_set_dart(void* dst, char const* key, void const* val) {
    return dart_obj_set_dart_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_obj_set_dart_len(void* dst, char const* key, size_t len, void const* val) {
    return dart_obj_set_dart_len_impl(dst, key, len, val);
  }

  dart_err_t dart_obj_set_take_dart(void* dst, char const* key, void* val) {
    return dart_obj_set_take_dart_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_obj_set_take_dart_len(void* dst, char const* key, size_t len, void* val) {
    return dart_obj_set_take_dart_len_impl(dst, key, len, val);
  }

  dart_err_t dart_obj_set_str(void* dst, char const* key, char const* val) {
    return dart_obj_set_str_len(dst, key, strlen(key), val, strlen(val));
  }

  dart_err_t dart_obj_set_str_len(void* dst, char const* key, size_t len, char const* val, size_t val_len) {
    return dart_obj_set_str_len_impl(dst, key, len, val, val_len);
  }

  dart_err_t dart_obj_set_int(void* dst, char const* key, int64_t val) {
    return dart_obj_set_int_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_obj_set_int_len(void* dst, char const* key, size_t len, int64_t val) {
    return dart_obj_set_int_len_impl(dst, key, len, val);
  }

  dart_err_t dart_obj_set_dcm(void* dst, char const* key, double val) {
    return dart_obj_set_dcm_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_obj_set_dcm_len(void* dst, char const* key, size_t len, double val) {
    return dart_obj_set_dcm_len_impl(dst, key, len, val);
  }

  dart_err_t dart_obj_set_bool(void* dst, char const* key, int val) {
    return dart_obj_set_bool_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_obj_set_bool_len(void* dst, char const* key, size_t len, int val) {
    return dart_obj_set_bool_len_impl(dst, key, len, val);
  }

  dart_err_t dart_obj_set_null(void* dst, char const* key) {
    return dart_obj_set_null_len(dst, key, strlen(key));
  }

  dart_err_t dart_obj_set_null_len(void* dst, char const* key, size_t len) {
    return dart_obj_set_null_len_impl(dst, key, len);
  }

  dart_err_t dart_obj_clear(void* dst) {
    return dart_obj_clear_impl(dst);
  }

  dart_err_t dart_obj_erase(void* dst, char const* key) {
    return dart_obj_erase_len(dst, key, strlen(key));
  }

  dart_err_t dart_obj_erase_len(void* dst, char const* key, size_t len) {
    return dart_obj_erase_len_impl(dst, key, len);
  }

  dart_err_t dart_arr_insert_dart(void* dst, size_t idx, void const* val) {
    return dart_arr_insert_dart_impl(dst, idx, val);
  }

  dart_err_t dart_arr_insert_take_dart(void* dst, size_t idx, void* val) {
    return dart_arr_insert_take_dart_impl(dst, idx, val);
  }

  dart_err_t dart_arr_insert_str(void* dst, size_t idx, char const* val) {
    return dart_arr_insert_str_len(dst, idx, val, strlen(val));
  }

  dart_err_t dart_arr_insert_str_len(void* dst, size_t idx, char const* val, size_t val_len) {
    return dart_arr_insert_str_len_impl(dst, idx, val, val_len);
  }
  
  dart_err_t dart_arr_insert_int(void* dst, size_t idx, int64_t val) {
    return dart_arr_insert_int_impl(dst, idx, val);
  }

  dart_err_t dart_arr_insert_dcm(void* dst, size_t idx, double val) {
    return dart_arr_insert_dcm_impl(dst, idx, val);
  }

  dart_err_t dart_arr_insert_bool(void* dst, size_t idx, int val) {
    return dart_arr_insert_bool_impl(dst, idx, val);
  }

  dart_err_t dart_arr_insert_null(void* dst, size_t idx) {
    return dart_arr_insert_null_impl(dst, idx);
  }

  dart_err_t dart_arr_set_dart(void* dst, size_t idx, void const* val) {
    return dart_arr_set_dart_impl(dst, idx, val);
  }

  dart_err_t dart_arr_set_take_dart(void* dst, size_t idx, void* val) {
    return dart_arr_set_take_dart_impl(dst, idx, val);
  }

  dart_err_t dart_arr_set_str(void* dst, size_t idx, char const* val) {
    return dart_arr_set_str_len(dst, idx, val, strlen(val));
  }

  dart_err_t dart_arr_set_str_len(void* dst, size_t idx, char const* val, size_t val_len) {
    return dart_arr_set_str_len_impl(dst, idx, val, val_len);
  }
  
  dart_err_t dart_arr_set_int(void* dst, size_t idx, int64_t val) {
    return dart_arr_set_int_impl(dst, idx, val);
  }

  dart_err_t dart_arr_set_dcm(void* dst, size_t idx, double val) {
    return dart_arr_set_dcm_impl(dst, idx, val);
  }

  dart_err_t dart_arr_set_bool(void* dst, size_t idx, int val) {
    return dart_arr_set_bool_impl(dst, idx, val);
  }

  dart_err_t dart_arr_set_null(void* dst, size_t idx) {
    return dart_arr_set_null_impl(dst, idx);
  }

  dart_err_t dart_arr_clear(void* dst) {
    return dart_arr_clear_impl(dst);
  }

  dart_err_t dart_arr_erase(void* dst, size_t idx) {
    return dart_arr_erase_impl(dst, idx);
  }

  dart_err_t dart_arr_resize(void* dst, size_t len) {
    return dart_arr_resize_impl(dst, len);
  }

  dart_err_t dart_arr_reserve(void* dst, size_t len) {
    return dart_arr_reserve_impl(dst, len);
  }

  int dart_obj_has_key(void const* src, char const* key) {
    return dart_obj_has_key_len(src, key, strlen(key));
  }

  int dart_obj_has_key_len(void const* src, char const* key, size_t len) {
    return dart_obj_has_key_len_impl(src, key, len);
  }

  dart_packet_t dart_obj_get(void const* src, char const* key) {
    dart_packet_t dst;
    auto err = dart_obj_get_err(&dst, src, key);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_obj_get_err(dart_packet_t* dst, void const* src, char const* key) {
    return dart_obj_get_len_err(dst, src, key, strlen(key));
  }

  dart_packet_t dart_obj_get_len(void const* src, char const* key, size_t len) {
    dart_packet_t dst;
    auto err = dart_obj_get_len_err(&dst, src, key, len);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_obj_get_len_err(dart_packet_t* dst, void const* src, char const* key, size_t len) {
    return dart_obj_get_len_err_impl(dst, src, key, len);
  }

  dart_packet_t dart_arr_get(void const* src, size_t idx) {
    dart_packet_t dst;
    auto err = dart_arr_get_err(&dst, src, idx);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_arr_get_err(dart_packet_t* dst, void const* src, size_t idx) {
    return dart_arr_get_err_impl(dst, src, idx);
  }

  char const* dart_str_get(void const* src) {
    size_t dummy;
    return dart_str_get_len(src, &dummy);
  }

  char const* dart_str_get_len(void const* src, size_t* len) {
    return dart_str_get_len_impl(src, len);
  }

  int64_t dart_int_get(void const* src) {
    // No way unique way to signal failure here,
    // the user needs to know this will succeed.
    int64_t val = 0;
    dart_int_get_err(src, &val);
    return val;
  }

  dart_err_t dart_int_get_err(void const* src, int64_t* val) {
    return dart_int_get_err_impl(src, val);
  }

  double dart_dcm_get(void const* src) {
    double val = std::numeric_limits<double>::quiet_NaN();
    dart_dcm_get_err(src, &val);
    return val;
  }

  dart_err_t dart_dcm_get_err(void const* src, double* val) {
    return dart_dcm_get_err_impl(src, val);
  }

  int dart_bool_get(void const* src) {
    // No way unique way to signal failure here,
    // the user needs to know this will succeed.
    int val = 0;
    dart_bool_get_err(src, &val);
    return val;
  }

  dart_err_t dart_bool_get_err(void const* src, int* val) {
    return dart_bool_get_err_impl(src, val);
  }

  size_t dart_size(void const* src) {
    return dart_size_impl(src);
  }

  int dart_equal(void const* lhs, void const* rhs) {
    return dart_equal_impl(lhs, rhs);
  }

  int dart_is_obj(void const* src) {
    return dart_get_type(src) == DART_OBJECT;
  }

  int dart_is_arr(void const* src) {
    return dart_get_type(src) == DART_ARRAY;
  }

  int dart_is_str(void const* src) {
    return dart_get_type(src) == DART_STRING;
  }

  int dart_is_int(void const* src) {
    return dart_get_type(src) == DART_INTEGER;
  }

  int dart_is_dcm(void const* src) {
    return dart_get_type(src) == DART_DECIMAL;
  }

  int dart_is_bool(void const* src) {
    return dart_get_type(src) == DART_BOOLEAN;
  }

  int dart_is_null(void const* src) {
    return dart_get_type(src) == DART_NULL;
  }

  int dart_is_finalized(void const* src) {
    return dart_is_finalized_impl(src);
  }

  dart_type_t dart_get_type(void const* src) {
    return dart_get_type_impl(src);
  }

  size_t dart_refcount(void const* src) {
    return dart_refcount_impl(src);
  }

  dart_packet_t dart_from_json(char const* str) {
    dart_packet_t dst;
    auto err = dart_from_json_err(&dst, str);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_from_json_err(dart_packet_t* dst, char const* str) {
    return dart_from_json_len_rc_err(dst, DART_RC_SAFE, str, strlen(str));
  }

  dart_packet_t dart_from_json_rc(dart_rc_type_t rc, char const* str) {
    dart_packet_t dst;
    auto err = dart_from_json_rc_err(&dst, rc, str);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_from_json_rc_err(dart_packet_t* dst, dart_rc_type_t rc, char const* str) {
    return dart_from_json_len_rc_err(dst, rc, str, strlen(str));
  }

  dart_packet_t dart_from_json_len(char const* str, size_t len) {
    dart_packet_t dst;
    auto err = dart_from_json_len_err(&dst, str, len);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_from_json_len_err(dart_packet_t* dst, char const* str, size_t len) {
    return dart_from_json_len_rc_err(dst, DART_RC_SAFE, str, len);
  }

  dart_packet_t dart_from_json_len_rc(dart_rc_type_t rc, char const* str, size_t len) {
    dart_packet_t dst;
    auto err = dart_from_json_len_rc_err(&dst, rc, str, len);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_from_json_len_rc_err(dart_packet_t* dst, dart_rc_type_t rc, char const* str, size_t len) {
    return packet_typed_constructor_access(
      compose(
        [=] (dart::packet& dst) {
          dst = dart::packet::from_json({str, len});
        },
        [=] (dart::unsafe_packet& dst) {
          dst = dart::unsafe_packet::from_json({str, len});
        }
      ),
      dst,
      rc
    );
  }

  char* dart_to_json(void const* src, size_t* len) {
    return dart_to_json_impl(src, len);
  }

  dart_heap_t dart_to_heap(void const* src) {
    dart_heap_t dst;
    auto err = dart_to_heap_err(&dst, src);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_to_heap_err(dart_heap_t* dst, void const* src) {
    return dart_to_heap_err_impl(dst, src);
  }

  dart_buffer_t dart_to_buffer(void const* src) {
    dart_buffer_t dst;
    auto err = dart_to_buffer_err(&dst, src);
    if (err) return dart_buffer_init();
    else return dst;
  }

  dart_err_t dart_to_buffer_err(dart_buffer_t* dst, void const* src) {
    return dart_to_buffer_err_impl(dst, src);
  }

  dart_packet_t dart_lower(void const* src) {
    dart_packet_t dst;
    auto err = dart_lower_err(&dst, src);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_lower_err(dart_packet_t* dst, void const* src) {
    return dart_lower_err_impl(dst, src);
  }

  dart_packet_t dart_lift(void const* src) {
    dart_packet_t dst;
    auto err = dart_lift_err(&dst, src);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_lift_err(dart_packet_t* dst, void const* src) {
    return dart_lift_err_impl(dst, src);
  }

  dart_packet_t dart_finalize(void const* src) {
    return dart_lower(src);
  }

  dart_err_t dart_finalize_err(dart_packet_t* dst, void const* src) {
    return dart_lower_err(dst, src);
  }

  dart_packet_t dart_definalize(void const* src) {
    return dart_lift(src);
  }

  dart_err_t dart_definalize_err(dart_packet_t* dst, void const* src) {
    return dart_lift_err(dst, src);
  }

  void const* dart_get_bytes(void const* src, size_t* len) {
    return dart_get_bytes_impl(src, len);
  }

  void* dart_dup_bytes(void const* src, size_t* len) {
    return dart_dup_bytes_impl(src, len);
  }

  dart_packet_t dart_from_bytes(void const* bytes, size_t len) {
    dart_packet_t dst;
    auto err = dart_from_bytes_err(&dst, bytes, len);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_from_bytes_err(dart_packet_t* dst, void const* bytes, size_t len) {
    return dart_from_bytes_rc_err(dst, DART_RC_SAFE, bytes, len);
  }

  dart_packet_t dart_from_bytes_rc(void const* bytes, dart_rc_type_t rc, size_t len) {
    dart_packet_t dst;
    auto err = dart_from_bytes_rc_err(&dst, rc, bytes, len);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_from_bytes_rc_err(dart_packet_t* dst, dart_rc_type_t rc, void const* bytes, size_t len) {
    return dart_from_bytes_rc_err_impl(dst, rc, bytes, len);
  }

  dart_packet_t dart_take_bytes(void* bytes) {
    dart_packet_t dst;
    auto err = dart_take_bytes_err(&dst, bytes);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_take_bytes_err(dart_packet_t* dst, void* bytes) {
    return dart_take_bytes_rc_err(dst, DART_RC_SAFE, bytes);
  }

  dart_packet_t dart_take_bytes_rc(void* bytes, dart_rc_type_t rc) {
    dart_packet_t dst;
    auto err = dart_take_bytes_rc_err(&dst, rc, bytes);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_take_bytes_rc_err(dart_packet_t* dst, dart_rc_type_t rc, void* bytes) {
    return dart_take_bytes_rc_err_impl(dst, rc, bytes);
  }

  int dart_buffer_is_valid(void const* bytes, size_t len) {
    return dart::is_valid(reinterpret_cast<gsl::byte const*>(bytes), len);
  }

  dart_err_t dart_iterator_init_err(dart_iterator_t* dst) {
    // Initialize.
    dst->rtti.p_id = DART_PACKET;
    dst->rtti.rc_id = DART_RC_SAFE;
    return iterator_construct([] (dart::packet::iterator* begin, dart::packet::iterator* end) {
      new(begin) dart::packet::iterator();
      new(end) dart::packet::iterator();
    }, dst);
  }

  dart_err_t dart_iterator_init_from_err(dart_iterator_t* dst, void const* src) {
    return dart_iterator_init_from_err_impl(dst, src);
  }

  dart_err_t dart_iterator_init_key_from_err(dart_iterator_t* dst, void const* src) {
    return dart_iterator_init_key_from_err_impl(dst, src);
  }

  dart_err_t dart_iterator_copy_err(dart_iterator_t* dst, dart_iterator_t const* src) {
    return dart_iterator_copy_err_impl(dst, src);
  }

  dart_err_t dart_iterator_move_err(dart_iterator_t* dst, dart_iterator_t* src) {
    return dart_iterator_move_err_impl(dst, src);
  }

  dart_err_t dart_iterator_destroy(dart_iterator_t* dst) {
    return dart_iterator_destroy_impl(dst);
  }

  dart_packet_t dart_iterator_get(dart_iterator_t const* src) {
    dart_packet_t dst;
    auto err = dart_iterator_get_err(&dst, src);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_iterator_get_err(dart_packet_t* dst, dart_iterator_t const* src) {
    return dart_iterator_get_err_impl(dst, src);
  }

  dart_err_t dart_iterator_next(dart_iterator_t* dst) {
    return dart_iterator_next_impl(dst);
  }

  int dart_iterator_done(dart_iterator_t const* src) {
    return dart_iterator_done_impl(src);
  }

  int dart_iterator_done_destroy(dart_iterator_t* dst, dart_packet_t* pkt) {
    if (!dart_iterator_done(dst)) return false;
    dart_iterator_destroy(dst);
    if (pkt) dart_destroy(pkt);
    return true;
  }

  char const* dart_get_error() {
    return dart::detail::errmsg.data();
  }

  void dart_aligned_free(void* ptr) {
    dart::shim::aligned_free(ptr);
  }

}
