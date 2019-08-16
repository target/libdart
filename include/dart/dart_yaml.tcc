#ifndef DART_YAML_IMPL_H
#define DART_YAML_IMPL_H
#if DART_HAS_YAML

/*----- System Includes -----*/

#include <yaml.h>
#include <errno.h>

/*----- Local Includes -----*/

#include "dart_intern.h"

/*----- Function Implementations -----*/

namespace dart {

  namespace detail {

    enum class parse_state {
      scalar,
      mapping_key,
      mapping_value,
      sequence_value
    };

    template <template <class> class RefCount>
    basic_heap<RefCount> parse_yaml(shim::string_view yaml);

    inline std::tuple<type, int64_t, double> parse_maybe_number(char const* str, ssize_t len);

    template <template <class> class RefCount>
    basic_heap<RefCount> parse_scalar(yaml_event_t const& event);

  };

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_heap<RefCount> basic_heap<RefCount>::from_yaml(shim::string_view yaml) {
    return detail::parse_yaml<RefCount>(yaml);
  }
  
  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_buffer<RefCount> basic_buffer<RefCount>::from_yaml(shim::string_view yaml) {
    return basic_buffer {detail::parse_yaml<RefCount>(yaml)};
  }
  
  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount> basic_packet<RefCount>::from_yaml(shim::string_view yaml, bool finalized) {
    auto tmp = basic_heap<RefCount>::from_yaml(yaml);
    if (finalized) return basic_buffer<RefCount> {std::move(tmp)};
    else return std::move(tmp);
  }

  namespace detail {

    template <template <class> class RefCount>
    basic_heap<RefCount> parse_yaml(shim::string_view yaml) {
      // Initialize libyaml.
      yaml_event_t event;
      yaml_parser_t parser;
      if (!yaml_parser_initialize(&parser)) {
        throw std::runtime_error("dart::packet failed to initialize yaml parser");
      }
      yaml_parser_set_input_string(&parser, reinterpret_cast<unsigned char const*>(yaml.data()), yaml.size());

      // Explicit stacks to manage parser state.
      basic_heap<RefCount> curr_key, curr_obj;
      std::vector<detail::parse_state> state_stack;
      std::vector<basic_heap<RefCount>> key_stack, obj_stack;

      // Parse through the input string and handle stream events.
      yaml_event_type_t event_type = YAML_NO_EVENT;
      detail::parse_state state = detail::parse_state::scalar;
      try {
        do {
          // Get our next event.
          if (!yaml_parser_parse(&parser, &event)) throw std::runtime_error(parser.problem);
          event_type = event.type;

          // Run our state machine.
          // Yuck.
          switch (event_type) {
            case YAML_STREAM_START_EVENT:
            case YAML_STREAM_END_EVENT:
            case YAML_DOCUMENT_START_EVENT:
            case YAML_DOCUMENT_END_EVENT:
              yaml_event_delete(&event);
              continue;
            case YAML_MAPPING_START_EVENT:
              // Beginning a new object, push onto our stacks if we were holding anything.
              if (curr_key) key_stack.push_back(std::move(curr_key));
              if (curr_obj) obj_stack.push_back(std::move(curr_obj));
              state_stack.push_back(state);
              state = detail::parse_state::mapping_key;
              curr_obj = basic_heap<RefCount>::make_object();
              break;
            case YAML_SEQUENCE_START_EVENT:
              // Beginning a new array, push onto our stacks if we were holding anything.
              if (curr_key) key_stack.push_back(std::move(curr_key));
              if (curr_obj) obj_stack.push_back(std::move(curr_obj));
              state_stack.push_back(state);
              state = detail::parse_state::sequence_value;
              curr_obj = basic_heap<RefCount>::make_array();
              break;
            case YAML_MAPPING_END_EVENT:
            case YAML_SEQUENCE_END_EVENT:
              // If there's anything on our stack...
              if (!obj_stack.empty()) {
                // Move the current object into its parent.
                auto& parent = obj_stack.back();
                if (parent.is_object()) parent.add_field(std::move(key_stack.back()), std::move(curr_obj));
                else parent.push_back(std::move(curr_obj));

                // Move the parent back to being the current object.
                curr_obj = std::move(parent);

                // Update.
                obj_stack.pop_back();
                if (curr_obj.is_object()) key_stack.pop_back();
              }

              // Update our state.
              state = state_stack.back();
              if (state == detail::parse_state::mapping_value) state = detail::parse_state::mapping_key;
              state_stack.pop_back();
              break;
            case YAML_SCALAR_EVENT:
              if (state == detail::parse_state::scalar) {
                throw std::runtime_error("dart::packet does not support naked YAML scalars.");
              } else if (state == detail::parse_state::mapping_key) {
                // Store the key for later.
                char const* val = reinterpret_cast<char const*>(event.data.scalar.value);
                curr_key = basic_heap<RefCount>::make_string({val, event.data.scalar.length});
                state = detail::parse_state::mapping_value;
              } else if (state == detail::parse_state::mapping_value) {
                curr_obj.add_field(std::move(curr_key), detail::parse_scalar<RefCount>(event));
                state = detail::parse_state::mapping_key;
              } else if (state == detail::parse_state::sequence_value) {
                curr_obj.push_back(detail::parse_scalar<RefCount>(event));
              }
              break;
            case YAML_ALIAS_EVENT:
              // FIXME: This.
              throw std::runtime_error("dart::packet does not currently support YAML aliases");
            default:
              throw std::runtime_error("dart::packet encountered an unexpected YAML parse state");
          }

          // Cleanup anything allocated for this event.
          yaml_event_delete(&event);
        } while (event_type != YAML_STREAM_END_EVENT && event_type != YAML_DOCUMENT_END_EVENT);
      } catch (std::exception) {
        // Release memory and rethrow;
        yaml_event_delete(&event);
        yaml_parser_delete(&parser);
        throw;
      }

      // Cleanup and return our parsed dart::packet.
      yaml_parser_delete(&parser);
      if (!curr_obj) curr_obj = basic_heap<RefCount>::make_object();
      return curr_obj;

    }

    inline std::tuple<type, int64_t, double> parse_maybe_number(char const* str, ssize_t len) {
      using parse_state = std::tuple<type, int64_t, double>;

      errno = 0;
      char* problem;
      long long first_try = strtoll(str, &problem, 0);
      if (!errno && problem - str == len) {
        // Value is an integer.
        return parse_state {type::integer, first_try, 0.0};
      } else if (*problem != '.' && *problem != 'e' && *problem != 'E') {
        // Value is a string.
        return parse_state {type::string, 0, 0.0};
      } else {
        // It's either a decimal, or it's a string.
        errno = 0;
        double second_try = strtod(str, &problem);
        if (!errno && problem - str == len) return parse_state {type::decimal, 0, second_try};
        return parse_state {type::string, 0, 0.0};
      }
    }

    template <template <class> class RefCount>
    basic_heap<RefCount> parse_scalar(yaml_event_t const& event) {
      size_t len = event.data.scalar.length;
      char const* str = reinterpret_cast<char const*>(event.data.scalar.value);

      // Try to figure out what this is.
      int64_t int_val = 0;
      double dcm_val = 0.0;
      type decision = type::string;
      switch (*str) {
        case 'n':
          if (!strcmp(str, "null")) return basic_heap<RefCount>::make_null();
          return basic_heap<RefCount>::make_string({str, len});
        case 't':
        case 'f':
          // Could be a true or false literal.
          if (!strcmp(str, "true") || !strcmp(str, "false")) {
            return basic_heap<RefCount>::make_boolean(!strcmp(str, "true"));
          }
          return basic_heap<RefCount>::make_string({str, len});
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '+':
        case '-':
          // Could be an integer or decimal literal.
          std::tie(decision, int_val, dcm_val) = parse_maybe_number(str, static_cast<ssize_t>(len));
          if (decision == type::integer) {
            return basic_heap<RefCount>::make_integer(int_val);
          } else if (decision == type::decimal) {
            return basic_heap<RefCount>::make_decimal(dcm_val);
          }
        default:
          // Give up, it's a string.
          return basic_heap<RefCount>::make_string({str, len});
      }
    }

  }

}

#endif
#endif
