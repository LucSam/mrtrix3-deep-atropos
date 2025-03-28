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

#include "adapter/base.h"

namespace MR::Adapter {

template <class ImageType> class PermuteAxes : public Base<PermuteAxes<ImageType>, ImageType> {
public:
  using base_type = Base<PermuteAxes<ImageType>, ImageType>;
  using value_type = typename ImageType::value_type;

  using base_type::parent;
  using base_type::size;

  PermuteAxes(const ImageType &original, const std::vector<int> &axes) : base_type(original), axes_(axes) {
    for (int i = 0; i < static_cast<int>(parent().ndim()); ++i) {
      for (size_t a = 0; a < axes_.size(); ++a) {
        if (axes_[a] >= int(parent().ndim()))
          throw Exception("axis " + str(axes_[a]) + " exceeds image dimensionality");
        if (axes_[a] == i)
          goto next_axis;
      }
      if (parent().size(i) != 1)
        throw Exception("omitted axis \"" + str(i) + "\" has dimension greater than 1");
    next_axis:
      continue;
    }

    int non_existent_index = -1;
    for (auto &a : axes_) {
      if (a < 0) {
        a = non_existent_index--;
        non_existent_axes.push_back(0);
      }
    }
  }

  size_t ndim() const { return axes_.size(); }
  ssize_t size(size_t axis) const { return axes_[axis] < 0 ? 1 : parent().size(axes_[axis]); }
  default_type spacing(size_t axis) const {
    return axes_[axis] < 0 ? std::numeric_limits<default_type>::quiet_NaN() : parent().spacing(axes_[axis]);
  }
  ssize_t stride(size_t axis) const { return axes_[axis] < 0 ? 0 : parent().stride(axes_[axis]); }

  void reset() { parent().reset(); }

  ssize_t get_index(size_t axis) const {
    const auto a = axes_[axis];
    return a < 0 ? non_existent_axes[-1 - a] : parent().index(a);
  }
  void move_index(size_t axis, ssize_t increment) {
    const auto a = axes_[axis];
    if (a < 0)
      non_existent_axes[-1 - a] += increment;
    else
      parent().index(a) += increment;
  }

private:
  std::vector<int> axes_;
  std::vector<size_t> non_existent_axes;
};

} // namespace MR::Adapter
