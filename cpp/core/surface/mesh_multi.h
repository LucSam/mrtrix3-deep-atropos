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

#include "types.h"

#include "surface/mesh.h"

namespace MR::Surface {

// Class to handle multiple meshes per file
// For now, this will only be supported using the .obj file type
// TODO Another alternative may be .vtp: XML-based polydata by VTK
//   (would allow embedding binary data within the file, rather than
//   everything being ASCII as in .obj)

class MeshMulti : public std::vector<Mesh> {
public:
  using std::vector<Mesh>::vector;

  void load(const std::string &);
  void save(const std::string &) const;
};

} // namespace MR::Surface
