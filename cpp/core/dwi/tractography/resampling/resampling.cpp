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

#include "dwi/tractography/resampling/resampling.h"

#include "types.h"

#include "dwi/tractography/resampling/arc.h"
#include "dwi/tractography/resampling/downsampler.h"
#include "dwi/tractography/resampling/endpoints.h"
#include "dwi/tractography/resampling/fixed_num_points.h"
#include "dwi/tractography/resampling/fixed_step_size.h"
#include "dwi/tractography/resampling/upsampler.h"

namespace MR::DWI::Tractography::Resampling {

using namespace App;

// clang-format off
const OptionGroup ResampleOption =
    OptionGroup("Streamline resampling options")
    + Option("upsample",
             "increase the density of points along the length of each streamline by some factor"
             " (may improve mapping streamlines to ROIs, and/or visualisation)")
      + Argument("ratio").type_integer(1)
    + Option("downsample",
             "increase the density of points along the length of each streamline by some factor"
             " (decreases required storage space)")
      + Argument("ratio").type_integer(1)
    + Option("step_size",
             "re-sample the streamlines to a desired step size (in mm)")
      + Argument("value").type_float(0.0)
    + Option("num_points",
             "re-sample each streamline to a fixed number of points")
      + Argument("count").type_integer(2)
    + Option("endpoints",
             "only output the two endpoints of each streamline")
    + Option("line",
             "resample tracks at 'num' equidistant locations along a line between 'start' and 'end'"
             " (specified as comma-separated 3-vectors in scanner coordinates)")
      + Argument("num").type_integer(2)
      + Argument("start").type_sequence_float()
      + Argument("end").type_sequence_float()

    + Option("arc",
             "resample tracks at 'num' equidistant locations"
             " along a circular arc specified by points 'start', 'mid' and 'end'"
             " (specified as comma-separated 3-vectors in scanner coordinates)")
      + Argument("num").type_integer(2)
      + Argument("start").type_sequence_float()
      + Argument("mid").type_sequence_float()
      + Argument("end").type_sequence_float();
// clang-format on

namespace {

using value_type = float;
using point_type = Eigen::Vector3f;

point_type get_pos(const std::vector<default_type> &s) {
  if (s.size() != 3)
    throw Exception("position expected as a comma-seperated list of 3 values");
  return {value_type(s[0]), value_type(s[1]), value_type(s[2])};
}
} // namespace

Base *get_resampler() {
  const size_t count = (!get_options("upsample").empty() ? 1 : 0) + (!get_options("downsample").empty() ? 1 : 0) +
                       (!get_options("step_size").empty() ? 1 : 0) + (!get_options("num_points").empty() ? 1 : 0) +
                       (!get_options("endpoints").empty() ? 1 : 0) + (!get_options("line").empty() ? 1 : 0) +
                       (!get_options("arc").empty() ? 1 : 0);
  if (!count)
    throw Exception("Must specify a mechanism for resampling streamlines");
  if (count > 1)
    throw Exception("Can only use one form of streamline resampling");

  auto opt = get_options("upsample");
  if (!opt.empty())
    return new Upsampler(opt[0][0]);
  opt = get_options("downsample");
  if (!opt.empty())
    return new Downsampler(opt[0][0]);
  opt = get_options("step_size");
  if (!opt.empty())
    return new FixedStepSize(opt[0][0]);
  opt = get_options("num_points");
  if (!opt.empty())
    return new FixedNumPoints(opt[0][0]);
  opt = get_options("endpoints");
  if (!opt.empty())
    return new Endpoints;
  opt = get_options("line");
  if (!opt.empty())
    return new Arc(opt[0][0], get_pos(opt[0][1].as_sequence_float()), get_pos(opt[0][2].as_sequence_float()));
  opt = get_options("arc");
  if (!opt.empty())
    return new Arc(opt[0][0],
                   get_pos(opt[0][1].as_sequence_float()),
                   get_pos(opt[0][2].as_sequence_float()),
                   get_pos(opt[0][3].as_sequence_float()));

  assert(0);
  return nullptr;
}

} // namespace MR::DWI::Tractography::Resampling
