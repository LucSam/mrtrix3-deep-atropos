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

#include <memory>
#include <stdint.h>

#include "types.h"

#include "connectome/mat2vec.h"
#include "stats/enhance.h"
#include "stats/tfce.h"

namespace MR::Connectome::Enhance {

using value_type = Math::Stats::value_type;

// This should be possible to use for any domain of inference
class PassThrough : public Stats::EnhancerBase {
public:
  PassThrough() {}
  virtual ~PassThrough() {}

private:
  void operator()(in_column_type, out_column_type) const override;
};

class NBS : public Stats::TFCE::EnhancerBase {
public:
  NBS() = delete;
  NBS(const node_t i) : threshold(0.0) { initialise(i); }
  NBS(const node_t i, const value_type t) : threshold(t) { initialise(i); }
  NBS(const NBS &that) = default;
  virtual ~NBS() {}

  void set_threshold(const value_type t) { threshold = t; }

  void operator()(in_column_type in, out_column_type out) const override { (*this)(in, threshold, out); }

  void operator()(in_column_type, const value_type, out_column_type) const override;

protected:
  std::shared_ptr<std::vector<std::vector<size_t>>> adjacency;
  value_type threshold;

private:
  void initialise(const node_t);
};

} // namespace MR::Connectome::Enhance
