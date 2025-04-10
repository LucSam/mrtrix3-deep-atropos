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

#include <mutex>

#include "dwi/tractography/SIFT2/fixel_updater.h"
#include "dwi/tractography/SIFT2/tckfactor.h"

#include "dwi/tractography/SIFT/track_contribution.h"

namespace MR::DWI::Tractography::SIFT2 {

FixelUpdater::FixelUpdater(TckFactor &tckfactor)
    : master(tckfactor),
      fixel_coeff_sums(tckfactor.fixels.size(), 0.0),
      fixel_TDs(tckfactor.fixels.size(), 0.0),
      fixel_counts(tckfactor.fixels.size(), 0) {}

FixelUpdater::~FixelUpdater() {
  std::lock_guard<std::mutex> lock(master.mutex);
  for (size_t i = 0; i != master.fixels.size(); ++i) {
    master.fixels[i].add_to_mean_coeff(fixel_coeff_sums[i]);
    master.fixels[i].add_TD(fixel_TDs[i], fixel_counts[i]);
  }
}

bool FixelUpdater::operator()(const SIFT::TrackIndexRange &range) {
  for (SIFT::track_t track_index = range.first; track_index != range.second; ++track_index) {
    const double coefficient = master.coefficients[track_index];
    const SIFT::TrackContribution &this_contribution(*(master.contributions[track_index]));
    const double weighting_factor = (coefficient > master.min_coeff) ? std::exp(coefficient) : 0.0;
    for (size_t j = 0; j != this_contribution.dim(); ++j) {
      const size_t fixel_index = this_contribution[j].get_fixel_index();
      const float length = this_contribution[j].get_length();
      fixel_coeff_sums[fixel_index] += length * coefficient;
      fixel_TDs[fixel_index] += length * weighting_factor;
      fixel_counts[fixel_index]++;
    }
  }
  return true;
}

} // namespace MR::DWI::Tractography::SIFT2
