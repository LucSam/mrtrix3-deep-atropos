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

#include "app.h"
#include "mrview/mode/base.h"
#include "opengl/transformation.h"

namespace MR::GUI::MRView {
namespace Tool {
class View;
}

namespace Mode {

class Volume : public Base {
public:
  Volume()
      : Base(FocusContrast | MoveTarget | TiltRotate | ShaderTransparency | ShaderThreshold | ShaderClipping),
        volume_shader(*this) {}

  virtual void paint(Projection &projection);
  virtual void tilt_event();

protected:
  GL::VertexBuffer volume_VB, volume_VI;
  GL::VertexArrayObject volume_VAO;
  GL::Texture depth_texture;
  std::vector<GL::vec4> clip;

  class Shader : public Displayable::Shader {
  public:
    Shader(const Volume &mode) : mode(mode), active_clip_planes(0), cliphighlight(true), clipintersectionmode(false) {}
    virtual std::string vertex_shader_source(const Displayable &object);
    virtual std::string fragment_shader_source(const Displayable &object);
    virtual bool need_update(const Displayable &object) const;
    virtual void update(const Displayable &object);
    const Volume &mode;
    size_t active_clip_planes;
    bool cliphighlight;
    bool clipintersectionmode;
  } volume_shader;

  Tool::View *get_view_tool() const;
  std::vector<std::pair<GL::vec4, bool>> get_active_clip_planes() const;
  std::vector<GL::vec4 *> get_clip_planes_to_be_edited() const;
  bool get_cliphighlightstate() const;
  bool get_clipintersectionmodestate() const;
};

} // namespace Mode
} // namespace MR::GUI::MRView
