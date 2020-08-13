#ifndef DART_API_SWAP_H
#define DART_API_SWAP_H

/*----- Local Includes -----*/

#include "abi.h"
#include "common.h"

/*----- Function Implementations -----*/

namespace dart {

  /*----- Swap from C++ to C -----*/

  /**
   *  @brief
   *  When used correctly, function can efficiently swap between
   *  the C++ and C APIs for Dart, without copies/allocations/etc
   *  Function is for expert use, here be dragons
   *
   *  @details
   *  Dart is primarily a header-only template library, which means
   *  it has interesting ABI properties.
   *  The pure C interface exports a shared library, and if you
   *  _*exclusively*_ use the C interface, you can swap out different
   *  versions of the shared library without having to recompile
   *  your program.
   *  The C++ interface, however, has almost no binary portability,
   *  and so if your program is compiled against one version of this
   *  library, but dynamically links against another, and you call
   *  this function, it will explode in fantastically awful ways.
   */
  inline void unsafe_api_swap(dart_heap_t* dst, heap const& src) {
    // Initialize our RTTI structure properly so the C functions
    // will know what this object is.
    dst->rtti.p_id = DART_HEAP;
    dst->rtti.rc_id = DART_RC_SAFE;

    // Placement-new copy-construct the packet we were given into
    // the C structure.
    new(&dst->bytes) heap(src);
  }

  /**
   *  @brief
   *  When used correctly, function can efficiently swap between
   *  the C++ and C APIs for Dart, without copies/allocations/etc
   *  Function is for expert use, here be dragons
   *
   *  @details
   *  Dart is primarily a header-only template library, which means
   *  it has interesting ABI properties.
   *  The pure C interface exports a shared library, and if you
   *  _*exclusively*_ use the C interface, you can swap out different
   *  versions of the shared library without having to recompile
   *  your program.
   *  The C++ interface, however, has almost no binary portability,
   *  and so if your program is compiled against one version of this
   *  library, but dynamically links against another, and you call
   *  this function, it will explode in fantastically awful ways.
   */
  inline void unsafe_api_swap(dart_heap_t* dst, heap&& src) {
    // Initialize our RTTI structure properly so the C functions
    // will know what this object is.
    dst->rtti.p_id = DART_HEAP;
    dst->rtti.rc_id = DART_RC_SAFE;

    // Placement-new copy-construct the packet we were given into
    // the C structure.
    new(&dst->bytes) heap(std::move(src));
  }

  /**
   *  @brief
   *  When used correctly, function can efficiently swap between
   *  the C++ and C APIs for Dart, without copies/allocations/etc
   *  Function is for expert use, here be dragons
   *
   *  @details
   *  Dart is primarily a header-only template library, which means
   *  it has interesting ABI properties.
   *  The pure C interface exports a shared library, and if you
   *  _*exclusively*_ use the C interface, you can swap out different
   *  versions of the shared library without having to recompile
   *  your program.
   *  The C++ interface, however, has almost no binary portability,
   *  and so if your program is compiled against one version of this
   *  library, but dynamically links against another, and you call
   *  this function, it will explode in fantastically awful ways.
   */
  inline void unsafe_api_swap(dart_buffer_t* dst, buffer const& src) {
    // Initialize our RTTI structure properly so the C functions
    // will know what this object is.
    dst->rtti.p_id = DART_BUFFER;
    dst->rtti.rc_id = DART_RC_SAFE;

    // Placement-new copy-construct the packet we were given into
    // the C structure.
    new(&dst->bytes) buffer(src);
  }

  /**
   *  @brief
   *  When used correctly, function can efficiently swap between
   *  the C++ and C APIs for Dart, without copies/allocations/etc
   *  Function is for expert use, here be dragons
   *
   *  @details
   *  Dart is primarily a header-only template library, which means
   *  it has interesting ABI properties.
   *  The pure C interface exports a shared library, and if you
   *  _*exclusively*_ use the C interface, you can swap out different
   *  versions of the shared library without having to recompile
   *  your program.
   *  The C++ interface, however, has almost no binary portability,
   *  and so if your program is compiled against one version of this
   *  library, but dynamically links against another, and you call
   *  this function, it will explode in fantastically awful ways.
   */
  inline void unsafe_api_swap(dart_buffer_t* dst, buffer&& src) {
    // Initialize our RTTI structure properly so the C functions
    // will know what this object is.
    dst->rtti.p_id = DART_BUFFER;
    dst->rtti.rc_id = DART_RC_SAFE;

    // Placement-new copy-construct the packet we were given into
    // the C structure.
    new(&dst->bytes) buffer(std::move(src));
  }

  /**
   *  @brief
   *  When used correctly, function can efficiently swap between
   *  the C++ and C APIs for Dart, without copies/allocations/etc
   *  Function is for expert use, here be dragons
   *
   *  @details
   *  Dart is primarily a header-only template library, which means
   *  it has interesting ABI properties.
   *  The pure C interface exports a shared library, and if you
   *  _*exclusively*_ use the C interface, you can swap out different
   *  versions of the shared library without having to recompile
   *  your program.
   *  The C++ interface, however, has almost no binary portability,
   *  and so if your program is compiled against one version of this
   *  library, but dynamically links against another, and you call
   *  this function, it will explode in fantastically awful ways.
   */
  inline void unsafe_api_swap(dart_packet_t* dst, packet const& src) {
    // Initialize our RTTI structure properly so the C functions
    // will know what this object is.
    dst->rtti.p_id = DART_PACKET;
    dst->rtti.rc_id = DART_RC_SAFE;

    // Placement-new copy-construct the packet we were given into
    // the C structure.
    new(&dst->bytes) packet(src);
  }

  /**
   *  @brief
   *  When used correctly, function can efficiently swap between
   *  the C++ and C APIs for Dart, without copies/allocations/etc
   *  Function is for expert use, here be dragons
   *
   *  @details
   *  Dart is primarily a header-only template library, which means
   *  it has interesting ABI properties.
   *  The pure C interface exports a shared library, and if you
   *  _*exclusively*_ use the C interface, you can swap out different
   *  versions of the shared library without having to recompile
   *  your program.
   *  The C++ interface, however, has almost no binary portability,
   *  and so if your program is compiled against one version of this
   *  library, but dynamically links against another, and you call
   *  this function, it will explode in fantastically awful ways.
   */
  inline void unsafe_api_swap(dart_packet_t* dst, packet&& src) {
    // Initialize our RTTI structure properly so the C functions
    // will know what this object is.
    dst->rtti.p_id = DART_PACKET;
    dst->rtti.rc_id = DART_RC_SAFE;

    // Placement-new copy-construct the packet we were given into
    // the C structure.
    new(&dst->bytes) packet(std::move(src));
  }

  /**
   *  @brief
   *  When used correctly, function can efficiently swap between
   *  the C++ and C APIs for Dart, without copies/allocations/etc
   *  Function is for expert use, here be dragons
   *
   *  @details
   *  Dart is primarily a header-only template library, which means
   *  it has interesting ABI properties.
   *  The pure C interface exports a shared library, and if you
   *  _*exclusively*_ use the C interface, you can swap out different
   *  versions of the shared library without having to recompile
   *  your program.
   *  The C++ interface, however, has almost no binary portability,
   *  and so if your program is compiled against one version of this
   *  library, but dynamically links against another, and you call
   *  this function, it will explode in fantastically awful ways.
   */
  inline void unsafe_api_swap(dart_heap_t* dst, unsafe_heap const& src) {
    // Initialize our RTTI structure properly so the C functions
    // will know what this object is.
    dst->rtti.p_id = DART_HEAP;
    dst->rtti.rc_id = DART_RC_UNSAFE;

    // Placement-new copy-construct the packet we were given into
    // the C structure.
    new(&dst->bytes) unsafe_heap(src);
  }

  /**
   *  @brief
   *  When used correctly, function can efficiently swap between
   *  the C++ and C APIs for Dart, without copies/allocations/etc
   *  Function is for expert use, here be dragons
   *
   *  @details
   *  Dart is primarily a header-only template library, which means
   *  it has interesting ABI properties.
   *  The pure C interface exports a shared library, and if you
   *  _*exclusively*_ use the C interface, you can swap out different
   *  versions of the shared library without having to recompile
   *  your program.
   *  The C++ interface, however, has almost no binary portability,
   *  and so if your program is compiled against one version of this
   *  library, but dynamically links against another, and you call
   *  this function, it will explode in fantastically awful ways.
   */
  inline void unsafe_api_swap(dart_heap_t* dst, unsafe_heap&& src) {
    // Initialize our RTTI structure properly so the C functions
    // will know what this object is.
    dst->rtti.p_id = DART_HEAP;
    dst->rtti.rc_id = DART_RC_UNSAFE;

    // Placement-new copy-construct the packet we were given into
    // the C structure.
    new(&dst->bytes) unsafe_heap(std::move(src));
  }

  /**
   *  @brief
   *  When used correctly, function can efficiently swap between
   *  the C++ and C APIs for Dart, without copies/allocations/etc
   *  Function is for expert use, here be dragons
   *
   *  @details
   *  Dart is primarily a header-only template library, which means
   *  it has interesting ABI properties.
   *  The pure C interface exports a shared library, and if you
   *  _*exclusively*_ use the C interface, you can swap out different
   *  versions of the shared library without having to recompile
   *  your program.
   *  The C++ interface, however, has almost no binary portability,
   *  and so if your program is compiled against one version of this
   *  library, but dynamically links against another, and you call
   *  this function, it will explode in fantastically awful ways.
   */
  inline void unsafe_api_swap(dart_buffer_t* dst, unsafe_buffer const& src) {
    // Initialize our RTTI structure properly so the C functions
    // will know what this object is.
    dst->rtti.p_id = DART_BUFFER;
    dst->rtti.rc_id = DART_RC_UNSAFE;

    // Placement-new copy-construct the packet we were given into
    // the C structure.
    new(&dst->bytes) unsafe_buffer(src);
  }

  /**
   *  @brief
   *  When used correctly, function can efficiently swap between
   *  the C++ and C APIs for Dart, without copies/allocations/etc
   *  Function is for expert use, here be dragons
   *
   *  @details
   *  Dart is primarily a header-only template library, which means
   *  it has interesting ABI properties.
   *  The pure C interface exports a shared library, and if you
   *  _*exclusively*_ use the C interface, you can swap out different
   *  versions of the shared library without having to recompile
   *  your program.
   *  The C++ interface, however, has almost no binary portability,
   *  and so if your program is compiled against one version of this
   *  library, but dynamically links against another, and you call
   *  this function, it will explode in fantastically awful ways.
   */
  inline void unsafe_api_swap(dart_buffer_t* dst, unsafe_buffer&& src) {
    // Initialize our RTTI structure properly so the C functions
    // will know what this object is.
    dst->rtti.p_id = DART_BUFFER;
    dst->rtti.rc_id = DART_RC_UNSAFE;

    // Placement-new copy-construct the packet we were given into
    // the C structure.
    new(&dst->bytes) unsafe_buffer(std::move(src));
  }

  /**
   *  @brief
   *  When used correctly, function can efficiently swap between
   *  the C++ and C APIs for Dart, without copies/allocations/etc
   *  Function is for expert use, here be dragons
   *
   *  @details
   *  Dart is primarily a header-only template library, which means
   *  it has interesting ABI properties.
   *  The pure C interface exports a shared library, and if you
   *  _*exclusively*_ use the C interface, you can swap out different
   *  versions of the shared library without having to recompile
   *  your program.
   *  The C++ interface, however, has almost no binary portability,
   *  and so if your program is compiled against one version of this
   *  library, but dynamically links against another, and you call
   *  this function, it will explode in fantastically awful ways.
   */
  inline void unsafe_api_swap(dart_packet_t* dst, unsafe_packet const& src) {
    // Initialize our RTTI structure properly so the C functions
    // will know what this object is.
    dst->rtti.p_id = DART_PACKET;
    dst->rtti.rc_id = DART_RC_UNSAFE;

    // Placement-new copy-construct the packet we were given into
    // the C structure.
    new(&dst->bytes) unsafe_packet(src);
  }

  /**
   *  @brief
   *  When used correctly, function can efficiently swap between
   *  the C++ and C APIs for Dart, without copies/allocations/etc
   *  Function is for expert use, here be dragons
   *
   *  @details
   *  Dart is primarily a header-only template library, which means
   *  it has interesting ABI properties.
   *  The pure C interface exports a shared library, and if you
   *  _*exclusively*_ use the C interface, you can swap out different
   *  versions of the shared library without having to recompile
   *  your program.
   *  The C++ interface, however, has almost no binary portability,
   *  and so if your program is compiled against one version of this
   *  library, but dynamically links against another, and you call
   *  this function, it will explode in fantastically awful ways.
   */
  inline void unsafe_api_swap(dart_packet_t* dst, unsafe_packet&& src) {
    // Initialize our RTTI structure properly so the C functions
    // will know what this object is.
    dst->rtti.p_id = DART_PACKET;
    dst->rtti.rc_id = DART_RC_UNSAFE;

    // Placement-new copy-construct the packet we were given into
    // the C structure.
    new(&dst->bytes) unsafe_packet(std::move(src));
  }

  /*----- Swap from C to C++ -----*/

  /**
   *  @brief
   *  When used correctly, function can efficiently swap between
   *  the C++ and C APIs for Dart, without copies/allocations/etc
   *  Function is for expert use, here be dragons
   *
   *  @details
   *  Dart is primarily a header-only template library, which means
   *  it has interesting ABI properties.
   *  The pure C interface exports a shared library, and if you
   *  _*exclusively*_ use the C interface, you can swap out different
   *  versions of the shared library without having to recompile
   *  your program.
   *  The C++ interface, however, has almost no binary portability,
   *  and so if your program is compiled against one version of this
   *  library, but dynamically links against another, and you call
   *  this function, it will explode in fantastically awful ways.
   */
  inline void unsafe_api_swap(heap& dst, dart_heap_t const* src) {
    // Check to make sure the user is respecting our API invariants
    if (src->rtti.rc_id != DART_RC_SAFE) {
      throw type_error("dart::heap cannot be"
          " initialized from a C type with unsafe reference counting");
    }

    // Unfortunately we don't have access to the ABI layer helpers
    // in this header, so we have to just do the reinterpret cast manually
    // The function has "unsafe" in its name, so I'm going to say it's fine
    dst = *reinterpret_cast<heap const*>(&src->bytes);
  }

  /**
   *  @brief
   *  When used correctly, function can efficiently swap between
   *  the C++ and C APIs for Dart, without copies/allocations/etc
   *  Function is for expert use, here be dragons
   *
   *  @details
   *  Dart is primarily a header-only template library, which means
   *  it has interesting ABI properties.
   *  The pure C interface exports a shared library, and if you
   *  _*exclusively*_ use the C interface, you can swap out different
   *  versions of the shared library without having to recompile
   *  your program.
   *  The C++ interface, however, has almost no binary portability,
   *  and so if your program is compiled against one version of this
   *  library, but dynamically links against another, and you call
   *  this function, it will explode in fantastically awful ways.
   */
  inline void unsafe_api_swap(buffer& dst, dart_buffer_t const* src) {
    // Check to make sure the user is respecting our API invariants
    if (src->rtti.rc_id != DART_RC_SAFE) {
      throw type_error("dart::buffer cannot be"
          " initialized from a C type with unsafe reference counting");
    }

    // Unfortunately we don't have access to the ABI layer helpers
    // in this header, so we have to just do the reinterpret cast manually
    // The function has "unsafe" in its name, so I'm going to say it's fine
    dst = *reinterpret_cast<buffer const*>(&src->bytes);
  }

  /**
   *  @brief
   *  When used correctly, function can efficiently swap between
   *  the C++ and C APIs for Dart, without copies/allocations/etc
   *  Function is for expert use, here be dragons
   *
   *  @details
   *  Dart is primarily a header-only template library, which means
   *  it has interesting ABI properties.
   *  The pure C interface exports a shared library, and if you
   *  _*exclusively*_ use the C interface, you can swap out different
   *  versions of the shared library without having to recompile
   *  your program.
   *  The C++ interface, however, has almost no binary portability,
   *  and so if your program is compiled against one version of this
   *  library, but dynamically links against another, and you call
   *  this function, it will explode in fantastically awful ways.
   */
  inline void unsafe_api_swap(packet& dst, dart_packet_t const* src) {
    // Check to make sure the user is respecting our API invariants
    if (src->rtti.rc_id != DART_RC_SAFE) {
      throw type_error("dart::packet cannot be"
          " initialized from a C type with unsafe reference counting");
    }

    // Unfortunately we don't have access to the ABI layer helpers
    // in this header, so we have to just do the reinterpret cast manually
    // The function has "unsafe" in its name, so I'm going to say it's fine
    dst = *reinterpret_cast<packet const*>(&src->bytes);
  }

  /**
   *  @brief
   *  When used correctly, function can efficiently swap between
   *  the C++ and C APIs for Dart, without copies/allocations/etc
   *  Function is for expert use, here be dragons
   *
   *  @details
   *  Dart is primarily a header-only template library, which means
   *  it has interesting ABI properties.
   *  The pure C interface exports a shared library, and if you
   *  _*exclusively*_ use the C interface, you can swap out different
   *  versions of the shared library without having to recompile
   *  your program.
   *  The C++ interface, however, has almost no binary portability,
   *  and so if your program is compiled against one version of this
   *  library, but dynamically links against another, and you call
   *  this function, it will explode in fantastically awful ways.
   */
  inline void unsafe_api_swap(unsafe_heap& dst, dart_heap_t const* src) {
    // Check to make sure the user is respecting our API invariants
    if (src->rtti.rc_id != DART_RC_UNSAFE) {
      throw type_error("dart::unsafe_heap cannot be"
          " initialized from a C type with safe reference counting");
    }

    // Unfortunately we don't have access to the ABI layer helpers
    // in this header, so we have to just do the reinterpret cast manually
    // The function has "unsafe" in its name, so I'm going to say it's fine
    dst = *reinterpret_cast<unsafe_heap const*>(&src->bytes);
  }

  /**
   *  @brief
   *  When used correctly, function can efficiently swap between
   *  the C++ and C APIs for Dart, without copies/allocations/etc
   *  Function is for expert use, here be dragons
   *
   *  @details
   *  Dart is primarily a header-only template library, which means
   *  it has interesting ABI properties.
   *  The pure C interface exports a shared library, and if you
   *  _*exclusively*_ use the C interface, you can swap out different
   *  versions of the shared library without having to recompile
   *  your program.
   *  The C++ interface, however, has almost no binary portability,
   *  and so if your program is compiled against one version of this
   *  library, but dynamically links against another, and you call
   *  this function, it will explode in fantastically awful ways.
   */
  inline void unsafe_api_swap(unsafe_buffer& dst, dart_buffer_t const* src) {
    // Check to make sure the user is respecting our API invariants
    if (src->rtti.rc_id != DART_RC_UNSAFE) {
      throw type_error("dart::unsafe_buffer cannot be"
          " initialized from a C type with safe reference counting");
    }

    // Unfortunately we don't have access to the ABI layer helpers
    // in this header, so we have to just do the reinterpret cast manually
    // The function has "unsafe" in its name, so I'm going to say it's fine
    dst = *reinterpret_cast<unsafe_buffer const*>(&src->bytes);
  }

  /**
   *  @brief
   *  When used correctly, function can efficiently swap between
   *  the C++ and C APIs for Dart, without copies/allocations/etc
   *  Function is for expert use, here be dragons
   *
   *  @details
   *  Dart is primarily a header-only template library, which means
   *  it has interesting ABI properties.
   *  The pure C interface exports a shared library, and if you
   *  _*exclusively*_ use the C interface, you can swap out different
   *  versions of the shared library without having to recompile
   *  your program.
   *  The C++ interface, however, has almost no binary portability,
   *  and so if your program is compiled against one version of this
   *  library, but dynamically links against another, and you call
   *  this function, it will explode in fantastically awful ways.
   */
  inline void unsafe_api_swap(unsafe_packet& dst, dart_packet_t const* src) {
    // Check to make sure the user is respecting our API invariants
    if (src->rtti.rc_id != DART_RC_UNSAFE) {
      throw type_error("dart::unsafe_packet cannot be"
          " initialized from a C type with safe reference counting");
    }

    // Unfortunately we don't have access to the ABI layer helpers
    // in this header, so we have to just do the reinterpret cast manually
    // The function has "unsafe" in its name, so I'm going to say it's fine
    dst = *reinterpret_cast<unsafe_packet const*>(&src->bytes);
  }

}

#endif
