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

#include <iostream>

#include "math/least_squares.h"
#include "opengl/glutils.h"

namespace MR::GUI::GL {

class vec4 {
public:
  vec4() {}
  vec4(float x, float y, float z, float w) {
    v[0] = x;
    v[1] = y;
    v[2] = z;
    v[3] = w;
  }
  vec4(const Eigen::Quaternionf &V) {
    v[0] = V.x();
    v[1] = V.y();
    v[2] = V.z();
    v[3] = V.w();
  }
  template <class Cont> vec4(const Cont &p, float w) {
    v[0] = p[0];
    v[1] = p[1];
    v[2] = p[2];
    v[3] = w;
  }
  vec4(const float *p) { memcpy(v, p, sizeof(v)); }

  void zero() { memset(v, 0, sizeof(v)); }

  operator const GLfloat *() const { return v; }
  operator GLfloat *() { return v; }

  friend std::ostream &operator<<(std::ostream &stream, const vec4 &v) {
    for (size_t i = 0; i < 4; ++i)
      stream << v[i] << " ";
    return stream;
  }

protected:
  GLfloat v[4];
};

class mat4 {
public:
  mat4() {}
  mat4(const mat4 &a) { memcpy(m, a.m, sizeof(m)); }
  mat4(const float *p) { memcpy(m, p, sizeof(m)); }
  mat4(const Eigen::Quaternionf &v) {
    const auto R = v.matrix();
    zero();
    for (size_t i = 0; i != 3; ++i) {
      for (size_t j = 0; j != 3; ++j)
        (*this)(i, j) = R(i, j);
    }
    (*this)(3, 3) = 1.0f;
  }
  template <class M> mat4(const M &a) {
    for (size_t i = 0; i != size_t(a.rows()); ++i) {
      for (size_t j = 0; j != 4; ++j)
        (*this)(i, j) = a(i, j);
    }
    if (a.rows() == 3) {
      (*this)(3, 0) = (*this)(3, 1) = (*this)(3, 2) = 0.0f;
      (*this)(3, 3) = 1.0f;
    }
  }

  mat4 &operator=(const mat4 &a) {
    memcpy(m, a.m, sizeof(m));
    return *this;
  }

  void zero() { memset(m, 0, sizeof(m)); }

  operator const GLfloat *() const { return m; }
  operator GLfloat *() { return m; }

  GLfloat &operator()(size_t i, size_t j) { return m[i + 4 * j]; }
  const GLfloat &operator()(size_t i, size_t j) const { return m[i + 4 * j]; }

  mat4 operator*(const mat4 &a) const {
    mat4 t;
    for (size_t j = 0; j < 4; ++j) {
      for (size_t i = 0; i < 4; ++i) {
        t(i, j) = 0.0f;
        for (size_t k = 0; k < 4; ++k)
          t(i, j) += (*this)(i, k) * a(k, j);
      }
    }
    return t;
  }

  vec4 operator*(const vec4 &v) const {
    vec4 r;
    r.zero();
    for (size_t j = 0; j < 4; ++j)
      for (size_t i = 0; i < 4; ++i)
        r[i] += (*this)(i, j) * v[j];
    return r;
  }

  mat4 &operator*=(const mat4 &a) {
    *this = (*this) * a;
    return *this;
  }

  friend std::ostream &operator<<(std::ostream &stream, const mat4 &m) {
    for (size_t i = 0; i < 4; ++i) {
      for (size_t j = 0; j < 4; ++j)
        stream << m(i, j) << " ";
      stream << "\n";
    }
    return stream;
  }

protected:
  GLfloat m[16];
};

inline mat4 identity() {
  mat4 m;
  m.zero();
  m(0, 0) = m(1, 1) = m(2, 2) = m(3, 3) = 1.0f;
  return m;
}

inline mat4 transpose(const mat4 &a) {
  mat4 b;
  for (size_t j = 0; j < 4; ++j)
    for (size_t i = 0; i < 4; ++i)
      b(i, j) = a(j, i);
  return b;
}

inline mat4 inv(const mat4 &a) {
  Eigen::Matrix<float, 4, 4> A;
  for (size_t i = 0; i != 4; ++i) {
    for (size_t j = 0; j != 4; ++j)
      A(i, j) = a(i, j);
  }
  return A.inverse().eval();
}

inline mat4 ortho(float L, float R, float B, float T, float N, float F) {
  mat4 m;
  m.zero();
  m(0, 0) = 2.0f / (R - L);
  m(1, 1) = 2.0f / (T - B);
  m(2, 2) = 2.0f / (N - F);
  m(3, 3) = 1.0f;

  m(0, 3) = (R + L) / (R - L);
  m(1, 3) = (T + B) / (T - B);
  m(2, 3) = (F + N) / (F - N);

  return m;
}

inline mat4 frustum(float L, float R, float B, float T, float N, float F) {
  mat4 m;
  m.zero();

  m(0, 0) = 2.0f * N / (R - L);
  m(1, 1) = 2.0f * N / (T - B);
  m(0, 2) = (R + L) / (R - L);
  m(1, 2) = (T + B) / (T - B);
  m(2, 2) = (F + N) / (N - F);
  m(3, 2) = -1.0f;
  m(2, 3) = 2.0f * F * N / (N - F);

  return m;
}

inline mat4 translate(float x, float y, float z) {
  mat4 m = identity();
  m(0, 3) = x;
  m(1, 3) = y;
  m(2, 3) = z;
  m(3, 3) = 1.0f;
  return m;
}

template <class Cont> inline mat4 translate(const Cont &x) { return translate(x[0], x[1], x[2]); }

inline mat4 scale(float x, float y, float z) {
  mat4 m;
  m.zero();
  m(0, 0) = x;
  m(1, 1) = y;
  m(2, 2) = z;
  m(3, 3) = 1.0f;
  return m;
}

inline mat4 scale(float s) { return scale(s, s, s); }

} // namespace MR::GUI::GL
