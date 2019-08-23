#ifndef DART_BUFFER_STRING_H
#define DART_BUFFER_STRING_H

/*----- Project Includes -----*/

#include "../common.h"

/*----- Function Implementations -----*/

namespace dart {

  template <template <class> class RefCount>
  char const* basic_buffer<RefCount>::str() const {
    return strv().data();
  }

  template <template <class> class RefCount>
  shim::string_view basic_buffer<RefCount>::strv() const {
    return detail::string_deref([] (auto& val) { return val.get_strv(); }, raw);
  }

}

#endif
