#ifndef DART_CPP14_SHIM_H
#define DART_CPP14_SHIM_H

// Figure out what compiler we have.
#if defined(__clang__)
#define DART_USING_CLANG 1
#elif defined(__GNUC__) || defined(__GNUG__)
#define DART_USING_GCC 1
#elif defined(_MSC_VER)
#define DART_USING_MSVC 1
#endif

#if DART_USING_CLANG && __clang_major__ >= 5 && __clang_major__ <= 7
// Side-step a disagreement between clang (5/6) and GNU std::variant.
#define DART_USE_MPARK_VARIANT 1
#elif defined(__APPLE__)
// Side-step AppleClang misreporting compiler capabilities on macos 10.13 and below.
#include <availability.h>
#ifndef __MAC_10_14
#define DART_USE_MPARK_VARIANT 1
#endif
#endif

// MSVC doesn't have a signed size type, obviously
#if DART_USING_MSVC
#include <BaseTsd.h>
using ssize_t = SSIZE_T;
#endif

// Make sure we have a fallback for compilers that don't support attributes at all.
#ifndef __has_cpp_attribute
#define __has_cpp_attribute(name) 0
#endif

// Figure out how to declare things [[nodiscard]]
#if __has_cpp_attribute(gnu::warn_unused_result)
#define DART_NODISCARD [[gnu::warn_unused_result]]
#elif __has_cpp_attribute(nodiscard)
#define DART_NODISCARD [[nodiscard]]
#else
#define DART_NODISCARD
#endif

#define DART_STRINGIFY_IMPL(x) #x
#define DART_STRINGIFY(x) DART_STRINGIFY_IMPL(x)

#if DART_USING_MSVC
#define DART_UNLIKELY(x) !!(x)
#else
#define DART_UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif

#ifndef NDEBUG

#if DART_USING_MSVC
#include <io.h>
#define DART_WRITE(fd, ptr, bytes) _write(fd, ptr, static_cast<unsigned int>(bytes))
#define DART_STDERR_FILENO _fileno(stderr)
#else
#include <unistd.h>
#define DART_WRITE(fd, ptr, bytes) write(fd, ptr, bytes)
#define DART_STDERR_FILENO STDERR_FILENO
#endif

/**
 *  @brief
 *  Macro customizes functionality usually provided by assert().
 *
 *  @details
 *  Not strictly necessary, but tries to provide a bit more context and
 *  information as to why I just murdered the user's program (in production, no doubt).
 *
 *  @remarks
 *  Don't actually know if Doxygen lets you document macros, guess we'll see.
 */
#define DART_ASSERT(cond)                                                                                     \
  if (DART_UNLIKELY(!(cond))) {                                                                               \
    auto& msg = "dart::packet has detected fatal memory corruption and cannot continue execution.\n"          \
      "\"" DART_STRINGIFY(cond) "\" violated.\nSee " __FILE__ ":" DART_STRINGIFY(__LINE__);                   \
    int errfd = DART_STDERR_FILENO;                                                                           \
    ssize_t spins {0}, written {0}, total {sizeof(msg)};                                                      \
    do {                                                                                                      \
      ssize_t ret = DART_WRITE(errfd, msg + written, total - written);                                        \
      if (ret >= 0) written += ret;                                                                           \
    } while (written != total && spins++ < 16);                                                               \
    std::abort();                                                                                             \
  }
#else
#define DART_ASSERT(cond) 
#endif


// Conditionally include different implementations of different data structures
// depending on what standard we have access to.
#if __cplusplus >= 201703L && !DART_USE_MPARK_VARIANT
#define DART_HAS_CPP17 1
#include <variant>
#include <optional>
#include <string_view>
#else
#define DART_HAS_CPP14 1
#include "support/variant.h"
#include "support/optional.h"
#include "support/string_view.h"
#endif

// Conditionally pull each of those types into our namespace.
namespace dart {
  namespace shim {

#if DART_USING_MSVC
    inline int aligned_alloc(void** memptr, size_t alignment, size_t size) {
      void* ret = _aligned_malloc(size, alignment);
      if (ret) {
        *memptr = ret;
        return 0;
      } else {
        return -1;
      }
    }

    inline void aligned_free(void* ptr) {
      _aligned_free(ptr);
    }
#else
    inline int aligned_alloc(void** memptr, size_t alignment, size_t size) {
      return posix_memalign(memptr, alignment, size);
    }

    inline void aligned_free(void* ptr) {
      free(ptr);
    }
#endif

#ifdef DART_HAS_CPP17
    // Pull in names of types.
    using std::optional;
    using std::nullopt_t;
    using std::variant;
    using std::monostate;
    using std::string_view;

    // Pull in constants.
    static inline constexpr auto nullopt = std::nullopt;

    // Pull in non-member helpers.
    using std::get;
    using std::visit;
    using std::get_if;
    using std::launder;
    using std::holds_alternative;
    using std::variant_alternative;
    using std::variant_alternative_t;

    // Define a way to compose lambdas.
    template <class... Ls>
    struct compose : Ls... {
      using Ls::operator ()...;
    };
    template <class... Ls>
    compose(Ls...) -> compose<Ls...>;

    template <class... Ls>
    auto compose_together(Ls&&... lambdas) {
      return compose {std::forward<Ls>(lambdas)...};
    }
#else
    // Pull in names of types.
    using dart::optional;
    using dart::nullopt_t;
    using dart::string_view;
    using mpark::variant;
    using mpark::monostate;

    // Pull in non-member helpers.
    using mpark::get;
    using mpark::visit;
    using mpark::get_if;
    using mpark::holds_alternative;
    using mpark::variant_alternative;
    using mpark::variant_alternative_t;

    // Pull in constants.
    static constexpr auto nullopt = dart::nullopt;

    // Define a way to compose lambdas.
    template <class... Ls>
    struct compose;
    template <class L, class... Ls>
    struct compose<L, Ls...> : L, compose<Ls...> {
      compose(L l, Ls... the_rest) : L(std::move(l)), compose<Ls...>(std::move(the_rest)...) {}
      using L::operator ();
      using compose<Ls...>::operator ();
    };
    template <class L>
    struct compose<L> : L {
      compose(L l) : L(std::move(l)) {}
      using L::operator ();
    };

    template <class... Ls>
    auto compose_together(Ls&&... lambdas) {
      return compose<std::decay_t<Ls>...> {std::forward<Ls>(lambdas)...};
    }
#endif
  }
}

#endif
