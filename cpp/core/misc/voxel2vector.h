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

#include <limits>

#include "algo/loop.h"
#include "exception.h"
#include "header.h"
#include "image.h"
#include "types.h"

#include "adapter/replicate.h"

namespace MR {

// A class to achieve a mapping from a voxel position in an image
//   with any number of axes, to an index within a 1D vector of data.
class Voxel2Vector {
public:
  typedef uint32_t index_t;

  static const index_t invalid = std::numeric_limits<index_t>::max();

  template <class MaskType> Voxel2Vector(MaskType &mask, const Header &data);

  template <class MaskType> Voxel2Vector(MaskType &mask) : Voxel2Vector(mask, Header(mask)) {}

  Voxel2Vector(const Header &header)
      : forward(Image<index_t>::scratch(header, "Voxel to vector index conversion scratch image")) {
    reverse.reserve(voxel_count(header));
    index_t counter = 0;
    for (auto l = Loop(header)(forward); l; ++l) {
      forward.value() = counter++;
      reverse.push_back(pos());
    }
    DEBUG("Voxel2vector class for image \"" + header.name() + "\" of size " + join(pos(), "x") + " initialised with " +
          str(reverse.size()) + " elements");
  }

  size_t size() const { return reverse.size(); }

  const std::vector<index_t> &operator[](const size_t index) const {
    assert(index < reverse.size());
    return reverse[index];
  }

  template <class PosType> index_t operator()(const PosType &pos) const {
    Image<index_t> temp(forward); // For thread-safety
    assign_pos_of(pos).to(temp);
    if (is_out_of_bounds(temp))
      return invalid;
    return temp.value();
  }

private:
  Image<index_t> forward;
  std::vector<std::vector<index_t>> reverse;

  std::vector<index_t> pos() const {
    std::vector<index_t> result;
    for (size_t index = 0; index != forward.ndim(); ++index)
      result.push_back(forward.index(index));
    return result;
  }
};

template <class MaskType>
Voxel2Vector::Voxel2Vector(MaskType &mask, const Header &data)
    : forward(Image<index_t>::scratch(data, "Voxel to vector index conversion scratch image")) {
  if (!dimensions_match(mask, data, 0, std::min(mask.ndim(), data.ndim())))
    throw Exception("Dimension mismatch between image data and processing mask");
  // E.g. Mask may be 3D but data are 4D; for any voxel where the mask is
  //   true, want to include data from all volumes
  Adapter::Replicate<MaskType> r_mask(mask, data);
  // Loop in axis order so that those voxels contiguous in memory are still
  //   contiguous in the vectorised data
  index_t counter = 0;
  for (auto l = Loop(data)(r_mask, forward); l; ++l) {
    if (r_mask.value()) {
      forward.value() = counter++;
      reverse.push_back(pos());
    } else {
      forward.value() = invalid;
    }
  }
  DEBUG("Voxel2vector class for image \"" + data.name() + "\" of size " + join(pos(), "x") + " initialised with " +
        str(reverse.size()) + " elements");
}

} // namespace MR
