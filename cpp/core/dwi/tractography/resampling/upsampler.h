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

class Upsampler : public BaseCRTP<Upsampler> {

public:
  Upsampler() : data(4, 3) {}

  Upsampler(const size_t os_ratio) : data(4, 3) { set_ratio(os_ratio); }

  Upsampler(const Upsampler &that) : M(that.M), temp(M.rows(), 3), data(4, 3) {}

  ~Upsampler() {}

  bool operator()(const Streamline<> &, Streamline<> &) const override;
  bool valid() const override { return true; }

  void set_ratio(const size_t);
  size_t get_ratio() const { return (M.rows() ? (M.rows() + 1) : 1); }

private:
  Eigen::MatrixXf M;
  mutable Eigen::MatrixXf temp, data;

  void interp_prepare(Streamline<> &) const;
  void increment(const point_type &) const;
};

} // namespace MR::DWI::Tractography::Resampling
