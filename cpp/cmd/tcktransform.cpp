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
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "image.h"
#include "interp/linear.h"
#include "ordered_thread_queue.h"
#include "progressbar.h"

using namespace MR;
using namespace MR::DWI;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Apply a spatial transformation to a tracks file";

  ARGUMENTS
  + Argument ("tracks", "the input track file.").type_tracks_in()
  + Argument ("transform", "the image containing the transform.").type_image_in()
  + Argument ("output", "the output track file").type_tracks_out();

}
// clang-format on

using value_type = float;
using TrackType = Tractography::Streamline<value_type>;

class Loader {
public:
  Loader(const std::string &file) : reader(file, properties) {}

  bool operator()(TrackType &item) { return reader(item); }

  Tractography::Properties properties;

protected:
  Tractography::Reader<value_type> reader;
};

class Warper {
public:
  Warper(const Image<value_type> &warp) : interp(warp) {}

  bool operator()(const TrackType &in, TrackType &out) {
    out.clear();
    out.set_index(in.get_index());
    out.weight = in.weight;
    for (size_t n = 0; n < in.size(); ++n) {
      auto vertex = pos(in[n]);
      if (vertex.allFinite())
        out.push_back(vertex);
    }
    return true;
  }

  Eigen::Matrix<value_type, 3, 1> pos(const Eigen::Matrix<value_type, 3, 1> &x) {
    Eigen::Matrix<value_type, 3, 1> p;
    if (interp.scanner(x)) {
      interp.index(3) = 0;
      p[0] = interp.value();
      interp.index(3) = 1;
      p[1] = interp.value();
      interp.index(3) = 2;
      p[2] = interp.value();
    }
    return p;
  }

protected:
  Interp::Linear<Image<value_type>> interp;
};

class Writer {
public:
  Writer(const std::string &file, const Tractography::Properties &properties)
      : progress("applying spatial transformation to tracks",
                 properties.find("count") == properties.end() ? 0 : to<size_t>(properties.find("count")->second)),
        writer(file, properties) {}

  bool operator()(const TrackType &item) {
    writer(item);
    ++progress;
    return true;
  }

protected:
  ProgressBar progress;
  Tractography::Properties properties;
  Tractography::Writer<value_type> writer;
};

void run() {
  Loader loader(argument[0]);

  auto data = Image<value_type>::open(argument[1]).with_direct_io(3);
  Warper warper(data);

  Writer writer(argument[2], loader.properties);

  Thread::run_ordered_queue(
      loader, Thread::batch(TrackType(), 1024), Thread::multi(warper), Thread::batch(TrackType(), 1024), writer);
}
