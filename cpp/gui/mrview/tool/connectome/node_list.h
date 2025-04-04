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

#include "mrview/tool/base.h"
#include "mrview/tool/connectome/selection.h"

#include <QAbstractItemModel>
#include <QTableView>

namespace MR::GUI::MRView {

class Window;

namespace Tool {

class Connectome;

class Node_list_model : public QAbstractItemModel {
public:
  Node_list_model(Connectome *parent);

  QVariant data(const QModelIndex &index, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

  Qt::ItemFlags flags(const QModelIndex &index) const override {
    if (!index.isValid())
      return {};
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  }

  QModelIndex parent(const QModelIndex &) const override { return {}; }

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;

  QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override {
    (void)parent;
    return createIndex(row, column);
  }

  void clear() {
    beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
    endRemoveRows();
  }

  void initialize() {
    beginInsertRows(QModelIndex(), 0, rowCount() - 1);
    endInsertRows();
  }

  void reset_pixmaps();

private:
  Connectome &connectome;
};

class Node_list_view : public QTableView {
public:
  Node_list_view(QWidget *parent) : QTableView(parent) {}
  void setModel(QAbstractItemModel *model) {
    QTableView::setModel(model);
    // setColumnWidth (0, model->original_headerData (0, Qt::Horizontal, Qt::SizeHintRole).toInt());
    // setColumnWidth (1, model->original_headerData (1, Qt::Horizontal, Qt::SizeHintRole).toInt());
  }
};

class Node_list : public Tool::Base {

  Q_OBJECT

public:
  Node_list(Tool::Dock *, Connectome *);

  void initialize();
  void colours_changed();
  int row_height() const;

private slots:
  void clear_selection_slot();
  void node_selection_changed_slot(const QItemSelection &, const QItemSelection &);
  void node_selection_settings_dialog_slot();

private:
  Connectome &connectome;

  QPushButton *clear_selection_button;
  QPushButton *node_selection_settings_button;
  Node_list_model *node_list_model;
  Node_list_view *node_list_view;

  // Settings related to how visual elements are changed on selection / non-selection
  std::unique_ptr<NodeSelectionSettingsDialog> node_selection_dialog;
};

} // namespace Tool
} // namespace MR::GUI::MRView
