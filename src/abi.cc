/*----- System Includes -----*/

#include <stdarg.h>

/*----- Local Includes -----*/

#include "../include/dart.h"
#include "../include/dart/abi.h"

/*----- Build Sanity Checks -----*/

static_assert(sizeof(dart::heap) <= DART_HEAP_MAX_SIZE, "Dart ABI is misconfigured");
static_assert(sizeof(dart::buffer) <= DART_BUFFER_MAX_SIZE, "Dart ABI is misconfigured");
static_assert(sizeof(dart::packet) <= DART_PACKET_MAX_SIZE, "Dart ABI is misconfigured");
static_assert(sizeof(dart::heap::iterator) * 2 <= DART_ITERATOR_MAX_SIZE, "Dart ABI is misconfigured");
static_assert(sizeof(dart::buffer::iterator) * 2 <= DART_ITERATOR_MAX_SIZE, "Dart ABI is misconfigured");
static_assert(sizeof(dart::packet::iterator) * 2 <= DART_ITERATOR_MAX_SIZE, "Dart ABI is misconfigured");

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

template <template <template <class> class> class Packet>
struct packet_builder {
  template <template <class> class RefCount>
  using rebind = Packet<RefCount>;
};
template <class PacketWrapper, template <class> class RefCount>
using packet_t = typename PacketWrapper::template rebind<RefCount>;

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

  template <class TargetPacket, class SourcePacket>
  void safe_construct_impl(TargetPacket*, SourcePacket const&, std::false_type) {
    throw dart::type_error("Unsupported packet type conversion requested. Did you mix rc types?");
  }

  template <class TargetPacket, class SourcePacket>
  void safe_construct_impl(TargetPacket* dst, SourcePacket&& src, std::true_type) {
    new(dst) TargetPacket(dart::convert::cast<TargetPacket>(std::forward<SourcePacket>(src)));
  }

  template <class TargetPacket, class SourcePacket>
  void safe_construct(TargetPacket* dst, SourcePacket&& src) {
    safe_construct_impl(dst,
        std::forward<SourcePacket>(src), dart::convert::is_castable<SourcePacket, TargetPacket> {});
  }

  template <class Packet, class Key, class Value>
  void safe_set_impl(Packet&, Key&&, Value&&, std::false_type) {
    throw dart::type_error("Unsupported packet type setion requested");
  }

  template <class Packet, class Key, class Value>
  void safe_set_impl(Packet& pkt, Key&& key, Value&& val, std::true_type) {
    pkt.set(std::forward<Key>(key), std::forward<Value>(val));
  }

  template <class Packet, class Key, class Value>
  void safe_set(Packet& pkt, Key&& key, Value&& val) {
    safe_set_impl(pkt, std::forward<Key>(key), std::forward<Value>(val),
      dart::meta::conjunction<
        dart::convert::is_castable<Key, std::decay_t<Packet>>,
        dart::convert::is_castable<Value, std::decay_t<Packet>>
      > {}
    );
  }

  template <class Packet, class Key, class Value>
  void safe_insert_impl(Packet&, Key&&, Value&&, std::false_type) {
    throw dart::type_error("Unsupported packet type insertion requested");
  }

  template <class Packet, class Key, class Value>
  void safe_insert_impl(Packet& pkt, Key&& key, Value&& val, std::true_type) {
    pkt.insert(std::forward<Key>(key), std::forward<Value>(val));
  }

  template <class Packet, class Key, class Value>
  void safe_insert(Packet& pkt, Key&& key, Value&& val) {
    safe_insert_impl(pkt, std::forward<Key>(key), std::forward<Value>(val),
      dart::meta::conjunction<
        dart::convert::is_castable<Key, std::decay_t<Packet>>,
        dart::convert::is_castable<Value, std::decay_t<Packet>>
      > {}
    );
  }

  template <class Func>
  auto mutable_visitor(Func&& cb) {
    return compose(
      [=] (dart::heap& pkt) { cb(pkt); },
      [=] (dart::unsafe_heap& pkt) { cb(pkt); },
      [=] (dart::packet& pkt) { cb(pkt); },
      [=] (dart::unsafe_packet& pkt) { cb(pkt); }
    );
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
      [] (std::false_type, auto&&, auto&&...) {
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
          return safe_call(std::forward<Func>(cb), rt_ptr);
        }
      case DART_RC_UNSAFE:
        {
          auto* rt_ptr = reinterpret_cast<dart::unsafe_heap*>(DART_RAW_BYTES(pkt));
          return safe_call(std::forward<Func>(cb), rt_ptr);
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
          return safe_call(std::forward<Func>(cb), rt_ptr);
        }
      case DART_RC_UNSAFE:
        {
          auto* rt_ptr = reinterpret_cast<dart::unsafe_buffer*>(DART_RAW_BYTES(pkt));
          return safe_call(std::forward<Func>(cb), rt_ptr);
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
          return safe_call(std::forward<Func>(cb), rt_ptr);
        }
      case DART_RC_UNSAFE:
        {
          auto* rt_ptr = reinterpret_cast<dart::unsafe_packet*>(DART_RAW_BYTES(pkt));
          return safe_call(std::forward<Func>(cb), rt_ptr);
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
  dart_err_t generic_construct(Func&& cb, void* pkt) {
    auto* rtti = reinterpret_cast<dart_type_id_t*>(pkt);
    switch (rtti->p_id) {
      case DART_HEAP:
        return heap_construct(std::forward<Func>(cb), reinterpret_cast<dart_heap_t*>(pkt));
      case DART_BUFFER:
        return buffer_construct(std::forward<Func>(cb), reinterpret_cast<dart_buffer_t*>(pkt));
      case DART_PACKET:
        return packet_construct(std::forward<Func>(cb), reinterpret_cast<dart_packet_t*>(pkt));
      default:
        errmsg = "Corrupted dart object encountered in generic function call.";
        return DART_CLIENT_ERROR;
    }
  }

  template <class Func, class Ptr>
  dart_err_t iterator_construct(Func&& cb, Ptr* it) {
    // Should do this in two functions, but honestly this was kind of fun.
    auto rc_switch = [&] (auto id) {
      constexpr auto is_const = std::is_const<Ptr>::value;
      using safe_iterator = maybe_const_t<typename packet_t<decltype(id), std::shared_ptr>::iterator, is_const>;
      using unsafe_iterator = maybe_const_t<typename packet_t<decltype(id), dart::unsafe_ptr>::iterator, is_const>;

      switch (it->rtti.rc_id) {
        case DART_RC_SAFE:
          {
            auto* rt_ptr = reinterpret_cast<safe_iterator*>(DART_RAW_BYTES(it));
            return safe_call(std::forward<Func>(cb), rt_ptr, rt_ptr + 1);
          }
        case DART_RC_UNSAFE:
          {
            auto* rt_ptr = reinterpret_cast<unsafe_iterator*>(DART_RAW_BYTES(it));
            return safe_call(std::forward<Func>(cb), rt_ptr, rt_ptr + 1);
          }
        default:
          errmsg = "Unknown reference counter passed for dart_iterator";
          return DART_CLIENT_ERROR;
      }
    };

    // Start this thing off.
    switch (it->rtti.p_id) {
      case DART_HEAP:
        return rc_switch(packet_builder<dart::basic_heap> {});
      case DART_BUFFER:
        return rc_switch(packet_builder<dart::basic_buffer> {});
      case DART_PACKET:
        return rc_switch(packet_builder<dart::basic_packet> {});
      default:
        errmsg = "Unknown packet type passed for dart_iterator";
        return DART_CLIENT_ERROR;
    }
  }

  template <class Func, class Ptr>
  dart_err_t iterator_unwrap(Func&& cb, Ptr* it) {
    return iterator_construct([&] (auto* begin, auto* end) { std::forward<Func>(cb)(*begin, *end); }, it);
  }

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
    auto ret = dart_init_rc_err(pkt, rc);
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

  template <class Func, class Ptr>
  dart_err_t generic_access(Func&& cb, Ptr* pkt) noexcept {
    return err_handler([&cb, pkt] { return generic_unwrap(std::forward<Func>(cb), pkt); });
  }

  template <class Func>
  dart_err_t generic_constructor_access(Func&& cb, void* pkt) noexcept {
    return err_handler([&cb, pkt] { return generic_construct(std::forward<Func>(cb), pkt); });
  }

  template <class Func, class Ptr>
  dart_err_t iterator_access(Func&& cb, Ptr* it) noexcept {
    return err_handler([&cb, it] { return iterator_unwrap(std::forward<Func>(cb), it); });
  }

  template <class Func, class Ptr>
  dart_err_t iterator_constructor_access(Func&& cb, Ptr* it) noexcept {
    return err_handler([&cb, it] { return iterator_construct(std::forward<Func>(cb), it); });
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
    if (*format == ',') format++;
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
    if (*format == ',') format++;
  }

}

/*----- Function Implementations -----*/

extern "C" {

  /*----- Dart Heap Functions -----*/

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
    dst->rtti = {DART_HEAP, rc};
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
    int val = 0;
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

  size_t dart_buffer_size(dart_buffer_t const* src) {
    size_t val = 0;
    auto err = buffer_access([&val] (auto& src) { val = src.size(); }, src);
    if (err) return DART_FAILURE;
    else return val;
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

  dart_packet_t dart_init() {
    // Cannot meaningfully fail.
    dart_packet_t dst;
    dart_init_rc_err(&dst, DART_RC_SAFE);
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
    // Initialize.
    dart_rtti_propagate(dst, src);
    return generic_access(
      [=] (auto& src) { return generic_construct([&src] (auto* dst) { safe_construct(dst, src); }, dst); },
      src
    );
  }

  dart_packet_t dart_move(void* src) {
    dart_packet_t dst;
    auto err = dart_move_err(&dst, src);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_move_err(void* dst, void* src) {
    // Initialize.
    dart_rtti_propagate(dst, src);
    return generic_access(
      [=] (auto& src) { return generic_construct([&src] (auto* dst) { safe_construct(dst, std::move(src)); }, dst); },
      src
    );
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
    return dart_str_init_rc_len_err(dst, DART_RC_SAFE, str, strlen(str));
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

  dart_err_t dart_obj_insert_take_dart(void* dst, char const* key, void* val) {
    return dart_obj_insert_take_dart_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_obj_insert_take_dart_len(void* dst, char const* key, size_t len, void* val) {
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

  dart_err_t dart_obj_insert_str(void* dst, char const* key, char const* val) {
    return dart_obj_insert_str_len(dst, key, strlen(key), val, strlen(val));
  }

  dart_err_t dart_obj_insert_str_len(void* dst, char const* key, size_t len, char const* val, size_t val_len) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.insert(string_view {key, len}, string_view {val, val_len}); }),
      dst
    );
  }

  dart_err_t dart_obj_insert_int(void* dst, char const* key, int64_t val) {
    return dart_obj_insert_int_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_obj_insert_int_len(void* dst, char const* key, size_t len, int64_t val) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.insert(string_view {key, len}, val); }),
      dst
    );
  }

  dart_err_t dart_obj_insert_dcm(void* dst, char const* key, double val) {
    return dart_obj_insert_dcm_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_obj_insert_dcm_len(void* dst, char const* key, size_t len, double val) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.insert(string_view {key, len}, val); }),
      dst
    );
  }

  dart_err_t dart_obj_insert_bool(void* dst, char const* key, int val) {
    return dart_obj_insert_bool_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_obj_insert_bool_len(void* dst, char const* key, size_t len, int val) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.insert(string_view {key, len}, static_cast<bool>(val)); }),
      dst
    );
  }

  dart_err_t dart_obj_insert_null(void* dst, char const* key) {
    return dart_obj_insert_null_len(dst, key, strlen(key));
  }

  dart_err_t dart_obj_insert_null_len(void* dst, char const* key, size_t len) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.insert(string_view {key, len}, nullptr); }),
      dst
    );
  }

  dart_err_t dart_obj_set_dart(void* dst, char const* key, void const* val) {
    return dart_obj_set_dart_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_obj_set_dart_len(void* dst, char const* key, size_t len, void const* val) {
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

  dart_err_t dart_obj_set_take_dart(void* dst, char const* key, void* val) {
    return dart_obj_set_take_dart_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_obj_set_take_dart_len(void* dst, char const* key, size_t len, void* val) {
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

  dart_err_t dart_obj_set_str(void* dst, char const* key, char const* val) {
    return dart_obj_set_str_len(dst, key, strlen(key), val, strlen(val));
  }

  dart_err_t dart_obj_set_str_len(void* dst, char const* key, size_t len, char const* val, size_t val_len) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.set(string_view {key, len}, string_view {val, val_len}); }),
      dst
    );
  }

  dart_err_t dart_obj_set_int(void* dst, char const* key, int64_t val) {
    return dart_obj_set_int_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_obj_set_int_len(void* dst, char const* key, size_t len, int64_t val) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.set(string_view {key, len}, val); }),
      dst
    );
  }

  dart_err_t dart_obj_set_dcm(void* dst, char const* key, double val) {
    return dart_obj_set_dcm_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_obj_set_dcm_len(void* dst, char const* key, size_t len, double val) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.set(string_view {key, len}, val); }),
      dst
    );
  }

  dart_err_t dart_obj_set_bool(void* dst, char const* key, int val) {
    return dart_obj_set_bool_len(dst, key, strlen(key), val);
  }

  dart_err_t dart_obj_set_bool_len(void* dst, char const* key, size_t len, int val) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.set(string_view {key, len}, static_cast<bool>(val)); }),
      dst
    );
  }

  dart_err_t dart_obj_set_null(void* dst, char const* key) {
    return dart_obj_set_null_len(dst, key, strlen(key));
  }

  dart_err_t dart_obj_set_null_len(void* dst, char const* key, size_t len) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.set(string_view {key, len}, nullptr); }),
      dst
    );
  }

  dart_err_t dart_obj_erase(void* dst, char const* key) {
    return dart_obj_erase_len(dst, key, strlen(key));
  }

  dart_err_t dart_obj_erase_len(void* dst, char const* key, size_t len) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.erase(string_view {key, len}); }),
      dst
    );
  }

  dart_err_t dart_arr_insert_dart(void* dst, size_t idx, void const* val) {
    return generic_access(
      mutable_visitor(
        [idx, val] (auto& dst) {
          return generic_access([&dst, idx] (auto& val) { safe_insert(dst, idx, val); }, val);
        }
      ),
      dst
    );
  }

  dart_err_t dart_arr_insert_take_dart(void* dst, size_t idx, void* val) {
    return generic_access(
      mutable_visitor(
        [idx, val] (auto& dst) {
          return generic_access([&dst, idx] (auto& val) { safe_insert(dst, idx, std::move(val)); }, val);
        }
      ),
      dst
    );
  }

  dart_err_t dart_arr_insert_str(void* dst, size_t idx, char const* val) {
    return dart_arr_insert_str_len(dst, idx, val, strlen(val));
  }

  dart_err_t dart_arr_insert_str_len(void* dst, size_t idx, char const* val, size_t val_len) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.insert(idx, string_view {val, val_len}); }),
      dst
    );
  }
  
  dart_err_t dart_arr_insert_int(void* dst, size_t idx, int64_t val) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.insert(idx, val); }),
      dst
    );
  }

  dart_err_t dart_arr_insert_dcm(void* dst, size_t idx, double val) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.insert(idx, val); }),
      dst
    );
  }

  dart_err_t dart_arr_insert_bool(void* dst, size_t idx, int val) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.insert(idx, val); }),
      dst
    );
  }

  dart_err_t dart_arr_insert_null(void* dst, size_t idx) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.insert(idx, nullptr); }),
      dst
    );
  }

  dart_err_t dart_arr_set_dart(void* dst, size_t idx, void const* val) {
    return generic_access(
      mutable_visitor(
        [idx, val] (auto& dst) {
          return generic_access([&dst, idx] (auto& val) { safe_set(dst, idx, val); }, val);
        }
      ),
      dst
    );
  }

  dart_err_t dart_arr_set_take_dart(void* dst, size_t idx, void* val) {
    return generic_access(
      mutable_visitor(
        [idx, val] (auto& dst) {
          return generic_access([&dst, idx] (auto& val) { safe_set(dst, idx, std::move(val)); }, val);
        }
      ),
      dst
    );
  }

  dart_err_t dart_arr_set_str(void* dst, size_t idx, char const* val) {
    return dart_arr_set_str_len(dst, idx, val, strlen(val));
  }

  dart_err_t dart_arr_set_str_len(void* dst, size_t idx, char const* val, size_t val_len) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.set(idx, string_view {val, val_len}); }),
      dst
    );
  }
  
  dart_err_t dart_arr_set_int(void* dst, size_t idx, int64_t val) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.set(idx, val); }),
      dst
    );
  }

  dart_err_t dart_arr_set_dcm(void* dst, size_t idx, double val) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.set(idx, val); }),
      dst
    );
  }

  dart_err_t dart_arr_set_bool(void* dst, size_t idx, int val) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.set(idx, val); }),
      dst
    );
  }

  dart_err_t dart_arr_set_null(void* dst, size_t idx) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.set(idx, nullptr); }),
      dst
    );
  }

  dart_err_t dart_arr_erase(void* dst, size_t idx) {
    return generic_access(
      mutable_visitor([=] (auto& dst) { dst.erase(idx); }),
      dst
    );
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

  dart_packet_t dart_arr_get(void const* src, int64_t idx) {
    dart_packet_t dst;
    auto err = dart_arr_get_err(&dst, src, idx);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_arr_get_err(dart_packet_t* dst, void const* src, int64_t idx) {
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

  char const* dart_str_get(void const* src) {
    size_t dummy;
    return dart_str_get_len(src, &dummy);
  }

  char const* dart_str_get_len(void const* src, size_t* len) {
    char const* ptr;
    auto ret = generic_access([&] (auto& src) {
      auto strv = src.strv();
      ptr = strv.data();
      *len = strv.size();
    }, src);
    if (ret) return nullptr;
    else return ptr;
  }

  int64_t dart_int_get(void const* src) {
    // No way unique way to signal failure here,
    // the user needs to know this will succeed.
    int64_t val = 0;
    dart_int_get_err(src, &val);
    return val;
  }

  dart_err_t dart_int_get_err(void const* src, int64_t* val) {
    return generic_access([=] (auto& str) { *val = str.integer(); }, src);
  }

  double dart_dcm_get(void const* src) {
    double val = std::numeric_limits<double>::quiet_NaN();
    dart_dcm_get_err(src, &val);
    return val;
  }

  dart_err_t dart_dcm_get_err(void const* src, double* val) {
    return generic_access([=] (auto& str) { *val = str.decimal(); }, src);
  }

  int dart_bool_get(void const* src) {
    // No way unique way to signal failure here,
    // the user needs to know this will succeed.
    int val = 0;
    dart_bool_get_err(src, &val);
    return val;
  }

  dart_err_t dart_bool_get_err(void const* src, int* val) {
    return generic_access([=] (auto& src) { *val = src.boolean(); }, src);
  }

  size_t dart_size(void const* src) {
    size_t val = 0;
    auto err = generic_access([&val] (auto& src) { val = src.size(); }, src);
    if (err) return DART_FAILURE;
    else return val;
  }

  bool dart_equal(void const* lhs, void const* rhs) {
    bool equal = false;
    dart::detail::typeless_comparator comp {};
    auto err = generic_access(
      [&] (auto& lhs) { generic_access([&] (auto& rhs) { equal = comp(lhs, rhs); }, rhs); },
      lhs
    );
    if (err) return false;
    else return equal;
  }

  bool dart_is_obj(void const* src) {
    return dart_get_type(src) == DART_OBJECT;
  }

  bool dart_is_arr(void const* src) {
    return dart_get_type(src) == DART_ARRAY;
  }

  bool dart_is_str(void const* src) {
    return dart_get_type(src) == DART_STRING;
  }

  bool dart_is_int(void const* src) {
    return dart_get_type(src) == DART_INTEGER;
  }

  bool dart_is_dcm(void const* src) {
    return dart_get_type(src) == DART_DECIMAL;
  }

  bool dart_is_bool(void const* src) {
    return dart_get_type(src) == DART_BOOLEAN;
  }

  bool dart_is_null(void const* src) {
    return dart_get_type(src) == DART_NULL;
  }

  dart_type_t dart_get_type(void const* src) {
    dart_type_t type;
    auto err = generic_access([&] (auto& str) { type = abi_type(str.get_type()); }, src);
    if (err) return DART_INVALID;
    else return type;
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
      src
    );
    if (ret) return nullptr;
    return outstr;
  }

  dart_err_t dart_iterator_init_err(dart_iterator_t* dst, void const* src) {
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

  dart_err_t dart_iterator_init_key_err(dart_iterator_t* dst, void const* src) {
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

  dart_err_t dart_iterator_copy_err(dart_iterator_t* dst, dart_iterator_t const* src) {
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

  dart_err_t dart_iterator_move_err(dart_iterator_t* dst, dart_iterator_t* src) {
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

  dart_err_t dart_iterator_destroy(dart_iterator_t* dst) {
    return iterator_access([] (auto& start, auto& end) {
      using type = std::decay_t<decltype(start)>;
      start.~type();
      end.~type();
    }, dst);
  }

  dart_packet_t dart_iterator_get(dart_iterator_t const* src) {
    dart_packet_t dst;
    auto err = dart_iterator_get_err(&dst, src);
    if (err) return dart_init();
    else return dst;
  }

  dart_err_t dart_iterator_get_err(dart_packet_t* dst, dart_iterator_t const* src) {
    // Initialize.
    dart_rtti_propagate(dst, src);
    return iterator_access(
      [dst] (auto& src_curr, auto& src_end) {
        using type = typename std::decay_t<decltype(src_curr)>::value_type;
        if (src_curr == src_end) throw std::runtime_error("dart_iterator has been exhausted");
        return packet_construct([&src_curr] (type* dst) { new(dst) type(*src_curr); }, dst);
      },
      src
    );
  }

  dart_err_t dart_iterator_next(dart_iterator_t* dst) {
    return iterator_access([] (auto& curr, auto& end) { if (curr != end) curr++; }, dst);
  }

  bool dart_iterator_done(dart_iterator_t const* src) {
    bool ended = false;
    auto err = iterator_access([&] (auto& curr, auto& end) { ended = (curr == end); }, src);
    if (err) return true;
    else return ended;
  }

  bool dart_iterator_done_destroy(dart_iterator_t* dst, dart_packet_t* pkt) {
    if (!dart_iterator_done(dst)) return false;
    dart_iterator_destroy(dst);
    if (pkt) dart_destroy(pkt);
    return true;
  }

  char const* dart_get_error() {
    return errmsg;
  }

}
