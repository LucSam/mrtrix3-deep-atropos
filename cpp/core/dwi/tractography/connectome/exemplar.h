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

#include <mutex>

#include "dwi/tractography/connectome/connectome.h"
#include "dwi/tractography/connectome/streamline.h"
#include "dwi/tractography/streamline.h"

namespace MR::DWI::Tractography::Connectome {

class Exemplar : private Tractography::Streamline<float> {
public:
  using Tractography::Streamline<float>::point_type;
  Exemplar(const size_t exemplar_index,
           const size_t length,
           const NodePair &nodes,
           const std::pair<point_type, point_type> &COMs)
      : Tractography::Streamline<float>(length, {0.0f, 0.0f, 0.0f}),
        nodes(nodes),
        node_COMs(COMs),
        is_finalized(false) {
    set_index(exemplar_index);
    weight = 0.0f;
  }

  Exemplar(Exemplar &&that)
      : Tractography::Streamline<float>(std::move(that)),
        mutex(),
        nodes(that.nodes),
        node_COMs(that.node_COMs),
        is_finalized(that.is_finalized) {}

  Exemplar(const Exemplar &that)
      : Tractography::Streamline<float>(that),
        mutex(),
        nodes(that.nodes),
        node_COMs(that.node_COMs),
        is_finalized(that.is_finalized) {}

  Exemplar &operator=(const Exemplar &that);

  void add(const Connectome::Streamline_nodepair &);
  void add(const Connectome::Streamline_nodelist &);
  void finalize(const float);

  const Tractography::Streamline<float> &get() const {
    assert(is_finalized);
    return *this;
  }

  bool is_diagonal() const { return nodes.first == nodes.second; }

  float get_weight() const { return weight; }

private:
  std::mutex mutex;
  NodePair nodes;
  std::pair<point_type, point_type> node_COMs;
  bool is_finalized;

  void add(const Tractography::Streamline<float> &, const bool is_reversed);
};

} // namespace MR::DWI::Tractography::Connectome
