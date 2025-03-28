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

#include <Eigen/Dense>

namespace MR::Math::Stats {

using value_type = MR::default_type;
using matrix_type = Eigen::Matrix<value_type, Eigen::Dynamic, Eigen::Dynamic>;
using vector_type = Eigen::Array<value_type, Eigen::Dynamic, 1>;
using index_type = uint32_t;
using index_array_type = Eigen::Array<index_type, Eigen::Dynamic, 1>;

} // namespace MR::Math::Stats
