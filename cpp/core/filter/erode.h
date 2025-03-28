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

#include "algo/copy.h"
#include "algo/loop.h"
#include "filter/base.h"
#include "image.h"
#include "image_helpers.h"
#include "memory.h"
#include "progressbar.h"

namespace MR::Filter {

/** \addtogroup Filters
  @{ */

//! a filter to erode a mask
/*!
 * Typical usage:
 * \code
 * auto input = Image<bool>::open (argument[0]);
 *
 * Filter::Erode erode (input);
 *
 * Image<bool> output (erode, argument[1]);
 * erode (input, output);
 *
 * \endcode
 */
class Erode : public Base {

public:
  template <class HeaderType> Erode(const HeaderType &in) : Base(in), npass(1) {
    check_3D_nonunity(in);
    datatype_ = DataType::Bit;
  }

  template <class HeaderType> Erode(const HeaderType &in, const std::string &message) : Base(in, message), npass(1) {
    check_3D_nonunity(in);
    datatype_ = DataType::Bit;
  }

  template <class InputImageType, class OutputImageType>
  void operator()(InputImageType &input, OutputImageType &output) {
    std::shared_ptr<Image<bool>> in = std::make_shared<Image<bool>>(Image<bool>::scratch(input));
    copy(input, *in);
    std::shared_ptr<Image<bool>> out;
    std::shared_ptr<ProgressBar> progress(!message.empty() ? new ProgressBar(message, npass + 1) : nullptr);

    for (unsigned int pass = 0; pass < npass; pass++) {
      out = std::make_shared<Image<bool>>(Image<bool>::scratch(input));
      for (auto l = Loop(*in)(*in, *out); l; ++l)
        out->value() = erode(*in);

      if (pass < npass - 1)
        in = out;
      if (progress)
        ++(*progress);
    }
    copy(*out, output);
  }

  void set_npass(unsigned int npasses) { npass = npasses; }

protected:
  bool erode(Image<bool> &in) {
    if (!in.value())
      return false;
    if ((in.index(0) == 0) || (in.index(0) == in.size(0) - 1) || (in.index(1) == 0) ||
        (in.index(1) == in.size(1) - 1) || (in.index(2) == 0) || (in.index(2) == in.size(2) - 1))
      return false;
    bool val;
    if (in.index(0) > 0) {
      in.index(0)--;
      val = in.value();
      in.index(0)++;
      if (!val)
        return false;
    }
    if (in.index(1) > 0) {
      in.index(1)--;
      val = in.value();
      in.index(1)++;
      if (!val)
        return false;
    }
    if (in.index(2) > 0) {
      in.index(2)--;
      val = in.value();
      in.index(2)++;
      if (!val)
        return false;
    }
    if (in.index(0) < in.size(0) - 1) {
      in.index(0)++;
      val = in.value();
      in.index(0)--;
      if (!val)
        return false;
    }
    if (in.index(1) < in.size(1) - 1) {
      in.index(1)++;
      val = in.value();
      in.index(1)--;
      if (!val)
        return false;
    }
    if (in.index(2) < in.size(2) - 1) {
      in.index(2)++;
      val = in.value();
      in.index(2)--;
      if (!val)
        return false;
    }
    return true;
  }

  unsigned int npass;
};
//! @}
} // namespace MR::Filter
