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

#include "command.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/scalar_file.h"
#include "dwi/tractography/streamline.h"
#include "math/median.h"

#define DEFAULT_SMOOTHING 4.0

using namespace MR;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  SYNOPSIS = "Gaussian filter a track scalar file";

  ARGUMENTS
  + Argument ("input", "the input track scalar file.").type_file_in()
  + Argument ("output", "the output track scalar file").type_file_out();

  OPTIONS
  + Option ("stdev", "apply Gaussian smoothing with the specified standard deviation."
                     " The standard deviation is defined in units of track points"
                     " (default: " + str(DEFAULT_SMOOTHING, 2) + ")")
    + Argument ("sigma").type_float(1e-6);

}
// clang-format on

using value_type = float;

void run() {
  DWI::Tractography::Properties properties;
  DWI::Tractography::ScalarReader<value_type> reader(argument[0], properties);
  DWI::Tractography::ScalarWriter<value_type> writer(argument[1], properties);

  float stdev = get_option_value("stdev", DEFAULT_SMOOTHING);

  std::vector<float> kernel(2 * ceil(2.5 * stdev) + 1, 0);
  float norm_factor = 0.0;
  float radius = (kernel.size() - 1.0) / 2.0;
  for (size_t c = 0; c < kernel.size(); ++c) {
    kernel[c] = exp(-(c - radius) * (c - radius) / (2 * stdev * stdev));
    norm_factor += kernel[c];
  }
  for (size_t c = 0; c < kernel.size(); c++)
    kernel[c] /= norm_factor;

  DWI::Tractography::TrackScalar<value_type> tck_scalar;
  while (reader(tck_scalar)) {
    DWI::Tractography::TrackScalar<value_type> tck_scalars_smoothed(tck_scalar.size());
    tck_scalars_smoothed.set_index(tck_scalar.get_index());

    for (int i = 0; i < (int)tck_scalar.size(); ++i) {
      float norm_factor = 0.0;
      float value = 0.0;
      for (int k = -(int)radius; k <= (int)radius; ++k) {
        if (i + k >= 0 && i + k < (int)tck_scalar.size()) {
          value += kernel[k + radius] * tck_scalar[i + k];
          norm_factor += kernel[k + radius];
        }
      }
      tck_scalars_smoothed[i] = value / norm_factor;
    }
    writer(tck_scalars_smoothed);
  }
}
