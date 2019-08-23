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
