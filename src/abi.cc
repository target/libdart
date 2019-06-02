/*----- System Includes -----*/

#include <stdarg.h>

/*----- Local Includes -----*/

#include "../include/dart.h"
#include "../include/dart/abi.h"

/*----- Build Sanity Checks -----*/

static_assert(sizeof(dart::heap) <= DART_HEAP_MAX_SIZE, "Dart ABI is misconfigured");
static_assert(sizeof(dart::buffer) <= DART_BUFFER_MAX_SIZE, "Dart ABI is misconfigured");
static_assert(sizeof(dart::packet) <= DART_PACKET_MAX_SIZE, "Dart ABI is misconfigured");

/*----- Macros -----*/

#define DART_RAW_TYPE(input) (input)->rtti.p_id
#define DART_RAW_BYTES(input) &((input)->bytes)

/*----- Globals -----*/

thread_local char const* errmsg = nullptr;

/*----- Private Types -----*/

namespace dart {
  using unsafe_heap = dart::basic_heap<dart::unsafe_ptr>;
  using unsafe_buffer = dart::basic_buffer<dart::unsafe_ptr>;
  using unsafe_packet = dart::basic_packet<dart::unsafe_ptr>;
}

template <class Func, class... Args>
struct is_callable_impl {

  private:

    struct nonesuch {};

  public:

    template <class F, class... A>
    static decltype(std::declval<F>()(std::declval<A>()...)) detect(int);
    template <class F, class... A>
    static nonesuch detect(...);

    using type = decltype(detect<Func, Args...>(0));
    using result_type = std::integral_constant<bool, !std::is_same<type, nonesuch>::value>;

};

template <class Func, class... Args>
struct is_callable : is_callable_impl<Func, Args...>::result_type {
  using type = typename is_callable_impl<Func, Args...>::type;
};
template <class Func, class... Args>
using is_callable_t = typename is_callable<Func, Args...>::type;

template <class Func, class... Args>
struct returns_error :
  std::is_same<
    is_callable_t<Func, Args...>,
    dart_err_t
  >
{};

template <class Target, bool is_const>
using maybe_const_t = std::conditional_t<
  is_const,
  Target const,
  Target
>;

using string_view = dart::shim::string_view;

struct abi_error : std::logic_error {
  abi_error(char const* msg) : logic_error(msg) {}
};

enum class parse_type {
  object,                   // o
  array,                    // a
  string,                   // s
  sized_string,             // S
  integer,                  // i
  unsigned_integer,         // ui
  long_int,                 // l
  unsigned_long_int,        // ul
  decimal,                  // d
  boolean,                  // b
  null,                     // <whitespace>
  invalid
};

/*----- Private Helper Function Implementations -----*/

namespace {

  template <class... Funcs>
  auto compose(Funcs&&... fs) {
    return dart::shim::compose_together(std::forward<Funcs>(fs)...);
  }

  template <class Func, class... Args>
  dart_err_t call_indirection(std::true_type, Func&& cb, Args&&... the_args) {
    return std::forward<Func>(cb)(std::forward<Args>(the_args)...);
  }
  template <class Func, class... Args>
  dart_err_t call_indirection(std::false_type, Func&& cb, Args&&... the_args) {
    std::forward<Func>(cb)(std::forward<Args>(the_args)...);
    return DART_NO_ERROR;
  }

  template <class Func, class... Args>
  dart_err_t safe_call(Func&& cb, Args&&... the_args) {
    return compose(
      [] (std::true_type, auto&& c, auto&&... as) {
        return call_indirection(returns_error<Func, Args...> {},
            std::forward<decltype(c)>(c), std::forward<decltype(as)>(as)...);
      },
      [] (std::false_type, auto&&, auto&&) {
        errmsg = "Avoided a type-mismatched call of some sort. "
              "Are your rc types correct? Did you perform a bad cast?";
        return DART_CLIENT_ERROR;
      }
    )(is_callable<Func, Args...> {}, std::forward<Func>(cb), std::forward<Args>(the_args)...);
  }

  template <bool is_const, class Func>
  dart_err_t heap_unwrap_impl(Func&& cb, dart_heap_t* pkt) {
    switch (pkt->rtti.rc_id) {
      case DART_RC_SAFE:
        {
          auto* rt_ptr = reinterpret_cast<dart::heap*>(DART_RAW_BYTES(pkt));
          return safe_call(std::forward<Func>(cb),
              const_cast<maybe_const_t<dart::heap, is_const>&>(*rt_ptr));
        }
      case DART_RC_UNSAFE:
        {
          auto* rt_ptr = reinterpret_cast<dart::unsafe_heap*>(DART_RAW_BYTES(pkt));
          return safe_call(std::forward<Func>(cb),
              const_cast<maybe_const_t<dart::unsafe_heap, is_const>&>(*rt_ptr));
        }
      default:
        errmsg = "Unknown reference counter passed for dart_heap";
        return DART_CLIENT_ERROR;
    }
  }

  template <bool is_const, class Func>
  dart_err_t buffer_unwrap_impl(Func&& cb, dart_buffer_t* pkt) {
    switch (pkt->rtti.rc_id) {
      case DART_RC_SAFE:
        {
          auto* rt_ptr = reinterpret_cast<dart::buffer*>(DART_RAW_BYTES(pkt));
          return safe_call(std::forward<Func>(cb),
              const_cast<maybe_const_t<dart::buffer, is_const>&>(*rt_ptr));
        }
      case DART_RC_UNSAFE:
        {
          auto* rt_ptr = reinterpret_cast<dart::unsafe_buffer*>(DART_RAW_BYTES(pkt));
          return safe_call(std::forward<Func>(cb),
              const_cast<maybe_const_t<dart::unsafe_buffer, is_const>&>(*rt_ptr));
        }
      default:
        errmsg = "Unknown reference counter passed for dart_buffer";
        return DART_CLIENT_ERROR;
    }
  }

  template <bool is_const, class Func>
  dart_err_t packet_unwrap_impl(Func&& cb, dart_packet_t* pkt) {
    switch (pkt->rtti.rc_id) {
      case DART_RC_SAFE:
        {
          auto* rt_ptr = reinterpret_cast<dart::packet*>(DART_RAW_BYTES(pkt));
          return safe_call(std::forward<Func>(cb),
              const_cast<maybe_const_t<dart::packet, is_const>&>(*rt_ptr));
        }
      case DART_RC_UNSAFE:
        {
          auto* rt_ptr = reinterpret_cast<dart::unsafe_packet*>(DART_RAW_BYTES(pkt));
          return safe_call(std::forward<Func>(cb),
              const_cast<maybe_const_t<dart::unsafe_packet, is_const>&>(*rt_ptr));
        }
      default:
        errmsg = "Unknown reference counter passed for dart_packet";
        return DART_CLIENT_ERROR;
    }
  }

  template <bool is_const, class Func>
  dart_err_t generic_unwrap_impl(Func&& cb, void* pkt) {
    auto* rtti = reinterpret_cast<dart_type_id_t*>(pkt);
    switch (rtti->p_id) {
      case DART_HEAP:
        return heap_unwrap_impl<is_const>(std::forward<Func>(cb), reinterpret_cast<dart_heap_t*>(pkt));
      case DART_BUFFER:
        return buffer_unwrap_impl<is_const>(std::forward<Func>(cb), reinterpret_cast<dart_buffer_t*>(pkt));
      case DART_PACKET:
        return packet_unwrap_impl<is_const>(std::forward<Func>(cb), reinterpret_cast<dart_packet_t*>(pkt));
      default:
        errmsg = "Corrupted dart object encountered in generic function call.";
        return DART_CLIENT_ERROR;
    }
  }

  template <class Func>
  dart_err_t generic_unwrap(Func&& cb, void* pkt) {
    return generic_unwrap_impl<false>(std::forward<Func>(cb), pkt);
  }

  template <class Func>
  dart_err_t generic_unwrap(Func&& cb, void const* pkt) {
    return generic_unwrap_impl<true>(std::forward<Func>(cb), const_cast<void*>(pkt));
  }

  template <class Func>
  dart_err_t heap_unwrap(Func&& cb, dart_heap_t* pkt) {
    return heap_unwrap_impl<false>(std::forward<Func>(cb), pkt);
  }

  template <class Func>
  dart_err_t heap_unwrap(Func&& cb, dart_heap_t const* pkt) {
    return heap_unwrap_impl<true>(std::forward<Func>(cb), const_cast<dart_heap_t*>(pkt));
  }

  template <class Func>
  dart_err_t heap_construct(Func&& cb, dart_heap_t* pkt) {
    switch (pkt->rtti.rc_id) {
      case DART_RC_SAFE:
        {
          auto* rt_ptr = reinterpret_cast<dart::heap*>(DART_RAW_BYTES(pkt));
          auto ret = safe_call(std::forward<Func>(cb), rt_ptr);
          return ret ? ret : DART_NO_ERROR;
        }
      case DART_RC_UNSAFE:
        {
          auto* rt_ptr = reinterpret_cast<dart::unsafe_heap*>(DART_RAW_BYTES(pkt));
          auto ret = safe_call(std::forward<Func>(cb), rt_ptr);
          return ret ? ret : DART_NO_ERROR;
        }
      default:
        errmsg = "Unknown reference counter passed for dart_heap";
        return DART_CLIENT_ERROR;
    }
  }

  template <class Func>
  dart_err_t buffer_unwrap(Func&& cb, dart_buffer_t* pkt) {
    return buffer_unwrap_impl<false>(std::forward<Func>(cb), pkt);
  }

  template <class Func>
  dart_err_t buffer_unwrap(Func&& cb, dart_buffer_t const* pkt) {
    return buffer_unwrap_impl<true>(std::forward<Func>(cb), const_cast<dart_buffer_t*>(pkt));
  }

  template <class Func>
  dart_err_t buffer_construct(Func&& cb, dart_buffer_t* pkt) {
    switch (pkt->rtti.rc_id) {
      case DART_RC_SAFE:
        {
          auto* rt_ptr = reinterpret_cast<dart::buffer*>(DART_RAW_BYTES(pkt));
          auto ret = safe_call(std::forward<Func>(cb), rt_ptr);
          return ret ? ret : DART_NO_ERROR;
        }
      case DART_RC_UNSAFE:
        {
          auto* rt_ptr = reinterpret_cast<dart::unsafe_buffer*>(DART_RAW_BYTES(pkt));
          auto ret = safe_call(std::forward<Func>(cb), rt_ptr);
          return ret ? ret : DART_NO_ERROR;
        }
      default:
        errmsg = "Unknown reference counter passed for dart_buffer";
        return DART_CLIENT_ERROR;
    }
  }

  template <class Func>
  dart_err_t packet_unwrap(Func&& cb, dart_packet_t* pkt) {
    return packet_unwrap_impl<false>(std::forward<Func>(cb), pkt);
  }

  template <class Func>
  dart_err_t packet_unwrap(Func&& cb, dart_packet_t const* pkt) {
    return packet_unwrap_impl<true>(std::forward<Func>(cb), const_cast<dart_packet_t*>(pkt));
  }

  template <class Func>
  dart_err_t packet_construct(Func&& cb, dart_packet_t* pkt) {
    switch (pkt->rtti.rc_id) {
      case DART_RC_SAFE:
        {
          auto* rt_ptr = reinterpret_cast<dart::packet*>(DART_RAW_BYTES(pkt));
          auto ret = safe_call(std::forward<Func>(cb), rt_ptr);
          return ret ? ret : DART_NO_ERROR;
        }
      case DART_RC_UNSAFE:
        {
          auto* rt_ptr = reinterpret_cast<dart::unsafe_packet*>(DART_RAW_BYTES(pkt));
          auto ret = safe_call(std::forward<Func>(cb), rt_ptr);
          return ret ? ret : DART_NO_ERROR;
        }
      default:
        errmsg = "Unknown reference counter passed for dart_packet";
        return DART_CLIENT_ERROR;
    }
  }

  template <class Func>
  dart_err_t err_handler(Func&& cb) noexcept try {
    return std::forward<Func>(cb)();
  } catch (dart::type_error const& err) {
    errmsg = err.what();
    return DART_TYPE_ERROR;
  } catch (dart::state_error const& err) {
    errmsg = err.what();
    return DART_STATE_ERROR;
  } catch (dart::parse_error const& err) {
    errmsg = err.what();
    return DART_PARSE_ERROR;
  } catch (std::logic_error const& err) {
    errmsg = err.what();
    return DART_LOGIC_ERROR;
  } catch (std::runtime_error const& err) {
    errmsg = err.what();
    return DART_RUNTIME_ERROR;
  } catch (...) {
    errmsg = "Dart caught an unexpected error type. This is a bug, please make a report";
    return DART_UNKNOWN_ERROR;
  }

  template <class Func, class Ptr>
  dart_err_t generic_access(Func&& cb, Ptr* pkt) noexcept {
    return err_handler([&cb, pkt] { return generic_unwrap(std::forward<Func>(cb), pkt); });
  }

  template <class Func, class Ptr>
  dart_err_t heap_access(Func&& cb, Ptr* pkt) noexcept {
    return err_handler([&cb, pkt] { return heap_unwrap(std::forward<Func>(cb), pkt); });
  }

  template <class Func>
  dart_err_t heap_constructor_access(Func&& cb, dart_heap_t* pkt) noexcept {
    return err_handler([&cb, pkt] { return heap_construct(std::forward<Func>(cb), pkt); });
  }

  template <class Func>
  dart_err_t heap_typed_constructor_access(Func&& cb, dart_heap_t* pkt, dart_rc_type_t rc) noexcept {
    // Default construct our heap.
    auto ret = dart_heap_init_rc_err(pkt, rc);
    if (ret) return ret;

    // Assign to it.
    return err_handler([&cb, pkt] { return heap_unwrap(std::forward<Func>(cb), pkt); });
  }

  template <class Func>
  dart_err_t packet_typed_constructor_access(Func&& cb, dart_packet_t* pkt, dart_rc_type_t rc) noexcept {
    // Default construct our packet.
    auto ret = dart_packet_init_rc(pkt, rc);
    if (ret) return ret;

    // Assign to it.
    return err_handler([&cb, pkt] { return packet_unwrap(std::forward<Func>(cb), pkt); });
  }

  template <class Func, class Ptr>
  dart_err_t buffer_access(Func&& cb, Ptr* pkt) noexcept {
    return err_handler([&cb, pkt] { return buffer_unwrap(std::forward<Func>(cb), pkt); });
  }

  template <class Func>
  dart_err_t buffer_constructor_access(Func&& cb, dart_buffer_t* pkt) noexcept {
    return err_handler([&cb, pkt] { return buffer_construct(std::forward<Func>(cb), pkt); });
  }

  template <class Func, class Ptr>
  dart_err_t packet_access(Func&& cb, Ptr* pkt) noexcept {
    return err_handler([&cb, pkt] { return packet_unwrap(std::forward<Func>(cb), pkt); });
  }

  template <class Func>
  dart_err_t packet_constructor_access(Func&& cb, dart_packet_t* pkt) noexcept {
    return err_handler([&cb, pkt] { return packet_construct(std::forward<Func>(cb), pkt); });
  }

  // Forgive me
  parse_type identify_vararg(char const*& c) {
    switch (*c++) {
      case 'o':
        return parse_type::object;
      case 'a':
        return parse_type::array;
      case 's':
        return parse_type::string;
      case 'S':
        return parse_type::sized_string;
      case 'u':
        switch (*c++) {
          case 'i':
            return parse_type::unsigned_integer;
          case 'l':
            return parse_type::unsigned_long_int;
          default:
            return parse_type::invalid;
        }
      case 'i':
        return parse_type::integer;
      case 'l':
        return parse_type::long_int;
      case 'd':
        return parse_type::decimal;
      case 'b':
        return parse_type::boolean;
      case ' ':
        return parse_type::null;
      default:
        return parse_type::invalid;
    }
  }

  dart_type_t abi_type(dart::detail::type type) {
    switch (type) {
      case dart::detail::type::object:
        return DART_OBJECT;
      case dart::detail::type::array:
        return DART_ARRAY;
      case dart::detail::type::string:
        return DART_STRING;
      case dart::detail::type::integer:
        return DART_INTEGER;
      case dart::detail::type::decimal:
        return DART_DECIMAL;
      case dart::detail::type::boolean:
        return DART_BOOLEAN;
      case dart::detail::type::null:
        return DART_NULL;
      default:
        return DART_INVALID;
    }
  }

  template <class Packet, class VaList>
  void parse_vals(Packet& pkt, char const*& format, VaList&& args);

  template <class Packet, class VaList>
  void parse_pairs(Packet& pkt, char const*& format, VaList&& args);

  template <class Packet, class VaList>
  Packet parse_val(char const*& format, VaList&& args) {
    switch (identify_vararg(format)) {
      case parse_type::object:
        {
          auto obj = Packet::make_object();
          parse_pairs(obj, format, args);
          return obj;
        }
      case parse_type::array:
        {
          auto arr = Packet::make_array();
          parse_vals(arr, format, args);
          return arr;
        }
      case parse_type::string:
        return Packet::make_string(va_arg(args, char const*));
      case parse_type::sized_string:
        {
          auto* str = va_arg(args, char const*);
          auto len = va_arg(args, size_t);
          return Packet::make_string({str, len});
        }
      case parse_type::integer:
        return Packet::make_integer(va_arg(args, int));
      case parse_type::unsigned_integer:
        return Packet::make_integer(va_arg(args, unsigned int));
      case parse_type::long_int:
        return Packet::make_integer(va_arg(args, long long));
      case parse_type::unsigned_long_int:
        return Packet::make_integer(va_arg(args, unsigned long long));
      case parse_type::decimal:
        return Packet::make_decimal(va_arg(args, double));
      case parse_type::boolean:
        return Packet::make_boolean(va_arg(args, int));
      case parse_type::null:
        return Packet::make_null();
      default:
        throw abi_error("invalid varargs character");
    }
  }

  template <class Packet, class VaList>
  void parse_vals(Packet& pkt, char const*& format, VaList&& args) {
    while (*format && *format != ',') {
      pkt.push_back(parse_val<Packet>(format, args));
    }
  }

  template <class Packet, class VaList>
  void parse_pairs(Packet& pkt, char const*& format, VaList&& args) {
    while (*format && *format != ',') {
      // va_list is assumed to consist of key value pairs.
      // These two MUST exist as separate lines, as argument
      // parameter evaluation order is implementation defined.
      // Fun way to corrupt the stack.
      auto key = Packet::make_string(va_arg(args, char const*));
      pkt.add_field(std::move(key), parse_val<Packet>(format, args));
    }
  }

}

/*----- Function Implementations -----*/

extern "C" {

  /*----- Dart Heap Functions -----*/

  dart_heap_t dart_heap_init() {
    // Cannot meaningfully fail.
    dart_heap_t pkt;
    dart_heap_init_rc_err(&pkt, DART_RC_SAFE);
    return pkt;
  }

  dart_err_t dart_heap_init_err(dart_heap_t* pkt) {
    return dart_heap_init_rc_err(pkt, DART_RC_SAFE);
  }

  dart_heap_t dart_heap_init_rc(dart_rc_type_t rc) {
    // Cannot meaningfully fail.
    dart_heap_t pkt;
    dart_heap_init_rc_err(&pkt, rc);
    return pkt;
  }

  dart_err_t dart_heap_init_rc_err(dart_heap_t* pkt, dart_rc_type_t rc) {
    // Initialize.
    pkt->rtti = {DART_HEAP, rc};
    return heap_constructor_access(
      compose(
        [] (dart::heap* ptr) { new(ptr) dart::heap(); },
        [] (dart::unsafe_heap* ptr) { new(ptr) dart::unsafe_heap(); }
      ),
      pkt
    );
  }

  dart_heap_t dart_heap_copy(dart_heap_t const* src) {
    dart_heap_t pkt;
    auto err = dart_heap_copy_err(&pkt, src);
    if (err) return dart_heap_init();
    else return pkt;
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
    dart_heap_t pkt;
    auto err = dart_heap_move_err(&pkt, src);
    if (err) return dart_heap_init();
    else return pkt;
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

  dart_err_t dart_heap_destroy(dart_heap_t* pkt) {
    // Destroy.
    return heap_access(
      compose(
        [] (dart::heap& pkt) { pkt.~basic_heap(); },
        [] (dart::unsafe_heap& pkt) { pkt.~basic_heap(); }
      ),
      pkt
    );
  }

  dart_heap_t dart_heap_obj_init() {
    dart_heap_t pkt;
    auto err = dart_heap_obj_init_err(&pkt);
    if (err) return dart_heap_init();
    else return pkt;
  }

  dart_err_t dart_heap_obj_init_err(dart_heap_t* pkt) {
    return dart_heap_obj_init_rc_err(pkt, DART_RC_SAFE);
  }

  dart_heap_t dart_heap_obj_init_rc(dart_rc_type_t rc) {
    dart_heap_t pkt;
    auto err = dart_heap_obj_init_rc_err(&pkt, rc);
    if (err) return dart_heap_init();
    else return pkt;
  }

  dart_err_t dart_heap_obj_init_rc_err(dart_heap_t* pkt, dart_rc_type_t rc) {
    // Default initialize, then assign.
    return heap_typed_constructor_access(
      compose(
        [] (dart::heap& pkt) { pkt = dart::heap::make_object(); },
        [] (dart::unsafe_heap& pkt) { pkt = dart::unsafe_heap::make_object(); }
      ),
      pkt,
      rc
    );
  }

  static dart_err_t dart_heap_obj_init_va_impl(dart_heap_t* pkt, dart_rc_type_t rc, char const* format, va_list args) {
    return heap_typed_constructor_access(
      compose(
        [format, args] (dart::heap& pkt) mutable {
          pkt = dart::heap::make_object();
          parse_pairs(pkt, format, args);
        },
        [format, args] (dart::unsafe_heap& pkt) mutable {
          pkt = dart::unsafe_heap::make_object();
          parse_pairs(pkt, format, args);
        }
      ),
      pkt,
      rc
    );
  }

  dart_heap_t dart_heap_obj_init_va(char const* format, ...) {
    va_list args;
    dart_heap_t pkt;
    va_start(args, format);
    auto ret = dart_heap_obj_init_va_impl(&pkt, DART_RC_SAFE, format, args);
    va_end(args);
    if (ret) return dart_heap_init();
    else return pkt;
  }

  dart_err_t dart_heap_obj_init_va_err(dart_heap_t* pkt, char const* format, ...) {
    va_list args;
    va_start(args, format);
    auto ret = dart_heap_obj_init_va_impl(pkt, DART_RC_SAFE, format, args);
    va_end(args);
    return ret;
  }

  dart_heap_t dart_heap_obj_init_va_rc(dart_rc_type_t rc, char const* format, ...) {
    va_list args;
    dart_heap_t pkt;
    va_start(args, format);
    auto ret = dart_heap_obj_init_va_impl(&pkt, rc, format, args);
    va_end(args);
    if (ret) return dart_heap_init();
    else return pkt;
  }

  dart_err_t dart_heap_obj_init_va_rc_err(dart_heap_t* pkt, dart_rc_type_t rc, char const* format, ...) {
    va_list args;
    va_start(args, format);
    auto ret = dart_heap_obj_init_va_impl(pkt, rc, format, args);
    va_end(args);
    return ret;
  }

  dart_heap_t dart_heap_arr_init() {
    dart_heap_t pkt;
    auto err = dart_heap_arr_init_err(&pkt);
    if (err) return dart_heap_init();
    else return pkt;
  }

  dart_err_t dart_heap_arr_init_err(dart_heap_t* pkt) {
    return dart_heap_arr_init_rc_err(pkt, DART_RC_SAFE);
  }

  dart_heap_t dart_heap_arr_init_rc(dart_rc_type_t rc) {
    dart_heap_t pkt;
    auto err = dart_heap_arr_init_rc_err(&pkt, rc);
    if (err) return dart_heap_init();
    else return pkt;
  }

  dart_err_t dart_heap_arr_init_rc_err(dart_heap_t* pkt, dart_rc_type_t rc) {
    // Default initialize, then assign.
    return heap_typed_constructor_access(
      compose(
        [] (dart::heap& pkt) { pkt = dart::heap::make_array(); },
        [] (dart::unsafe_heap& pkt) { pkt = dart::unsafe_heap::make_array(); }
      ),
      pkt,
      rc
    );
  }

  static dart_err_t dart_heap_arr_init_va_impl(dart_heap_t* pkt, dart_rc_type_t rc, char const* format, va_list args) {
    return heap_typed_constructor_access(
      compose(
        [format, args] (dart::heap& pkt) mutable {
          pkt = dart::heap::make_array();
          parse_vals(pkt, format, args);
        },
        [format, args] (dart::unsafe_heap& pkt) mutable {
          pkt = dart::unsafe_heap::make_array();
          parse_vals(pkt, format, args);
        }
      ),
      pkt,
      rc
    );
  }

  dart_heap_t dart_heap_arr_init_va(char const* format, ...) {
    va_list args;
    dart_heap_t pkt;
    va_start(args, format);
    auto ret = dart_heap_arr_init_va_impl(&pkt, DART_RC_SAFE, format, args);
    if (ret) return dart_heap_init();
    else return pkt;
  }

  dart_err_t dart_heap_arr_init_va_err(dart_heap_t* pkt, char const* format, ...) {
    va_list args;
    va_start(args, format);
    auto ret = dart_heap_arr_init_va_impl(pkt, DART_RC_SAFE, format, args);
    va_end(args);
    return ret;
  }

  dart_heap_t dart_heap_arr_init_va_rc(dart_rc_type_t rc, char const* format, ...) {
    va_list args;
    dart_heap_t pkt;
    va_start(args, format);
    auto ret = dart_heap_arr_init_va_impl(&pkt, rc, format, args);
    if (ret) return dart_heap_init();
    else return pkt;
  }

  dart_err_t dart_heap_arr_init_va_rc_err(dart_heap_t* pkt, dart_rc_type_t rc, char const* format, ...) {
    va_list args;
    va_start(args, format);
    auto ret = dart_heap_arr_init_va_impl(pkt, rc, format, args);
    va_end(args);
    return ret;
  }

  dart_heap_t dart_heap_str_init(char const* str) {
    dart_heap_t pkt;
    auto err = dart_heap_str_init_err(&pkt, str);
    if (err) return dart_heap_init();
    else return pkt;
  }

  dart_err_t dart_heap_str_init_err(dart_heap_t* pkt, char const* str) {
    return dart_heap_str_init_rc_len_err(pkt, DART_RC_SAFE, str, strlen(str));
  }

  dart_heap_t dart_heap_str_init_len(char const* str, size_t len) {
    dart_heap_t pkt;
    auto err = dart_heap_str_init_len_err(&pkt, str, len);
    if (err) return dart_heap_init();
    else return pkt;
  }

  dart_err_t dart_heap_str_init_err_len(dart_heap_t* pkt, char const* str, size_t len) {
    return dart_heap_str_init_rc_len_err(pkt, DART_RC_SAFE, str, len);
  }

  dart_heap_t dart_heap_str_init_rc(dart_rc_type_t rc, char const* str) {
    dart_heap_t pkt;
    auto err = dart_heap_str_init_rc_err(&pkt, rc, str);
    if (err) return dart_heap_init();
    else return pkt;
  }

  dart_err_t dart_heap_str_init_rc_err(dart_heap_t* pkt, dart_rc_type_t rc, char const* str) {
    return dart_heap_str_init_rc_len_err(pkt, rc, str, strlen(str));
  }

  dart_heap_t dart_heap_str_init_rc_len(dart_rc_type_t rc, char const* str, size_t len) {
    dart_heap_t pkt;
    auto err = dart_heap_str_init_rc_len_err(&pkt, rc, str, len);
    if (err) return dart_heap_init();
    else return pkt;
  }

  dart_err_t dart_heap_str_init_rc_len_err(dart_heap_t* pkt, dart_rc_type_t rc, char const* str, size_t len) {
    // Default initialize, then assign
    return heap_typed_constructor_access(
      compose(
        [str, len] (dart::heap& pkt) { pkt = dart::heap::make_string({str, len}); },
        [str, len] (dart::unsafe_heap& pkt) { pkt = dart::unsafe_heap::make_string({str, len}); }
      ),
      pkt,
      rc
    );
  }

  dart_heap_t dart_heap_int_init(int64_t val) {
    dart_heap_t pkt;
    auto err = dart_heap_int_init_err(&pkt, val);
    if (err) return dart_heap_init();
    else return pkt;
  }

  dart_err_t dart_heap_int_init_err(dart_heap_t* pkt, int64_t val) {
    return dart_heap_int_init_rc_err(pkt, DART_RC_SAFE, val);
  }

  dart_heap_t dart_heap_int_init_rc(dart_rc_type_t rc, int64_t val) {
    dart_heap_t pkt;
    auto err = dart_heap_int_init_rc_err(&pkt, rc, val);
    if (err) return dart_heap_init();
    else return pkt;
  }

  dart_err_t dart_heap_int_init_rc_err(dart_heap_t* pkt, dart_rc_type_t rc, int64_t val) {
    // Default initialize, then assign
    return heap_typed_constructor_access(
      compose(
        [val] (dart::heap& pkt) { pkt = dart::heap::make_integer(val); },
        [val] (dart::unsafe_heap& pkt) { pkt = dart::unsafe_heap::make_integer(val); }
      ),
      pkt,
      rc
    );
  }

  dart_heap_t dart_heap_dcm_init(double val) {
    dart_heap_t pkt;
    auto err = dart_heap_dcm_init_err(&pkt, val);
    if (err) return dart_heap_init();
    else return pkt;
  }

  dart_err_t dart_heap_dcm_init_err(dart_heap_t* pkt, double val) {
    return dart_heap_dcm_init_rc_err(pkt, DART_RC_SAFE, val);
  }

  dart_heap_t dart_heap_dcm_init_rc(dart_rc_type_t rc, double val) {
    dart_heap_t pkt;
    auto err = dart_heap_dcm_init_rc_err(&pkt, rc, val);
    if (err) return dart_heap_init();
    else return pkt;
  }

  dart_err_t dart_heap_dcm_init_rc_err(dart_heap_t* pkt, dart_rc_type_t rc, double val) {
    // Default initialize, then assign
    return heap_typed_constructor_access(
      compose(
        [val] (dart::heap& pkt) { pkt = dart::heap::make_decimal(val); },
        [val] (dart::unsafe_heap& pkt) { pkt = dart::unsafe_heap::make_decimal(val); }
      ),
      pkt,
      rc
    );
  }

  dart_heap_t dart_heap_bool_init(int val) {
    dart_heap_t pkt;
    auto err = dart_heap_bool_init_err(&pkt, val);
    if (err) return dart_heap_init();
    else return pkt;
  }

  dart_err_t dart_heap_bool_init_err(dart_heap_t* pkt, int val) {
    return dart_heap_bool_init_rc_err(pkt, DART_RC_SAFE, val);
  }

  dart_heap_t dart_heap_bool_init_rc(dart_rc_type_t rc, int val) {
    dart_heap_t pkt;
    auto err = dart_heap_bool_init_rc_err(&pkt, rc, val);
    if (err) return dart_heap_init();
    else return pkt;
  }

  dart_err_t dart_heap_bool_init_rc_err(dart_heap_t* pkt, dart_rc_type_t rc, int val) {
    // Default initialize, then assign
    return heap_typed_constructor_access(
      compose(
        [val] (dart::heap& pkt) { pkt = dart::heap::make_boolean(val); },
        [val] (dart::unsafe_heap& pkt) { pkt = dart::unsafe_heap::make_boolean(val); }
      ),
      pkt,
      rc
    );
  }

  dart_heap_t dart_heap_null_init() {
    dart_heap_t pkt;
    auto err = dart_heap_null_init_err(&pkt);
    if (err) return dart_heap_init();
    else return pkt;
  }

  dart_err_t dart_heap_null_init_err(dart_heap_t* pkt) {
    return dart_heap_null_init_rc_err(pkt, DART_RC_SAFE);
  }

  dart_heap_t dart_heap_null_init_rc(dart_rc_type_t rc) {
    dart_heap_t pkt;
    auto err = dart_heap_null_init_rc_err(&pkt, rc);
    if (err) return dart_heap_init();
    else return pkt;
  }

  dart_err_t dart_heap_init_null_rc_err(dart_heap_t* pkt, dart_rc_type_t rc) {
    // Default initialize, then assign.
    // Unnecessary, but done for consistency of code formatting.
    return heap_typed_constructor_access(
      compose(
        [] (dart::heap& pkt) { pkt = dart::heap::make_null(); },
        [] (dart::unsafe_heap& pkt) { pkt = dart::unsafe_heap::make_null(); }
      ),
      pkt,
      rc
    );
  }

  dart_err_t dart_heap_obj_insert_heap(dart_heap_t* pkt, char const* key, dart_heap_t const* val) {
    return dart_heap_obj_insert_heap_len(pkt, key, strlen(key), val);
  }

  dart_err_t dart_heap_obj_insert_heap_len(dart_heap_t* pkt, char const* key, size_t len, dart_heap_t const* val) {
    auto insert = [=] (auto& pkt, auto& val) { pkt.insert(string_view {key, len}, val); };
    return heap_access(
      compose(
        [=] (dart::heap& pkt) {
          return heap_access([=, &pkt] (dart::heap const& val) { insert(pkt, val); }, val);
        },
        [=] (dart::unsafe_heap& pkt) {
          return heap_access([=, &pkt] (dart::unsafe_heap const& val) { insert(pkt, val); }, val);
        }
      ),
      pkt
    );
  }

  dart_err_t dart_heap_obj_take_heap(dart_heap_t* pkt, char const* key, dart_heap_t* val) {
    return dart_heap_obj_take_heap_len(pkt, key, strlen(key), val);
  }

  dart_err_t dart_heap_obj_take_heap_len(dart_heap_t* pkt, char const* key, size_t len, dart_heap_t* val) {
    auto insert = [=] (auto& pkt, auto& val) { pkt.insert(string_view {key, len}, std::move(val)); };
    return heap_access(
      compose(
        [=] (dart::heap& pkt) {
          return heap_access([=, &pkt] (dart::heap& val) { insert(pkt, val); }, val);
        },
        [=] (dart::unsafe_heap& pkt) {
          return heap_access([=, &pkt] (dart::unsafe_heap& val) { insert(pkt, val); }, val);
        }
      ),
      pkt
    );
  }

  dart_err_t dart_heap_obj_insert_str(dart_heap_t* pkt, char const* key, char const* val) {
    return dart_heap_obj_insert_str_len(pkt, key, strlen(key), val, strlen(val));
  }

  dart_err_t dart_heap_obj_insert_str_len(dart_heap_t* pkt, char const* key, size_t len, char const* val, size_t val_len) {
    auto insert = [=] (auto& pkt) { pkt.insert(string_view {key, len}, string_view {val, val_len}); };
    return heap_access(
      compose(
        [insert] (dart::heap& pkt) { insert(pkt); },
        [insert] (dart::unsafe_heap& pkt) { insert(pkt); }
      ),
      pkt
    );
  }

  dart_err_t dart_heap_obj_insert_int(dart_heap_t* pkt, char const* key, int64_t val) {
    return dart_heap_obj_insert_int_len(pkt, key, strlen(key), val);
  }

  dart_err_t dart_heap_obj_insert_int_len(dart_heap_t* pkt, char const* key, size_t len, int64_t val) {
    auto insert = [=] (auto& pkt) { pkt.insert(string_view {key, len}, val); };
    return heap_access(
      compose(
        [insert] (dart::heap& pkt) { insert(pkt); },
        [insert] (dart::unsafe_heap& pkt) { insert(pkt); }
      ),
      pkt
    );
  }

  dart_err_t dart_heap_obj_insert_dcm(dart_heap_t* pkt, char const* key, double val) {
    return dart_heap_obj_insert_dcm_len(pkt, key, strlen(key), val);
  }

  dart_err_t dart_heap_obj_insert_dcm_len(dart_heap_t* pkt, char const* key, size_t len, double val) {
    auto insert = [=] (auto& pkt) { pkt.insert(string_view {key, len}, val); };
    return heap_access(
      compose(
        [insert] (dart::heap& pkt) { insert(pkt); },
        [insert] (dart::unsafe_heap& pkt) { insert(pkt); }
      ),
      pkt
    );
  }

  dart_err_t dart_heap_obj_insert_bool(dart_heap_t* pkt, char const* key, int val) {
    return dart_heap_obj_insert_bool_len(pkt, key, strlen(key), val);
  }

  dart_err_t dart_heap_obj_insert_bool_len(dart_heap_t* pkt, char const* key, size_t len, int val) {
    auto insert = [=] (auto& pkt) { pkt.insert(string_view {key, len}, static_cast<bool>(val)); };
    return heap_access(
      compose(
        [insert] (dart::heap& pkt) { insert(pkt); },
        [insert] (dart::unsafe_heap& pkt) { insert(pkt); }
      ),
      pkt
    );
  }

  dart_err_t dart_heap_obj_insert_null(dart_heap_t* pkt, char const* key) {
    return dart_heap_obj_insert_null_len(pkt, key, strlen(key));
  }

  dart_err_t dart_heap_obj_insert_null_len(dart_heap_t* pkt, char const* key, size_t len) {
    auto insert = [=] (auto& pkt) { pkt.insert(string_view {key, len}, nullptr); };
    return heap_access(
      compose(
        [insert] (dart::heap& pkt) { insert(pkt); },
        [insert] (dart::unsafe_heap& pkt) { insert(pkt); }
      ),
      pkt
    );
  }

  dart_err_t dart_heap_obj_erase(dart_heap_t* pkt, char const* key) {
    return dart_heap_obj_erase_len(pkt, key, strlen(key));
  }

  dart_err_t dart_heap_obj_erase_len(dart_heap_t* pkt, char const* key, size_t len) {
    auto erase = [=] (auto& pkt) { pkt.erase(string_view {key, len}); };
    return heap_access(
      compose(
        [erase] (dart::heap& pkt) { erase(pkt); },
        [erase] (dart::unsafe_heap& pkt) { erase(pkt); }
      ),
      pkt
    );
  }

  dart_err_t dart_heap_arr_insert_heap(dart_heap_t* pkt, size_t idx, dart_heap_t const* val) {
    auto insert = [=] (auto& pkt, auto& val) { pkt.insert(idx, val); };
    return heap_access(
      compose(
        [=] (dart::heap& pkt) {
          return heap_access([=, &pkt] (dart::heap const& val) { insert(pkt, val); }, val);
        },
        [=] (dart::unsafe_heap& pkt) {
          return heap_access([=, &pkt] (dart::unsafe_heap const& val) { insert(pkt, val); }, val);
        }
      ),
      pkt
    );
  }

  dart_err_t dart_heap_arr_take_heap(dart_heap_t* pkt, size_t idx, dart_heap_t* val) {
    auto insert = [=] (auto& pkt, auto& val) { pkt.insert(idx, std::move(val)); };
    return heap_access(
      compose(
        [=] (dart::heap& pkt) {
          return heap_access([=, &pkt] (dart::heap& val) { insert(pkt, val); }, val);
        },
        [=] (dart::unsafe_heap& pkt) {
          return heap_access([=, &pkt] (dart::unsafe_heap& val) { insert(pkt, val); }, val);
        }
      ),
      pkt
    );
  }

  dart_err_t dart_heap_arr_insert_str(dart_heap_t* pkt, size_t idx, char const* val) {
    return dart_heap_arr_insert_str_len(pkt, idx, val, strlen(val));
  }

  dart_err_t dart_heap_arr_insert_str_len(dart_heap_t* pkt, size_t idx, char const* val, size_t val_len) {
    auto insert = [=] (auto& pkt) { pkt.insert(idx, string_view {val, val_len}); };
    return heap_access(
      compose(
        [insert] (dart::heap& pkt) { insert(pkt); },
        [insert] (dart::unsafe_heap& pkt) { insert(pkt); }
      ),
      pkt
    );
  }

  dart_err_t dart_heap_arr_insert_int(dart_heap_t* pkt, size_t idx, int64_t val) {
    auto insert = [=] (auto& pkt) { pkt.insert(idx, val); };
    return heap_access(
      compose(
        [insert] (dart::heap& pkt) { insert(pkt); },
        [insert] (dart::unsafe_heap& pkt) { insert(pkt); }
      ),
      pkt
    );
  }

  dart_err_t dart_heap_arr_insert_dcm(dart_heap_t* pkt, size_t idx, double val) {
    auto insert = [=] (auto& pkt) { pkt.insert(idx, val); };
    return heap_access(
      compose(
        [insert] (dart::heap& pkt) { insert(pkt); },
        [insert] (dart::unsafe_heap& pkt) { insert(pkt); }
      ),
      pkt
    );
  }

  dart_err_t dart_heap_arr_insert_bool(dart_heap_t* pkt, size_t idx, int val) {
    auto insert = [=] (auto& pkt) { pkt.insert(idx, static_cast<bool>(val)); };
    return heap_access(
      compose(
        [insert] (dart::heap& pkt) { insert(pkt); },
        [insert] (dart::unsafe_heap& pkt) { insert(pkt); }
      ),
      pkt
    );
  }

  dart_err_t dart_heap_arr_insert_null(dart_heap_t* pkt, size_t idx) {
    auto insert = [=] (auto& pkt) { pkt.insert(idx, nullptr); };
    return heap_access(
      compose(
        [=] (dart::heap& pkt) { insert(pkt); },
        [=] (dart::unsafe_heap& pkt) { insert(pkt); }
      ),
      pkt
    );
  }

  dart_err_t dart_heap_arr_erase(dart_heap_t* pkt, size_t idx) {
    auto erase = [=] (auto& pkt) { pkt.erase(idx); };
    return heap_access(
      compose(
        [erase] (dart::heap& pkt) { erase(pkt); },
        [erase] (dart::unsafe_heap& pkt) { erase(pkt); }
      ),
      pkt
    );
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
            new(dst) dart::heap(src[{key, len}]);
          }, dst);
        },
        [=] (dart::unsafe_heap const& src) {
          return heap_construct([&] (dart::unsafe_heap* dst) {
            new(dst) dart::unsafe_heap(src[{key, len}]);
          }, dst);
        }
      ),
      src
    );
  }

  dart_heap_t dart_heap_arr_get(dart_heap_t const* src, int64_t idx) {
    dart_heap_t dst;
    auto err = dart_heap_arr_get_err(&dst, src, idx);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_err_t dart_heap_arr_get_err(dart_heap_t* dst, dart_heap_t const* src, int64_t idx) {
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
    int val = 1;
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

  bool dart_heap_equal(dart_heap_t const* lhs, dart_heap_t const* rhs) {
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

  bool dart_heap_is_obj(dart_heap_t const* src) {
    return dart_heap_get_type(src) == DART_OBJECT;
  }

  bool dart_heap_is_arr(dart_heap_t const* src) {
    return dart_heap_get_type(src) == DART_ARRAY;
  }

  bool dart_heap_is_str(dart_heap_t const* src) {
    return dart_heap_get_type(src) == DART_STRING;
  }

  bool dart_heap_is_int(dart_heap_t const* src) {
    return dart_heap_get_type(src) == DART_INTEGER;
  }

  bool dart_heap_is_dcm(dart_heap_t const* src) {
    return dart_heap_get_type(src) == DART_DECIMAL;
  }

  bool dart_heap_is_bool(dart_heap_t const* src) {
    return dart_heap_get_type(src) == DART_BOOLEAN;
  }

  bool dart_heap_is_null(dart_heap_t const* src) {
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
    dart_heap_t pkt;
    auto err = dart_heap_from_json_err(&pkt, str);
    if (err) return dart_heap_init();
    else return pkt;
  }

  dart_err_t dart_heap_from_json_err(dart_heap_t* pkt, char const* str) {
    return dart_heap_from_json_len_rc_err(pkt, DART_RC_SAFE, str, strlen(str));
  }

  dart_heap_t dart_heap_from_json_rc(dart_rc_type_t rc, char const* str) {
    dart_heap_t pkt;
    auto err = dart_heap_from_json_rc_err(&pkt, rc, str);
    if (err) return dart_heap_init();
    else return pkt;
  }

  dart_err_t dart_heap_from_json_rc_err(dart_heap_t* pkt, dart_rc_type_t rc, char const* str) {
    return dart_heap_from_json_len_rc_err(pkt, rc, str, strlen(str));
  }

  dart_heap_t dart_heap_from_json_len(char const* str, size_t len) {
    dart_heap_t pkt;
    auto err = dart_heap_from_json_len_err(&pkt, str, len);
    if (err) return dart_heap_init();
    else return pkt;
  }

  dart_err_t dart_heap_from_json_len_err(dart_heap_t* pkt, char const* str, size_t len) {
    return dart_heap_from_json_len_rc_err(pkt, DART_RC_SAFE, str, len);
  }

  dart_heap_t dart_heap_from_json_len_rc(dart_rc_type_t rc, char const* str, size_t len) {
    dart_heap_t pkt;
    auto err = dart_heap_from_json_len_rc_err(&pkt, rc, str, len);
    if (err) return dart_heap_init();
    else return pkt;
  }

  dart_err_t dart_heap_from_json_len_rc_err(dart_heap_t* pkt, dart_rc_type_t rc, char const* str, size_t len) {
    return heap_typed_constructor_access(
      compose(
        [=] (dart::heap& pkt) {
          pkt = dart::heap::from_json({str, len});
        },
        [=] (dart::unsafe_heap& pkt) {
          pkt = dart::unsafe_heap::from_json({str, len});
        }
      ),
      pkt,
      rc
    );
  }

  char* dart_heap_to_json(dart_heap_t const* pkt, size_t* len) {
    // How long has it been since I've called a raw malloc like this...
    char* outstr;
    auto print = [&] (auto& pkt) {
      // Call these first so they throw before allocation.
      auto instr = pkt.to_json();
      auto inlen = instr.size() + 1;
      if (len) *len = inlen;
      outstr = reinterpret_cast<char*>(malloc(inlen));
      memcpy(outstr, instr.data(), inlen);
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

  dart_buffer_t dart_buffer_init() {
    // Cannot meaningfully fail.
    dart_buffer_t pkt;
    dart_buffer_init_rc_err(&pkt, DART_RC_SAFE);
    return pkt;
  }

  dart_err_t dart_buffer_init_err(dart_buffer_t* pkt) {
    return dart_buffer_init_rc_err(pkt, DART_RC_SAFE);
  }

  dart_buffer_t dart_buffer_init_rc(dart_rc_type_t rc) {
    // Cannot meaningfully fail.
    dart_buffer_t pkt;
    dart_buffer_init_rc_err(&pkt, rc);
    return pkt;
  }

  dart_err_t dart_buffer_init_rc_err(dart_buffer_t* pkt, dart_rc_type_t rc) {
    // Initialize.
    pkt->rtti = {DART_HEAP, rc};
    return buffer_constructor_access(
      compose(
        [] (dart::buffer* ptr) { new(ptr) dart::buffer(); },
        [] (dart::unsafe_buffer* ptr) { new(ptr) dart::unsafe_buffer(); }
      ),
      pkt
    );
  }

  dart_buffer_t dart_buffer_copy(dart_buffer_t const* src) {
    dart_buffer_t pkt;
    auto err = dart_buffer_copy_err(&pkt, src);
    if (err) return dart_buffer_init();
    else return pkt;
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
    dart_buffer_t pkt;
    auto err = dart_buffer_move_err(&pkt, src);
    if (err) return dart_buffer_init();
    else return pkt;
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

  dart_err_t dart_buffer_destroy(dart_buffer_t* pkt) {
    // Destroy.
    return buffer_access(
      compose(
        [] (dart::buffer& pkt) { pkt.~basic_buffer(); },
        [] (dart::unsafe_buffer& pkt) { pkt.~basic_buffer(); }
      ),
      pkt
    );
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

  dart_buffer_t dart_buffer_arr_get(dart_buffer_t const* src, int64_t idx) {
    dart_buffer_t dst;
    auto err = dart_buffer_arr_get_err(&dst, src, idx);
    if (err) return dart_buffer_init();
    else return dst;
  }

  dart_err_t dart_buffer_arr_get_err(dart_buffer_t* dst, dart_buffer_t const* src, int64_t idx) {
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

  int64_t dart_buffer_int_get(dart_buffer_t const* src) {
    // No way unique way to signal failure here,
    // the user needs to know this will succeed.
    int64_t val = 0;
    dart_buffer_int_get_err(src, &val);
    return val;
  }

  dart_err_t dart_buffer_int_get_err(dart_buffer_t const* src, int64_t* val) {
    auto get_int = [=] (auto& src) { *val = src.integer(); };
    return buffer_access(
      compose(
        [get_int] (dart::buffer const& src) { get_int(src); },
        [get_int] (dart::unsafe_buffer const& src) { get_int(src); }
      ),
      src
    );
  }

  double dart_buffer_dcm_get(dart_buffer_t const* src) {
    double val = std::numeric_limits<double>::quiet_NaN();
    dart_buffer_dcm_get_err(src, &val);
    return val;
  }

  dart_err_t dart_buffer_dcm_get_err(dart_buffer_t const* src, double* val) {
    auto get_dcm = [=] (auto& src) { *val = src.decimal(); };
    return buffer_access(
      compose(
        [get_dcm] (dart::buffer const& src) { get_dcm(src); },
        [get_dcm] (dart::unsafe_buffer const& src) { get_dcm(src); }
      ),
      src
    );
  }

  int dart_buffer_bool_get(dart_buffer_t const* src) {
    // No way unique way to signal failure here,
    // the user needs to know this will succeed.
    int val = 1;
    dart_buffer_bool_get_err(src, &val);
    return val;
  }

  dart_err_t dart_buffer_bool_get_err(dart_buffer_t const* src, int* val) {
    auto get_bool = [=] (auto& src) { *val = src.boolean(); };
    return buffer_access(
      compose(
        [get_bool] (dart::buffer const& src) { get_bool(src); },
        [get_bool] (dart::unsafe_buffer const& src) { get_bool(src); }
      ),
      src
    );
  }

  bool dart_buffer_equal(dart_buffer_t const* lhs, dart_buffer_t const* rhs) {
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

  bool dart_buffer_is_obj(dart_buffer_t const* src) {
    return dart_buffer_get_type(src) == DART_OBJECT;
  }

  bool dart_buffer_is_arr(dart_buffer_t const* src) {
    return dart_buffer_get_type(src) == DART_ARRAY;
  }

  bool dart_buffer_is_str(dart_buffer_t const* src) {
    return dart_buffer_get_type(src) == DART_STRING;
  }

  bool dart_buffer_is_int(dart_buffer_t const* src) {
    return dart_buffer_get_type(src) == DART_INTEGER;
  }

  bool dart_buffer_is_dcm(dart_buffer_t const* src) {
    return dart_buffer_get_type(src) == DART_DECIMAL;
  }

  bool dart_buffer_is_bool(dart_buffer_t const* src) {
    return dart_buffer_get_type(src) == DART_BOOLEAN;
  }

  bool dart_buffer_is_null(dart_buffer_t const* src) {
    return dart_buffer_get_type(src) == DART_NULL;
  }

  dart_type_t dart_buffer_get_type(dart_buffer_t const* src) {
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

  dart_buffer_t dart_buffer_from_json(char const* str) {
    dart_buffer_t pkt;
    auto err = dart_buffer_from_json_err(&pkt, str);
    if (err) return dart_buffer_init();
    else return pkt;
  }

  dart_err_t dart_buffer_from_json_err(dart_buffer_t* pkt, char const* str) {
    return dart_buffer_from_json_len_rc_err(pkt, DART_RC_SAFE, str, strlen(str));
  }

  dart_buffer_t dart_buffer_from_json_rc(dart_rc_type_t rc, char const* str) {
    dart_buffer_t pkt;
    auto err = dart_buffer_from_json_rc_err(&pkt, rc, str);
    if (err) return dart_buffer_init();
    else return pkt;
  }

  dart_err_t dart_buffer_from_json_rc_err(dart_buffer_t* pkt, dart_rc_type_t rc, char const* str) {
    return dart_buffer_from_json_len_rc_err(pkt, rc, str, strlen(str));
  }

  dart_buffer_t dart_buffer_from_json_len(char const* str, size_t len) {
    dart_buffer_t pkt;
    auto err = dart_buffer_from_json_len_err(&pkt, str, len);
    if (err) return dart_buffer_init();
    else return pkt;
  }

  dart_err_t dart_buffer_from_json_len_err(dart_buffer_t* pkt, char const* str, size_t len) {
    return dart_buffer_from_json_len_rc_err(pkt, DART_RC_SAFE, str, len);
  }

  dart_buffer_t dart_buffer_from_json_len_rc(dart_rc_type_t rc, char const* str, size_t len) {
    dart_buffer_t pkt;
    auto err = dart_buffer_from_json_len_rc_err(&pkt, rc, str, len);
    if (err) return dart_buffer_init();
    else return pkt;
  }

  dart_err_t dart_buffer_from_json_len_rc_err(dart_buffer_t* pkt, dart_rc_type_t rc, char const* str, size_t len) {
    auto err = dart_buffer_init_rc_err(pkt, rc);
    if (err) return err;

    // Assign to it.
    return err_handler([=] {
      return buffer_unwrap(
        compose(
          [=] (dart::buffer& pkt) { pkt = dart::buffer::from_json({str, len}); },
          [=] (dart::unsafe_buffer& pkt) { pkt = dart::unsafe_buffer::from_json({str, len}); }
        ),
        pkt
      );
    });
  }

  char* dart_buffer_to_json(dart_buffer_t const* pkt, size_t* len) {
    // How long has it been since I've called a raw malloc like this...
    char* outstr;
    auto print = [&] (auto& pkt) {
      // Call these first so they throw before allocation.
      auto instr = pkt.to_json();
      auto inlen = instr.size() + 1;
      if (len) *len = inlen;
      outstr = reinterpret_cast<char*>(malloc(inlen));
      memcpy(outstr, instr.data(), inlen);
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

  dart_heap_t dart_buffer_lift(dart_buffer_t const* src) {
    dart_heap_t dst;
    auto err = dart_buffer_lift_err(&dst, src);
    if (err) return dart_heap_init();
    else return dst;
  }

  dart_heap_t dart_buffer_definalize(dart_buffer_t const* src) {
    dart_heap_t dst;
    auto err = dart_buffer_definalize_err(&dst, src);
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

  dart_err_t dart_buffer_definalize_err(dart_heap_t* dst, dart_buffer_t const* src) {
    return dart_buffer_lift_err(dst, src);
  }

  dart_err_t dart_packet_init(dart_packet_t* pkt) {
    return dart_packet_init_rc(pkt, DART_RC_SAFE);
  }

  dart_err_t dart_packet_init_rc(dart_packet_t* pkt, dart_rc_type_t rc) {
    // Initialize.
    pkt->rtti = {DART_PACKET, rc};
    return packet_constructor_access(
      compose(
        [] (dart::packet* ptr) { new(ptr) dart::packet(); },
        [] (dart::unsafe_packet* ptr) { new(ptr) dart::unsafe_packet(); }
      ),
      pkt
    );
  }

  dart_err_t dart_packet_copy(dart_packet_t* dst, dart_packet_t const* src) {
    // Initialize.
    dst->rtti = src->rtti;
    return packet_access(
      compose(
        [dst] (dart::packet const& src) {
          return packet_construct([&src] (dart::packet* dst) { new(dst) dart::packet(src); }, dst);
        },
        [dst] (dart::unsafe_packet const& src) {
          return packet_construct([&src] (dart::unsafe_packet* dst) { new(dst) dart::unsafe_packet(src); }, dst);
        }
      ),
      src
    );
  }

  dart_err_t dart_packet_move(dart_packet_t* dst, dart_packet_t* src) {
    // Initialize.
    dst->rtti = src->rtti;
    return packet_access(
      compose(
        [dst] (dart::packet& src) {
          return packet_construct([&src] (dart::packet* dst) { new(dst) dart::packet(std::move(src)); }, dst);
        },
        [dst] (dart::unsafe_packet& src) {
          return packet_construct([&src] (dart::unsafe_packet* dst) { new(dst) dart::unsafe_packet(std::move(src)); }, dst);
        }
      ),
      src
    );
  }

  dart_err_t dart_packet_destroy(dart_packet_t* pkt) {
    return packet_access(
      compose(
        [] (dart::packet& pkt) { pkt.~basic_packet(); },
        [] (dart::unsafe_packet& pkt) { pkt.~basic_packet(); }
      ),
      pkt
    );
  }

  dart_err_t dart_packet_init_obj(dart_packet_t* pkt) {
    return dart_packet_init_obj_rc(pkt, DART_RC_SAFE);
  }

  dart_err_t dart_packet_init_obj_rc(dart_packet_t* pkt, dart_rc_type_t rc) {
    // Default initialize, then assign.
    return packet_typed_constructor_access(
      compose(
        [] (dart::packet& pkt) { pkt = dart::packet::make_object(); },
        [] (dart::unsafe_packet& pkt) { pkt = dart::unsafe_packet::make_object(); }
      ),
      pkt,
      rc
    );
  }

  static dart_err_t dart_packet_init_obj_va_impl(dart_packet_t* pkt, dart_rc_type_t rc, char const* format, va_list args) {
    return packet_typed_constructor_access(
      compose(
        [format, args] (dart::packet& pkt) mutable {
          pkt = dart::packet::make_object();
          parse_pairs(pkt, format, args);
        },
        [format, args] (dart::unsafe_packet& pkt) mutable {
          pkt = dart::unsafe_packet::make_object();
          parse_pairs(pkt, format, args);
        }
      ),
      pkt,
      rc
    );
  }

  dart_err_t dart_packet_init_obj_va(dart_packet_t* pkt, char const* format, ...) {
    va_list args;
    va_start(args, format);
    auto ret = dart_packet_init_obj_va_impl(pkt, DART_RC_SAFE, format, args);
    va_end(args);
    return ret;
  }

  dart_err_t dart_packet_init_obj_va_rc(dart_packet_t* pkt, dart_rc_type_t rc, char const* format, ...) {
    va_list args;
    va_start(args, format);
    auto ret = dart_packet_init_obj_va_impl(pkt, rc, format, args);
    va_end(args);
    return ret;
  }

  dart_err_t dart_packet_init_arr(dart_packet_t* pkt) {
    return dart_packet_init_arr_rc(pkt, DART_RC_SAFE);
  }

  dart_err_t dart_packet_init_arr_rc(dart_packet_t* pkt, dart_rc_type_t rc) {
    // Default initialize, then assign.
    return packet_typed_constructor_access(
      compose(
        [] (dart::packet& pkt) { pkt = dart::packet::make_array(); },
        [] (dart::unsafe_packet& pkt) { pkt = dart::unsafe_packet::make_array(); }
      ),
      pkt,
      rc
    );
  }

  static dart_err_t dart_packet_init_arr_va_impl(dart_packet_t* pkt, dart_rc_type_t rc, char const* format, va_list args) {
    return packet_typed_constructor_access(
      compose(
        [format, args] (dart::packet& pkt) mutable {
          pkt = dart::packet::make_array();
          parse_vals(pkt, format, args);
        },
        [format, args] (dart::unsafe_packet& pkt) mutable {
          pkt = dart::unsafe_packet::make_array();
          parse_vals(pkt, format, args);
        }
      ),
      pkt,
      rc
    );
  }

  dart_err_t dart_packet_init_arr_va(dart_packet_t* pkt, char const* format, ...) {
    va_list args;
    va_start(args, format);
    auto ret = dart_packet_init_arr_va_impl(pkt, DART_RC_SAFE, format, args);
    va_end(args);
    return ret;
  }

  dart_err_t dart_packet_init_arr_va_rc(dart_packet_t* pkt, dart_rc_type_t rc, char const* format, ...) {
    va_list args;
    va_start(args, format);
    auto ret = dart_packet_init_arr_va_impl(pkt, rc, format, args);
    va_end(args);
    return ret;
  }

  dart_err_t dart_packet_init_str(dart_packet_t* pkt, char const* str, size_t len) {
    return dart_packet_init_str_rc(pkt, DART_RC_SAFE, str, len);
  }

  dart_err_t dart_packet_init_str_rc(dart_packet_t* pkt, dart_rc_type_t rc, char const* str, size_t len) {
    // Default initialize, then assign
    return packet_typed_constructor_access(
      compose(
        [str, len] (dart::packet& pkt) { pkt = dart::packet::make_string({str, len}); },
        [str, len] (dart::unsafe_packet& pkt) { pkt = dart::unsafe_packet::make_string({str, len}); }
      ),
      pkt,
      rc
    );
  }

  dart_err_t dart_packet_init_int(dart_packet_t* pkt, int64_t val) {
    return dart_packet_init_int_rc(pkt, DART_RC_SAFE, val);
  }

  dart_err_t dart_packet_init_int_rc(dart_packet_t* pkt, dart_rc_type_t rc, int64_t val) {
    // Default initialize, then assign
    return packet_typed_constructor_access(
      compose(
        [val] (dart::packet& pkt) { pkt = dart::packet::make_integer(val); },
        [val] (dart::unsafe_packet& pkt) { pkt = dart::unsafe_packet::make_integer(val); }
      ),
      pkt,
      rc
    );
  }

  dart_err_t dart_packet_init_dcm(dart_packet_t* pkt, double val) {
    return dart_packet_init_dcm_rc(pkt, DART_RC_SAFE, val);
  }

  dart_err_t dart_packet_init_dcm_rc(dart_packet_t* pkt, dart_rc_type_t rc, double val) {
    // Default initialize, then assign
    return packet_typed_constructor_access(
      compose(
        [val] (dart::packet& pkt) { pkt = dart::packet::make_decimal(val); },
        [val] (dart::unsafe_packet& pkt) { pkt = dart::unsafe_packet::make_decimal(val); }
      ),
      pkt,
      rc
    );
  }

  dart_err_t dart_packet_init_bool(dart_packet_t* pkt, int val) {
    return dart_packet_init_bool_rc(pkt, DART_RC_SAFE, val);
  }

  dart_err_t dart_packet_init_bool_rc(dart_packet_t* pkt, dart_rc_type_t rc, int val) {
    // Default initialize, then assign
    return packet_typed_constructor_access(
      compose(
        [val] (dart::packet& pkt) { pkt = dart::packet::make_boolean(val); },
        [val] (dart::unsafe_packet& pkt) { pkt = dart::unsafe_packet::make_boolean(val); }
      ),
      pkt,
      rc
    );
  }

  dart_err_t dart_packet_init_null(dart_packet_t* pkt) {
    return dart_packet_init_null_rc(pkt, DART_RC_SAFE);
  }

  dart_err_t dart_packet_init_null_rc(dart_packet_t* pkt, dart_rc_type_t rc) {
    // Default initialize, then assign.
    // Unnecessary, but done for consistency of code formatting.
    return packet_typed_constructor_access(
      compose(
        [] (dart::packet& pkt) { pkt = dart::packet::make_null(); },
        [] (dart::unsafe_packet& pkt) { pkt = dart::unsafe_packet::make_null(); }
      ),
      pkt,
      rc
    );
  }

  dart_err_t dart_packet_obj_add_packet(dart_packet_t* pkt, char const* key, size_t len, dart_packet_t const* val) {
    auto insert = [=] (auto& pkt, auto& val) { pkt.add_field(string_view {key, len}, val); };
    return packet_access(
      compose(
        [=] (dart::packet& pkt) {
          return packet_access([=, &pkt] (dart::packet const& val) { insert(pkt, val); }, val);
        },
        [=] (dart::unsafe_packet& pkt) {
          return packet_access([=, &pkt] (dart::unsafe_packet const& val) { insert(pkt, val); }, val);
        }
      ),
      pkt
    );
  }

  dart_err_t dart_packet_obj_take_packet(dart_packet_t* pkt, char const* key, size_t len, dart_packet_t* val) {
    auto insert = [=] (auto& pkt, auto& val) { pkt.add_field(string_view {key, len}, std::move(val)); };
    return packet_access(
      compose(
        [=] (dart::packet& pkt) {
          return packet_access([=, &pkt] (dart::packet& val) { insert(pkt, val); }, val);
        },
        [=] (dart::unsafe_packet& pkt) {
          return packet_access([=, &pkt] (dart::unsafe_packet& val) { insert(pkt, val); }, val);
        }
      ),
      pkt
    );
  }

  dart_err_t dart_packet_obj_add_str(dart_packet_t* pkt, char const* key, size_t len, char const* val, size_t val_len) {
    auto insert = [=] (auto& pkt) { pkt.add_field(string_view {key, len}, string_view {val, val_len}); };
    return packet_access(
      compose(
        [=] (dart::packet& pkt) { insert(pkt); },
        [=] (dart::unsafe_packet& pkt) { insert(pkt); }
      ),
      pkt
    );
  }

  dart_err_t dart_packet_obj_add_int(dart_packet_t* pkt, char const* key, size_t len, int64_t val) {
    auto insert = [=] (auto& pkt) { pkt.add_field(string_view {key, len}, val); };
    return packet_access(
      compose(
        [=] (dart::packet& pkt) { insert(pkt); },
        [=] (dart::unsafe_packet& pkt) { insert(pkt); }
      ),
      pkt
    );
  }

  dart_err_t dart_packet_obj_add_dcm(dart_packet_t* pkt, char const* key, size_t len, double val) {
    auto insert = [=] (auto& pkt) { pkt.add_field(string_view {key, len}, val); };
    return packet_access(
      compose(
        [=] (dart::packet& pkt) { insert(pkt); },
        [=] (dart::unsafe_packet& pkt) { insert(pkt); }
      ),
      pkt
    );
  }

  dart_err_t dart_packet_obj_add_bool(dart_packet_t* pkt, char const* key, size_t len, int val) {
    auto insert = [=] (auto& pkt) { pkt.add_field(string_view {key, len}, static_cast<bool>(val)); };
    return packet_access(
      compose(
        [=] (dart::packet& pkt) { insert(pkt); },
        [=] (dart::unsafe_packet& pkt) { insert(pkt); }
      ),
      pkt
    );
  }

  dart_err_t dart_packet_obj_add_null(dart_packet_t* pkt, char const* key, size_t len) {
    auto insert = [=] (auto& pkt) { pkt.add_field(string_view {key, len}, nullptr); };
    return packet_access(
      compose(
        [=] (dart::packet& pkt) { insert(pkt); },
        [=] (dart::unsafe_packet& pkt) { insert(pkt); }
      ),
      pkt
    );
  }

  dart_err_t dart_packet_from_json(dart_packet_t* pkt, char const* str) {
    return dart_packet_from_json_len_rc(pkt, DART_RC_SAFE, str, strlen(str));
  }

  dart_err_t dart_packet_from_json_rc(dart_packet_t* pkt, dart_rc_type_t rc, char const* str) {
    return dart_packet_from_json_len_rc(pkt, rc, str, strlen(str));
  }

  dart_err_t dart_packet_from_json_len(dart_packet_t* pkt, char const* str, size_t len) {
    return dart_packet_from_json_len_rc(pkt, DART_RC_SAFE, str, len);
  }

  dart_err_t dart_packet_from_json_len_rc(dart_packet_t* pkt, dart_rc_type_t rc, char const* str, size_t len) {
    return packet_typed_constructor_access(
      compose(
        [=] (dart::packet& pkt) {
          pkt = dart::packet::from_json({str, len});
        },
        [=] (dart::unsafe_packet& pkt) {
          pkt = dart::unsafe_packet::from_json({str, len});
        }
      ),
      pkt,
      rc
    );
  }

  char* dart_packet_to_json(dart_packet_t const* pkt, size_t* len) {
    // How long has it been since I've called a raw malloc like this...
    char* outstr;
    auto print = [&] (auto& pkt) {
      // Call these first so they throw before allocation.
      auto instr = pkt.to_json();
      auto inlen = instr.size() + 1;
      if (len) *len = inlen;
      outstr = reinterpret_cast<char*>(malloc(inlen));
      memcpy(outstr, instr.data(), inlen);
    };
    auto ret = packet_access(
      compose(
        [=] (dart::packet const& pkt) { print(pkt); },
        [=] (dart::unsafe_packet const& pkt) { print(pkt); }
      ),
      pkt
    );
    if (ret) return nullptr;
    return outstr;
  }

  char* dart_to_json(void* pkt, size_t* len) {
    char* outstr;
    auto ret = generic_access(
      [&] (auto& pkt) {
        // Call these first so they throw before allocation.
        auto instr = pkt.to_json();
        auto inlen = instr.size() + 1;
        if (len) *len = inlen;
        outstr = reinterpret_cast<char*>(malloc(inlen));
        memcpy(outstr, instr.data(), inlen);
      },
      pkt
    );
    if (ret) return nullptr;
    return outstr;
  }

  dart_err_t dart_destroy(void* pkt) {
    // Generic destroy function.
    // Get rid of it whatever it is.
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

  char const* dart_get_error() {
    return errmsg;
  }

}
