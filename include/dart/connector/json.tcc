#ifndef DART_JSON_IMPL_H
#define DART_JSON_IMPL_H

/*----- Local Includes -----*/

#include "../common.h"

/*----- Type Declarations -----*/

#if DART_HAS_RAPIDJSON
namespace dart {
  namespace detail {
    template <template <class> class RefCount>
    struct heap_parser {

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
#endif

#ifdef DART_USE_SAJSON
namespace dart {
  namespace detail {
    template <class Stack>
    sajson::document sajson_parse(sajson::string json, Stack& stack);
    template <template <class> class RefCount>
    void sajson_lower(sajson::value curr_val, heap_parser<RefCount>& parser);
  }
}
#endif

/*----- Function Implementations -----*/

namespace dart {

#ifdef DART_USE_SAJSON
  template <template <class> class RefCount>
  template <unsigned parse_stack_size, bool, class EnableIf>
  basic_heap<RefCount> basic_heap<RefCount>::from_json(shim::string_view json) {
    // Allocate however much stack space was requested.
    std::array<size_t, parse_stack_size / sizeof(size_t)> stack;

    // Have sajson parse our string, potentially using the stack space.
    sajson::string view {json.data(), json.size()};
    auto doc = detail::sajson_parse(view, stack);

    // Convert into dart::heap and return.
    detail::heap_parser<RefCount> builder;
    detail::sajson_lower(doc.get_root(), builder);
    return std::move(builder.curr_obj);
  }

  template <template <class> class RefCount>
  template <unsigned parse_stack_size, bool, class EnableIf>
  basic_buffer<RefCount> basic_buffer<RefCount>::from_json(shim::string_view json) {
    // Allocate however much stack space was requested.
    std::array<size_t, parse_stack_size / sizeof(size_t)> stack;

    // Have sajson parse our string, potentially using the stack space.
    sajson::string view {json.data(), json.size()};
    auto doc = detail::sajson_parse(view, stack);

    // Allocate space to store our finalized representation.
    // FIXME: Make this allocation size more intelligent.
    auto block = detail::aligned_alloc<RefCount>(json.size() * 8, detail::raw_type::object, [&] (auto* buf) {
      detail::json_lower<RefCount>(buf, doc.get_root());
    });

    // Export our buffer to the user.
    return basic_buffer {std::move(block)};
  }

  template <template <class> class RefCount>
  template <unsigned parse_stack_size, bool, class EnableIf>
  basic_packet<RefCount> basic_packet<RefCount>::from_json(shim::string_view json, bool finalized) {
    if (finalized) return basic_buffer<RefCount>::template from_json<parse_stack_size>(json);
    else return basic_heap<RefCount>::template from_json<parse_stack_size>(json);
  }

  template <template <class> class RefCount>
  template <unsigned parse_stack_size, bool, class EnableIf>
  basic_heap<RefCount> basic_heap<RefCount>::parse(shim::string_view json) {
    return basic_heap::from_json<parse_stack_size>(json);
  }

  template <template <class> class RefCount>
  template <unsigned parse_stack_size, bool, class EnableIf>
  basic_buffer<RefCount> basic_buffer<RefCount>::parse(shim::string_view json) {
    return basic_buffer::from_json<parse_stack_size>(json);
  }

  template <template <class> class RefCount>
  template <unsigned parse_stack_size, bool, class EnableIf>
  basic_packet<RefCount> basic_packet<RefCount>::parse(shim::string_view json, bool finalized) {
    return basic_packet::from_json(json, finalized);
  }
#elif DART_HAS_RAPIDJSON
  template <template <class> class RefCount>
  template <unsigned flags, bool enabled, class EnableIf>
  basic_heap<RefCount> basic_heap<RefCount>::from_json(shim::string_view json) {
    // Construct a reader class to parse this string.
    rapidjson::Reader reader;

    // Construct a dart::packet heap_parser context to go with it.
    detail::heap_parser<RefCount> context;

    // Parse!
    rapidjson::StringStream ss(json.data());
    if (reader.Parse<flags>(ss, context)) {
      return context.curr_obj;
    } else {
      auto err = reader.GetParseErrorCode();
      auto off = reader.GetErrorOffset();
      std::string errmsg = "dart::heap could not parse the given string due to: \"";
      errmsg += rapidjson::GetParseError_En(err);
      errmsg += "\" near  \"";
      errmsg += std::string {json.substr(off ? off - 1 : off, 10)};
      errmsg += "\"";
      throw parse_error(errmsg.data());
    }
  }

  template <template <class> class RefCount>
  template <unsigned flags, bool enabled, class EnableIf>
  basic_buffer<RefCount> basic_buffer<RefCount>::from_json(shim::string_view json) {
    // Duplicate the given string locally so we can use the RapidJSON in-situ parser.
    auto buf = std::make_unique<char[]>(json.size() + 1);
    std::copy_n(json.data(), json.size(), buf.get());
    buf[json.size()] = '\0';

    // Fire up RapidJSON.
    rapidjson::Document doc;
    if (doc.ParseInsitu<flags>(buf.get()).HasParseError()) {
      auto err = doc.GetParseError();
      auto off = doc.GetErrorOffset();
      std::string errmsg = "dart::buffer could not parse the given string due to: \"";
      errmsg += rapidjson::GetParseError_En(err);
      errmsg += "\" near \"";
      errmsg += std::string {json.substr(off ? off - 1 : off, 10)};
      errmsg += "...\"";
      throw parse_error(errmsg.data());
    } else if (!doc.IsObject()) {
      throw type_error("dart::buffer root must be an object.");
    }

    // Allocate space to store our finalized representation.
    // FIXME: Make this allocation size more intelligent.
    auto block = detail::aligned_alloc<RefCount>(json.size() * 8, detail::raw_type::object, [&] (auto* buf) {
      detail::json_lower<RefCount>(buf, doc);
    });

    // Export our buffer to the user.
    return basic_buffer {std::move(block)};
  }

  template <template <class> class RefCount>
  template <unsigned flags, bool enabled, class EnableIf>
  basic_packet<RefCount> basic_packet<RefCount>::from_json(shim::string_view json, bool finalized) {
    if (finalized) return basic_buffer<RefCount>::template from_json<flags>(json);
    else return basic_heap<RefCount>::template from_json<flags>(json);
  }

  template <template <class> class RefCount>
  template <unsigned flags, bool enabled, class EnableIf>
  basic_heap<RefCount> basic_heap<RefCount>::parse(shim::string_view json) {
    return basic_heap::from_json<flags>(json);
  }

  template <template <class> class RefCount>
  template <unsigned flags, bool enabled, class EnableIf>
  basic_buffer<RefCount> basic_buffer<RefCount>::parse(shim::string_view json) {
    return basic_buffer::from_json<flags>(json);
  }

  template <template <class> class RefCount>
  template <unsigned flags, bool enabled, class EnableIf>
  basic_packet<RefCount> basic_packet<RefCount>::parse(shim::string_view json, bool finalized) {
    return basic_packet::from_json<flags>(json, finalized);
  }
#endif

#if DART_HAS_RAPIDJSON
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
    return value_type::make_null().to_json();
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
#endif

}

namespace dart {
  namespace detail {
#if DART_HAS_RAPIDJSON
    template <template <class> class RefCount>
    bool heap_parser<RefCount>::StartObject() {
      if (curr_key) key_stack.push_back(std::move(curr_key));
      if (curr_obj) obj_stack.push_back(std::move(curr_obj));
      curr_obj = basic_heap<RefCount>::make_object();
      return true;
    }

    template <template <class> class RefCount>
    bool heap_parser<RefCount>::Key(char const* str, size_type len, bool) {
      curr_key = basic_heap<RefCount>::make_string({str, len});
      return true;
    }

    template <template <class> class RefCount>
    bool heap_parser<RefCount>::EndObject(size_type) {
      EndAggregate();
      return true;
    }

    template <template <class> class RefCount>
    bool heap_parser<RefCount>::StartArray() {
      if (curr_key) key_stack.push_back(std::move(curr_key));
      if (curr_obj) obj_stack.push_back(std::move(curr_obj));
      curr_obj = basic_heap<RefCount>::make_array();
      return true;
    }

    template <template <class> class RefCount>
    bool heap_parser<RefCount>::EndArray(size_type) {
      EndAggregate();
      return true;
    }

    template <template <class> class RefCount>
    bool heap_parser<RefCount>::EndAggregate() {
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
    bool heap_parser<RefCount>::String(char const* str, size_type len, bool) {
      // Embed the string.
      auto str_obj = basic_heap<RefCount>::make_string({str, len});
      if (curr_obj.is_object()) curr_obj.add_field(std::move(curr_key), std::move(str_obj));
      else curr_obj.push_back(std::move(str_obj));
      return true;
    }

    template <template <class> class RefCount>
    bool heap_parser<RefCount>::Int(int num) {
      return Int64(num);
    }

    template <template <class> class RefCount>
    bool heap_parser<RefCount>::Uint(unsigned num) {
      return Int64(num);
    }

    template <template <class> class RefCount>
    bool heap_parser<RefCount>::Int64(int64_t num) {
      // Embed the integer.
      if (curr_obj.is_object()) curr_obj.add_field(std::move(curr_key), num);
      else curr_obj.push_back(num);
      return true;
    }

    template <template <class> class RefCount>
    bool heap_parser<RefCount>::Uint64(uint64_t num) {
      return Int64(num);
    }

    template <template <class> class RefCount>
    bool heap_parser<RefCount>::Double(double num) {
      // Embed the double.
      if (curr_obj.is_object()) curr_obj.add_field(std::move(curr_key), num);
      else curr_obj.push_back(num);
      return true;
    }

    template <template <class> class RefCount>
    bool heap_parser<RefCount>::RawNumber(char const*, size_type, bool) {
      // Rapidjson doesn't detect characteristics of incoming parse-type, so this has to be implemented
      // regardless of whether it'll be used.
      throw std::logic_error("dart::packet library is misconfigured, unimplemented RawNumber handler called");
    }

    template <template <class> class RefCount>
    bool heap_parser<RefCount>::Bool(bool val) {
      // Embed the boolean.
      if (curr_obj.is_object()) curr_obj.add_field(std::move(curr_key), val);
      else curr_obj.push_back(val);
      return true;
    }

    template <template <class> class RefCount>
    bool heap_parser<RefCount>::Null() {
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
#endif

#ifdef DART_USE_SAJSON
    template <class Stack>
    sajson::document sajson_parse(sajson::string json, Stack& stack) {
      // If we'll be able to fit the sajson parse tree in our statically allocated
      // space, do so.
      shim::optional<sajson::document> doc;
      if (json.length() <= stack.max_size()) {
        doc.emplace(sajson::parse(sajson::bounded_allocation {stack.data(), stack.size()}, json));
      } else {
        doc.emplace(sajson::parse(sajson::single_allocation {}, json));
      }

      // If the parse failed, figure out why.
      if (!doc->is_valid()) {
        std::string errmsg {"dart::heap could not parse the given string due to: \""};
        errmsg += doc->get_error_message_as_cstring();
        errmsg += "\"";
        throw parse_error(errmsg.data());
      }
      return std::move(*doc);
    }

    template <template <class> class RefCount>
    void sajson_lower(sajson::value curr_val, heap_parser<RefCount>& parser) {
      switch (curr_val.get_type()) {
        case sajson::TYPE_OBJECT:
          // Begin the object.
          parser.StartObject();

          // Recurse over each key/value in the object.
          for (auto idx = 0U; idx < curr_val.get_length(); ++idx) {
            // Layout the key.
            auto key = curr_val.get_object_key(idx);
            parser.Key(key.data(), key.length(), false);

            // Recurse through the value
            sajson_lower(curr_val.get_object_value(idx), parser);
          }

          // Finish our object.
          parser.EndAggregate();
          break;
        case sajson::TYPE_ARRAY:
          // Begin the array.
          parser.StartArray();

          // Recurse over each value in the array.
          for (auto idx = 0U; idx < curr_val.get_length(); ++idx) {
            sajson_lower(curr_val.get_array_element(idx), parser);
          }

          // Finish the array.
          parser.EndAggregate();
          break;
        case sajson::TYPE_STRING:
          // Figure out what type of string we've been given.
          parser.String(curr_val.as_cstring(), curr_val.get_string_length(), false);
          break;
        case sajson::TYPE_INTEGER:
          parser.Int64(curr_val.get_integer_value());
          break;
        case sajson::TYPE_DOUBLE:
          parser.Double(curr_val.get_double_value());
          break;
        case sajson::TYPE_FALSE:
          parser.Bool(false);
          break;
        case sajson::TYPE_TRUE:
          parser.Bool(true);
          break;
        default:
          DART_ASSERT(curr_val.get_type() == sajson::TYPE_NULL);
          parser.Null();
      }
    }
#endif

  }
}

#endif
