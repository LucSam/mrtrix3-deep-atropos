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

#include "image.h"
#include "interp/linear.h"
#include "math/SH.h"
#include "types.h"

#include "dwi/directions/set.h"

#include "dwi/tractography/mapping/twi_stats.h"
#include "dwi/tractography/streamline.h"

namespace MR::DWI::Tractography::Mapping {

class DixelMappingPlugin {
public:
  DixelMappingPlugin(const DWI::Directions::FastLookupSet &directions) : dirs(directions) {}
  DixelMappingPlugin(const DixelMappingPlugin &that) : dirs(that.dirs) {}
  DWI::Directions::index_type operator()(const Eigen::Vector3d &d) const { return dirs.select_direction(d); }

private:
  const DWI::Directions::FastLookupSet &dirs;
};

class TODMappingPlugin {
public:
  TODMappingPlugin(const size_t N) : generator(new Math::SH::aPSF<float>(Math::SH::LforN(N))) {}
  template <class VectorType, class UnitVectorType> void operator()(VectorType &sh, const UnitVectorType &d) const {
    (*generator)(sh, d);
  }

private:
  std::shared_ptr<Math::SH::aPSF<float>> generator;
};

class TWIImagePluginBase {
public:
  TWIImagePluginBase(const std::string &input_image, const tck_stat_t track_statistic)
      : statistic(track_statistic), interp(Image<float>::open(input_image).with_direct_io()), backtrack(false) {}

  TWIImagePluginBase(Image<float> &input_image, const tck_stat_t track_statistic)
      : statistic(track_statistic), interp(input_image), backtrack(false) {}

  TWIImagePluginBase(const TWIImagePluginBase &that)
      : statistic(that.statistic),
        interp(that.interp),
        backtrack(that.backtrack),
        backtrack_mask(that.backtrack_mask) {}

  virtual ~TWIImagePluginBase() {}

  virtual TWIImagePluginBase *clone() const = 0;

  void set_backtrack();

  virtual void load_factors(const Streamline<> &, std::vector<default_type> &) const = 0;

protected:
  const tck_stat_t statistic;

  // Image<float> voxel;
  //  Each instance of the class has its own interpolator for obtaining values
  //    in a thread-safe fashion
  mutable Interp::Linear<Image<float>> interp;

  // For handling backtracking for endpoint-based track-wise statistics
  bool backtrack;
  mutable Image<bool> backtrack_mask;

  // Helper functions; find the last point on the streamline from which valid image information can be read
  ssize_t get_end_index(const Streamline<> &, const bool) const;
  const Streamline<>::point_type get_end_point(const Streamline<> &, const bool) const;
};

class TWIScalarImagePlugin : public TWIImagePluginBase {
public:
  TWIScalarImagePlugin(const std::string &input_image, const tck_stat_t track_statistic)
      : TWIImagePluginBase(input_image, track_statistic) {
    assert(statistic != ENDS_CORR);
    if (!((interp.ndim() == 3) || (interp.ndim() == 4 && interp.size(3) == 1)))
      throw Exception("Scalar image used for TWI must be a 3D image");
    if (interp.ndim() == 4)
      interp.index(3) = 0;
  }

  TWIScalarImagePlugin(const TWIScalarImagePlugin &that) : TWIImagePluginBase(that) {
    if (interp.ndim() == 4)
      interp.index(3) = 0;
  }

  TWIScalarImagePlugin *clone() const override { return new TWIScalarImagePlugin(*this); }

  void load_factors(const Streamline<> &, std::vector<default_type> &) const override;
};

class TWIFODImagePlugin : public TWIImagePluginBase {
public:
  TWIFODImagePlugin(const std::string &input_image, const tck_stat_t track_statistic)
      : TWIImagePluginBase(input_image, track_statistic),
        sh_coeffs(interp.size(3)),
        precomputer(new Math::SH::PrecomputedAL<default_type>()) {
    if (track_statistic == ENDS_CORR)
      throw Exception("Cannot use ends_corr track statistic with an FOD image");
    Math::SH::check(Header(interp));
    precomputer->init(Math::SH::LforN(sh_coeffs.size()));
  }

  TWIFODImagePlugin(const TWIFODImagePlugin &that) = default;

  TWIFODImagePlugin *clone() const override { return new TWIFODImagePlugin(*this); }

  void load_factors(const Streamline<> &, std::vector<default_type> &) const override;

private:
  mutable Eigen::Matrix<default_type, Eigen::Dynamic, 1> sh_coeffs;
  std::shared_ptr<Math::SH::PrecomputedAL<default_type>> precomputer;
};

class TWDFCStaticImagePlugin : public TWIImagePluginBase {
public:
  TWDFCStaticImagePlugin(Image<float> &input_image) : TWIImagePluginBase(input_image, ENDS_CORR) {}

  TWDFCStaticImagePlugin(const TWDFCStaticImagePlugin &that) = default;

  TWDFCStaticImagePlugin *clone() const override { return new TWDFCStaticImagePlugin(*this); }

  void load_factors(const Streamline<> &, std::vector<default_type> &) const override;
};

class TWDFCDynamicImagePlugin : public TWIImagePluginBase {
public:
  TWDFCDynamicImagePlugin(Image<float> &input_image, const std::vector<float> &kernel, const ssize_t timepoint)
      : TWIImagePluginBase(input_image, ENDS_CORR),
        kernel(kernel),
        kernel_centre((kernel.size() - 1) / 2),
        sample_centre(timepoint) {}

  TWDFCDynamicImagePlugin(const TWDFCDynamicImagePlugin &that) = default;

  TWDFCDynamicImagePlugin *clone() const override { return new TWDFCDynamicImagePlugin(*this); }

  void load_factors(const Streamline<> &, std::vector<default_type> &) const override;

protected:
  const std::vector<float> kernel;
  const ssize_t kernel_centre, sample_centre;
};

} // namespace MR::DWI::Tractography::Mapping
