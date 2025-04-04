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

#include <memory>

#include "header.h"
#include "image_io/scratch.h"

namespace MR::ImageIO {

bool Scratch::is_file_backed() const { return false; }

void Scratch::load(const Header &header, size_t buffer_size) {
  assert(buffer_size);
  DEBUG("allocating scratch buffer for image \"" + header.name() + "\"...");
  try {
    addresses.push_back(std::unique_ptr<uint8_t[]>(new uint8_t[buffer_size]));
    memset(addresses[0].get(), 0, buffer_size);
  } catch (...) {
    throw Exception("Error allocating memory for scratch buffer");
  }
}

void Scratch::unload(const Header &header) {
  if (!addresses.empty()) {
    DEBUG("deleting scratch buffer for image \"" + header.name() + "\"...");
    addresses[0].reset();
  }
}

} // namespace MR::ImageIO
