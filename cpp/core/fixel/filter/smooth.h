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

#include "fixel/filter/base.h"
#include "fixel/fixel.h"
#include "fixel/matrix.h"

#define DEFAULT_FIXEL_SMOOTHING_FWHM 10.0
#define DEFAULT_FIXEL_SMOOTHING_MINWEIGHT 0.01

namespace MR::Fixel::Filter {

/** \addtogroup Filters
@{ */

/*! Smooth fixel data using a fixel-fixel connectivity matrix.
 *
 * Typical usage:
 * \code
 * auto fixel_index = Image<index_type>::open (index_image_path);
 * auto fixel_data_in = Image<float>::open (fixel_data_path);
 * auto fixel_matrix = Fixel::Matrix::Reader (fixel_matrix_path);
 * Fixel::Filter::Smooth smooth_filter (fixel_index, fixel_matrix);
 * auto fixel_data_out = Image::create<float> (fixel_data_out, fixel_data_in);
 * smooth_filter (fixel_data_in, fixel_data_out);
 *
 * \endcode
 */

class Smooth : public Base {

public:
  Smooth(Image<index_type> index_image,
         const Matrix::Reader &matrix,
         const Image<bool> &mask_image,
         const float smoothing_fwhm,
         const float smoothing_threshold);
  Smooth(Image<index_type> index_image, const Matrix::Reader &matrix, const Image<bool> &mask_image);
  Smooth(Image<index_type> index_image,
         const Matrix::Reader &matrix,
         const float smoothing_fwhm,
         const float smoothing_threshold);
  Smooth(Image<index_type> index_image, const Matrix::Reader &matrix);

  void set_fwhm(const float fwhm);

  void operator()(Image<float> &input, Image<float> &output) const override;

protected:
  Image<bool> mask_image;
  Matrix::Reader matrix;
  std::vector<Eigen::Vector3f> fixel_positions;
  float stdev, gaussian_const1, gaussian_const2, threshold;
};
//! @}

} // namespace MR::Fixel::Filter
