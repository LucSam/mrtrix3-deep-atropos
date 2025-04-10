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

#include <map> // Used for sorting FOD samples

#include "algo/loop.h"
#include "dwi/directions/mask.h"
#include "dwi/directions/set.h"
#include "image.h"
#include "math/SH.h"
#include "memory.h"

#define FMLS_INTEGRAL_THRESHOLD_DEFAULT 0.0 // By default, don't threshold by integral (tough to get a good number)
#define FMLS_PEAK_VALUE_THRESHOLD_DEFAULT 0.1
#define FMLS_MERGE_RATIO_BRIDGE_TO_PEAK_DEFAULT                                                                        \
  1.0 // By default, never perform merging of lobes generated from discrete peaks such that a single lobe contains
      // multiple peaks

// By default, the mean direction of each FOD lobe is calculated by taking a weighted average of the
//   Euclidean unit vectors (weights are FOD amplitudes). This is not strictly mathematically correct, and
//   a method is provided for optimising the mean direction estimate based on minimising the weighted sum of squared
//   geodesic distances on the sphere. However the coarse estimate is in fact accurate enough for our applications.
//   Uncomment this line to activate the optimal mean direction calculation (about a 20% performance penalty).
// #define FMLS_OPTIMISE_MEAN_DIR

namespace MR::DWI::FMLS {

using DWI::Directions::index_type;
using DWI::Directions::Mask;

class Segmenter;

// These are for configuring the FMLS segmenter at the command line, particularly for fod_metric command
extern const App::OptionGroup FMLSSegmentOption;
void load_fmls_thresholds(Segmenter &);

class FOD_lobe {

public:
  FOD_lobe(const DWI::Directions::Set &dirs, const index_type seed, const default_type value, const default_type weight)
      : mask(dirs),
        values(Eigen::Array<default_type, Eigen::Dynamic, 1>::Zero(dirs.size())),
        max_peak_value(abs(value)),
        peak_dirs(1, dirs.get_dir(seed)),
        mean_dir(peak_dirs.front() * abs(value) * weight),
        integral(abs(value * weight)),
        neg(value <= 0.0) {
    mask[seed] = true;
    values[seed] = value;
  }

  // This is used for creating a `null lobe' i.e. an FOD lobe with zero size, containing all directions not
  //   assigned to any other lobe in the voxel
  FOD_lobe(const DWI::Directions::Mask &i)
      : mask(i),
        values(Eigen::Array<default_type, Eigen::Dynamic, 1>::Zero(i.size())),
        max_peak_value(0.0),
        integral(0.0),
        neg(false) {}

  void add(const index_type bin, const default_type value, const default_type weight) {
    assert((value <= 0.0 && neg) || (value >= 0.0 && !neg));
    mask[bin] = true;
    values[bin] = value;
    const Eigen::Vector3d &dir = mask.get_dirs()[bin];
    const default_type multiplier = (mean_dir.dot(dir)) > 0.0 ? 1.0 : -1.0;
    mean_dir += dir * multiplier * abs(value) * weight;
    integral += abs(value * weight);
  }

  void revise_peak(const size_t index, const Eigen::Vector3d &revised_peak_dir, const default_type revised_peak_value) {
    assert(!neg);
    assert(index < num_peaks());
    peak_dirs[index] = revised_peak_dir;
    if (!index)
      max_peak_value = revised_peak_value;
  }

#ifdef FMLS_OPTIMISE_MEAN_DIR
  void revise_mean_dir(const Eigen::Vector3f &real_mean) {
    assert(!neg);
    mean_dir = real_mean;
  }
#endif

  void finalise() {
    // This is calculated as the lobe is built; just needs to be set to unit length
    mean_dir.normalize();
  }

  void merge(const FOD_lobe &that) {
    assert(neg == that.neg);
    mask |= that.mask;
    for (size_t i = 0; i != mask.size(); ++i)
      values[i] += that.values[i];
    if (that.max_peak_value > max_peak_value) {
      max_peak_value = that.max_peak_value;
      peak_dirs.insert(peak_dirs.begin(), that.peak_dirs.begin(), that.peak_dirs.end());
    } else {
      peak_dirs.insert(peak_dirs.end(), that.peak_dirs.begin(), that.peak_dirs.end());
    }
    const default_type multiplier = (mean_dir.dot(that.mean_dir)) > 0.0 ? 1.0 : -1.0;
    mean_dir += that.mean_dir * that.integral * multiplier;
    integral += that.integral;
  }

  const DWI::Directions::Mask &get_mask() const { return mask; }
  const Eigen::Array<default_type, Eigen::Dynamic, 1> &get_values() const { return values; }
  default_type get_max_peak_value() const { return max_peak_value; }
  size_t num_peaks() const { return peak_dirs.size(); }
  const Eigen::Vector3d &get_peak_dir(const size_t i) const {
    assert(i < num_peaks());
    return peak_dirs[i];
  }
  const Eigen::Vector3d &get_mean_dir() const { return mean_dir; }
  default_type get_integral() const { return integral; }
  bool is_negative() const { return neg; }

private:
  DWI::Directions::Mask mask;
  Eigen::Array<default_type, Eigen::Dynamic, 1> values;
  default_type max_peak_value;
  std::vector<Eigen::Vector3d> peak_dirs;
  Eigen::Vector3d mean_dir;
  default_type integral;
  bool neg;
};

class FOD_lobes : public std::vector<FOD_lobe> {
public:
  Eigen::Array3i vox;
  std::vector<uint8_t> lut;
};

class SH_coefs : public Eigen::Matrix<default_type, Eigen::Dynamic, 1> {
public:
  SH_coefs() : vox(-1, -1, -1) {}
  SH_coefs(const Eigen::Matrix<default_type, Eigen::Dynamic, 1> &that)
      : Eigen::Matrix<default_type, Eigen::Dynamic, 1>(that), vox(-1, -1, -1) {}
  Eigen::Array3i vox;
};

class FODQueueWriter {

  using FODImageType = Image<float>;
  using MaskImageType = Image<float>;

public:
  FODQueueWriter(const FODImageType &fod_image, const MaskImageType &mask_image = MaskImageType())
      : fod(fod_image), mask(mask_image), loop(Loop("segmenting FODs", 0, 3)(fod)) {}

  bool operator()(SH_coefs &out) {
    if (!loop)
      return false;
    if (mask.valid()) {
      do {
        assign_pos_of(fod, 0, 3).to(mask);
        if (!mask.value())
          ++loop;
      } while (loop && !mask.value());
    }
    assign_pos_of(fod).to(out.vox);
    out.resize(fod.size(3));
    for (auto l = Loop(3)(fod); l; ++l)
      out[fod.index(3)] = fod.value();
    ++loop;
    return true;
  }

private:
  FODImageType fod;
  MaskImageType mask;
  decltype(Loop("text", 0, 3)(fod)) loop;
};

// Store a vector of weights to be applied when computing integrals, to account for non-uniformities in direction
// distribution These weights are applied to the amplitude along each direction as the integral for each lobe is summed,
//   in order to take into account the relative spacing between adjacent directions
class IntegrationWeights {
public:
  IntegrationWeights(const DWI::Directions::Set &dirs);
  default_type operator[](const size_t i) {
    assert(i < size_t(data.size()));
    return data[i];
  }

private:
  Eigen::Array<default_type, Eigen::Dynamic, 1> data;
};

class Segmenter {

public:
  Segmenter(const DWI::Directions::FastLookupSet &, const size_t);

  bool operator()(const SH_coefs &, FOD_lobes &) const;

  default_type get_integral_threshold() const { return integral_threshold; }
  void set_integral_threshold(const default_type i) { integral_threshold = i; }
  default_type get_peak_value_threshold() const { return peak_value_threshold; }
  void set_peak_value_threshold(const default_type i) { peak_value_threshold = i; }
  default_type get_lobe_merge_ratio() const { return lobe_merge_ratio; }
  void set_lobe_merge_ratio(const default_type i) { lobe_merge_ratio = i; }
  bool get_create_null_lobe() const { return create_null_lobe; }
  void set_create_null_lobe(const bool i) {
    create_null_lobe = i;
    verify_settings();
  }
  bool get_create_lookup_table() const { return create_lookup_table; }
  void set_create_lookup_table(const bool i) {
    create_lookup_table = i;
    verify_settings();
  }
  bool get_dilate_lookup_table() const { return dilate_lookup_table; }
  void set_dilate_lookup_table(const bool i) {
    dilate_lookup_table = i;
    verify_settings();
  }

private:
  // FastLookupSet is required for ensuring that when a peak direction is
  //   revised using Newton optimisation, that direction is still reflective
  //   of the original peak; i.e. it hasn't 'leaped' across to a different peak
  const DWI::Directions::FastLookupSet &dirs;

  const size_t lmax;

  std::shared_ptr<Math::SH::Transform<default_type>> transform;
  std::shared_ptr<Math::SH::PrecomputedAL<default_type>> precomputer;
  std::shared_ptr<IntegrationWeights> weights;

  default_type integral_threshold;   // Integral of positive lobe must be at least this value
  default_type peak_value_threshold; // Absolute threshold for the peak amplitude of the lobe
  default_type lobe_merge_ratio;     // Determines whether two lobes get agglomerated into one, depending on the FOD
                                 // amplitude at the current point and how it compares to the maximal amplitudes of the
                                 // lobes to which it could be assigned
  bool create_null_lobe;    // If this is set, an additional lobe will be created after segmentation with zero size,
                            // containing all directions not assigned to any other lobe
  bool create_lookup_table; // If this is set, an additional lobe will be created after segmentation with zero size,
                            // containing all directions not assigned to any other lobe
  bool dilate_lookup_table; // If this is set, the lookup table created for each voxel will be dilated so that all
                            // directions correspond to the nearest positive non-zero FOD lobe

  void verify_settings() const {
    if (create_null_lobe && dilate_lookup_table)
      throw Exception(
          "For FOD segmentation, options 'create_null_lobe' and 'dilate_lookup_table' are mutually exclusive");
    if (!create_lookup_table && dilate_lookup_table)
      throw Exception("For FOD segmentation, 'create_lookup_table' must be set in order for lookup tables to be "
                      "dilated ('dilate_lookup_table')");
  }

#ifdef FMLS_OPTIMISE_MEAN_DIR
  void optimise_mean_dir(FOD_lobe &) const;
#endif
};

} // namespace MR::DWI::FMLS
