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

#include <deque>
#include <iterator>
// #include <algorithm> // std::min_element
#include "math/math.h"
#include "math/median.h"
// #include "math/gradient_descent.h"

namespace MR {
using namespace MR::Math;

namespace Registration::Transform {
// convergence check using linear trend of each parameter during gradient descent
// we use double exponential smoothing to get rid of small oscillations
class DoubleExpSmoothSlopeCheck {
public:
  DoubleExpSmoothSlopeCheck() : stop_cnt(0), iter_count(0), len(0), is_initialised(false) {}

  void set_parameters(const Eigen::Matrix<default_type, Eigen::Dynamic, 1> &slope_threshold,
                      default_type alpha_in,
                      default_type beta_in,
                      size_t buffer_length,
                      size_t min_iter_in);

  bool go_on(const Eigen::Matrix<default_type, Eigen::Dynamic, 1> &element);

  bool last_b(Eigen::Matrix<default_type, Eigen::Dynamic, 1> &b) const;

  bool last_s(Eigen::Matrix<default_type, Eigen::Dynamic, 1> &s) const;

  void debug(const Eigen::Matrix<default_type, Eigen::Dynamic, 1> &vec) const;

private:
  size_t stop_cnt;
  default_type alpha, beta;
  Eigen::Matrix<default_type, Eigen::Dynamic, 1> thresh;
  Eigen::Matrix<default_type, Eigen::Dynamic, 1> x0;
  size_t buffer_len, min_iter;
  size_t iter_count, len;
  std::deque<Eigen::Matrix<default_type, Eigen::Dynamic, 1>> ds, db;
  bool is_initialised;

  inline bool check_all(const Eigen::Matrix<default_type, Eigen::Dynamic, 1> &vec) {
    return (vec.array().abs() < thresh.array()).all();
  }
};
} // namespace Registration::Transform

} // namespace MR
