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

class FixedStepSize : public BaseCRTP<FixedStepSize> {

public:
  FixedStepSize() : step_size(value_type(0)) {}

  FixedStepSize(const float ss) : step_size(ss) {}

  bool operator()(const Streamline<> &, Streamline<> &) const override;
  bool valid() const override { return step_size; }

  void set_step_size(const value_type ss) { step_size = ss; }
  float get_step_size() const { return step_size; }

private:
  value_type step_size;
};

} // namespace MR::DWI::Tractography::Resampling
