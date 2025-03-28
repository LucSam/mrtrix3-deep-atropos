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

#include "app.h"
#include "progressbar.h"
#include "types.h"

#include "misc/bitset.h"

#include "math/stats/typedefs.h"
#include <vector>

#define DEFAULT_NUMBER_SHUFFLES 5000
#define DEFAULT_NUMBER_SHUFFLES_NONSTATIONARITY 5000

namespace MR::Math::Stats {

// Generic command-line options:
// - Set nature of errors
// - Set number of shuffles (actual & nonstationarity correction)
// - Import permutations (actual & nonstationarity correction)
// - (future) Set exchangeability blocks

extern std::vector<std::string> error_types;
App::OptionGroup shuffle_options(const bool include_nonstationarity, const default_type default_skew = 1.0);

class Shuffle {
public:
  index_type index;
  matrix_type data;
};

class Shuffler {
public:
  typedef std::vector<index_type> PermuteLabels;
  enum class error_t { EE, ISE, BOTH };

  // First version reads command-line options in order to determine parameters prior to running initialise();
  //   second and third versions more-or-less call initialise() directly
  Shuffler(const index_type num_rows, const bool is_nonstationarity, const std::string msg = "");

  Shuffler(const index_type num_rows,
           const index_type num_shuffles,
           const error_t error_types,
           const bool is_nonstationarity,
           const std::string msg = "");

  Shuffler(const index_type num_rows,
           const index_type num_shuffles,
           const error_t error_types,
           const bool is_nonstationarity,
           const index_array_type &eb_within,
           const index_array_type &eb_whole,
           const std::string msg = "");

  // Don't store the full set of shuffling matrices;
  //   generate each as it is required, based on the more compressed representations
  bool operator()(Shuffle &output);

  index_type size() const { return nshuffles; }

  // Go back to the first permutation
  void reset();

private:
  const index_type rows;
  std::vector<PermuteLabels> permutations;
  std::vector<BitSet> signflips;
  index_type nshuffles, counter;
  std::unique_ptr<ProgressBar> progress;

  void initialise(const error_t error_types,
                  const bool nshuffles_explicit,
                  const bool is_nonstationarity,
                  const index_array_type &eb_within,
                  const index_array_type &eb_whole);

  // For exchangeability blocks (either within or whole)
  index_array_type load_blocks(const std::string &filename, const bool equal_sizes);

  // For generating unique permutations
  bool is_duplicate(const PermuteLabels &, const PermuteLabels &) const;
  bool is_duplicate(const PermuteLabels &) const;

  // Note that this function does not take into account identical rows and therefore generated
  // permutations are not guaranteed to be unique wrt the computed test statistic.
  // Providing the number of rows is large then the likelihood of generating duplicates is low.
  void generate_random_permutations(const index_type num_perms,
                                    const index_type num_rows,
                                    const index_array_type &eb_within,
                                    const index_array_type &eb_whole,
                                    const bool include_default,
                                    const bool permit_duplicates);

  void generate_all_permutations(const index_type num_rows,
                                 const index_array_type &eb_within,
                                 const index_array_type &eb_whole);

  void load_permutations(const std::string &filename);

  // Similar functions required for sign-flipping
  bool is_duplicate(const BitSet &) const;

  void generate_random_signflips(const index_type num_signflips,
                                 const index_type num_rows,
                                 const index_array_type &blocks,
                                 const bool include_default,
                                 const bool permit_duplicates);

  void generate_all_signflips(const index_type num_rows, const index_array_type &blocks);

  std::vector<std::vector<index_type>> indices2blocks(const index_array_type &) const;
};

} // namespace MR::Math::Stats
