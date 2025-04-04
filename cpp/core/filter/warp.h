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

#include "adapter/reslice.h"
#include "adapter/warp.h"
#include "algo/threaded_copy.h"
#include "algo/threaded_loop.h"
#include "datatype.h"
#include "filter/reslice.h"
#include "interp/cubic.h"

namespace MR::Filter {

// TODO if there is a use for this elsewhere then we should have threaded_copy4D convenience functions
class CopyKernel4D {
public:
  template <class InputImageType, class OutputImageType>
  FORCE_INLINE void operator()(InputImageType &in, OutputImageType &out) const {
    out.row(3) = in.row(3);
  }
};

//! convenience function to warp one image onto another
/*! This function resamples (regrids) the Image \a source onto the
 * Image& \a destination, using the templated interpolator class and a supplied deformation field.
 *
 * For example:
 * \code
 * // source and destination data:
 * auto source = Image<float>::open(argument[0]);
 *
 * auto warp = Image<float>::open(argument[1]);
 *
 * auto template = Header::open(argument[2]);
 *
 * auto destination = Image<float>::create (argument[3], template)
 *
 * // regrid source onto destination using linear interpolation:
 * Filter::warp<Image::Interp::Linear> (source, destination, warp);
 * \endcode
 */
template <template <class VoxelType> class Interpolator,
          class ImageTypeDestination,
          class ImageTypeSource,
          class WarpType>
void warp(ImageTypeSource &source,
          ImageTypeDestination &destination,
          WarpType &warp,
          const typename ImageTypeDestination::value_type value_when_out_of_bounds =
              Interpolator<ImageTypeSource>::default_out_of_bounds_value(),
          const std::vector<uint32_t> oversample = Adapter::AutoOverSample,
          const bool jacobian_modulate = false) {

  // reslice warp onto destination grid
  if (warp.transform().matrix() != destination.transform().matrix() || !dimensions_match(warp, destination, 0, 3) ||
      !spacings_match(warp, destination, 0, 3)) {

    Header header(destination);
    header.ndim() = 4;
    header.size(3) = 3;
    Stride::set(header, Stride::contiguous_along_axis(3, header));
    auto warp_resliced = Image<typename WarpType::value_type>::scratch(header);
    reslice<Interp::Cubic>(warp, warp_resliced, Adapter::NoTransform, oversample);

    Adapter::Warp<Interpolator, ImageTypeSource, Image<typename WarpType::value_type>> interp(
        source, warp_resliced, value_when_out_of_bounds, jacobian_modulate);

    if (destination.ndim() == 4)
      ThreadedLoop("warping \"" + source.name() + "\"" +
                       (jacobian_modulate ? " with Jacobian intensity modulation" : ""),
                   interp,
                   0,
                   3,
                   1)
          .run(CopyKernel4D(), interp, destination);
    else
      threaded_copy_with_progress_message("warping \"" + source.name() + "\"" +
                                              (jacobian_modulate ? " with Jacobian intensity modulation" : ""),
                                          interp,
                                          destination);

    // no need to reslice warp
  } else {
    Adapter::Warp<Interpolator, ImageTypeSource, Image<typename WarpType::value_type>> interp(
        source, warp, value_when_out_of_bounds, jacobian_modulate);
    if (destination.ndim() == 4 && destination.is_direct_io())
      ThreadedLoop("warping \"" + source.name() + "\"" +
                       (jacobian_modulate ? " with Jacobian intensity modulation" : ""),
                   interp,
                   0,
                   3,
                   1)
          .run(CopyKernel4D(), interp, destination);
    else
      threaded_copy_with_progress_message("warping \"" + source.name() + "\"" +
                                              (jacobian_modulate ? " with Jacobian intensity modulation" : ""),
                                          interp,
                                          destination,
                                          0,
                                          destination.ndim(),
                                          2);
  }
}

//! @}
} // namespace MR::Filter
