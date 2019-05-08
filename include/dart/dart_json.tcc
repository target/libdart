#ifndef DART_JSON_IMPL_H
#define DART_JSON_IMPL_H

/*----- Local Includes -----*/

#include "dart_intern.h"

/*----- Type Declarations -----*/

#if DART_HAS_RAPIDJSON
namespace dart {
  namespace detail {
    template <template <class> class RefCount>
    struct json_parser {

      /*----- Types -----*/

      using size_type = rapidjson::SizeType;

      /*----- API -----*/

      // JSON event handlers.
      // API mandated by RapidJSON, not my preferred naming scheme.
      bool StartObject();
      bool Key(char const* str, size_type len, bool);
      bool EndObject(size_type);
      bool StartArray();
      bool EndArray(size_type);
      bool EndAggregate();
      bool String(char const* str, size_type len, bool);
      bool Int(int num);
      bool Uint(unsigned u);
      bool Int64(int64_t num);
      bool Uint64(uint64_t num);
      bool Double(double num);
      bool RawNumber(char const*, size_type, bool);
      bool Bool(bool val);
      bool Null();

      /*----- Members -----*/

      basic_heap<RefCount> curr_key, curr_obj;
      std::vector<basic_heap<RefCount>> key_stack, obj_stack;
    };
  }
}

/*----- Function Declarations -----*/

namespace dart {
  namespace detail {
    template <class Writer, class Packet>
    void json_serialize(Writer& writer, Packet const& packet);
  }
}

/*----- Function Implementations -----*/

namespace dart {

  template <template <class> class RefCount>
  template <unsigned flags>
  basic_heap<RefCount> basic_heap<RefCount>::from_json(shim::string_view json) {
    // Construct a reader class to parse this string.
    rapidjson::Reader reader;

    // Construct a dart::packet json_parser context to go with it.
    detail::json_parser<RefCount> context;

    // Parse!
    rapidjson::StringStream ss(json.data());
    reader.Parse<flags>(ss, context);

    // Convert.
    return context.curr_obj;
  }

  template <template <class> class RefCount>
  template <unsigned flags>
  basic_buffer<RefCount> basic_buffer<RefCount>::from_json(shim::string_view json) {
    return basic_buffer {basic_heap<RefCount>::template from_json<flags>(json)};
  }

  template <template <class> class RefCount>
  template <unsigned flags>
  basic_packet<RefCount> basic_packet<RefCount>::from_json(shim::string_view json, bool finalized) {
    auto tmp = basic_heap<RefCount>::template from_json<flags>(json);
    if (finalized) return basic_buffer<RefCount> {std::move(tmp)};
    else return basic_packet {std::move(tmp)};
  }

  template <class Object>
  template <unsigned flags>
  std::string basic_object<Object>::to_json() const {
    return val.to_json();
  }

  template <class Array>
  template <unsigned flags>
  std::string basic_array<Array>::to_json() const {
    return val.to_json();
  }

  template <class String>
  template <unsigned flags>
  std::string basic_string<String>::to_json() const {
    return val.to_json();
  }

  template <class Number>
  template <unsigned flags>
  std::string basic_number<Number>::to_json() const {
    return val.to_json();
  }

  template <class Boolean>
  template <unsigned flags>
  std::string basic_flag<Boolean>::to_json() const {
    return val.to_json();
  }

  template <class Null>
  template <unsigned flags>
  std::string basic_null<Null>::to_json() const {
    return (typename value_type::null()).to_json();
  }

  template <template <class> class RefCount>
  template <unsigned flags>
  std::string basic_heap<RefCount>::to_json() const {
    namespace rj = rapidjson;

    // Get some space to output our json.
    rj::StringBuffer buf;
    rj::Writer<rj::StringBuffer, rj::UTF8<>, rj::UTF8<>, rj::CrtAllocator, flags> writer(buf);

    // Perform the serialization.
    detail::json_serialize(writer, *this);

    // Copy and return.
    // FIXME: Check and see if there's any more efficient way to do this.
    return buf.GetString();
  }

  template <template <class> class RefCount>
  template <unsigned flags>
  std::string basic_buffer<RefCount>::to_json() const {
    namespace rj = rapidjson;

    // Get some space to output our json.
    rj::StringBuffer buf;
    rj::Writer<rj::StringBuffer, rj::UTF8<>, rj::UTF8<>, rj::CrtAllocator, flags> writer(buf);

    // Perform the serialization.
    detail::json_serialize(writer, *this);

    // Copy and return.
    // FIXME: Check and see if there's any more efficient way to do this.
    return buf.GetString();
  }

  template <template <class> class RefCount>
  template <unsigned flags>
  std::string basic_packet<RefCount>::to_json() const {
    return shim::visit([] (auto& v) { return v.template to_json<flags>(); }, impl);
  }

}

namespace dart {
  namespace detail {
    template <template <class> class RefCount>
    bool json_parser<RefCount>::StartObject() {
      if (curr_key) key_stack.push_back(std::move(curr_key));
      if (curr_obj) obj_stack.push_back(std::move(curr_obj));
      curr_obj = basic_heap<RefCount>::make_object();
      return true;
    }

    template <template <class> class RefCount>
    bool json_parser<RefCount>::Key(char const* str, size_type len, bool) {
      curr_key = basic_heap<RefCount>::make_string({str, len});
      return true;
    }

    template <template <class> class RefCount>
    bool json_parser<RefCount>::EndObject(size_type) {
      EndAggregate();
      return true;
    }

    template <template <class> class RefCount>
    bool json_parser<RefCount>::StartArray() {
      if (curr_key) key_stack.push_back(std::move(curr_key));
      if (curr_obj) obj_stack.push_back(std::move(curr_obj));
      curr_obj = basic_heap<RefCount>::make_array();
      return true;
    }

    template <template <class> class RefCount>
    bool json_parser<RefCount>::EndArray(size_type) {
      EndAggregate();
      return true;
    }

    template <template <class> class RefCount>
    bool json_parser<RefCount>::EndAggregate() {
      if (!obj_stack.empty()) {
        // Move the current object into its parent.
        auto& parent = obj_stack.back();
        if (parent.is_object()) parent.add_field(std::move(key_stack.back()), std::move(curr_obj));
        else parent.push_back(std::move(curr_obj));

        // Move the parent back to being the current object.
        curr_obj = std::move(parent);

        // Adjust our stacks.
        obj_stack.pop_back();
        if (curr_obj.is_object()) key_stack.pop_back();
      }
      return true;
    }

    template <template <class> class RefCount>
    bool json_parser<RefCount>::String(char const* str, size_type len, bool) {
      // Embed the string.
      auto str_obj = basic_heap<RefCount>::make_string({str, len});
      if (curr_obj.is_object()) curr_obj.add_field(std::move(curr_key), std::move(str_obj));
      else curr_obj.push_back(std::move(str_obj));
      return true;
    }

    template <template <class> class RefCount>
    bool json_parser<RefCount>::Int(int num) {
      return Int64(num);
    }

    template <template <class> class RefCount>
    bool json_parser<RefCount>::Uint(unsigned num) {
      return Int64(num);
    }

    template <template <class> class RefCount>
    bool json_parser<RefCount>::Int64(int64_t num) {
      // Embed the integer.
      if (curr_obj.is_object()) curr_obj.add_field(std::move(curr_key), num);
      else curr_obj.push_back(num);
      return true;
    }

    template <template <class> class RefCount>
    bool json_parser<RefCount>::Uint64(uint64_t num) {
      return Int64(num);
    }

    template <template <class> class RefCount>
    bool json_parser<RefCount>::Double(double num) {
      // Embed the double.
      if (curr_obj.is_object()) curr_obj.add_field(std::move(curr_key), num);
      else curr_obj.push_back(num);
      return true;
    }

    template <template <class> class RefCount>
    bool json_parser<RefCount>::RawNumber(char const*, size_type, bool) {
      // Rapidjson doesn't detect characteristics of incoming parse-type, so this has to be implemented
      // regardless of whether it'll be used.
      throw std::logic_error("dart::packet library is misconfigured, unimplemented RawNumber handler called");
    }

    template <template <class> class RefCount>
    bool json_parser<RefCount>::Bool(bool val) {
      // Embed the boolean.
      if (curr_obj.is_object()) curr_obj.add_field(std::move(curr_key), val);
      else curr_obj.push_back(val);
      return true;
    }

    template <template <class> class RefCount>
    bool json_parser<RefCount>::Null() {
      auto null = basic_heap<RefCount>::make_null();
      if (curr_obj.is_object()) curr_obj.add_field(std::move(curr_key), null);
      else curr_obj.push_back(null);
      return true;
    }

    template <class Writer, class Packet>
    void json_serialize(Writer& writer, Packet const& packet) {
      switch (packet.get_type()) {
        case detail::type::object:
          {
            writer.StartObject();
            typename Packet::iterator k, v;
            std::tie(k, v) = packet.kvbegin();
            while (v != packet.end()) {
              writer.Key(k->str(), static_cast<rapidjson::SizeType>(k->size()));
              json_serialize(writer, *v);
              ++k, ++v;
            }
            writer.EndObject();
            break;
          }
        case detail::type::array:
          writer.StartArray();
          for (auto elem : packet) json_serialize(writer, elem);
          writer.EndArray();
          break;
        case detail::type::string:
          writer.String(packet.str(), static_cast<rapidjson::SizeType>(packet.size()));
          break;
        case detail::type::integer:
          writer.Int64(packet.integer());
          break;
        case detail::type::decimal:
          writer.Double(packet.decimal());
          break;
        case detail::type::boolean:
          writer.Bool(packet.boolean());
          break;
        default:
          writer.Null();
          break;
      }
    }

  }
}
#endif

#endif
