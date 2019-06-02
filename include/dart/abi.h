#ifndef DART_ABI_H
#define DART_ABI_H

/*----- System Includes -----*/

#include <stdlib.h>
#include <stdint.h>

/*----- Macros ------*/

#define DART_BUFFER_MAX_SIZE  (1U << 5U)
#define DART_HEAP_MAX_SIZE    (1U << 6U)
#define DART_PACKET_MAX_SIZE  DART_HEAP_MAX_SIZE

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
  dart_err_t dart_heap_obj_take_heap(dart_heap_t* pkt, char const* key, dart_heap_t* val);
  dart_err_t dart_heap_obj_take_heap_len(dart_heap_t* pkt, char const* key, size_t len, dart_heap_t* val);
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

  // dart::heap object erase operations.
  dart_err_t dart_heap_obj_erase(dart_heap_t* pkt, char const* key);
  dart_err_t dart_heap_obj_erase_len(dart_heap_t* pkt, char const* key, size_t len);

  // dart::heap array insert operations.
  dart_err_t dart_heap_arr_insert_heap(dart_heap_t* pkt, size_t idx, dart_heap_t const* val);
  dart_err_t dart_heap_arr_take_heap(dart_heap_t* pkt, size_t idx, dart_heap_t* val);
  dart_err_t dart_heap_arr_insert_str(dart_heap_t* pkt, size_t idx, char const* val);
  dart_err_t dart_heap_arr_insert_str_len(dart_heap_t* pkt, size_t idx, char const* val, size_t val_len);
  dart_err_t dart_heap_arr_insert_int(dart_heap_t* pkt, size_t idx, int64_t val);
  dart_err_t dart_heap_arr_insert_dcm(dart_heap_t* pkt, size_t idx, double val);
  dart_err_t dart_heap_arr_insert_bool(dart_heap_t* pkt, size_t idx, int val);
  dart_err_t dart_heap_arr_insert_null(dart_heap_t* pkt, size_t idx);

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

  // dart::packet lifecycle functions.
  dart_err_t dart_packet_init(dart_packet_t* pkt);
  dart_err_t dart_packet_init_rc(dart_packet_t* pkt, dart_rc_type_t rc);
  dart_err_t dart_packet_copy(dart_packet_t* dst, dart_packet_t const* src);
  dart_err_t dart_packet_move(dart_packet_t* dst, dart_packet_t* src);
  dart_err_t dart_packet_destroy(dart_packet_t* pkt);

  // dart::packet typed constructors.
  dart_err_t dart_packet_init_obj(dart_packet_t* pkt);
  dart_err_t dart_packet_init_obj_rc(dart_packet_t* pkt, dart_rc_type_t rc);
  dart_err_t dart_packet_init_obj_va(dart_packet_t* pkt, char const* format, ...);
  dart_err_t dart_packet_init_obj_va_rc(dart_packet_t* pkt, dart_rc_type_t rc, char const* format, ...);
  dart_err_t dart_packet_init_arr(dart_packet_t* pkt);
  dart_err_t dart_packet_init_arr_rc(dart_packet_t* pkt, dart_rc_type_t rc);
  dart_err_t dart_packet_init_arr_va(dart_packet_t* pkt, char const* format, ...);
  dart_err_t dart_packet_init_arr_va_rc(dart_packet_t* pkt, dart_rc_type_t rc, char const* format, ...);
  dart_err_t dart_packet_init_str(dart_packet_t* pkt, char const* str, size_t len);
  dart_err_t dart_packet_init_str_rc(dart_packet_t* pkt, dart_rc_type_t rc, char const* str, size_t len);
  dart_err_t dart_packet_init_int(dart_packet_t* pkt, int64_t val);
  dart_err_t dart_packet_init_int_rc(dart_packet_t* pkt, dart_rc_type_t rc, int64_t val);
  dart_err_t dart_packet_init_dcm(dart_packet_t* pkt, double val);
  dart_err_t dart_packet_init_dcm_rc(dart_packet_t* pkt, dart_rc_type_t rc, double val);
  dart_err_t dart_packet_init_bool(dart_packet_t* pkt, int val);
  dart_err_t dart_packet_init_bool_rc(dart_packet_t* pkt, dart_rc_type_t rc, int val);
  dart_err_t dart_packet_init_null(dart_packet_t* pkt);
  dart_err_t dart_packet_init_null_rc(dart_packet_t* pkt, dart_rc_type_t rc);

  // dart::packet mutation operations.
  dart_err_t dart_packet_obj_insert_packet(dart_packet_t* pkt, char const* key, size_t len, dart_packet_t const* val);
  dart_err_t dart_packet_obj_take_packet(dart_packet_t* pkt, char const* key, size_t len, dart_packet_t* val);
  dart_err_t dart_packet_obj_insert_str(dart_packet_t* pkt, char const* key, size_t len, char const* val, size_t val_len);
  dart_err_t dart_packet_obj_insert_int(dart_packet_t* pkt, char const* key, size_t len, int64_t val);
  dart_err_t dart_packet_obj_insert_dcm(dart_packet_t* pkt, char const* key, size_t len, double val);
  dart_err_t dart_packet_obj_insert_bool(dart_packet_t* pkt, char const* key, size_t len, int val);
  dart_err_t dart_packet_obj_insert_null(dart_packet_t* pkt, char const* key, size_t len);

  // dart::packet json operations.
  dart_err_t dart_packet_from_json(dart_packet_t* pkt, char const* str);
  dart_err_t dart_packet_from_json_rc(dart_packet_t* pkt, dart_rc_type_t rc, char const* str);
  dart_err_t dart_packet_from_json_len(dart_packet_t* pkt, char const* str, size_t len);
  dart_err_t dart_packet_from_json_len_rc(dart_packet_t* pkt, dart_rc_type_t rc, char const* str, size_t len);
  char* dart_packet_to_json(dart_packet_t const* pkt, size_t* len);

  // generic json functions.
  char* dart_to_json(void* pkt, size_t* len);

  // generic lifecycle functions.
  dart_err_t dart_destroy(void* pkt);

  // error handling functions.
  char const* dart_get_error();

#ifdef __cplusplus
}
#endif

#endif
