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

#include "types.h"

namespace MR {

/** \defgroup loop Looping functions
  @{ */

//! a dummy image to iterate over, useful for multi-threaded looping.
class Iterator {
public:
  Iterator() = delete;
  template <class InfoType> Iterator(const InfoType &S) : d(S.ndim()), p(S.ndim(), 0) {
    for (size_t i = 0; i < S.ndim(); ++i)
      d[i] = S.size(i);
  }

  size_t ndim() const { return d.size(); }
  ssize_t size(size_t axis) const { return d[axis]; }

  const ssize_t &index(size_t axis) const { return p[axis]; }
  ssize_t &index(size_t axis) { return p[axis]; }

  friend std::ostream &operator<<(std::ostream &stream, const Iterator &V) {
    stream << "iterator, position [ ";
    for (size_t n = 0; n < V.ndim(); ++n)
      stream << V.index(n) << " ";
    stream << "]";
    return stream;
  }

private:
  std::vector<ssize_t> d, p;

  void value() const { assert(0); }
};

//! @}
} // namespace MR
