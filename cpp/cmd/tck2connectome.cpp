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

#include <set>

#include "command.h"
#include "image.h"
#include "thread_queue.h"
#include "types.h"

#include "dwi/tractography/connectome/connectome.h"
#include "dwi/tractography/connectome/mapper.h"
#include "dwi/tractography/connectome/matrix.h"
#include "dwi/tractography/connectome/metric.h"
#include "dwi/tractography/connectome/tck2nodes.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/weights.h"

using namespace MR;
using namespace App;
using namespace MR::DWI;
using namespace MR::DWI::Tractography;
using namespace MR::DWI::Tractography::Connectome;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Generate a connectome matrix from a streamlines file and a node parcellation image";

  EXAMPLES
  + Example ("Default usage",
             "tck2connectome tracks.tck nodes.mif connectome.csv -tck_weights_in weights.csv -out_assignments assignments.txt",
             "By default,"
             " the metric of connectivity quantified in the connectome matrix is the number of streamlines;"
             " or, if tcksift2 is used,"
             " the sum of streamline weights via the -tck_weights_in option."
             " Use of the -out_assignments option is recommended"
             " as this enables subsequent use of the connectome2tck command.")

  + Example ("Generate a matrix consisting of the mean streamline length between each node pair",
             "tck2connectome tracks.tck nodes.mif distances.csv -scale_length -stat_edge mean",
             "By multiplying the contribution of each streamline to the connectome"
             " by the length of that streamline,"
             " and then, for each edge,"
             " computing the mean value across the contributing streamlines,"
             " one obtains a matrix where the value in each entry is the"
             " mean length across those streamlines belonging to that edge.")

  + Example ("Generate a connectome matrix where the value of connectivity is the \"mean FA\"",
             "tcksample tracks.tck FA.mif mean_FA_per_streamline.csv -stat_tck mean; "
             "tck2connectome tracks.tck nodes.mif mean_FA_connectome.csv -scale_file mean_FA_per_streamline.csv -stat_edge mean",
             "Here, a connectome matrix that is \"weighted by FA\" is generated in multiple steps:"
             " firstly, for each streamline,"
             " the value of the underlying FA image is sampled at each vertex,"
             " and the mean of these values is calculated to produce"
             " a single scalar value of \"mean FA\" per streamline;"
             " then, as each streamline is assigned to nodes within the connectome,"
             " the magnitude of the contribution of that streamline to the matrix"
             " is multiplied by the mean FA value calculated prior for that streamline;"
             " finally, for each connectome edge,"
             " across the values of \"mean FA\" that were contributed by all of the streamlines"
             " assigned to that particular edge,"
             " the mean value is calculated.")

  + Example ("Generate the connectivity fingerprint for streamlines seeded from a particular region",
             "tck2connectome fixed_seed_tracks.tck nodes.mif fingerprint.csv -vector",
             "This usage assumes that the streamlines being provided to the command "
             " have all been seeded from the (effectively) same location,"
             " and as such, only the endpoint of each streamline"
             " (not their starting point)"
             " is assigned based on the provided parcellation image."
             " Accordingly, the output file contains only"
             " a vector of connectivity values rather than a matrix,"
             " since each streamline is assigned to only one node rather than two.");


  ARGUMENTS
  + Argument ("tracks_in", "the input track file").type_tracks_in()
  + Argument ("nodes_in", "the input node parcellation image").type_image_in()
  + Argument ("connectome_out", "the output file containing edge weights").type_file_out();


  OPTIONS
  + MR::DWI::Tractography::Connectome::AssignmentOptions
  + MR::DWI::Tractography::Connectome::MetricOptions
  + MR::Connectome::MatrixOutputOptions

  + OptionGroup ("Other options for tck2connectome")

  + MR::DWI::Tractography::Connectome::EdgeStatisticOption

  + Tractography::TrackWeightsInOption

  + Option ("keep_unassigned", "By default,"
                               " the program discards the information regarding those streamlines"
                               " that are not successfully assigned to a node pair."
                               " Set this option to keep these values"
                               " (will be the first row/column in the output matrix)")

  + Option ("out_assignments", "output the node assignments of each streamline to a file;"
                               " this can be used subsequently e.g. by the command connectome2tck")
    + Argument ("path").type_file_out()

  + Option ("vector", "output a vector representing connectivities from a given seed point to target nodes,"
                      " rather than a matrix of node-node connectivities");

  REFERENCES
  + "If using the default streamline-parcel assignment mechanism"
    " (or -assignment_radial_search option):\n" // Internal
    "Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. "
    "The effects of SIFT on the reproducibility and biological accuracy of the structural connectome. "
    "NeuroImage, 2015, 104, 253-265"

  + "If using -scale_invlength or -scale_invnodevol options:\n"
    "Hagmann, P.; Cammoun, L.; Gigandet, X.; Meuli, R.; Honey, C.; Wedeen, V. & Sporns, O. "
    "Mapping the Structural Core of Human Cerebral Cortex. "
    "PLoS Biology 6(7), e159";

}
// clang-format on

template <typename T>
void execute(Image<node_t> &node_image, const node_t max_node_index, const std::set<node_t> &missing_nodes) {
  // Are we generating a matrix or a vector?
  const bool vector_output = !get_options("vector").empty();

  // Do we need to keep track of the nodes to which each streamline is
  //   assigned, or would it be a waste of memory?
  const bool track_assignments = !get_options("out_assignments").empty();

  // Get the metric, assignment mechanism & per-edge statistic for connectome construction
  Metric metric;
  Tractography::Connectome::setup_metric(metric, node_image);
  std::unique_ptr<Tck2nodes_base> tck2nodes(load_assignment_mode(node_image));
  auto opt = get_options("stat_edge");
  const stat_edge statistic = !opt.empty() ? stat_edge(int(opt[0][0])) : stat_edge::SUM;

  // Prepare for reading the track data
  Tractography::Properties properties;
  Tractography::Reader<float> reader(argument[0], properties);

  // Initialise classes in preparation for multi-threading
  Mapping::TrackLoader loader(
      reader, properties["count"].empty() ? 0 : to<size_t>(properties["count"]), "Constructing connectome");
  Tractography::Connectome::Mapper mapper(*tck2nodes, metric);
  Tractography::Connectome::Matrix<T> connectome(max_node_index, statistic, vector_output, track_assignments);

  // Multi-threaded connectome construction
  if (tck2nodes->provides_pair()) {
    Thread::run_queue(loader,
                      Thread::batch(Tractography::Streamline<float>()),
                      Thread::multi(mapper),
                      Thread::batch(Mapped_track_nodepair()),
                      connectome);
  } else {
    Thread::run_queue(loader,
                      Thread::batch(Tractography::Streamline<float>()),
                      Thread::multi(mapper),
                      Thread::batch(Mapped_track_nodelist()),
                      connectome);
  }

  connectome.finalize();
  connectome.error_check(missing_nodes);

  connectome.save(argument[2],
                  get_options("keep_unassigned").size(),
                  get_options("symmetric").size(),
                  get_options("zero_diagonal").size());

  opt = get_options("out_assignments");
  if (!opt.empty())
    connectome.write_assignments(opt[0][0]);
}

void run() {
  auto node_header = Header::open(argument[1]);
  MR::Connectome::check(node_header);
  auto node_image = node_header.get_image<node_t>();

  // First, find out how many segmented nodes there are, so the matrix can be pre-allocated
  // Also check for node volume for all nodes
  std::vector<uint32_t> node_volumes(1, 0);
  node_t max_node_index = 0;
  for (auto i = Loop(node_image, 0, 3)(node_image); i; ++i) {
    if (node_image.value() > max_node_index) {
      max_node_index = node_image.value();
      node_volumes.resize(max_node_index + 1, 0);
    }
    ++node_volumes[node_image.value()];
  }

  std::set<node_t> missing_nodes;
  for (size_t i = 1; i != node_volumes.size(); ++i) {
    if (!node_volumes[i])
      missing_nodes.insert(i);
  }
  if (!missing_nodes.empty()) {
    WARN("The following nodes are missing from the parcellation image:");
    std::set<node_t>::iterator i = missing_nodes.begin();
    std::string list = str(*i);
    for (++i; i != missing_nodes.end(); ++i)
      list += ", " + str(*i);
    WARN(list);
    WARN("(This may indicate poor parcellation image preparation, use of incorrect or incomplete LUT file(s) in "
         "labelconvert, or very poor registration)");
  }

  if (max_node_index >= node_count_ram_limit) {
    INFO("Very large number of nodes detected; using single-precision floating-point storage");
    execute<float>(node_image, max_node_index, missing_nodes);
  } else {
    execute<double>(node_image, max_node_index, missing_nodes);
  }
}
