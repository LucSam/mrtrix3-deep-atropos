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
#include "debug.h"
#include "file/dicom/definitions.h"
#include "file/dicom/element.h"
#include "file/dicom/quick_scan.h"
#include "file/path.h"

using namespace MR;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Edit DICOM file in-place";

  DESCRIPTION
  + "Note that this command simply replaces the existing values"
    " without modifying the DICOM structure in any way."
    " Replacement text will be truncated"
    " if it is too long to fit inside the existing tag."

  + "WARNING: this command will modify existing data!"
    " It is recommended to run this command"
    " on a copy of the original data set to avoid loss of data."

  + "Command-line option -anonymise attempts to remove identifiable information"
    " by replacing the following tags: \n"
    "- any tag with Value Representation PN will be replaced with 'anonymous'; \n"
    "- tag (0010,0030) PatientBirthDate will be replaced with an empty string. \n"
    "WARNING: there is no guarantee that this command will remove all identiable information,"
    " since such information may be contained in any number of private vendor-specific tags."
    " You will need to double-check the results independently"
    " if you need to ensure anonymity.";

  ARGUMENTS
  + Argument ("file", "the DICOM file to be edited.").type_file_in();

  OPTIONS
  + Option ("anonymise", "remove identifiable information (see Description).")

  + Option ("id", "replace all ID tags with string supplied."
                  " This consists of tags (0010, 0020) PatientID"
                  " and (0010, 1000) OtherPatientIDs")
    + Argument ("text").type_text()

  + Option ("tag", "replace specific tag.").allow_multiple()
    + Argument ("group")
    + Argument ("element")
    + Argument ("newvalue");
}
// clang-format on

class Tag {
public:
  Tag(uint16_t group, uint16_t element, const std::string &newvalue)
      : group(group), element(element), newvalue(newvalue) {}
  uint16_t group, element;
  std::string newvalue;
};

inline std::string hex(uint16_t value) {
  std::ostringstream hex;
  hex << std::hex << value;
  return hex.str();
}

inline uint16_t read_hex(const std::string &m) {
  uint16_t value;
  std::istringstream hex(m);
  hex >> std::hex >> value;
  return value;
}

void run() {
  std::vector<Tag> tags;
  std::vector<uint16_t> VRs;

  if (!get_options("anonymise").empty()) {
    tags.push_back(Tag(0x0010U, 0x0030U, "")); // PatientBirthDate
    VRs.push_back(VR_PN);
  }

  auto opt = get_options("tag");
  if (!opt.empty())
    for (size_t n = 0; n < opt.size(); ++n)
      tags.push_back(Tag(read_hex(opt[n][0]), read_hex(opt[n][1]), opt[n][2]));

  opt = get_options("id");
  if (!opt.empty()) {
    std::string newid = opt[0][0];
    tags.push_back(Tag(0x0010U, 0x0020U, newid)); // PatientID
    tags.push_back(Tag(0x0010U, 0x1000U, newid)); // OtherPatientIDs
  }

  for (size_t n = 0; n < VRs.size(); ++n) {
    union __VR {
      uint16_t i;
      char c[2];
    } VR;
    VR.i = VRs[n];
    INFO(std::string("clearing entries with VR \"") + VR.c[1] + VR.c[0] + "\"");
  }
  for (size_t n = 0; n < tags.size(); ++n)
    INFO("replacing tag (" + hex(tags[n].group) + "," + hex(tags[n].element) + ") with value \"" + tags[n].newvalue +
         "\"");

  File::Dicom::Element item;
  item.set(argument[0], true, true);
  while (item.read()) {
    for (size_t n = 0; n < VRs.size(); ++n) {
      if (item.VR == VRs[n]) {
        memset(item.data, 32, item.size);
        memcpy(item.data, "anonymous", std::min<int>(item.size, 9));
      }
    }
    for (size_t n = 0; n < tags.size(); ++n) {
      if (item.is(tags[n].group, tags[n].element)) {
        memset(item.data, 32, item.size);
        memcpy(item.data, tags[n].newvalue.c_str(), std::min<int>(item.size, tags[n].newvalue.size()));
      }
    }
  }
}
