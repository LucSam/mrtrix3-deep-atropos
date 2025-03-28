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

#include <memory>
#include <string>

#include "types.h"

#include "mrview/tool/odf/item.h"
#include "mrview/tool/odf/type.h"

namespace MR::GUI::MRView::Tool {

class ODF_Model : public QAbstractItemModel {
public:
  ODF_Model(QObject *parent) : QAbstractItemModel(parent) {}

  QVariant data(const QModelIndex &index, int role) const {
    if (!index.isValid())
      return {};
    if (role != Qt::DisplayRole && role != Qt::ToolTipRole)
      return {};
    return qstr(items[index.row()]->image.get_filename());
  }

  bool setData(const QModelIndex &index, const QVariant &value, int role) {
    return QAbstractItemModel::setData(index, value, role);
  }

  Qt::ItemFlags flags(const QModelIndex &index) const {
    if (!index.isValid())
      return {};
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  }

  QModelIndex parent(const QModelIndex &) const { return {}; }

  int rowCount(const QModelIndex &parent = QModelIndex()) const {
    (void)parent; // to suppress warnings about unused parameters
    return items.size();
  }

  int columnCount(const QModelIndex &parent = QModelIndex()) const {
    (void)parent; // to suppress warnings about unused parameters
    return 1;
  }

  size_t add_items(const std::vector<std::string> &list,
                   const odf_type_t type,
                   bool colour_by_direction,
                   bool hide_negative_lobes,
                   float scale);

  QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const {
    (void)parent; // to suppress warnings about unused parameters
    return createIndex(row, column);
  }

  void remove_item(QModelIndex &index) {
    beginRemoveRows(QModelIndex(), index.row(), index.row());
    items.erase(items.begin() + index.row());
    endRemoveRows();
  }

  ODF_Item *get_image(QModelIndex &index) {
    return index.isValid() ? dynamic_cast<ODF_Item *>(items[index.row()].get()) : NULL;
  }

  std::vector<std::unique_ptr<ODF_Item>> items;
};

} // namespace MR::GUI::MRView::Tool
