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

#include "file/mmap.h"
#include "image_io/base.h"
#include "memory.h"

namespace MR::ImageIO {

class Pipe : public Base {
public:
  Pipe(Base &&io_handler) : Base(std::move(io_handler)) {}

  static bool delete_piped_images;

protected:
  std::unique_ptr<File::MMap> mmap;

  virtual void load(const Header &, size_t);
  virtual void unload(const Header &);
};

} // namespace MR::ImageIO
