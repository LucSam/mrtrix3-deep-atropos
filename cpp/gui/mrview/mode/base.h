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

#include "mrview/tool/base.h"
#include "mrview/window.h"
#include "opengl/glutils.h"
#include "opengl/transformation.h"
#include "projection.h"

#define ROTATION_INC 0.002
#define MOVE_IN_OUT_FOV_MULTIPLIER 1.0e-3f

namespace MR::GUI::MRView {

namespace Tool {
class Dock;
}

namespace Mode {

const int FocusContrast = 0x00000001;
const int MoveTarget = 0x00000002;
const int TiltRotate = 0x00000004;
const int MoveSlice = 0x00000008;
const int ShaderThreshold = 0x10000000;
const int ShaderTransparency = 0x20000000;
const int ShaderLighting = 0x40000000;
const int ShaderClipping = 0x80000008;

class Slice;
class Ortho;
class Volume;
class LightBox;
class ModeGuiVisitor {
public:
  virtual void update_base_mode_gui(const Base &) {}
  virtual void update_slice_mode_gui(const Slice &) {}
  virtual void update_ortho_mode_gui(const Ortho &) {}
  virtual void update_volume_mode_gui(const Volume &) {}
  virtual void update_lightbox_mode_gui(const LightBox &) {}
};

class Base : public QObject {
public:
  Base(int flags = FocusContrast | MoveTarget);
  virtual ~Base();

  Window &window() const { return *Window::main; }
  Projection projection;
  const int features;
  QList<ImageBase *> overlays_for_3D;
  bool update_overlays;

  virtual void paint(Projection &projection);
  virtual void mouse_press_event();
  virtual void mouse_release_event();
  virtual void reset_event();
  virtual void slice_move_event(float x);
  virtual void set_focus_event();
  virtual void contrast_event();
  virtual void pan_event();
  virtual void panthrough_event();
  virtual void tilt_event();
  virtual void rotate_event();
  virtual void image_changed_event() {}
  virtual const Projection *get_current_projection() const;
  virtual void reset_windowing();

  virtual void request_update_mode_gui(ModeGuiVisitor &visitor) const { visitor.update_base_mode_gui(*this); }

  void paintGL();

  const Image *image() const { return window().image(); }
  const Eigen::Vector3f &focus() const { return window().focus(); }
  const Eigen::Vector3f &target() const { return window().target(); }
  float FOV() const { return window().FOV(); }
  int plane() const { return window().plane(); }
  Eigen::Quaternionf orientation() const {
    if (snap_to_image()) {
      if (image())
        return Eigen::Quaternionf(image()->header().transform().rotation().cast<float>());
      else
        return Eigen::Quaternionf::Identity();
    }
    return window().orientation();
  }

  int width() const { return glarea()->width(); }
  int height() const { return glarea()->height(); }
  bool snap_to_image() const { return window().snap_to_image(); }

  Image *image() { return window().image(); }

  void move_target_to_focus_plane(const ModelViewProjection &projection) {
    Eigen::Vector3f in_plane_target = projection.model_to_screen(target());
    in_plane_target[2] = projection.depth_of(focus());
    set_target(projection.screen_to_model(in_plane_target));
  }
  void set_visible(bool v) {
    if (visible != v) {
      visible = v;
      updateGL();
    }
  }
  void set_focus(const Eigen::Vector3f &p) { window().set_focus(p); }
  void set_target(const Eigen::Vector3f &p) { window().set_target(p); }
  void set_FOV(float value) { window().set_FOV(value); }
  void set_plane(int p) { window().set_plane(p); }
  void set_orientation(const Eigen::Quaternionf &V) { window().set_orientation(V); }
  void reset_orientation() {
    if (image())
      set_orientation(Eigen::Quaternionf(image()->header().transform().rotation().cast<float>()));
    else
      set_orientation(Eigen::Quaternionf::Identity());
  }

  GL::Area *glarea() const { return reinterpret_cast<GL::Area *>(window().glarea); }

  Eigen::Vector3f get_through_plane_translation(float distance, const ModelViewProjection &projection) const {
    Eigen::Vector3f move(projection.screen_normal());
    move.normalize();
    move *= distance;
    return move;
  }

  Eigen::Vector3f get_through_plane_translation_FOV(int increment, const ModelViewProjection &projection) {
    return get_through_plane_translation(MOVE_IN_OUT_FOV_MULTIPLIER * increment * FOV(), projection);
  }

  void render_tools(const Projection &projection, bool is_3D = false, int axis = 0, int slice = 0) {
    QList<QAction *> tools = window().tools()->actions();
    for (int i = 0; i < tools.size(); ++i) {
      Tool::Dock *dock = dynamic_cast<Tool::__Action__ *>(tools[i])->dock;
      if (dock) {
        GL::assert_context_is_current();
        dock->tool->draw(projection, is_3D, axis, slice);
        GL::assert_context_is_current();
      }
    }
  }

  void setup_projection(const int, ModelViewProjection &) const;
  void setup_projection(const Eigen::Quaternionf &, ModelViewProjection &) const;
  void setup_projection(const GL::mat4 &, ModelViewProjection &) const;

  Eigen::Quaternionf get_tilt_rotation(const ModelViewProjection &proj) const;
  Eigen::Quaternionf get_rotate_rotation(const ModelViewProjection &proj) const;

  Eigen::Vector3f voxel_at(const Eigen::Vector3f &pos) const {
    if (!image())
      return Eigen::Vector3f{NAN, NAN, NAN};
    const Eigen::Vector3f result = image()->scanner2voxel().cast<float>() * pos;
    return result;
  }

  void draw_crosshairs(const Projection &with_projection) const {
    if (window().show_crosshairs())
      with_projection.render_crosshairs(focus());
  }

  void draw_orientation_labels(const Projection &with_projection) const {
    if (window().show_orientation_labels())
      with_projection.draw_orientation_labels();
  }

  int slice(int axis) const { return std::round(voxel_at(focus())[axis]); }
  int slice() const { return slice(plane()); }

  void updateGL() { window().updateGL(); }

protected:
  void slice_move_event(const ModelViewProjection &proj, float x);
  void set_focus_event(const ModelViewProjection &proj);
  void pan_event(const ModelViewProjection &proj);
  void panthrough_event(const ModelViewProjection &proj);
  void tilt_event(const ModelViewProjection &proj);
  void rotate_event(const ModelViewProjection &proj);

  GL::mat4 adjust_projection_matrix(const GL::mat4 &Q, int proj) const;
  GL::mat4 adjust_projection_matrix(const GL::mat4 &Q) const { return adjust_projection_matrix(Q, plane()); }

  void reset_view();

  bool visible;
};

//! \cond skip
class __Action__ : public QAction {
public:
  __Action__(QActionGroup *parent, const char *const name, const char *const description, int index)
      : QAction(name, parent) {
    setCheckable(true);
    setShortcut(tr(std::string("F" + str(index)).c_str()));
    setStatusTip(tr(description));
  }

  virtual Base *create() const = 0;
};
//! \endcond

template <class T> class Action : public __Action__ {
public:
  Action(QActionGroup *parent, const char *const name, const char *const description, int index)
      : __Action__(parent, name, description, index) {}

  virtual Base *create() const { return new T; }
};

} // namespace Mode

} // namespace MR::GUI::MRView
