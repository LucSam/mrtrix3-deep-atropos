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

#include "file/config.h"
#include "header.h"
#include "raw.h"
#include "types.h"

#include <array>

namespace MR {
class Header;

namespace File::NIfTI {
extern bool right_left_warning_issued;

void axes_on_write(const Header &H, std::vector<size_t> &order, std::array<bool, 3> &flip);
transform_type adjust_transform(const Header &H, std::vector<size_t> &order);

bool check(int VERSION, Header &H, const size_t num_axes, const std::vector<std::string> &suffixes);

template <int VERSION> std::unique_ptr<ImageIO::Base> read(Header &H);
template <int VERSION> std::unique_ptr<ImageIO::Base> read_gz(Header &H);

template <int VERSION> std::unique_ptr<ImageIO::Base> create(Header &H);
template <int VERSION> std::unique_ptr<ImageIO::Base> create_gz(Header &H);

int version(Header &H);
std::string get_json_path(const std::string &nifti_path);

} // namespace File::NIfTI

} // namespace MR
