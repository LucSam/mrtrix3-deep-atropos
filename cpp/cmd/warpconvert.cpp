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

#include "adapter/extract.h"
#include "command.h"
#include "file/nifti_utils.h"
#include "image.h"
#include "registration/warp/compose.h"
#include "registration/warp/convert.h"
#include "registration/warp/helpers.h"

using namespace MR;
using namespace App;

const std::vector<std::string> conversion_types = {
    "deformation2displacement", "displacement2deformation", "warpfull2deformation", "warpfull2displacement"};

// clang-format off
void usage() {

  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  SYNOPSIS = "Convert between different representations of a non-linear warp";

  DESCRIPTION
  + "A deformation field is defined as an image"
    " where each voxel defines the corresponding position in the other image"
    " (in scanner space coordinates)."
    " A displacement field stores the displacements (in mm) to the other image"
    " from each voxel's position (in scanner space)."
    " The warpfull file is the 5D format output from mrregister -nl_warp_full,"
    " which contains linear transforms, warps and their inverses"
    " that map each image to a midway space.";
    //TODO at link to warp format documentation

  ARGUMENTS
  + Argument ("in", "the input warp image.").type_image_in()
  + Argument ("type", "the conversion type required;"
                      " valid choices are: " + join(conversion_types, ", ")).type_choice(conversion_types)
  + Argument ("out", "the output warp image.").type_image_out();

  OPTIONS
  + Option ("template",
            "define a template image when converting a warpfull file"
            " (which is defined on a grid in the midway space between image 1 & 2)."
            " For example, to generate the deformation field that maps image1 to image2,"
            " then supply image2 as the template image")
  + Argument ("image").type_image_in()

  + Option ("midway_space",
            "to be used only with warpfull2deformation and warpfull2displacement conversion types."
            " The output will only contain the non-linear warp to map an input image to the midway space"
            " (defined by the warpfull grid)."
            " If a linear transform exists in the warpfull file header"
            " then it will be composed and included in the output.")

  + Option ("from",
      "to be used only with warpfull2deformation and warpfull2displacement conversion types."
      " Used to define the direction of the desired output field."
      " Use -from 1 to obtain the image1->image2 field and from 2 for image2->image1."
      " Can be used in combination with the -midway_space option "
      " to produce a field that only maps to midway space.")
  +   Argument ("image").type_integer(1, 2);

}
// clang-format on

void run() {
  const int type = argument[1];
  const bool midway_space = !get_options("midway_space").empty();
  const std::string template_filename = get_option_value<std::string>("template", "");
  const int from = get_option_value("from", 1);

  // deformation2displacement
  if (type == 0) {
    if (midway_space)
      WARN("-midway_space option ignored with deformation2displacement conversion type");
    if (!get_options("template").empty())
      WARN("-template option ignored with deformation2displacement conversion type");
    if (!get_options("from").empty())
      WARN("-from option ignored with deformation2displacement conversion type");

    auto deformation = Image<default_type>::open(argument[0]).with_direct_io(3);
    Registration::Warp::check_warp(deformation);

    Header header(deformation);
    header.datatype() = DataType::from_command_line(DataType::Float32);
    Image<default_type> displacement = Image<default_type>::create(argument[2], header).with_direct_io();
    Registration::Warp::deformation2displacement(deformation, displacement);

    // displacement2deformation
  } else if (type == 1) {
    auto displacement = Image<default_type>::open(argument[0]).with_direct_io(3);
    Registration::Warp::check_warp(displacement);

    if (midway_space)
      WARN("-midway_space option ignored with displacement2deformation conversion type");
    if (!get_options("template").empty())
      WARN("-template option ignored with displacement2deformation conversion type");
    if (!get_options("from").empty())
      WARN("-from option ignored with displacement2deformation conversion type");

    Header header(displacement);
    header.datatype() = DataType::from_command_line(DataType::Float32);
    Image<default_type> deformation = Image<default_type>::create(argument[2], header).with_direct_io();
    Registration::Warp::displacement2deformation(displacement, deformation);

    // warpfull2deformation & warpfull2displacement
  } else if (type == 2 || type == 3) {
    if (!Path::is_mrtrix_image(argument[0]) &&                  //
        !(Path::has_suffix(argument[0], {".nii", ".nii.gz"}) && //
          File::Config::get_bool("NIfTIAutoLoadJSON", false) && //
          Path::exists(File::NIfTI::get_json_path(argument[0])))) {
      WARN("warp_full image is not in original .mif/.mih file format or in NIfTI file format with associated JSON.  "
           "Converting to other file formats may remove linear transformations stored in the image header.");
    }
    auto warp = Image<default_type>::open(argument[0]).with_direct_io(3);
    Registration::Warp::check_warp_full(warp);

    Image<default_type> warp_output;
    if (midway_space) {
      warp_output = Registration::Warp::compute_midway_deformation(warp, from);
    } else {
      if (get_options("template").empty())
        throw Exception("-template option required with warpfull2deformation or warpfull2displacement conversion type");
      auto template_header = Header::open(template_filename);
      warp_output = Registration::Warp::compute_full_deformation(warp, template_header, from);
    }

    if (type == 3)
      Registration::Warp::deformation2displacement(warp_output, warp_output);

    Header header(warp_output);
    header.datatype() = DataType::from_command_line(DataType::Float32);
    Image<default_type> output = Image<default_type>::create(argument[2], header);
    threaded_copy_with_progress_message("converting warp", warp_output, output);

  } else {
    throw Exception("Unsupported warp conversion type");
  }
}
