#ifndef DART_ABI_HELPERS_H
#define DART_ABI_HELPERS_H

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

namespace dart {
  namespace detail {
    extern thread_local std::string errmsg;
  }
}


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
    throw dart::type_error("Unsupported packet type insertion requested");
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

  template <class Packet, class Value>
  void safe_assign_impl(Packet&, Value&&, std::false_type) {
    throw dart::type_error("Unsupported packet assignment requested");
  }

  template <class Packet, class Value>
  void safe_assign_impl(Packet& pkt, Value&& val, std::true_type) {
    pkt = dart::convert::cast<Packet>(std::forward<Value>(val));
  }

  template <class Packet, class Value>
  void safe_assign(Packet& pkt, Value&& val) {
    safe_assign_impl(pkt, std::forward<Value>(val),
        dart::convert::is_castable<Value, std::decay_t<Packet>> {});
  }

  template <class Func>
  auto mutable_visitor(Func&& cb) {
    return compose(
      [=] (dart::heap& pkt) { return cb(pkt); },
      [=] (dart::unsafe_heap& pkt) { return cb(pkt); },
      [=] (dart::packet& pkt) { return cb(pkt); },
      [=] (dart::unsafe_packet& pkt) { return cb(pkt); }
    );
  }

  template <class Func>
  auto immutable_visitor(Func&& cb) {
    return compose(
      [=] (dart::buffer const& pkt) { return cb(pkt); },
      [=] (dart::unsafe_buffer const& pkt) { return cb(pkt); },
      [=] (dart::packet const& pkt) { return cb(pkt); },
      [=] (dart::unsafe_packet const& pkt) { return cb(pkt); }
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
      dart::detail::errmsg = "Avoided a type-mismatched call of some sort. "
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
        dart::detail::errmsg = "Unknown reference counter passed for dart_heap";
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
        dart::detail::errmsg = "Unknown reference counter passed for dart_buffer";
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
        dart::detail::errmsg = "Unknown reference counter passed for dart_packet";
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
        dart::detail::errmsg = "Unknown reference counter passed for dart_heap";
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
        dart::detail::errmsg = "Unknown reference counter passed for dart_buffer";
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
        dart::detail::errmsg = "Unknown reference counter passed for dart_packet";
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
        dart::detail::errmsg = "Corrupted dart object encountered in generic function call.";
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
        dart::detail::errmsg = "Corrupted dart object encountered in generic function call.";
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
          dart::detail::errmsg = "Unknown reference counter passed for dart_iterator";
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
        dart::detail::errmsg = "Unknown packet type passed for dart_iterator";
        return DART_CLIENT_ERROR;
    }
  }

  template <class Func, class Ptr>
  dart_err_t iterator_unwrap(Func&& cb, Ptr* it) {
    return iterator_construct([&] (auto* begin, auto* end) { std::forward<Func>(cb)(*begin, *end); }, it);
  }

  template <class Func>
  dart_err_t err_handler(Func&& cb) noexcept try {
    return std::forward<Func>(cb)();
  } catch (dart::type_error const& err) {
    dart::detail::errmsg = err.what();
    return DART_TYPE_ERROR;
  } catch (dart::state_error const& err) {
    dart::detail::errmsg = err.what();
    return DART_STATE_ERROR;
  } catch (dart::parse_error const& err) {
    dart::detail::errmsg = err.what();
    return DART_PARSE_ERROR;
  } catch (std::logic_error const& err) {
    dart::detail::errmsg = err.what();
    return DART_LOGIC_ERROR;
  } catch (std::runtime_error const& err) {
    dart::detail::errmsg = err.what();
    return DART_RUNTIME_ERROR;
  } catch (...) {
    dart::detail::errmsg = "Dart caught an unexpected error type. This is a bug, please make a report";
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
  inline parse_type identify_vararg(char const*& c) {
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
      case 'n':
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

#endif
