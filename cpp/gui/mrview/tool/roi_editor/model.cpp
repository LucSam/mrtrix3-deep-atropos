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

#include "mrview/tool/roi_editor/model.h"
#include "mrview/window.h"

namespace MR::GUI::MRView::Tool {

void ROI_Model::load(std::vector<std::unique_ptr<MR::Header>> &list) {
  beginInsertRows(QModelIndex(), items.size(), items.size() + list.size());
  for (size_t i = 0; i < list.size(); ++i) {
    GL::Context::Grab context;
    ROI_Item *roi = new ROI_Item(std::move(*list[i]));
    roi->load();
    items.push_back(std::unique_ptr<Displayable>(roi));
  }
  endInsertRows();
}

void ROI_Model::create(MR::Header &&image) {
  beginInsertRows(QModelIndex(), items.size(), items.size() + 1);
  {
    GL::Context::Grab context;
    ROI_Item *roi = new ROI_Item(std::move(image));
    roi->zero();
    items.push_back(std::unique_ptr<Displayable>(roi));
  }
  endInsertRows();
}

} // namespace MR::GUI::MRView::Tool
