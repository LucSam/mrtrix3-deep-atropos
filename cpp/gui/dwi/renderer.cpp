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

#include <map>

#include "dwi/renderer.h"
#include "gui.h"
#include "math/legendre.h"
#include "opengl/lighting.h"
#include "projection.h"

namespace MR::GUI::DWI {

Renderer::Renderer(QOpenGLWidget *widget)
    : mode(mode_t::SH), reverse_ID(0), origin_ID(0), sh(*this), tensor(*this), dixel(*this), context_(widget) {
  // CONF option: ObjectColor
  // CONF default: 1,1,0 (yellow)
  // CONF The default colour to use for objects (i.e. SH glyphs) when not
  // CONF colouring by direction.
  File::Config::get_RGB("ObjectColor", object_color, 1.0, 1.0, 0.0);
}

void Renderer::start(const Projection &projection,
                     const GL::Lighting &lighting,
                     float scale,
                     bool use_lighting,
                     bool colour_by_direction,
                     bool hide_neg_values,
                     bool orthographic,
                     const GL::mat4 *colour_relative_to_projection) {
  switch (mode) {
  case mode_t::SH:
    sh.bind();
    break;
  case mode_t::TENSOR:
    tensor.bind();
    break;
  case mode_t::DIXEL:
    dixel.bind();
    break;
  }

  if (mode == mode_t::TENSOR)
    scale *= 1000.0f;

  shader.start(mode, use_lighting, colour_by_direction, hide_neg_values, orthographic, colour_relative_to_projection);

  gl::UniformMatrix4fv(gl::GetUniformLocation(shader, "MV"), 1, gl::FALSE_, projection.modelview());
  gl::UniformMatrix4fv(gl::GetUniformLocation(shader, "MVP"), 1, gl::FALSE_, projection.modelview_projection());
  if (colour_relative_to_projection)
    gl::UniformMatrix4fv(gl::GetUniformLocation(shader, "rotation"), 1, gl::FALSE_, *colour_relative_to_projection);
  gl::Uniform3fv(gl::GetUniformLocation(shader, "light_pos"), 1, lighting.lightpos);
  gl::Uniform1f(gl::GetUniformLocation(shader, "ambient"), lighting.ambient);
  gl::Uniform1f(gl::GetUniformLocation(shader, "diffuse"), lighting.diffuse);
  gl::Uniform1f(gl::GetUniformLocation(shader, "specular"), lighting.specular);
  gl::Uniform1f(gl::GetUniformLocation(shader, "shine"), lighting.shine);
  gl::Uniform1f(gl::GetUniformLocation(shader, "scale"), scale);
  gl::Uniform3fv(gl::GetUniformLocation(shader, "constant_color"), 1, object_color);
  reverse_ID = gl::GetUniformLocation(shader, "reverse");
  origin_ID = gl::GetUniformLocation(shader, "origin");
}

void Renderer::Shader::start(mode_t mode,
                             bool use_lighting,
                             bool colour_by_direction,
                             bool hide_neg_values,
                             bool orthographic,
                             bool colour_relative_to_projection) {
  GL_CHECK_ERROR;
  if (!(*this) || mode != mode_ || use_lighting != use_lighting_ || colour_by_direction != colour_by_direction_ ||
      hide_neg_values != hide_neg_values_ || orthographic != orthographic_ ||
      colour_relative_to_projection != colour_relative_to_projection_) {
    mode_ = mode;
    use_lighting_ = use_lighting;
    colour_by_direction_ = colour_by_direction;
    hide_neg_values_ = hide_neg_values;
    orthographic_ = orthographic;
    colour_relative_to_projection_ = colour_relative_to_projection;
    if (*this)
      clear();
    GL::Shader::Vertex vertex_shader(vertex_shader_source());
    GL::Shader::Geometry geometry_shader(geometry_shader_source());
    GL::Shader::Fragment fragment_shader(fragment_shader_source());
    attach(vertex_shader);
    if ((GLuint)geometry_shader)
      attach(geometry_shader);
    attach(fragment_shader);
    link();
  }
  GL::Shader::Program::start();
  GL_CHECK_ERROR;
}

std::string Renderer::Shader::vertex_shader_source() const {
  std::string source;

  if (mode_ == mode_t::SH) {
    source += "layout(location = 0) in vec3 vertex;\n"
              "layout(location = 1) in vec3 r_del_daz;\n";
  } else if (mode_ == mode_t::TENSOR) {
    source += "layout(location = 0) in vec3 vertex;\n";
  } else if (mode_ == mode_t::DIXEL) {
    source += "layout(location = 0) in vec3 vertex;\n"
              "layout(location = 1) in float value;\n";
  }

  source += "uniform float scale;\n"
            "uniform int reverse;\n"
            "uniform vec3 constant_color, origin;\n"
            "uniform mat4 MV, MVP;\n";

  if (colour_relative_to_projection_)
    source += "uniform mat4 rotation;\n";

  if (mode_ == mode_t::TENSOR) {
    source += "uniform mat3 tensor;\n"
              "uniform mat3 inv_tensor;\n"
              "uniform vec3 dec;\n";
  }

  if (mode_ == mode_t::SH || mode_ == mode_t::TENSOR) {
    source += "out vec3 vertex_position, vertex_color, vertex_normal;\n"
              "out float amplitude;\n";
  } else if (mode_ == mode_t::DIXEL) {
    source += "out vec3 vertex_orig_direction, vertex_orig_position, vertex_orig_color;\n"
              "out float orig_amplitude;\n";
  }

  source += "void main () {\n";

  if (mode_ == mode_t::SH) {
    source += "  amplitude = r_del_daz[0];\n";
  } else if (mode_ == mode_t::TENSOR) {
    source += "  vec3 new_vertex = tensor * vertex;\n"
              "  amplitude = length(new_vertex);\n";
  } else if (mode_ == mode_t::DIXEL) {
    source += "  orig_amplitude = value;\n";
  }

  if (use_lighting_ && (mode_ == mode_t::SH || mode_ == mode_t::TENSOR)) {
    if (mode_ == mode_t::SH) {
      source += "  bool atpole = ( vertex.x == 0.0 && vertex.y == 0.0 );\n"
                "  float az = atpole ? 0.0 : atan (vertex.y, vertex.x);\n"
                "  float caz = cos (az), saz = sin (az), cel = vertex.z, sel = sqrt (1.0 - cel*cel);\n"
                "  vec3 d1;\n"
                "  if (atpole)\n"
                "    d1 = vec3 (-r_del_daz[0]*saz, r_del_daz[0]*caz, r_del_daz[2]);\n"
                "  else\n"
                "    d1 = vec3 (r_del_daz[2]*caz*sel - r_del_daz[0]*sel*saz, r_del_daz[2]*saz*sel + "
                "r_del_daz[0]*sel*caz, r_del_daz[2]*cel);\n"
                "  vec3 d2 = vec3 (-r_del_daz[1]*caz*sel - r_del_daz[0]*caz*cel,\n"
                "                  -r_del_daz[1]*saz*sel - r_del_daz[0]*saz*cel,\n"
                "                  -r_del_daz[1]*cel     + r_del_daz[0]*sel);\n"
                "  vertex_normal = cross (d1, d2);\n";
    } else if (mode_ == mode_t::TENSOR) {
      source += "  vertex_normal = normalize (inv_tensor * vertex);\n";
    }
    source += "  if (reverse != 0)\n"
              "    vertex_normal = -vertex_normal;\n"
              "  vertex_normal = normalize (mat3(MV) * vertex_normal);\n";
  }

  if (colour_by_direction_) {
    if (mode_ == mode_t::TENSOR) {
      source += "  vertex_color = dec;\n";
    } else {
      source += "  vertex_" + std::string(mode_ == mode_t::DIXEL ? "orig_" : "") + "color = abs (" +
                (colour_relative_to_projection_ ? "mat3(rotation) * " : "") + "vertex.xyz);\n";
    }
  } else {
    source += "  vertex_" + std::string(mode_ == mode_t::DIXEL ? "orig_" : "") + "color = constant_color;\n";
  }

  if (mode_ == mode_t::SH || mode_ == mode_t::TENSOR) {
    source += "  vec3 pos = " + std::string(mode_ == mode_t::TENSOR ? "new_vertex" : "vertex * amplitude") +
              " * scale;\n"
              "  if (reverse != 0)\n"
              "    pos = -pos;\n";
    if (orthographic_) {
      source += "  vertex_position = vec3(0.0, 0.0, 1.0);\n";
    } else {
      source += "  vertex_position = -(MV * vec4 (pos, 1.0)).xyz;\n";
    }
    source += "  gl_Position = MVP * vec4 (pos + origin, 1.0);\n";
  } else if (mode_ == mode_t::DIXEL) {
    source += "  vertex_orig_direction = vertex;\n"
              "  vertex_orig_position = vertex * orig_amplitude;\n"
              "  if (reverse != 0) {\n"
              "     vertex_orig_direction = -vertex_orig_direction;\n"
              "     vertex_orig_position = -vertex_orig_position;\n"
              "  }\n";
  }

  source += "}\n";

  return source;
}

std::string Renderer::Shader::geometry_shader_source() const {
  std::string source;
  if (mode_ == mode_t::DIXEL) {
    source += "layout(triangles) in;\n"
              "layout(triangle_strip, max_vertices = 3) out;\n"
              "uniform mat4 MV, MVP;\n"
              "uniform vec3 origin;\n"
              "uniform float scale;\n"
              "uniform int reverse;\n"
              "in vec3 vertex_orig_direction[], vertex_orig_position[], vertex_orig_color[];\n"
              "in float orig_amplitude[];\n"
              "out vec3 vertex_position, vertex_color, vertex_normal;\n"
              "out float amplitude;\n"
              "void main() {\n"
              "  vec3 mean_dir = normalize (vertex_orig_direction[0] + vertex_orig_direction[1] + "
              "vertex_orig_direction[2]);\n"
              "  vec3 vertices[3];\n"
              "  vec3 positions[3];\n"
              "  for (int v = 0; v < 3; ++v) {\n"
              "    if (dot (mean_dir, vertex_orig_direction[v]) > 0.0)\n"
              "      vertices[v] = vertex_orig_position[v];\n"
              "    else\n"
              "      vertices[v] = -vertex_orig_position[v];\n"
              "  }\n"
              "  vertex_normal = normalize (cross (vertices[1]-vertices[0], vertices[2]-vertices[1]));\n"
              "  if (reverse != 0)\n"
              "    vertex_normal = -vertex_normal;\n"
              "  vertex_normal = mat3(MV) * vertex_normal;\n"
              "  for (int v = 0; v < 3; ++v) {\n"
              "    gl_Position = MVP * vec4 (origin + (vertices[v] * scale), 1.0);\n";
    if (orthographic_)
      source += "    vertex_position = vec3(0.0, 0.0, 1.0);\n";
    else
      source += "    vertex_position = -(MV * vec4 (vertices[v] * scale, 1.0)).xyz;\n";
    source += "    vertex_color = vertex_orig_color[v];\n"
              "    amplitude = orig_amplitude[v];\n"
              "    EmitVertex();\n"
              "  }\n"
              "  EndPrimitive();\n"
              "}";
  }
  return source;
}

std::string Renderer::Shader::fragment_shader_source() const {
  std::string source;
  source += "uniform float ambient, diffuse, specular, shine;\n"
            "uniform vec3 light_pos;\n"
            "in float amplitude;\n"
            "in vec3 vertex_position, vertex_color, vertex_normal;\n"
            "out vec3 final_color;\n"
            "void main() {\n"
            "  if (amplitude < 0.0) {\n";

  if (hide_neg_values_) {
    source += "    discard;\n";
  } else {
    source += "    final_color = vec3(1.0,1.0,1.0);\n";
  }

  source += "  }\n"
            "  else final_color = vertex_color;\n";

  if (use_lighting_) {
    source += "  vec3 norm = normalize (vertex_normal);\n";
    source += "  if (amplitude < 0.0)\n"
              "    norm = -norm;\n"
              "  final_color *= ambient + diffuse * clamp (dot (norm, light_pos), 0, 1);\n"
              "  final_color += specular * pow (clamp (dot (reflect (-light_pos, norm), normalize(vertex_position)), "
              "0, 1), shine);\n";
  }

  source += "}\n";
  return source;
}

Renderer::SH::~SH() {
  GL::Context::Grab context(parent.context_);
  half_sphere.vertex_buffer.clear();
  half_sphere.index_buffer.clear();
  surface_buffer.clear();
  VAO.clear();
}

void Renderer::SH::initGL() {
  GL_CHECK_ERROR;
  GL::Context::Grab context(parent.context_);
  half_sphere.vertex_buffer.gen();
  surface_buffer.gen();
  half_sphere.index_buffer.gen();
  VAO.gen();
  VAO.bind();

  half_sphere.vertex_buffer.bind(gl::ARRAY_BUFFER);
  gl::EnableVertexAttribArray(0);
  gl::VertexAttribPointer(0, 3, gl::FLOAT, gl::FALSE_, sizeof(Shapes::HalfSphere::Vertex), (void *)0);

  surface_buffer.bind(gl::ARRAY_BUFFER);
  gl::EnableVertexAttribArray(1);
  gl::VertexAttribPointer(1, 3, gl::FLOAT, gl::FALSE_, 3 * sizeof(GLfloat), (void *)0);

  half_sphere.index_buffer.bind();
  GL_CHECK_ERROR;
}

void Renderer::SH::bind() {
  half_sphere.vertex_buffer.bind(gl::ARRAY_BUFFER);
  VAO.bind();
  half_sphere.index_buffer.bind();
}

void Renderer::SH::set_data(const vector_t &r_del_daz, int buffer_ID) const {
  (void)buffer_ID; // to silence unused-parameter warnings
  surface_buffer.bind(gl::ARRAY_BUFFER);
  gl::BufferData(gl::ARRAY_BUFFER, r_del_daz.size() * sizeof(float), &r_del_daz[0], gl::STREAM_DRAW);
  gl::VertexAttribPointer(1, 3, gl::FLOAT, gl::FALSE_, 3 * sizeof(GLfloat), (void *)0);
}

void Renderer::SH::update_mesh(const size_t lod, const int lmax) {
  INFO("updating ODF SH renderer transform...");
  QApplication::setOverrideCursor(Qt::BusyCursor);
  {
    GL::Context::Grab context(parent.context_);
    LOD = lod;
    half_sphere.LOD(LOD);
  }
  update_transform(half_sphere.vertices, lmax);
  QApplication::restoreOverrideCursor();
}

void Renderer::SH::update_transform(const std::vector<Shapes::HalfSphere::Vertex> &vertices, int lmax) {
  // order is r, del, daz

  transform = decltype(transform)::Zero(3 * vertices.size(), Math::SH::NforL(lmax));

  for (size_t n = 0; n < vertices.size(); ++n) {
    for (int l = 0; l <= lmax; l += 2) {
      for (int m = 0; m <= l; m++) {
        const int idx(Math::SH::index(l, m));
        transform(3 * n, idx) = transform(3 * n, idx - 2 * m) =
            (m ? Math::sqrt2 : 1.0) * Math::Legendre::Plm_sph<float>(l, m, vertices[n][2]);
      }
    }

    bool atpole(vertices[n][0] == 0.0 && vertices[n][1] == 0.0);
    float az = atpole ? 0.0 : atan2(vertices[n][1], vertices[n][0]);

    for (int l = 2; l <= lmax; l += 2) {
      const int idx(Math::SH::index(l, 0));
      transform(3 * n + 1, idx) = transform(3 * n, idx + 1) * sqrt(float(l * (l + 1)));
    }

    for (int m = 1; m <= lmax; m++) {
      float caz = cos(m * az);
      float saz = sin(m * az);
      for (int l = 2 * ((m + 1) / 2); l <= lmax; l += 2) {
        const int idx(Math::SH::index(l, m));
        transform(3 * n + 1, idx) = -transform(3 * n, idx - 1) * sqrt(float((l + m) * (l - m + 1)));
        if (l > m)
          transform(3 * n + 1, idx) += transform(3 * n, idx + 1) * sqrt(float((l - m) * (l + m + 1)));
        transform(3 * n + 1, idx) /= 2.0;

        const int idx2(idx - 2 * m);
        if (atpole) {
          transform(3 * n + 2, idx) = -transform(3 * n + 1, idx) * saz;
          transform(3 * n + 2, idx2) = transform(3 * n + 1, idx) * caz;
        } else {
          float tmp(m * transform(3 * n, idx));
          transform(3 * n + 2, idx) = -tmp * saz;
          transform(3 * n + 2, idx2) = tmp * caz;
        }

        transform(3 * n + 1, idx2) = transform(3 * n + 1, idx) * saz;
        transform(3 * n + 1, idx) *= caz;
      }
    }

    for (int m = 1; m <= lmax; m++) {
      float caz = cos(m * az);
      float saz = sin(m * az);
      for (int l = 2 * ((m + 1) / 2); l <= lmax; l += 2) {
        const int idx(Math::SH::index(l, m));
        transform(3 * n, idx) *= caz;
        transform(3 * n, idx - 2 * m) *= saz;
      }
    }
  }
}

Renderer::Tensor::~Tensor() {
  GL::Context::Grab context(parent.context_);
  half_sphere.vertex_buffer.clear();
  half_sphere.index_buffer.clear();
  VAO.clear();
}

void Renderer::Tensor::initGL() {
  GL_CHECK_ERROR;
  GL::Context::Grab context(parent.context_);
  half_sphere.vertex_buffer.gen();
  half_sphere.index_buffer.gen();
  VAO.gen();
  VAO.bind();

  half_sphere.vertex_buffer.bind(gl::ARRAY_BUFFER);
  gl::EnableVertexAttribArray(0);
  gl::VertexAttribPointer(0, 3, gl::FLOAT, gl::FALSE_, sizeof(Shapes::HalfSphere::Vertex), (void *)0);

  half_sphere.index_buffer.bind();
  GL_CHECK_ERROR;
}

void Renderer::Tensor::bind() {
  half_sphere.vertex_buffer.bind(gl::ARRAY_BUFFER);
  VAO.bind();
  half_sphere.index_buffer.bind();
}

void Renderer::Tensor::update_mesh(const size_t lod) {
  INFO("updating tensor renderer...");
  QApplication::setOverrideCursor(Qt::BusyCursor);
  {
    GL::Context::Grab context(parent.context_);
    LOD = lod;
    half_sphere.LOD(LOD);
  }
  QApplication::restoreOverrideCursor();
}

void Renderer::Tensor::set_data(const vector_t &data, int buffer_ID) const {
  (void)buffer_ID; // to silence unused-parameter warnings
  // For tensor overlay, send the (inverse) tensor coefficients and colour directly to the shader as a uniform
  assert(data.size() == 6);
  tensor_t D;
  D(0, 0) = data[0];
  D(1, 1) = data[1];
  D(2, 2) = data[2];
  D(0, 1) = D(1, 0) = data[3];
  D(0, 2) = D(2, 0) = data[4];
  D(1, 2) = D(2, 1) = data[5];
  Eigen::FullPivLU<tensor_t> lu_decomp(D);
  const tensor_t Dinv = lu_decomp.inverse();
  if (data[0] <= 0.0f || data[1] <= 0.0f || data[2] <= 0.0f || Dinv.diagonal().minCoeff() < 0.0f) {
    gl::UniformMatrix3fv(gl::GetUniformLocation(parent.shader, "tensor"), 1, gl::FALSE_, D.data());
    const tensor_t Dinv = tensor_t::Zero();
    gl::UniformMatrix3fv(gl::GetUniformLocation(parent.shader, "inv_tensor"), 1, gl::FALSE_, Dinv.data());
    const vector_t dec = vector_t::Zero(3);
    gl::Uniform3fv(gl::GetUniformLocation(parent.shader, "dec"), 1, dec.data());
  } else {
    gl::UniformMatrix3fv(gl::GetUniformLocation(parent.shader, "tensor"), 1, gl::FALSE_, D.data());
    gl::UniformMatrix3fv(gl::GetUniformLocation(parent.shader, "inv_tensor"), 1, gl::FALSE_, Dinv.data());
    eig.computeDirect(D);
    const vector_t dec = eig.eigenvectors().col(2).array().abs();
    gl::Uniform3fv(gl::GetUniformLocation(parent.shader, "dec"), 1, dec.data());
  }
}

Renderer::Dixel::~Dixel() {
  GL::Context::Grab context(parent.context_);
  vertex_buffer.clear();
  value_buffer.clear();
  index_buffer.clear();
  VAO.clear();
}

void Renderer::Dixel::initGL() {
  GL_CHECK_ERROR;
  GL::Context::Grab context(parent.context_);
  vertex_buffer.gen();
  value_buffer.gen();
  index_buffer.gen();
  VAO.gen();
  VAO.bind();

  vertex_buffer.bind(gl::ARRAY_BUFFER);
  gl::EnableVertexAttribArray(0);
  gl::VertexAttribPointer(0, 3, gl::FLOAT, gl::FALSE_, 3 * sizeof(GLfloat), (void *)0);

  value_buffer.bind(gl::ARRAY_BUFFER);
  gl::EnableVertexAttribArray(1);
  gl::VertexAttribPointer(1, 1, gl::FLOAT, gl::FALSE_, sizeof(GLfloat), (void *)0);
  GL_CHECK_ERROR;
}

void Renderer::Dixel::bind() {
  vertex_buffer.bind(gl::ARRAY_BUFFER);
  VAO.bind();
}

void Renderer::Dixel::set_data(const vector_t &data, int buffer_ID) const {
  (void)buffer_ID;
  assert(data.size() == vertex_count);

  GL_CHECK_ERROR;
  GL::Context::Grab context(parent.context_);
  VAO.bind();
  value_buffer.bind(gl::ARRAY_BUFFER);
  gl::BufferData(gl::ARRAY_BUFFER, vertex_count * sizeof(GLfloat), &data[0], gl::STREAM_DRAW);
  gl::VertexAttribPointer(1, 1, gl::FLOAT, gl::FALSE_, sizeof(GLfloat), (void *)0);
  GL_CHECK_ERROR;
}

void Renderer::Dixel::update_mesh(const MR::DWI::Directions::Set &dirs) {
  INFO("updating ODF dixel renderer transform...");
  QApplication::setOverrideCursor(Qt::BusyCursor);
  update_dixels(dirs);
  QApplication::restoreOverrideCursor();
}

void Renderer::Dixel::update_dixels(const MR::DWI::Directions::Set &dirs) {
  std::vector<Eigen::Vector3f> directions_data;
  std::vector<std::array<GLint, 3>> indices_data;

  for (size_t i = 0; i != dirs.size(); ++i) {
    directions_data.push_back(dirs[i].cast<float>());
    for (auto j : dirs.get_adj_dirs(i)) {
      if (j > i) {
        for (auto k : dirs.get_adj_dirs(j)) {
          if (k > j) {

            // k's adjacent direction list MUST contain i!
            for (auto I : dirs.get_adj_dirs(k)) {
              if (I == i) {

                // Invert a direction if required
                std::array<Eigen::Vector3d, 3> d{{dirs[i], dirs[j], dirs[k]}};
                const Eigen::Vector3d mean_dir((d[0] + d[1] + d[2]).normalized());
                for (size_t v = 0; v != 3; ++v) {
                  if (d[v].dot(mean_dir) < 0.0)
                    d[v] = -d[v];
                }
                // Conform to right hand rule
                const Eigen::Vector3d normal(((d[1] - d[0]).cross(d[2] - d[1])).normalized());
                if (normal.dot(mean_dir) < 0.0)
                  indices_data.push_back({{GLint(i), GLint(k), GLint(j)}});
                else
                  indices_data.push_back({{GLint(i), GLint(j), GLint(k)}});
              }
            }
          }
        }
      }
    }
  }

  GL_CHECK_ERROR;
  GL::Context::Grab context(parent.context_);
  VAO.bind();
  vertex_buffer.bind(gl::ARRAY_BUFFER);
  gl::BufferData(gl::ARRAY_BUFFER, dirs.size() * sizeof(Eigen::Vector3f), &directions_data[0], gl::STATIC_DRAW);
  gl::VertexAttribPointer(0, 3, gl::FLOAT, gl::FALSE_, 3 * sizeof(GLfloat), (void *)0);
  index_buffer.bind();
  gl::BufferData(
      gl::ELEMENT_ARRAY_BUFFER, indices_data.size() * sizeof(std::array<GLint, 3>), &indices_data[0], gl::STATIC_DRAW);
  GL_CHECK_ERROR;

  vertex_count = dirs.size();
  index_count = 3 * indices_data.size();
}

} // namespace MR::GUI::DWI
