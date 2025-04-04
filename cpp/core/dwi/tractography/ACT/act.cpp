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

#include "dwi/tractography/ACT/act.h"
#include "dwi/tractography/properties.h"

namespace MR::DWI::Tractography::ACT {

using namespace App;

const OptionGroup ACTOption =
    OptionGroup("Anatomically-Constrained Tractography options")

    + Option("act",
             "use the Anatomically-Constrained Tractography framework during tracking; "
             "provided image must be in the 5TT (five-tissue-type) format") +
    Argument("image").type_image_in()

    + Option("backtrack", "allow tracks to be truncated and re-tracked if a poor structural termination is encountered")

    + Option("crop_at_gmwmi", "crop streamline endpoints more precisely as they cross the GM-WM interface");

void load_act_properties(Properties &properties) {
  auto opt = App::get_options("act");
  if (!opt.empty()) {

    properties["act"] = std::string(opt[0][0]);
    opt = get_options("backtrack");
    if (!opt.empty())
      properties["backtrack"] = "1";
    opt = get_options("crop_at_gmwmi");
    if (!opt.empty())
      properties["crop_at_gmwmi"] = "1";

  } else {

    if (!get_options("backtrack").empty())
      WARN("ignoring -backtrack option: only valid if using ACT");
    if (!get_options("crop_at_gmwmi").empty())
      WARN("ignoring -crop_at_gmwmi option: only valid if using ACT");
  }
}

void verify_5TT_image(const Header &H) {
  if (!H.datatype().is_floating_point() || H.ndim() != 4 || H.size(3) != 5)
    throw Exception("Image " + H.name() + " is not a valid ACT 5TT image" +
                    " (expecting 4D image with 5 volumes and floating-point datatype)");
}

} // namespace MR::DWI::Tractography::ACT
