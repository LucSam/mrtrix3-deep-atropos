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

#include "dwi/tractography/SIFT2/streamline_stats.h"

namespace MR::DWI::Tractography::SIFT2 {

StreamlineStats::StreamlineStats()
    : min(std::numeric_limits<double>::infinity()),
      max(-std::numeric_limits<double>::infinity()),
      mean(0.0),
      mean_abs(0.0),
      var(0.0),
      count(0),
      nonzero(0) {}

StreamlineStats::StreamlineStats(const StreamlineStats &that)
    : min(std::numeric_limits<double>::infinity()),
      max(-std::numeric_limits<double>::infinity()),
      mean(0.0),
      mean_abs(0.0),
      var(0.0),
      count(0),
      nonzero(0) {}

StreamlineStats &StreamlineStats::operator+=(const double i) {
  min = std::min(min, i);
  max = std::max(max, i);
  mean += i;
  mean_abs += abs(i);
  var += Math::pow2(i);
  ++count;
  if (i)
    ++nonzero;
  return *this;
}

StreamlineStats &StreamlineStats::operator+=(const StreamlineStats &i) {
  min = std::min(min, i.min);
  max = std::max(max, i.max);
  mean += i.mean;
  mean_abs += i.mean_abs;
  var += i.var;
  count += i.count;
  nonzero += i.nonzero;
  return *this;
}

void StreamlineStats::normalise() {
  assert(count);
  mean /= double(count);
  mean_abs /= double(count);
  var /= double(count - 1);
}

} // namespace MR::DWI::Tractography::SIFT2
