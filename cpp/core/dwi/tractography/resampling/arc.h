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

#include "dwi/tractography/resampling/resampling.h"

namespace MR::DWI::Tractography::Resampling {

// Also handles resampling along a fixed line
class Arc : public BaseCRTP<Arc> {

  using value_type = float;
  using point_type = Eigen::Vector3f;

  enum class state_t { BEFORE_START, AFTER_START, BEFORE_END, AFTER_END };

private:
  class Plane {
  public:
    Plane(const point_type &pos, const point_type &dir) : n(dir) {
      n.normalize();
      d = n.dot(pos);
    }
    value_type dist(const point_type &pos) const { return n.dot(pos) - d; }

  private:
    point_type n;
    value_type d;
  };

  std::vector<Plane> planes;

public:
  Arc(const size_t n, const point_type &s, const point_type &e)
      : nsamples(n), start(s), mid(0.5 * (s + e)), end(e), idx_start(0), idx_end(0) {
    init_line();
  }

  Arc(const size_t n, const point_type &s, const point_type &w, const point_type &e)
      : nsamples(n), start(s), mid(w), end(e), idx_start(0), idx_end(0) {
    init_arc();
  }

  bool operator()(const Streamline<> &, Streamline<> &) const override;
  bool valid() const override { return nsamples; }

private:
  const size_t nsamples;
  const point_type start, mid, end;

  mutable size_t idx_start, idx_end;
  mutable point_type start_dir, mid_dir, end_dir;

  void init_line();
  void init_arc();

  state_t state(const point_type &) const;
};

} // namespace MR::DWI::Tractography::Resampling
