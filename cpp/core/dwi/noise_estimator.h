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

#include "dwi/gradient.h"
#include "image.h"
#include "math/SH.h"
#include "math/Sn_scale_estimator.h"
#include "math/least_squares.h"

namespace MR::DWI {

namespace {

template <class InputImageType, class OutputImageType, class MatrixType> class NoiseEstimatorFunctor {
public:
  NoiseEstimatorFunctor(const MatrixType &SH2amp_mapping, int axis, InputImageType &dwi, OutputImageType &noise)
      : dwi(dwi),
        noise(noise),
        H(SH2amp_mapping * Math::pinv(SH2amp_mapping)),
        S(H.cols(), dwi.size(axis)),
        R(S.cols(), S.rows()),
        leverage(H.rows()),
        axis(axis) {
    for (ssize_t n = 0; n < leverage.size(); ++n)
      leverage[n] = H(n, n) < 1.0 ? 1.0 / std::sqrt(1.0 - H(n, n)) : 1.0;
  }

  void operator()(const Iterator &pos) {
    assign_pos_of(pos).to(dwi, noise);
    for (auto l = Loop(axis)(dwi); l; ++l)
      for (auto l2 = Loop(3)(dwi); l2; ++l2)
        S(dwi.index(3), dwi.index(axis)) = dwi.value();

    R.noalias() = H.selfadjointView<Eigen::Lower>() * S - S;

    for (auto l = Loop(axis)(noise); l; ++l) {
      R.col(noise.index(axis)).array() *= leverage.array();
      noise.value() = scale_estimator(R.col(noise.index(axis)));
    }
  }

protected:
  InputImageType dwi;
  OutputImageType noise;
  Eigen::MatrixXd H, S, R;
  Eigen::VectorXd leverage;
  Math::Sn_scale_estimator<default_type> scale_estimator;
  int axis;
};
} // namespace

template <class InputImageType, class OutputImageType, class MatrixType>
inline void estimate_noise(InputImageType &dwi, OutputImageType &noise, const MatrixType &SH2amp_mapping) {
  auto loop = ThreadedLoop("estimating noise level", dwi, 0, 3);
  NoiseEstimatorFunctor<InputImageType, OutputImageType, MatrixType> functor(
      SH2amp_mapping, loop.inner_axes[0], dwi, noise);
  loop.run_outer(functor);
}

} // namespace MR::DWI
