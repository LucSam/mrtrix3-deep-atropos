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

#include "dwi/tractography/algorithms/calibrator.h"
#include "dwi/tractography/tracking/method.h"
#include "dwi/tractography/tracking/shared.h"
#include "dwi/tractography/tracking/tractography.h"
#include "dwi/tractography/tracking/types.h"
#include "math/SH.h"

namespace MR::DWI::Tractography::Algorithms {

extern const App::OptionGroup iFODOptions;
void load_iFOD_options(Tractography::Properties &);

using namespace MR::DWI::Tractography::Tracking;

class iFOD1 : public MethodBase {
public:
  class Shared : public SharedBase {
  public:
    Shared(const std::string &diff_path, DWI::Tractography::Properties &property_set)
        : SharedBase(diff_path, property_set),
          lmax(Math::SH::LforN(source.size(3))),
          max_trials(Defaults::max_trials_per_step),
          sin_max_angle_1o(std::sin(max_angle_1o)),
          fod_power(1.0f),
          mean_samples(0.0),
          mean_truncations(0.0),
          max_max_truncation(0.0),
          num_proc(0) {

      try {
        Math::SH::check(source);
      } catch (Exception &e) {
        e.display();
        throw Exception("Algorithm iFOD1 expects as input a spherical harmonic (SH) image");
      }

      set_step_and_angle(
          rk4 ? Defaults::stepsize_voxels_rk4 : Defaults::stepsize_voxels_firstorder, Defaults::angle_ifod1, rk4);

      // max_angle_1o needs to be set because it influences the cone in which FOD amplitudes are sampled
      if (rk4) {
        max_angle_1o = max_angle_ho;
        cos_max_angle_1o = std::cos(max_angle_1o);
        max_angle_ho = Math::pi_2;
        cos_max_angle_ho = 0.0;
      }
      sin_max_angle_1o = std::sin(max_angle_1o);
      set_num_points();
      set_cutoff(Defaults::cutoff_fod * (is_act() ? Defaults::cutoff_act_multiplier : 1.0));

      properties["method"] = "iFOD1";
      properties.set(lmax, "lmax");
      properties.set(max_trials, "max_trials");
      properties.set(fod_power, "fod_power");
      bool precomputed = true;
      properties.set(precomputed, "sh_precomputed");
      if (precomputed)
        precomputer.init(lmax);
    }

    ~Shared() {
      mean_samples /= double(num_proc);
      mean_truncations /= double(num_proc);
      INFO("mean number of samples per step = " + str(mean_samples));
      if (mean_truncations) {
        INFO("mean number of steps between rejection sampling truncations = " + str(1.0 / mean_truncations));
        INFO("maximum truncation error = " + str(max_max_truncation));
      } else {
        INFO("no rejection sampling truncations occurred");
      }
    }

    void update_stats(double mean_samples_per_run, double mean_truncations_per_run, double max_truncation) const {
      mean_samples += mean_samples_per_run;
      mean_truncations += mean_truncations_per_run;
      if (max_truncation > max_max_truncation)
        max_max_truncation = max_truncation;
      ++num_proc;
    }

    size_t lmax, max_trials;
    float sin_max_angle_1o, fod_power;
    Math::SH::PrecomputedAL<float> precomputer;

  private:
    mutable double mean_samples, mean_truncations, max_max_truncation;
    mutable int num_proc;
  };

  iFOD1(const Shared &shared)
      : MethodBase(shared),
        S(shared),
        source(S.source),
        mean_sample_num(0),
        num_sample_runs(0),
        num_truncations(0),
        max_truncation(0.0) {
    calibrate(*this);
  }

  ~iFOD1() {
    S.update_stats(calibrate_list.size() + float(mean_sample_num) / float(num_sample_runs),
                   float(num_truncations) / float(num_sample_runs),
                   max_truncation);
  }

  bool init() override {
    if (!get_data(source))
      return (false);

    if (!S.init_dir.allFinite()) {

      const Eigen::Vector3f init_dir(dir);

      for (size_t n = 0; n < S.max_seed_attempts; n++) {
        dir = init_dir.allFinite() ? rand_dir(init_dir) : random_direction();
        float val = FOD(dir);
        if (std::isfinite(val))
          if (val > S.init_threshold)
            return true;
      }

    } else {
      dir = S.init_dir;
      float val = FOD(dir);
      if (std::isfinite(val))
        if (val > S.init_threshold)
          return true;
    }

    return false;
  }

  term_t next() override {
    if (!get_data(source))
      return EXIT_IMAGE;

    float max_val = 0.0;
    for (size_t i = 0; i < calibrate_list.size(); ++i) {
      float val = FOD(rotate_direction(dir, calibrate_list[i]));
      if (std::isnan(val))
        return EXIT_IMAGE;
      else if (val > max_val)
        max_val = val;
    }

    if (max_val <= 0.0)
      return CALIBRATOR;

    max_val = std::pow(max_val, S.fod_power) * calibrate_ratio;

    num_sample_runs++;

    for (size_t n = 0; n < S.max_trials; n++) {
      Eigen::Vector3f new_dir = rand_dir(dir);
      float val = FOD(new_dir);

      if (val > S.threshold) {

        val = std::pow(val, S.fod_power);
        if (val > max_val) {
          DEBUG("max_val exceeded!!! (val = " + str(val) + ", max_val = " + str(max_val) + ")");
          ++num_truncations;
          if (val / max_val > max_truncation)
            max_truncation = val / max_val;
        }

        if (uniform(rng) < val / max_val) {
          dir = new_dir;
          dir.normalize();
          pos += S.step_size * dir;
          mean_sample_num += n;
          return CONTINUE;
        }
      }
    }

    return MODEL;
  }

  float get_metric(const Eigen::Vector3f &position, const Eigen::Vector3f &direction) override {
    if (!get_data(source, position))
      return 0.0;
    return FOD(direction);
  }

protected:
  const Shared &S;
  Interpolator<Image<float>>::type source;
  float calibrate_ratio;
  size_t mean_sample_num, num_sample_runs, num_truncations;
  float max_truncation;
  std::vector<Eigen::Vector3f> calibrate_list;

  float FOD(const Eigen::Vector3f &d) const {
    return (S.precomputer ? S.precomputer.value(values, d) : Math::SH::value(values, d, S.lmax));
  }

  Eigen::Vector3f rand_dir(const Eigen::Vector3f &d) {
    return (random_direction(d, S.max_angle_1o, S.sin_max_angle_1o));
  }

  class Calibrate {
  public:
    Calibrate(iFOD1 &method) : P(method), fod(P.values) {
      Math::SH::delta(fod, Eigen::Vector3f(0.0, 0.0, 1.0), P.S.lmax);
    }

    float operator()(float el) {
      return std::pow(Math::SH::value(P.values, Eigen::Vector3f(std::sin(el), 0.0, std::cos(el)), P.S.lmax),
                      P.S.fod_power);
    }

  private:
    iFOD1 &P;
    Eigen::VectorXf &fod;
  };

  friend void calibrate<iFOD1>(iFOD1 &method);
};

} // namespace MR::DWI::Tractography::Algorithms
