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

#include "command.h"
#include "dwi/directions/file.h"
#include "file/utils.h"
#include "math/SH.h"
#include "math/rng.h"
#include "progressbar.h"
#include "thread.h"

constexpr size_t default_number = 1e8;

using namespace MR;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Invert the polarity of individual directions"
             " so as to optimise a unipolar electrostatic repulsion model";

  DESCRIPTION
  + "The orientations themselves are not affected,"
    " only their polarity;"
    " this is necessary to ensure near-optimal distribution of DW directions"
    " for eddy-current correction.";

  ARGUMENTS
    + Argument ("in", "the input files for the directions.").type_file_in()
    + Argument ("out", "the output files for the directions.").type_file_out();


  OPTIONS
    + Option ("number", "number of shuffles to try"
                        " (default: " + str(default_number) + ")")
    +   Argument ("num").type_integer (1)

    + Option ("cartesian", "Output the directions in Cartesian coordinates [x y z]"
                           " instead of [az el].");
}
// clang-format on

using value_type = double;
using vector3_type = Eigen::Vector3d;

class Shared {
public:
  Shared(const Eigen::MatrixXd &directions, size_t target_num_shuffles)
      : directions(directions),
        target_num_shuffles(target_num_shuffles),
        num_shuffles(0),
        progress("optimising directions for eddy-currents", target_num_shuffles),
        best_signs(directions.rows(), 1),
        best_eddy(std::numeric_limits<value_type>::max()) {}

  bool update(value_type eddy, const std::vector<int> &signs) {
    std::lock_guard<std::mutex> lock(mutex);
    if (eddy < best_eddy) {
      best_eddy = eddy;
      best_signs = signs;
      progress.set_text(
          "optimising directions for eddy-currents (current best configuration: energy = " + str(best_eddy) + ")");
    }
    ++num_shuffles;
    ++progress;
    return num_shuffles < target_num_shuffles;
  }

  value_type eddy(size_t i, size_t j, const std::vector<int> &signs) const {
    vector3_type a = {directions(i, 0), directions(i, 1), directions(i, 2)};
    vector3_type b = {directions(j, 0), directions(j, 1), directions(j, 2)};
    if (signs[i] < 0)
      a = -a;
    if (signs[j] < 0)
      b = -b;
    return 1.0 / (a - b).norm();
  }

  std::vector<int> get_init_signs() const { return std::vector<int>(directions.rows(), 1); }
  const std::vector<int> &get_best_signs() const { return best_signs; }

protected:
  const Eigen::MatrixXd &directions;
  const size_t target_num_shuffles;
  size_t num_shuffles;
  ProgressBar progress;
  std::vector<int> best_signs;
  value_type best_eddy;
  std::mutex mutex;
};

class Processor {
public:
  Processor(Shared &shared) : shared(shared), signs(shared.get_init_signs()), uniform(0, signs.size() - 1) {}

  void execute() {
    while (eval())
      ;
  }

  void next_shuffle() { signs[uniform(rng)] *= -1; }

  bool eval() {
    next_shuffle();

    value_type eddy = 0.0;
    for (size_t i = 0; i < signs.size(); ++i)
      for (size_t j = i + 1; j < signs.size(); ++j)
        eddy += shared.eddy(i, j, signs);

    return shared.update(eddy, signs);
  }

protected:
  Shared &shared;
  std::vector<int> signs;
  Math::RNG rng;
  std::uniform_int_distribution<int> uniform;
};

void run() {
  auto directions = DWI::Directions::load_cartesian(argument[0]);

  size_t num_shuffles = get_option_value<size_t>("number", default_number);

  std::vector<int> signs;
  {
    Shared eddy_shared(directions, num_shuffles);
    Thread::run(Thread::multi(Processor(eddy_shared)), "eval thread");
    signs = eddy_shared.get_best_signs();
  }

  for (ssize_t n = 0; n < directions.rows(); ++n)
    if (signs[n] < 0)
      directions.row(n) *= -1.0;

  DWI::Directions::save(directions, argument[1], !get_options("cartesian").empty());
}
