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

#include "mrview/displayable.h"

namespace MR::GUI::MRView::Tool {

class ListModelBase : public QAbstractItemModel {
public:
  ListModelBase(QObject *parent) : QAbstractItemModel(parent) {}

  QVariant data(const QModelIndex &index, int role) const override {
    // The item may (temporarily) be null during an intermediate step of reordering items
    // see insertRows / removeRows
    if (!index.isValid())
      return QVariant();
    if (role == Qt::CheckStateRole) {
      return items[index.row()] && items[index.row()]->show ? Qt::Checked : Qt::Unchecked;
    }
    if (role != Qt::DisplayRole && role != Qt::ToolTipRole)
      return QVariant();
    return items[index.row()] ? qstr(items[index.row()]->get_filename()) : QString();
  }

  bool setData(const QModelIndex &idx, const QVariant &value, int role) override {
    if (role == Qt::CheckStateRole) {
      Qt::KeyboardModifiers keyMod = QApplication::keyboardModifiers();
      if (keyMod.testFlag(Qt::ShiftModifier)) {
        for (int i = 0; i < (int)items.size(); ++i) {
          if (i == idx.row())
            items[i]->show = true;
          else
            items[i]->show = false;
        }
        emit dataChanged(index(0, 0), index(items.size(), 0));
      } else {
        items[idx.row()]->show = (value == Qt::Checked);
        emit dataChanged(idx, idx);
      }
      return true;
    }
    return QAbstractItemModel::setData(idx, value, role);
  }

  Qt::DropActions supportedDropActions() const override { return Qt::CopyAction | Qt::MoveAction; }

  // For some reason, Qt calls insertRows prior to removeRows in the event of a drag-n-drop
  // item reordering within a given model.
  // Hence at this point, we simply want to cache where the rows should be moved
  bool insertRows(int row, int count, const QModelIndex &) override {
    if (count < 1 || row < 0 || row > rowCount()) {
      swapped_rows = {0, 0};
      return false;
    }

    swapped_rows = {row, count};

    return true;
  }

  // As alluded above in insertRows, in the case of a drag-n-drop item reordering,
  // we have to manually perform the swap within our underlying data store
  bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override {
    if (count < 1 || row < 0 || row > rowCount() || count != swapped_rows.second)
      return false;

    std::vector<std::unique_ptr<Displayable>> swapped_items;

    swapped_items.insert(swapped_items.begin(),
                         std::make_move_iterator(items.begin() + row),
                         std::make_move_iterator(items.begin() + row + count));

    beginRemoveRows(parent, row, row + count - 1);
    items.erase(items.begin() + row, items.begin() + row + count);
    endRemoveRows();

    // Cached row index was prior to removal, so may need to adjust offset
    if (swapped_rows.first >= row)
      swapped_rows.first -= count;

    beginInsertRows(parent, swapped_rows.first, swapped_rows.first + swapped_rows.second - 1);
    items.insert(items.begin() + swapped_rows.first,
                 std::make_move_iterator(swapped_items.begin()),
                 std::make_move_iterator(swapped_items.end()));
    endInsertRows();

    return true;
  }

  Qt::ItemFlags flags(const QModelIndex &index) const override {

    static const auto valid_flags =
        Qt::ItemIsDragEnabled | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
    static const auto invalid_flags = valid_flags | Qt::ItemIsDropEnabled;

    if (!index.isValid())
      return invalid_flags;
    return valid_flags;
  }

  QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override {
    (void)parent; // to suppress warnings about unused parameters
    return createIndex(row, column);
  }

  QModelIndex parent(const QModelIndex &) const override { return QModelIndex(); }

  int rowCount(const QModelIndex &parent = QModelIndex()) const override {
    (void)parent; // to suppress warnings about unused parameters
    return items.size();
  }

  int columnCount(const QModelIndex &parent = QModelIndex()) const override {
    (void)parent; // to suppress warnings about unused parameters
    return 1;
  }

  void remove_item(QModelIndex &index) {
    beginRemoveRows(QModelIndex(), index.row(), index.row());
    items.erase(items.begin() + index.row());
    endRemoveRows();
  }

  std::vector<std::unique_ptr<Displayable>> items;

private:
  std::pair<int, int> swapped_rows;
};

} // namespace MR::GUI::MRView::Tool
