/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#include "command.h"
#include "progressbar.h"
#include "thread_queue.h"
#include "algo/loop.h"
#include "transform.h"
#include "image.h"
#include "sparse/fixel_metric.h"
#include "sparse/keys.h"
#include "sparse/image.h"
#include "math/stats/permutation.h"
#include "math/stats/glm.h"
#include "stats/cfe.h"
#include "stats/permtest.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/scalar_file.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/writer.h"


using namespace MR;
using namespace App;
using namespace MR::DWI::Tractography::Mapping;
using Sparse::FixelMetric;

typedef float value_type;

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "Fixel-based analysis using connectivity-based fixel enhancement and non-parametric permutation testing.";

  REFERENCES
  + "Raffelt, D.; Smith, RE.; Ridgway, GR.; Tournier, JD.; Vaughan, DN.; Rose, S.; Henderson, R.; Connelly, A;." // Internal
    "Connectivity-based fixel enhancement: Whole-brain statistical analysis of diffusion MRI measures in the presence of crossing fibres. \n"
    "Neuroimage, 2015, 15(117):40-55\n"

  + "* If using the -nonstationary option: \n"
    "Salimi-Khorshidi, G. Smith, S.M. Nichols, T.E. \n"
    "Adjusting the effect of nonstationarity in cluster-based and TFCE inference. \n"
    "NeuroImage, 2011, 54(3), 2006-19\n" ;

  ARGUMENTS
  + Argument ("input", "a text file listing the file names of the input fixel images").type_file_in ()

  + Argument ("template", "the fixel mask used to define fixels of interest. This can be generated by "
                          "thresholding the group average AFD fixel image.").type_image_in ()

  + Argument ("design", "the design matrix. Note that a column of 1's will need to be added for correlations.").type_file_in ()

  + Argument ("contrast", "the contrast vector, specified as a single row of weights").type_file_in ()

  + Argument ("tracks", "the tracks used to determine fixel-fixel connectivity").type_tracks_in ()

  + Argument ("output", "the filename prefix for all output.").type_text();


  OPTIONS

  + Option ("notest", "don't perform permutation testing and only output population statistics (effect size, stdev etc)")

  + Option ("negative", "automatically test the negative (opposite) contrast. By computing the opposite contrast simultaneously "
                        "the computation time is reduced.")

  + Option ("nperms", "the number of permutations (default: 5000).")
  + Argument ("num").type_integer (1, 5000, 100000)

  + Option ("cfe_dh", "the height increment used in the cfe integration (default: 0.1)")
  + Argument ("value").type_float (0.001, 0.1, 100000)

  + Option ("cfe_e", "cfe extent exponent (default: 2.0)")
  + Argument ("value").type_float (0.0, 2.0, 100000)

  + Option ("cfe_h", "cfe height exponent (default: 3.0)")
  + Argument ("value").type_float (0.0, 3.0, 100000)

  + Option ("cfe_c", "cfe connectivity exponent (default: 0.5)")
  + Argument ("value").type_float (0.0, 0.5, 100000)

  + Option ("angle", "the max angle threshold for computing inter-subject fixel correspondence (Default: 30)")
  + Argument ("value").type_float (0.0, 30, 90)

  + Option ("connectivity", "a threshold to define the required fraction of shared connections to be included in the neighbourhood (default: 1%)")
  + Argument ("threshold").type_float (0.001, 0.01, 1.0)

  + Option ("smooth", "smooth the fixel value along the fibre tracts using a Gaussian kernel with the supplied FWHM (default: 10mm)")
  + Argument ("FWHM").type_float (0.0, 10.0, 200.0)

  + Option ("nonstationary", "do adjustment for non-stationarity")

  + Option ("nperms_nonstationary", "the number of permutations used when precomputing the empirical statistic image for nonstationary correction (Default: 5000)")
  + Argument ("num").type_integer (1, 5000, 100000);
}



template <class VectorType>
void write_fixel_output (const std::string& filename,
                         const VectorType data,
                         const Header& header,
                         Sparse::Image<FixelMetric>& mask_vox,
                         Image<int32_t>& indexer_vox) {
  Sparse::Image<FixelMetric> output (filename, header);
  for (auto i = Loop (mask_vox)(mask_vox, indexer_vox, output); i; ++i) {
    output.value().set_size (mask_vox.value().size());
    indexer_vox.index(3) = 0;
    int32_t index = indexer_vox.value();
    for (size_t f = 0; f != mask_vox.value().size(); ++f, ++index) {
     output.value()[f] = mask_vox.value()[f];
     output.value()[f].value = data[index];
    }
  }
}



void run() {

  auto opt = get_options ("negative");
  bool compute_negative_contrast = opt.size() ? true : false;

  value_type cfe_dh = get_option_value ("cfe_dh", 0.1);
  value_type cfe_h = get_option_value ("cfe_h", 3.0);
  value_type cfe_e = get_option_value ("cfe_e", 2.0);
  value_type cfe_c = get_option_value ("cfe_c", 0.5);
  int num_perms = get_option_value ("nperms", 5000);
  value_type angular_threshold = get_option_value ("angle", 30.0);
  const float angular_threshold_dp = cos (angular_threshold * (M_PI/180.0));

  value_type connectivity_threshold = get_option_value ("connectivity", 0.01);
  value_type smooth_std_dev = get_option_value ("smooth", 10.0) / 2.3548;

  bool do_nonstationary_adjustment = get_options ("nonstationary").size();

  int nperms_nonstationary = get_option_value ("nperms_nonstationary", 5000);
  
  // Read filenames
  std::vector<std::string> filenames;
  {
    std::string folder = Path::dirname (argument[0]);
    std::ifstream ifs (argument[0].c_str());
    std::string temp;
    while (getline (ifs, temp)) {
      std::string filename (Path::join (folder, temp));
      size_t p = filename.find_last_not_of(" \t");
      if (std::string::npos != p)
        filename.erase(p+1);
      if (!MR::Path::exists (filename))
        throw Exception ("input fixel image not found: " + filename);
      filenames.push_back (filename);
    }
  }

  // Load design matrix:
  Eigen::Matrix<value_type, Eigen::Dynamic, Eigen::Dynamic> design = load_matrix<value_type> (argument[2]);
  if (design.rows() != (ssize_t)filenames.size())
    throw Exception ("number of input files does not match number of rows in design matrix");

  // Load contrast matrix:
  Eigen::Matrix<value_type, Eigen::Dynamic, Eigen::Dynamic> contrast = load_matrix<value_type> (argument[3]);
  if (contrast.cols() != design.cols())
    throw Exception ("the number of contrasts does not equal the number of columns in the design matrix");

  auto input_header = Header::open (argument[1]);
  Sparse::Image<FixelMetric> mask_fixel_image (argument[1]);

  // Create an image to store the fixel indices, if we had a fixel scratch buffer this would be cleaner
  Header index_header (input_header);
  index_header.set_ndim(4);
  index_header.size(3) = 2;
  auto fixel_index_image = Image<int32_t>::scratch (index_header);
  for (auto i = Loop ()(fixel_index_image);i; ++i)
    fixel_index_image.value() = -1;

  std::vector<Eigen::Vector3> positions;
  std::vector<Eigen::Vector3f> directions;

  Transform image_transform (mask_fixel_image);
  for (auto i = Loop (mask_fixel_image)(mask_fixel_image, fixel_index_image); i; ++i) {
    fixel_index_image.index(3) = 0;
    fixel_index_image.value() = directions.size();
    int32_t fixel_count = 0;
    for (size_t f = 0; f != mask_fixel_image.value().size(); ++f, ++fixel_count) {
      directions.push_back (mask_fixel_image.value()[f].dir);
      Eigen::Vector3 pos (mask_fixel_image.index(0), mask_fixel_image.index(1), mask_fixel_image.index(2));
      positions.push_back (image_transform.voxel2scanner * pos);
    }
    fixel_index_image.index(3) = 1;
    fixel_index_image.value() = fixel_count;
  }

  uint32_t num_fixels = directions.size();
  CONSOLE ("number of fixels: " + str(num_fixels));

  // Compute fixel-fixel connectivity
  std::vector<std::map<int32_t, Stats::CFE::connectivity> > connectivity_matrix (num_fixels);
  std::vector<uint16_t> fixel_TDI (num_fixels, 0.0);
  std::string track_filename = argument[4];
  std::string output_prefix = argument[5];
  DWI::Tractography::Properties properties;
  DWI::Tractography::Reader<value_type> track_file (track_filename, properties);
  // Read in tracts, and compute whole-brain fixel-fixel connectivity
  const size_t num_tracks = properties["count"].empty() ? 0 : to<int> (properties["count"]);
  if (!num_tracks)
    throw Exception ("no tracks found in input file");
  if (num_tracks < 1000000)
    WARN ("more than 1 million tracks should be used to ensure robust fixel-fixel connectivity");
  {
    typedef DWI::Tractography::Mapping::SetVoxelDir SetVoxelDir;
    DWI::Tractography::Mapping::TrackLoader loader (track_file, num_tracks, "pre-computing fixel-fixel connectivity");
    DWI::Tractography::Mapping::TrackMapperBase mapper (input_header);
    mapper.set_upsample_ratio (DWI::Tractography::Mapping::determine_upsample_ratio (input_header, properties, 0.333f));
    mapper.set_use_precise_mapping (true);
    Stats::CFE::TrackProcessor tract_processor (fixel_index_image, directions, fixel_TDI, connectivity_matrix, angular_threshold);
    Thread::run_queue (
        loader,
        Thread::batch (DWI::Tractography::Streamline<float>()),
        mapper,
        Thread::batch (SetVoxelDir()),
        tract_processor);
  }
  track_file.close();


  // Normalise connectivity matrix and threshold, pre-compute fixel-fixel weights for smoothing.
  std::vector<std::map<int32_t, value_type> > smoothing_weights (num_fixels);
  bool do_smoothing = false;
  const value_type gaussian_const2 = 2.0 * smooth_std_dev * smooth_std_dev;
  value_type gaussian_const1 = 1.0;
  if (smooth_std_dev > 0.0) {
    do_smoothing = true;
    gaussian_const1 = 1.0 / (smooth_std_dev *  std::sqrt (2.0 * M_PI));
  }
  {
    ProgressBar progress ("normalising and thresholding fixel-fixel connectivity matrix", num_fixels);
    for (unsigned int fixel = 0; fixel < num_fixels; ++fixel) {
      auto it = connectivity_matrix[fixel].begin();
      while (it != connectivity_matrix[fixel].end()) {
        value_type connectivity = it->second.value / value_type (fixel_TDI[fixel]);
        if (connectivity < connectivity_threshold)  {
          connectivity_matrix[fixel].erase (it++);
        } else {
          if (do_smoothing) {
            value_type distance = std::sqrt (Math::pow2 (positions[fixel][0] - positions[it->first][0]) +
                                             Math::pow2 (positions[fixel][1] - positions[it->first][1]) +
                                             Math::pow2 (positions[fixel][2] - positions[it->first][2]));
            value_type smoothing_weight = connectivity * gaussian_const1 * std::exp (-std::pow (distance, 2) / gaussian_const2);
            if (smoothing_weight > connectivity_threshold)
              smoothing_weights[fixel].insert (std::pair<int32_t, value_type> (it->first, smoothing_weight));
          }
          // Here we pre-exponentiate each connectivity value by C
          it->second.value = std::pow (connectivity, cfe_c);
          ++it;
        }
      }
      // Make sure the fixel is fully connected to itself giving it a smoothing weight of 1
      Stats::CFE::connectivity self_connectivity;
      self_connectivity.value = 1.0;
      connectivity_matrix[fixel].insert (std::pair<int32_t, Stats::CFE::connectivity> (fixel, self_connectivity));
      smoothing_weights[fixel].insert (std::pair<int32_t, value_type> (fixel, gaussian_const1));
      progress++;
    }
  }

  // Normalise smoothing weights
  for (size_t fixel = 0; fixel < num_fixels; ++fixel) {
    value_type sum = 0.0;
    for (auto it = smoothing_weights[fixel].begin(); it != smoothing_weights[fixel].end(); ++it)
      sum += it->second;
    value_type norm_factor = 1.0 / sum;
    for (auto it = smoothing_weights[fixel].begin(); it != smoothing_weights[fixel].end(); ++it)
      it->second *= norm_factor;
  }

  // Load input data
  Eigen::Matrix<value_type, Eigen::Dynamic, Eigen::Dynamic> data (num_fixels, filenames.size());
  {
    ProgressBar progress ("loading input images", filenames.size());
    for (size_t subject = 0; subject < filenames.size(); subject++) {
      LogLevelLatch log_level (0);
      Sparse::Image<FixelMetric> fixel (filenames[subject]);
      check_dimensions (fixel, mask_fixel_image, 0, 3);
      std::vector<value_type> temp_fixel_data (num_fixels, 0.0);

      for (auto voxel = Loop(fixel)(fixel, fixel_index_image); voxel; ++voxel) {
         fixel_index_image.index(3) = 0;
         int32_t index = fixel_index_image.value();
         fixel_index_image.index(3) = 1;
         int32_t number_fixels = fixel_index_image.value();

         // for each fixel in the mask, find the corresponding fixel in this subject voxel
         for (int32_t i = index; i < index + number_fixels; ++i) {
           value_type largest_dp = 0.0;
           int index_of_closest_fixel = -1;
           for (size_t f = 0; f != fixel.value().size(); ++f) {
             value_type dp = std::abs (directions[i].dot(fixel.value()[f].dir));
             if (dp > largest_dp) {
               largest_dp = dp;
               index_of_closest_fixel = f;
             }
           }
           if (largest_dp > angular_threshold_dp)
             temp_fixel_data[i] = fixel.value()[index_of_closest_fixel].value;
         }
       }

      // Smooth the data
      for (size_t fixel = 0; fixel < num_fixels; ++fixel) {
        value_type value = 0.0;
        std::map<int32_t, value_type>::const_iterator it = smoothing_weights[fixel].begin();
        for (; it != smoothing_weights[fixel].end(); ++it)
          value += temp_fixel_data[it->first] * it->second;
        data (fixel, subject) = value;
      }
      progress++;
    }
  }

  {
    ProgressBar progress ("outputting beta coefficients, effect size and standard deviation");
    auto temp = Math::Stats::GLM::solve_betas (data, design);
    for (ssize_t i = 0; i < contrast.cols(); ++i)
      write_fixel_output (output_prefix + "beta" + str(i) + ".msf", temp.row(i), input_header, mask_fixel_image, fixel_index_image);
    temp = Math::Stats::GLM::abs_effect_size (data, design, contrast);
    write_fixel_output (output_prefix + "abs_effect.msf", temp.row(0), input_header, mask_fixel_image, fixel_index_image);
    temp = Math::Stats::GLM::std_effect_size (data, design, contrast);
    write_fixel_output (output_prefix + "std_effect.msf", temp.row(0), input_header, mask_fixel_image, fixel_index_image);
    temp = Math::Stats::GLM::stdev (data, design);
    write_fixel_output (output_prefix + "std_dev.msf", temp.row(0), input_header, mask_fixel_image, fixel_index_image);
  }

  Math::Stats::GLMTTest glm_ttest (data, design, contrast);
  Stats::CFE::Enhancer cfe_integrator (connectivity_matrix, cfe_dh, cfe_e, cfe_h);
  std::shared_ptr<std::vector<double> > empirical_cfe_statistic;

  Header output_header (input_header);
  output_header.keyval()["num permutations"] = str(num_perms);
  output_header.keyval()["dh"] = str(cfe_dh);
  output_header.keyval()["cfe_e"] = str(cfe_e);
  output_header.keyval()["cfe_h"] = str(cfe_h);
  output_header.keyval()["cfe_c"] = str(cfe_c);
  output_header.keyval()["angular threshold"] = str(angular_threshold);
  output_header.keyval()["connectivity threshold"] = str(connectivity_threshold);
  output_header.keyval()["smoothing FWHM"] = str(smooth_std_dev);

  // If performing non-stationarity adjustment we need to pre-compute the empirical CFE statistic
  if (do_nonstationary_adjustment) {
    empirical_cfe_statistic.reset(new std::vector<double> (num_fixels, 0.0));
    Stats::PermTest::precompute_empirical_stat (glm_ttest, cfe_integrator, nperms_nonstationary, *empirical_cfe_statistic);
    output_header.keyval()["nonstationary adjustment"] = str(true);
    write_fixel_output (output_prefix + "cfe_empirical.msf", *empirical_cfe_statistic, output_header, mask_fixel_image, fixel_index_image);
  } else {
    output_header.keyval()["nonstationary adjustment"] = str(false);
  }

  // Precompute default statistic and CFE statistic
  std::vector<value_type> cfe_output (num_fixels, 0.0);
  std::shared_ptr<std::vector<value_type> > cfe_output_neg;
  std::vector<value_type> tvalue_output (num_fixels, 0.0);
  if (compute_negative_contrast)
    cfe_output_neg.reset (new std::vector<value_type> (num_fixels, 0.0));

  Stats::PermTest::precompute_default_permutation (glm_ttest, cfe_integrator, empirical_cfe_statistic, cfe_output, cfe_output_neg, tvalue_output);

  write_fixel_output (output_prefix + "cfe.msf", cfe_output, output_header, mask_fixel_image, fixel_index_image);
  write_fixel_output (output_prefix + "tvalue.msf", tvalue_output, output_header, mask_fixel_image, fixel_index_image);
  if (compute_negative_contrast)
    write_fixel_output (output_prefix + "cfe_neg.msf", *cfe_output_neg, output_header, mask_fixel_image, fixel_index_image);

  // Perform permutation testing
  opt = get_options ("notest");
  if (!opt.size()) {
    Eigen::Matrix<value_type, Eigen::Dynamic, 1> perm_distribution (num_perms);
    std::shared_ptr<Eigen::Matrix<value_type, Eigen::Dynamic, 1> > perm_distribution_neg;
    std::vector<value_type> uncorrected_pvalues (num_fixels, 0.0);
    std::shared_ptr<std::vector<value_type> > uncorrected_pvalues_neg;

    if (compute_negative_contrast) {
      perm_distribution_neg.reset (new Eigen::Matrix<value_type, Eigen::Dynamic, 1> (num_perms));
      uncorrected_pvalues_neg.reset (new std::vector<value_type> (num_fixels, 0.0));
    }

    Stats::PermTest::run_permutations (glm_ttest, cfe_integrator, num_perms, empirical_cfe_statistic,
                                       cfe_output, cfe_output_neg,
                                       perm_distribution, perm_distribution_neg,
                                       uncorrected_pvalues, uncorrected_pvalues_neg);

    ProgressBar progress ("outputting final results");
    save_matrix (perm_distribution, output_prefix + "perm_dist.txt");

    std::vector<value_type> pvalue_output (num_fixels, 0.0);
    Math::Stats::statistic2pvalue (perm_distribution, cfe_output, pvalue_output);
    write_fixel_output (output_prefix + "fwe_pvalue.msf", pvalue_output, output_header, mask_fixel_image, fixel_index_image);
    write_fixel_output (output_prefix + "uncorrected_pvalue.msf", uncorrected_pvalues, output_header, mask_fixel_image, fixel_index_image);

    if (compute_negative_contrast) {
      save_matrix (*perm_distribution_neg, output_prefix + "perm_dist_neg.txt");
      std::vector<value_type> pvalue_output_neg (num_fixels, 0.0);
      Math::Stats::statistic2pvalue (*perm_distribution_neg, *cfe_output_neg, pvalue_output_neg);
      write_fixel_output (output_prefix + "fwe_pvalue_neg.msf", pvalue_output_neg, output_header, mask_fixel_image, fixel_index_image);
      write_fixel_output (output_prefix + "uncorrected_pvalue_neg.msf", *uncorrected_pvalues_neg, output_header, mask_fixel_image, fixel_index_image);
    }
  }
}
