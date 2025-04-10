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

#include <random>
#ifdef MRTRIX_WINDOWS
#include <sys/time.h>
#endif

#include <mutex>

#include "mrtrix.h"

namespace MR::Math {

//! random number generator
/*! this is a thin wrapper around the standard C++11 std::mt19937 random
 * number generator. It can be used in combination with the standard C++11
 * distributions. It differs from the standard in its constructors: the
 * default constructor will seed using std::random_device, unless a seed
 * has been expicitly passed using the MRTRIX_RNG_SEED environment
 * variable. The copy constructor will seed itself using 1 + the last seed
 * used - this ensures the seeds are unique across instances in
 * multi-threading. */
// TODO consider switch to std::mt19937_64
class RNG : public std::mt19937 {
public:
  RNG() : std::mt19937(get_seed()) {}
  RNG(std::mt19937::result_type seed) : std::mt19937(seed) {}
  RNG(const RNG &) : std::mt19937(get_seed()) {}
  template <typename ValueType> class Uniform;
  template <typename ValueType> class Normal;
  template <typename ValueType> class Integer;

  static std::mt19937::result_type get_seed() {
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    static std::mt19937::result_type current_seed = get_seed_private();
    return current_seed++;
  }

private:
  static std::mt19937::result_type get_seed_private() {
    // ENVVAR name: MRTRIX_RNG_SEED
    // ENVVAR Set the seed used for the random number generator.
    // ENVVAR Ordinarily, MRtrix applications will use random seeds to ensure
    // ENVVAR repeat runs of stochastic processes are never the same.
    // ENVVAR However, when experimenting or debugging, it may be useful to
    // ENVVAR explicitly set the RNG seed to ensure reproducible results across
    // ENVVAR runs. To do this, set this variable to a fixed number prior to
    // ENVVAR running the command(s).
    // ENVVAR
    // ENVVAR Note that to obtain the same results
    // ENVVAR from a multi-threaded command, you should also disable
    // ENVVAR multi-threading (using the option ``-nthread 0`` or by
    // ENVVAR setting the :envvar:`MRTRIX_NTHREADS` environment variable to zero).
    // ENVVAR Multi-threading introduces randomness in the order of execution, which
    // ENVVAR will generally also affect the reproducibility of results.
    const char *from_env = getenv("MRTRIX_RNG_SEED");
    if (from_env)
      return to<std::mt19937::result_type>(from_env);

    std::random_device rd;
    return rd();
  }
};

template <typename ValueType> class RNG::Uniform {
public:
  RNG rng;
  using result_type = ValueType;
  // static const ValueType max() const { return static_cast<ValueType>(1.0); }
  // static const ValueType min() const { return static_cast<ValueType>(0.0); }
  std::uniform_real_distribution<ValueType> dist;
  ValueType operator()() { return dist(rng); }
};

template <typename ValueType> class RNG::Normal {
public:
  RNG rng;
  using result_type = ValueType;
  std::normal_distribution<ValueType> dist;
  ValueType operator()() { return dist(rng); }
};

template <typename ValueType> class RNG::Integer {
public:
  Integer(const ValueType max) : dist(0, max) {}
  RNG rng;
  std::uniform_int_distribution<ValueType> dist;
  ValueType operator()() { return dist(rng); }
};

} // namespace MR::Math
