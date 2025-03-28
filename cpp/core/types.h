/* Copyright (c) 2008-2024 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#pragma once

#include <cinttypes>
#include <complex>
#include <cstddef>
#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

#ifdef _WIN32
#ifdef _WIN64
#define PRI_SIZET PRIu64
#else
#define PRI_SIZET PRIu32
#endif
#else
#define PRI_SIZET "zu"
#endif

namespace MR {

#ifdef MRTRIX_MAX_ALIGN_T_NOT_DEFINED
#ifdef MRTRIX_STD_MAX_ALIGN_T_NOT_DEFINED
// needed for clang 3.4:
using __max_align_t = struct {
  long long __clang_max_align_nonce1 __attribute__((__aligned__(__alignof__(long long))));
  long double __clang_max_align_nonce2 __attribute__((__aligned__(__alignof__(long double))));
};
constexpr size_t malloc_align = alignof(__max_align_t);
#else
constexpr size_t malloc_align = alignof(std::max_align_t);
#endif
#else
constexpr size_t malloc_align = alignof(::max_align_t);
#endif

namespace Helper {
template <class ImageType> class ConstRow;
template <class ImageType> class Row;
} // namespace Helper
} // namespace MR

#ifdef EIGEN_HAS_OPENMP
#undef EIGEN_HAS_OPENMP
#endif

#define EIGEN_DENSEBASE_PLUGIN "eigen_plugins/dense_base.h"
#define EIGEN_MATRIXBASE_PLUGIN "eigen_plugins/dense_base.h"
#define EIGEN_ARRAYBASE_PLUGIN "eigen_plugins/dense_base.h"
#define EIGEN_MATRIX_PLUGIN "eigen_plugins/matrix.h"
#define EIGEN_ARRAY_PLUGIN "eigen_plugins/array.h"

#include <Eigen/Geometry>

/*! \defgroup VLA Variable-length array macros
 *
 * The reason for defining these macros at all is that VLAs are not part of the
 * C++ standard, and not available on all compilers. Regardless of the
 * availability of VLAs, they should be avoided if possible since they run the
 * risk of overrunning the stack if the length of the array is large, or if the
 * function is called recursively. They can be used safely in cases where the
 * size of the array is expected to be small, and the function will not be
 * called recursively, and in these cases may avoid the overhead of allocation
 * that might be incurred by the use of e.g. a vector.
 */

//! \{

/*! \def VLA
 * define a variable-length array (VLA) if supported by the compiler, or a
 * vector otherwise. This may have performance implications in the latter
 * case if this forms part of a tight loop.
 * \sa VLA_MAX
 */

/*! \def VLA_MAX
 * define a variable-length array if supported by the compiler, or a
 * fixed-length array of size \a max  otherwise. This may have performance
 * implications in the latter case if this forms part of a tight loop.
 * \note this should not be used in recursive functions, unless the maximum
 * number of calls is known to be small. Large amounts of recursion will run
 * the risk of overrunning the stack.
 * \sa VLA
 */

#ifdef MRTRIX_NO_VLA
#define VLA(name, type, num)                                                                                           \
  std::vector<type> __vla__##name(num);                                                                                \
  type *name = &__vla__##name[0]
#define VLA_MAX(name, type, num, max) type name[max]
#else
#define VLA(name, type, num) type name[num]
#define VLA_MAX(name, type, num, max) type name[num]
#endif

/*! \def NON_POD_VLA
 * define a variable-length array of non-POD data if supported by the compiler,
 * or a vector otherwise. This may have performance implications in the
 * latter case if this forms part of a tight loop.
 * \sa VLA_MAX
 */

/*! \def NON_POD_VLA_MAX
 * define a variable-length array of non-POD data if supported by the compiler,
 * or a fixed-length array of size \a max  otherwise. This may have performance
 * implications in the latter case if this forms part of a tight loop.
 * \note this should not be used in recursive functions, unless the maximum
 * number of calls is known to be small. Large amounts of recursion will run
 * the risk of overrunning the stack.
 * \sa VLA
 */

#ifdef MRTRIX_NO_NON_POD_VLA
#define NON_POD_VLA(name, type, num)                                                                                   \
  std::vector<type> __vla__##name(num);                                                                                \
  type *name = &__vla__##name[0]
#define NON_POD_VLA_MAX(name, type, num, max) type name[max]
#else
#define NON_POD_VLA(name, type, num) type name[num]
#define NON_POD_VLA_MAX(name, type, num, max) type name[num]
#endif

//! \}

#ifdef NDEBUG
#define FORCE_INLINE inline __attribute__((always_inline))
#else // don't force inlining in debug mode, so we can get more informative backtraces
#define FORCE_INLINE inline
#endif

namespace MR {

using float32 = float;
using float64 = double;
using cdouble = std::complex<double>;
using cfloat = std::complex<float>;

template <typename T> struct container_cast : public T {
  template <typename U> container_cast(const U &x) : T(x.begin(), x.end()) {}
};

//! the default type used throughout MRtrix
using default_type = double;

constexpr default_type NaN = std::numeric_limits<default_type>::quiet_NaN();
constexpr default_type Inf = std::numeric_limits<default_type>::infinity();

//! the type for the affine transform of an image:
using transform_type = Eigen::Transform<default_type, 3, Eigen::AffineCompact>;

//! used in various places for storing key-value pairs
using KeyValues = std::map<std::string, std::string>;

//! check whether type is complex:
template <class ValueType> struct is_complex : std::false_type {};
template <class ValueType> struct is_complex<std::complex<ValueType>> : std::true_type {};

//! check whether type is compatible with MRtrix3's file IO backend:
template <class ValueType>
struct is_data_type
    : std::integral_constant<bool, std::is_arithmetic<ValueType>::value || is_complex<ValueType>::value> {};

// required to allow use of abs() call on unsigned integers in template
// functions, etc, since the standard labels such calls ill-formed:
// http://en.cppreference.com/w/cpp/numeric/math/abs
template <typename X>
inline constexpr typename std::enable_if<std::is_arithmetic<X>::value && std::is_unsigned<X>::value, X>::type abs(X x) {
  return x;
}
template <typename X>
inline constexpr typename std::enable_if<std::is_arithmetic<X>::value && !std::is_unsigned<X>::value, X>::type
abs(X x) {
  return std::abs(x);
}
} // namespace MR

namespace std {

template <class T> inline ostream &operator<<(ostream &stream, const std::vector<T> &V) {
  stream << "[ ";
  for (size_t n = 0; n < V.size(); n++)
    stream << V[n] << " ";
  stream << "]";
  return stream;
}

template <class T, std::size_t N> inline ostream &operator<<(ostream &stream, const array<T, N> &V) {
  stream << "[ ";
  for (size_t n = 0; n < N; n++)
    stream << V[n] << " ";
  stream << "]";
  return stream;
}

} // namespace std
