#ifndef DART_ABI_H
#define DART_ABI_H

/*----- System Includes -----*/

#include <stdlib.h>
#include <stdint.h>

/*----- Macros ------*/

#define DART_BUFFER_MAX_SIZE      (1U << 5U)
#define DART_HEAP_MAX_SIZE        (1U << 6U)
#define DART_PACKET_MAX_SIZE      DART_HEAP_MAX_SIZE

// This is embarrassing.
// Dart iterators have big
// jobs, and I need two of them.
#define DART_ITERATOR_MAX_SIZE    (1U << 8U)

#define DART_FAILURE              (-1)

#define DART_CONCAT_IMPL(a, b) a##b
#define DART_CONCAT(a, b) DART_CONCAT_IMPL(a, b)

#ifdef __COUNTER__
#define DART_GEN_UNIQUE_NAME(base)                                                          \
  DART_CONCAT(base, __COUNTER__)

#define DART_FOR_EACH_IMPL(aggr, value, it_func, it_name, err_name)                         \
  dart_iterator_t it_name;                                                                  \
  dart_err_t err_name = it_func(&it_name, aggr);                                            \
  err_name = (err_name) ? err_name : dart_iterator_get_err(value, &it_name);                \
  for (; err_name == DART_NO_ERROR && !dart_iterator_done_destroy(&it_name, value);         \
          err_name = (dart_iterator_next(&it_name),                                         \
                      dart_destroy(value),                                                  \
                      dart_iterator_get_err(value, &it_name)))

#define DART_FOR_EACH(aggr, value, it_func, it_name, err_name)                              \
  DART_FOR_EACH_IMPL(aggr, value, it_func, it_name, err_name)

#define dart_for_each(aggr, value)                                                          \
  DART_FOR_EACH(aggr, value, dart_iterator_init_err,                                        \
      DART_GEN_UNIQUE_NAME(__dart_iterator__), DART_GEN_UNIQUE_NAME(__dart_err__))

#define dart_for_each_key(aggr, value)                                                      \
  DART_FOR_EACH(aggr, value, dart_iterator_init_key_err,                                    \
      DART_GEN_UNIQUE_NAME(__dart_iterator__), DART_GEN_UNIQUE_NAME(__dart_err__))
#endif

#ifdef __cplusplus
extern "C" {
#endif

  /*----- Public Type Declarations -----*/

  enum dart_type {
    DART_OBJECT,
    DART_ARRAY,
    DART_STRING,
    DART_INTEGER,
    DART_DECIMAL,
    DART_BOOLEAN,
    DART_NULL,
    DART_INVALID
  };
  typedef enum dart_type dart_type_t;

  enum dart_packet_type {
    DART_HEAP,
    DART_BUFFER,
    DART_PACKET
  };
  typedef enum dart_packet_type dart_packet_type_t;

  enum dart_rc_type {
    DART_RC_SAFE,
    DART_RC_UNSAFE
  };
  typedef enum dart_rc_type dart_rc_type_t;

  enum dart_err {
    DART_NO_ERROR = 0,
    DART_TYPE_ERROR,
    DART_LOGIC_ERROR,
    DART_STATE_ERROR,
    DART_PARSE_ERROR,
    DART_RUNTIME_ERROR,
    DART_CLIENT_ERROR,
    DART_UNKNOWN_ERROR
  };
  typedef enum dart_err dart_err_t;

  struct dart_type_id {
    dart_packet_type_t p_id;
    dart_rc_type_t rc_id;
  };
  typedef struct dart_type_id dart_type_id_t;

  struct dart_iterator {
    dart_type_id_t rtti;
    char bytes[DART_ITERATOR_MAX_SIZE];
  };
  typedef struct dart_iterator dart_iterator_t;

  struct dart_heap {
    dart_type_id_t rtti;
    char bytes[DART_HEAP_MAX_SIZE];
  };
  typedef struct dart_heap dart_heap_t;

  struct dart_buffer {
    dart_type_id_t rtti;
    char bytes[DART_BUFFER_MAX_SIZE];
  };
  typedef struct dart_buffer dart_buffer_t;

  struct dart_packet {
    dart_type_id_t rtti;
    char bytes[DART_PACKET_MAX_SIZE];
  };
  typedef struct dart_packet dart_packet_t;

  struct dart_string_view {
    char const* ptr;
    size_t len;
  };
  typedef struct dart_string_view dart_string_view_t;

  /*----- Public Function Declarations -----*/

  /*----- dart_heap functions -----*/

  // dart::heap lifecycle functions.
  dart_heap_t dart_heap_init();
  dart_err_t dart_heap_init_err(dart_heap_t* pkt);
  dart_heap_t dart_heap_init_rc(dart_rc_type_t rc);
  dart_err_t dart_heap_init_rc_err(dart_heap_t* pkt, dart_rc_type_t rc);
  dart_heap_t dart_heap_copy(dart_heap_t const* src);
  dart_err_t dart_heap_copy_err(dart_heap_t* dst, dart_heap_t const* src);
  dart_heap_t dart_heap_move(dart_heap_t* src);
  dart_err_t dart_heap_move_err(dart_heap_t* dst, dart_heap_t* src);
  dart_err_t dart_heap_destroy(dart_heap_t* pkt);

  // dart::heap object constructors.
  dart_heap_t dart_heap_obj_init();
  dart_err_t dart_heap_obj_init_err(dart_heap_t* pkt);
  dart_heap_t dart_heap_obj_init_rc(dart_rc_type_t rc);
  dart_err_t dart_heap_obj_init_rc_err(dart_heap_t* pkt, dart_rc_type_t rc);
  dart_heap_t dart_heap_obj_init_va(char const* format, ...);
  dart_err_t dart_heap_obj_init_va_err(dart_heap_t* pkt, char const* format, ...);
  dart_heap_t dart_heap_obj_init_va_rc(dart_rc_type_t rc, char const* format, ...);
  dart_err_t dart_heap_obj_init_va_rc_err(dart_heap_t* pkt, dart_rc_type_t rc, char const* format, ...);

  // dart::heap array constructors.
  dart_heap_t dart_heap_arr_init();
  dart_err_t dart_heap_arr_init_err(dart_heap_t* pkt);
  dart_heap_t dart_heap_arr_init_rc(dart_rc_type_t rc);
  dart_err_t dart_heap_arr_init_rc_err(dart_heap_t* pkt, dart_rc_type_t rc);
  dart_heap_t dart_heap_arr_init_va(char const* format, ...);
  dart_err_t dart_heap_arr_init_va_err(dart_heap_t* pkt, char const* format, ...);
  dart_heap_t dart_heap_arr_init_va_rc(dart_rc_type_t rc, char const* format, ...);
  dart_err_t dart_heap_arr_init_va_rc_err(dart_heap_t* pkt, dart_rc_type_t rc, char const* format, ...);

  // dart::heap string constructors.
  dart_heap_t dart_heap_str_init(char const* str);
  dart_err_t dart_heap_str_init_err(dart_heap_t* pkt, char const* str);
  dart_heap_t dart_heap_str_init_len(char const* str, size_t len);
  dart_err_t dart_heap_str_init_len_err(dart_heap_t* pkt, char const* str, size_t len);
  dart_heap_t dart_heap_str_init_rc(dart_rc_type_t rc, char const* str);
  dart_err_t dart_heap_str_init_rc_err(dart_heap_t* pkt, dart_rc_type_t rc, char const* str);
  dart_heap_t dart_heap_str_init_rc_len(dart_rc_type_t rc, char const* str, size_t len);
  dart_err_t dart_heap_str_init_rc_len_err(dart_heap_t* pkt, dart_rc_type_t rc, char const* str, size_t len);

  // dart::heap integer constructors.
  dart_heap_t dart_heap_int_init(int64_t val);
  dart_err_t dart_heap_int_init_err(dart_heap_t* pkt, int64_t val);
  dart_heap_t dart_heap_int_init_rc(dart_rc_type_t rc, int64_t val);
  dart_err_t dart_heap_int_init_rc_err(dart_heap_t* pkt, dart_rc_type_t rc, int64_t val);

  // dart::heap decimal constructors.
  dart_heap_t dart_heap_dcm_init(double val);
  dart_err_t dart_heap_dcm_init_err(dart_heap_t* pkt, double val);
  dart_heap_t dart_heap_dcm_init_rc(dart_rc_type_t rc, double val);
  dart_err_t dart_heap_dcm_init_rc_err(dart_heap_t* pkt, dart_rc_type_t rc, double val);

  // dart::heap boolean constructors.
  dart_heap_t dart_heap_bool_init(int val);
  dart_err_t dart_heap_bool_init_err(dart_heap_t* pkt, int val);
  dart_heap_t dart_heap_bool_init_rc(dart_rc_type_t rc, int val);
  dart_err_t dart_heap_bool_init_rc_err(dart_heap_t* pkt, dart_rc_type_t rc, int val);

  // dart::heap null constructors.
  dart_heap_t dart_heap_null_init();
  dart_err_t dart_heap_null_init_err(dart_heap_t* pkt);
  dart_heap_t dart_heap_null_init_rc(dart_rc_type_t rc);
  dart_err_t dart_heap_null_init_rc_err(dart_heap_t* pkt, dart_rc_type_t rc);

  // dart::heap object insert operations.
  dart_err_t dart_heap_obj_insert_heap(dart_heap_t* pkt, char const* key, dart_heap_t const* val);
  dart_err_t dart_heap_obj_insert_heap_len(dart_heap_t* pkt, char const* key, size_t len, dart_heap_t const* val);
  dart_err_t dart_heap_obj_insert_take_heap(dart_heap_t* pkt, char const* key, dart_heap_t* val);
  dart_err_t dart_heap_obj_insert_take_heap_len(dart_heap_t* pkt, char const* key, size_t len, dart_heap_t* val);
  dart_err_t dart_heap_obj_insert_str(dart_heap_t* pkt, char const* key, char const* val);
  dart_err_t dart_heap_obj_insert_str_len(dart_heap_t* pkt, char const* key, size_t len, char const* val, size_t val_len);
  dart_err_t dart_heap_obj_insert_int(dart_heap_t* pkt, char const* key, int64_t val);
  dart_err_t dart_heap_obj_insert_int_len(dart_heap_t* pkt, char const* key, size_t len, int64_t val);
  dart_err_t dart_heap_obj_insert_dcm(dart_heap_t* pkt, char const* key, double val);
  dart_err_t dart_heap_obj_insert_dcm_len(dart_heap_t* pkt, char const* key, size_t len, double val);
  dart_err_t dart_heap_obj_insert_bool(dart_heap_t* pkt, char const* key, int val);
  dart_err_t dart_heap_obj_insert_bool_len(dart_heap_t* pkt, char const* key, size_t len, int val);
  dart_err_t dart_heap_obj_insert_null(dart_heap_t* pkt, char const* key);
  dart_err_t dart_heap_obj_insert_null_len(dart_heap_t* pkt, char const* key, size_t len);

  // dart::heap object set operations.
  dart_err_t dart_heap_obj_set_heap(dart_heap_t* pkt, char const* key, dart_heap_t const* val);
  dart_err_t dart_heap_obj_set_heap_len(dart_heap_t* pkt, char const* key, size_t len, dart_heap_t const* val);
  dart_err_t dart_heap_obj_set_take_heap(dart_heap_t* pkt, char const* key, dart_heap_t* val);
  dart_err_t dart_heap_obj_set_take_heap_len(dart_heap_t* pkt, char const* key, size_t len, dart_heap_t* val);
  dart_err_t dart_heap_obj_set_str(dart_heap_t* pkt, char const* key, char const* val);
  dart_err_t dart_heap_obj_set_str_len(dart_heap_t* pkt, char const* key, size_t len, char const* val, size_t val_len);
  dart_err_t dart_heap_obj_set_int(dart_heap_t* pkt, char const* key, int64_t val);
  dart_err_t dart_heap_obj_set_int_len(dart_heap_t* pkt, char const* key, size_t len, int64_t val);
  dart_err_t dart_heap_obj_set_dcm(dart_heap_t* pkt, char const* key, double val);
  dart_err_t dart_heap_obj_set_dcm_len(dart_heap_t* pkt, char const* key, size_t len, double val);
  dart_err_t dart_heap_obj_set_bool(dart_heap_t* pkt, char const* key, int val);
  dart_err_t dart_heap_obj_set_bool_len(dart_heap_t* pkt, char const* key, size_t len, int val);
  dart_err_t dart_heap_obj_set_null(dart_heap_t* pkt, char const* key);
  dart_err_t dart_heap_obj_set_null_len(dart_heap_t* pkt, char const* key, size_t len);

  // dart::heap object erase operations.
  dart_err_t dart_heap_obj_erase(dart_heap_t* pkt, char const* key);
  dart_err_t dart_heap_obj_erase_len(dart_heap_t* pkt, char const* key, size_t len);

  // dart::heap array insert operations.
  dart_err_t dart_heap_arr_insert_heap(dart_heap_t* pkt, size_t idx, dart_heap_t const* val);
  dart_err_t dart_heap_arr_insert_take_heap(dart_heap_t* pkt, size_t idx, dart_heap_t* val);
  dart_err_t dart_heap_arr_insert_str(dart_heap_t* pkt, size_t idx, char const* val);
  dart_err_t dart_heap_arr_insert_str_len(dart_heap_t* pkt, size_t idx, char const* val, size_t val_len);
  dart_err_t dart_heap_arr_insert_int(dart_heap_t* pkt, size_t idx, int64_t val);
  dart_err_t dart_heap_arr_insert_dcm(dart_heap_t* pkt, size_t idx, double val);
  dart_err_t dart_heap_arr_insert_bool(dart_heap_t* pkt, size_t idx, int val);
  dart_err_t dart_heap_arr_insert_null(dart_heap_t* pkt, size_t idx);

  // dart::heap array set operations.
  dart_err_t dart_heap_arr_set_heap(dart_heap_t* pkt, size_t idx, dart_heap_t const* val);
  dart_err_t dart_heap_arr_set_take_heap(dart_heap_t* pkt, size_t idx, dart_heap_t* val);
  dart_err_t dart_heap_arr_set_str(dart_heap_t* pkt, size_t idx, char const* val);
  dart_err_t dart_heap_arr_set_str_len(dart_heap_t* pkt, size_t idx, char const* val, size_t val_len);
  dart_err_t dart_heap_arr_set_int(dart_heap_t* pkt, size_t idx, int64_t val);
  dart_err_t dart_heap_arr_set_dcm(dart_heap_t* pkt, size_t idx, double val);
  dart_err_t dart_heap_arr_set_bool(dart_heap_t* pkt, size_t idx, int val);
  dart_err_t dart_heap_arr_set_null(dart_heap_t* pkt, size_t idx);

  // dart::heap array erase operations.
  dart_err_t dart_heap_arr_erase(dart_heap_t* pkt, size_t idx);

  // dart::heap object retrieval operations.
  dart_heap_t dart_heap_obj_get(dart_heap_t const* src, char const* key);
  dart_err_t dart_heap_obj_get_err(dart_heap_t* dst, dart_heap_t const* src, char const* key);
  dart_heap_t dart_heap_obj_get_len(dart_heap_t const* src, char const* key, size_t len);
  dart_err_t dart_heap_obj_get_len_err(dart_heap_t* dst, dart_heap_t const* src, char const* key, size_t len);

  // dart::heap array retrieval operations.
  dart_heap_t dart_heap_arr_get(dart_heap_t const* src, int64_t idx);
  dart_err_t dart_heap_arr_get_err(dart_heap_t* dst, dart_heap_t const* src, int64_t idx);

  // dart::heap string retrieval operations.
  char const* dart_heap_str_get(dart_heap_t const* src);
  char const* dart_heap_str_get_len(dart_heap_t const* src, size_t* len);

  // dart::heap integer retrieval operations.
  int64_t dart_heap_int_get(dart_heap_t const* src);
  dart_err_t dart_heap_int_get_err(dart_heap_t const* src, int64_t* val);

  // dart::heap decimal retrieval operations.
  double dart_heap_dcm_get(dart_heap_t const* src);
  dart_err_t dart_heap_dcm_get_err(dart_heap_t const* src, double* val);

  // dart::heap boolean retrieval operations.
  int dart_heap_bool_get(dart_heap_t const* src);
  dart_err_t dart_heap_bool_get_err(dart_heap_t const* src, int* val);

  // dart::heap introspection operations.
  size_t dart_heap_size(dart_heap_t const* src);
  bool dart_heap_equal(dart_heap_t const* lhs, dart_heap_t const* rhs);
  bool dart_heap_is_obj(dart_heap_t const* src);
  bool dart_heap_is_arr(dart_heap_t const* src);
  bool dart_heap_is_str(dart_heap_t const* src);
  bool dart_heap_is_int(dart_heap_t const* src);
  bool dart_heap_is_dcm(dart_heap_t const* src);
  bool dart_heap_is_bool(dart_heap_t const* src);
  bool dart_heap_is_null(dart_heap_t const* src);
  dart_type_t dart_heap_get_type(dart_heap_t const* src);

  // dart::heap json operations.
  dart_heap_t dart_heap_from_json(char const* str);
  dart_err_t dart_heap_from_json_err(dart_heap_t* pkt, char const* str);
  dart_heap_t dart_heap_from_json_rc(dart_rc_type_t rc, char const* str);
  dart_err_t dart_heap_from_json_rc_err(dart_heap_t* pkt, dart_rc_type_t rc, char const* str);
  dart_heap_t dart_heap_from_json_len(char const* str, size_t len);
  dart_err_t dart_heap_from_json_len_err(dart_heap_t* pkt, char const* str, size_t len);
  dart_heap_t dart_heap_from_json_len_rc(dart_rc_type_t rc, char const* str, size_t len);
  dart_err_t dart_heap_from_json_len_rc_err(dart_heap_t* pkt, dart_rc_type_t rc, char const* str, size_t len);
  char* dart_heap_to_json(dart_heap_t const* pkt, size_t* len);

  // dart::heap transition operations.
  dart_buffer_t dart_heap_lower(dart_heap_t const* pkt);
  dart_buffer_t dart_heap_finalize(dart_heap_t const* pkt);
  dart_err_t dart_heap_lower_err(dart_buffer_t* dst, dart_heap_t const* pkt);
  dart_err_t dart_heap_finalize_err(dart_buffer_t* dst, dart_heap_t const* pkt);

  // dart::buffer lifecycle functions.
  dart_buffer_t dart_buffer_init();
  dart_err_t dart_buffer_init_err(dart_buffer_t* pkt);
  dart_buffer_t dart_buffer_init_rc(dart_rc_type_t rc);
  dart_err_t dart_buffer_init_rc_err(dart_buffer_t* pkt, dart_rc_type_t rc);
  dart_buffer_t dart_buffer_copy(dart_buffer_t const* src);
  dart_err_t dart_buffer_copy_err(dart_buffer_t* dst, dart_buffer_t const* src);
  dart_buffer_t dart_buffer_move(dart_buffer_t* src);
  dart_err_t dart_buffer_move_err(dart_buffer_t* dst, dart_buffer_t* src);
  dart_err_t dart_buffer_destroy(dart_buffer_t* pkt);

  // dart::buffer object retrieval operations.
  dart_buffer_t dart_buffer_obj_get(dart_buffer_t const* src, char const* key);
  dart_err_t dart_buffer_obj_get_err(dart_buffer_t* dst, dart_buffer_t const* src, char const* key);
  dart_buffer_t dart_buffer_obj_get_len(dart_buffer_t const* src, char const* key, size_t len);
  dart_err_t dart_buffer_obj_get_len_err(dart_buffer_t* dst, dart_buffer_t const* src, char const* key, size_t len);

  // dart::buffer array retrieval operations.
  dart_buffer_t dart_buffer_arr_get(dart_buffer_t const* src, int64_t idx);
  dart_err_t dart_buffer_arr_get_err(dart_buffer_t* dst, dart_buffer_t const* src, int64_t idx);

  // dart::buffer string retrieval operations.
  char const* dart_buffer_str_get(dart_buffer_t const* src);
  char const* dart_buffer_str_get_len(dart_buffer_t const* src, size_t* len);

  // dart::buffer integer retrieval operations.
  int64_t dart_buffer_int_get(dart_buffer_t const* src);
  dart_err_t dart_buffer_int_get_err(dart_buffer_t const* src, int64_t* val);

  // dart::buffer decimal retrieval operations.
  double dart_buffer_dcm_get(dart_buffer_t const* src);
  dart_err_t dart_buffer_dcm_get_err(dart_buffer_t const* src, double* val);

  // dart::buffer boolean retrieval operations.
  int dart_buffer_bool_get(dart_buffer_t const* src);
  dart_err_t dart_buffer_bool_get_err(dart_buffer_t const* src, int* val);

  // dart::buffer introspection operations.
  size_t dart_buffer_size(dart_buffer_t const* src);
  bool dart_buffer_equal(dart_buffer_t const* lhs, dart_buffer_t const* rhs);
  bool dart_buffer_is_obj(dart_buffer_t const* src);
  bool dart_buffer_is_arr(dart_buffer_t const* src);
  bool dart_buffer_is_str(dart_buffer_t const* src);
  bool dart_buffer_is_int(dart_buffer_t const* src);
  bool dart_buffer_is_dcm(dart_buffer_t const* src);
  bool dart_buffer_is_bool(dart_buffer_t const* src);
  bool dart_buffer_is_null(dart_buffer_t const* src);
  dart_type_t dart_buffer_get_type(dart_buffer_t const* src);

  // dart::buffer json operations.
  dart_buffer_t dart_buffer_from_json(char const* str);
  dart_err_t dart_buffer_from_json_err(dart_buffer_t* pkt, char const* str);
  dart_buffer_t dart_buffer_from_json_rc(dart_rc_type_t rc, char const* str);
  dart_err_t dart_buffer_from_json_rc_err(dart_buffer_t* pkt, dart_rc_type_t rc, char const* str);
  dart_buffer_t dart_buffer_from_json_len(char const* str, size_t len);
  dart_err_t dart_buffer_from_json_len_err(dart_buffer_t* pkt, char const* str, size_t len);
  dart_buffer_t dart_buffer_from_json_len_rc(dart_rc_type_t rc, char const* str, size_t len);
  dart_err_t dart_buffer_from_json_len_rc_err(dart_buffer_t* pkt, dart_rc_type_t rc, char const* str, size_t len);
  char* dart_buffer_to_json(dart_buffer_t const* pkt, size_t* len);

  // dart::buffer transition functions.
  dart_heap_t dart_buffer_lift(dart_buffer_t const* src);
  dart_heap_t dart_buffer_definalize(dart_buffer_t const* src);
  dart_err_t dart_buffer_lift_err(dart_heap_t* dst, dart_buffer_t const* src);
  dart_err_t dart_buffer_definalize_err(dart_heap_t* dst, dart_buffer_t const* src);

  // generic lifecycle functions.
  dart_packet_t dart_init();
  dart_err_t dart_init_err(dart_packet_t* dst);
  dart_packet_t dart_init_rc(dart_rc_type_t rc);
  dart_err_t dart_init_rc_err(dart_packet_t* dst, dart_rc_type_t rc);
  dart_packet_t dart_copy(void const* src);
  dart_err_t dart_copy_err(void* dst, void const* src);
  dart_packet_t dart_move(void* src);
  dart_err_t dart_move_err(void* dst, void* src);
  dart_err_t dart_destroy(void* pkt);

  // generic object constructors.
  dart_packet_t dart_obj_init();
  dart_err_t dart_obj_init_err(dart_packet_t* dst);
  dart_packet_t dart_obj_init_rc(dart_rc_type_t rc);
  dart_err_t dart_obj_init_rc_err(dart_packet_t* dst, dart_rc_type_t rc);
  dart_packet_t dart_obj_init_va(char const* format, ...);
  dart_err_t dart_obj_init_va_err(dart_packet_t* dst, char const* format, ...);
  dart_packet_t dart_obj_init_va_rc(dart_rc_type_t rc, char const* format, ...);
  dart_err_t dart_obj_init_va_rc_err(dart_packet_t* dst, dart_rc_type_t rc, char const* format, ...);

  // generic array constructors.
  dart_packet_t dart_arr_init();
  dart_err_t dart_arr_init_err(dart_packet_t* pkt);
  dart_packet_t dart_arr_init_rc(dart_rc_type_t rc);
  dart_err_t dart_arr_init_rc_err(dart_packet_t* pkt, dart_rc_type_t rc);
  dart_packet_t dart_arr_init_va(char const* format, ...);
  dart_err_t dart_arr_init_va_err(dart_packet_t* pkt, char const* format, ...);
  dart_packet_t dart_arr_init_va_rc(dart_rc_type_t rc, char const* format, ...);
  dart_err_t dart_arr_init_va_rc_err(dart_packet_t* pkt, dart_rc_type_t rc, char const* format, ...);

  // generic string constructors.
  dart_packet_t dart_str_init(char const* str);
  dart_err_t dart_str_init_err(dart_packet_t* pkt, char const* str);
  dart_packet_t dart_str_init_len(char const* str, size_t len);
  dart_err_t dart_str_init_len_err(dart_packet_t* pkt, char const* str, size_t len);
  dart_packet_t dart_str_init_rc(dart_rc_type_t rc, char const* str);
  dart_err_t dart_str_init_rc_err(dart_packet_t* pkt, dart_rc_type_t rc, char const* str);
  dart_packet_t dart_str_init_rc_len(dart_rc_type_t rc, char const* str, size_t len);
  dart_err_t dart_str_init_rc_len_err(dart_packet_t* pkt, dart_rc_type_t rc, char const* str, size_t len);

  // generic integer constructors.
  dart_packet_t dart_int_init(int64_t val);
  dart_err_t dart_int_init_err(dart_packet_t* pkt, int64_t val);
  dart_packet_t dart_int_init_rc(dart_rc_type_t rc, int64_t val);
  dart_err_t dart_int_init_rc_err(dart_packet_t* pkt, dart_rc_type_t rc, int64_t val);

  // generic decimal constructors.
  dart_packet_t dart_dcm_init(double val);
  dart_err_t dart_dcm_init_err(dart_packet_t* pkt, double val);
  dart_packet_t dart_dcm_init_rc(dart_rc_type_t rc, double val);
  dart_err_t dart_dcm_init_rc_err(dart_packet_t* pkt, dart_rc_type_t rc, double val);

  // generic boolean constructors.
  dart_packet_t dart_bool_init(int val);
  dart_err_t dart_bool_init_err(dart_packet_t* pkt, int val);
  dart_packet_t dart_bool_init_rc(dart_rc_type_t rc, int val);
  dart_err_t dart_bool_init_rc_err(dart_packet_t* pkt, dart_rc_type_t rc, int val);

  // generic null constructors.
  dart_packet_t dart_null_init();
  dart_err_t dart_null_init_err(dart_packet_t* pkt);
  dart_packet_t dart_null_init_rc(dart_rc_type_t rc);
  dart_err_t dart_null_init_rc_err(dart_packet_t* pkt, dart_rc_type_t rc);

  // generic object insert operations.
  dart_err_t dart_obj_insert_dart(void* dst, char const* key, void const* val);
  dart_err_t dart_obj_insert_dart_len(void* dst, char const* key, size_t len, void const* val);
  dart_err_t dart_obj_insert_take_dart(void* dst, char const* key, void* val);
  dart_err_t dart_obj_insert_take_dart_len(void* dst, char const* key, size_t len, void* val);
  dart_err_t dart_obj_insert_str(void* dst, char const* key, char const* val);
  dart_err_t dart_obj_insert_str_len(void* dst, char const* key, size_t len, char const* val, size_t val_len);
  dart_err_t dart_obj_insert_int(void* dst, char const* key, int64_t val);
  dart_err_t dart_obj_insert_int_len(void* dst, char const* key, size_t len, int64_t val);
  dart_err_t dart_obj_insert_dcm(void* dst, char const* key, double val);
  dart_err_t dart_obj_insert_dcm_len(void* dst, char const* key, size_t len, double val);
  dart_err_t dart_obj_insert_bool(void* dst, char const* key, int val);
  dart_err_t dart_obj_insert_bool_len(void* dst, char const* key, size_t len, int val);
  dart_err_t dart_obj_insert_null(void* dst, char const* key);
  dart_err_t dart_obj_insert_null_len(void* dst, char const* key, size_t len);

  // generic object set operations.
  dart_err_t dart_obj_set_dart(void* dst, char const* key, void const* val);
  dart_err_t dart_obj_set_dart_len(void* dst, char const* key, size_t len, void const* val);
  dart_err_t dart_obj_set_take_dart(void* dst, char const* key, void* val);
  dart_err_t dart_obj_set_take_dart_len(void* dst, char const* key, size_t len, void* val);
  dart_err_t dart_obj_set_str(void* dst, char const* key, char const* val);
  dart_err_t dart_obj_set_str_len(void* dst, char const* key, size_t len, char const* val, size_t val_len);
  dart_err_t dart_obj_set_int(void* dst, char const* key, int64_t val);
  dart_err_t dart_obj_set_int_len(void* dst, char const* key, size_t len, int64_t val);
  dart_err_t dart_obj_set_dcm(void* dst, char const* key, double val);
  dart_err_t dart_obj_set_dcm_len(void* dst, char const* key, size_t len, double val);
  dart_err_t dart_obj_set_bool(void* dst, char const* key, int val);
  dart_err_t dart_obj_set_bool_len(void* dst, char const* key, size_t len, int val);
  dart_err_t dart_obj_set_null(void* dst, char const* key);
  dart_err_t dart_obj_set_null_len(void* dst, char const* key, size_t len);

  // generic object erase operations.
  dart_err_t dart_obj_erase(void* dst, char const* key);
  dart_err_t dart_obj_erase_len(void* dst, char const* key, size_t len);

  // generic array insert operations.
  dart_err_t dart_arr_insert_dart(void* dst, size_t idx, void const* val);
  dart_err_t dart_arr_insert_take_dart(void* dst, size_t idx, void* val);
  dart_err_t dart_arr_insert_str(void* dst, size_t idx, char const* val);
  dart_err_t dart_arr_insert_str_len(void* dst, size_t idx, char const* val, size_t val_len);
  dart_err_t dart_arr_insert_int(void* dst, size_t idx, int64_t val);
  dart_err_t dart_arr_insert_dcm(void* dst, size_t idx, double val);
  dart_err_t dart_arr_insert_bool(void* dst, size_t idx, int val);
  dart_err_t dart_arr_insert_null(void* dst, size_t idx);

  // generic array set operations.
  dart_err_t dart_arr_set_dart(void* dst, size_t idx, void const* val);
  dart_err_t dart_arr_set_take_dart(void* dst, size_t idx, void* val);
  dart_err_t dart_arr_set_str(void* dst, size_t idx, char const* val);
  dart_err_t dart_arr_set_str_len(void* dst, size_t idx, char const* val, size_t val_len);
  dart_err_t dart_arr_set_int(void* dst, size_t idx, int64_t val);
  dart_err_t dart_arr_set_dcm(void* dst, size_t idx, double val);
  dart_err_t dart_arr_set_bool(void* dst, size_t idx, int val);
  dart_err_t dart_arr_set_null(void* dst, size_t idx);

  // generic array erase operations.
  dart_err_t dart_arr_erase(void* pkt, size_t idx);

  // generic object retrieval operations.
  dart_packet_t dart_obj_get(void const* src, char const* key);
  dart_err_t dart_obj_get_err(dart_packet_t* dst, void const* src, char const* key);
  dart_packet_t dart_obj_get_len(void const* src, char const* key, size_t len);
  dart_err_t dart_obj_get_len_err(dart_packet_t* dst, void const* src, char const* key, size_t len);

  // generic array retrieval operations.
  dart_packet_t dart_arr_get(void const* src, int64_t idx);
  dart_err_t dart_arr_get_err(dart_packet_t* dst, void const* src, int64_t idx);

  // generic string retrieval operations.
  char const* dart_str_get(void const* src);
  char const* dart_str_get_len(void const* src, size_t* len);

  // generic integer retrieval operations.
  int64_t dart_int_get(void const* src);
  dart_err_t dart_int_get_err(void const* src, int64_t* val);

  // generic decimal retrieval operations.
  double dart_dcm_get(void const* src);
  dart_err_t dart_dcm_get_err(void const* src, double* val);

  // generic boolean retrieval operations.
  int dart_bool_get(void const* src);
  dart_err_t dart_bool_get_err(void const* src, int* val);

  // generic introspection operations.
  size_t dart_size(void const* src);
  bool dart_equal(void const* lhs, void const* rhs);
  bool dart_is_obj(void const* src);
  bool dart_is_arr(void const* src);
  bool dart_is_str(void const* src);
  bool dart_is_int(void const* src);
  bool dart_is_dcm(void const* src);
  bool dart_is_bool(void const* src);
  bool dart_is_null(void const* src);
  dart_type_t dart_get_type(void const* src);

  // generic json operations.
  dart_packet_t dart_from_json(char const* str);
  dart_err_t dart_from_json_err(dart_packet_t* dst, char const* str);
  dart_packet_t dart_from_json_rc(dart_rc_type_t rc, char const* str);
  dart_err_t dart_from_json_rc_err(dart_packet_t* dst, dart_rc_type_t rc, char const* str);
  dart_packet_t dart_from_json_len(char const* str, size_t len);
  dart_err_t dart_from_json_len_err(dart_packet_t* dst, char const* str, size_t len);
  dart_packet_t dart_from_json_len_rc(dart_rc_type_t rc, char const* str, size_t len);
  dart_err_t dart_from_json_len_rc_err(dart_packet_t* dst, dart_rc_type_t rc, char const* str, size_t len);

  // generic json functions.
  char* dart_to_json(void const* src, size_t* len);

  // Iterator operations.
  dart_err_t dart_iterator_init_err(dart_iterator_t* dst, void const* src);
  dart_err_t dart_iterator_init_key_err(dart_iterator_t* dst, void const* src);
  dart_err_t dart_iterator_copy_err(dart_iterator_t* dst, dart_iterator_t const* src);
  dart_err_t dart_iterator_move_err(dart_iterator_t* dst, dart_iterator_t* src);
  dart_err_t dart_iterator_destroy(dart_iterator_t* dst);
  dart_packet_t dart_iterator_get(dart_iterator_t const* src);
  dart_err_t dart_iterator_get_err(dart_packet_t* dst, dart_iterator_t const* src);
  dart_err_t dart_iterator_next(dart_iterator_t* dst);
  bool dart_iterator_done(dart_iterator_t const* src);
  bool dart_iterator_done_destroy(dart_iterator_t* dst, dart_packet_t* pkt);

  // error handling functions.
  char const* dart_get_error();

#ifdef __cplusplus
}
#endif

#endif
