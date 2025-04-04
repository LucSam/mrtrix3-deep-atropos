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

#include "mrview/mode/slice.h"

namespace MR::GUI::MRView::Mode {
class LightBox : public Slice {
  Q_OBJECT
  using proj_focusdelta = std::pair<Projection, float>;

public:
  LightBox();

  void paint(Projection &with_projection) override;
  void set_focus_event() override;
  void slice_move_event(float x) override;
  void pan_event() override;
  void panthrough_event() override;
  void tilt_event() override;
  void rotate_event() override;
  void image_changed_event() override;
  void reset_windowing() override;

  void request_update_mode_gui(ModeGuiVisitor &visitor) const override { visitor.update_lightbox_mode_gui(*this); }

  static size_t get_rows() { return n_rows; }
  static size_t get_cols() { return n_cols; }
  static size_t get_volume_increment() { return volume_increment; }
  static float get_slice_increment() { return slice_focus_increment; }
  static float get_slice_inc_adjust_rate() { return slice_focus_inc_adjust_rate; }
  static bool get_show_grid() { return show_grid_lines; }
  static bool get_show_volumes() { return show_volumes; }

  void set_rows(size_t rows);
  void set_cols(size_t cols);
  void set_volume_increment(size_t vol_inc);
  void set_slice_increment(float inc);
  void set_show_grid(bool show_grid);
  void set_show_volumes(bool show_vol);

public slots:
  void nrows_slot(int value) { set_rows(static_cast<size_t>(value)); }
  void ncolumns_slot(int value) { set_cols(static_cast<size_t>(value)); }
  void slice_inc_slot(float value) { set_slice_increment(value); }
  void volume_inc_slot(int value) { set_volume_increment(value); }
  void show_grid_slot(bool value) { set_show_grid(value); }
  void show_volumes_slot(bool value) { set_show_volumes(value); }
  void image_volume_changed_slot() { updateGL(); }

protected:
  void draw_plane_primitive(int axis, Displayable::Shader &shader_program, Projection &with_projection) override;

private:
  void draw_grid();
  bool render_volumes();

  // Want layout state to persist even after instance is destroyed
  static bool show_grid_lines, show_volumes;
  static std::string prev_image_name;
  static ssize_t n_rows, n_cols, volume_increment;
  static float slice_focus_increment;
  static float slice_focus_inc_adjust_rate;
  static ssize_t current_slice_index;

  ModelViewProjection get_projection_at(int row, int col) const;

  GL::VertexBuffer frame_VB;
  GL::VertexArrayObject frame_VAO;
  GL::Shader::Program frame_program;
  bool frames_dirty;
signals:
  void slice_increment_reset();
};

} // namespace MR::GUI::MRView::Mode
