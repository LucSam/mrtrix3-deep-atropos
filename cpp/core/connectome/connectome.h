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

#include <Eigen/Dense>

#include "app.h"
#include "exception.h"
#include "header.h"
#include "mrtrix.h"
#include "types.h"

namespace MR::Connectome {

using node_t = uint32_t;

using value_type = default_type;
using matrix_type = Eigen::Array<value_type, Eigen::Dynamic, Eigen::Dynamic>;
using vector_type = Eigen::Array<value_type, Eigen::Dynamic, 1>;
using mask_type = Eigen::Array<bool, Eigen::Dynamic, Eigen::Dynamic>;

extern const App::OptionGroup MatrixOutputOptions;

template <class MatrixType> void check(const MatrixType &in, const node_t num_nodes = 0) {
  if (in.rows() != in.cols())
    throw Exception("Connectome matrix is not square (" + str(in.rows()) + " x " + str(in.cols()) + ")");
  if (num_nodes && (in.rows() != num_nodes))
    throw Exception("Connectome matrix contains " + str(in.rows()) + " nodes; expected " + str(num_nodes));
}

template <class MatrixType> bool is_directed(MatrixType &in) {
  if (in.rows() != in.cols())
    throw Exception("Connectome matrix is not square (" + str(in.rows()) + " x " + str(in.cols()) + ")");

  for (node_t row = 0; row != in.rows(); ++row) {
    for (node_t col = row + 1; col != in.cols(); ++col) {

      const typename MatrixType::Scalar lower_value = in(col, row);
      const typename MatrixType::Scalar upper_value = in(row, col);

      if (upper_value && lower_value && (upper_value != lower_value))
        return true;
    }
  }

  return false;
}

template <class MatrixType> void to_symmetric(MatrixType &in) {
  if (is_directed(in))
    throw Exception("Cannot convert a non-symmetric directed matrix to be symmetric");

  for (node_t row = 0; row != in.rows(); ++row) {
    for (node_t col = row + 1; col != in.cols(); ++col) {

      const typename MatrixType::Scalar lower_value = in(col, row);
      const typename MatrixType::Scalar upper_value = in(row, col);

      if (upper_value && !lower_value)
        in(row, col) = in(col, row) = upper_value;
      else if (lower_value && !upper_value)
        in(row, col) = in(col, row) = lower_value;
    }
  }
}

template <class MatrixType> void to_upper(MatrixType &in) {
  if (is_directed(in))
    throw Exception("Cannot convert a non-symmetric directed matrix to upper triangular");

  for (node_t row = 0; row != in.rows(); ++row) {
    for (node_t col = row + 1; col != in.cols(); ++col) {

      const typename MatrixType::Scalar lower_value = in(col, row);
      const typename MatrixType::Scalar upper_value = in(row, col);

      if (!upper_value && lower_value)
        in(row, col) = lower_value;
      in(col, row) = typename MatrixType::Scalar(0);
    }
  }
}

void check(const Header &);

} // namespace MR::Connectome
