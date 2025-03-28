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

#include "dialog/opengl.h"
#include "dialog/list.h"
#include "opengl/glutils.h"

namespace MR::GUI::Dialog {

OpenGL::OpenGL(QWidget *parent, const GL::Format &format) : QDialog(parent) {
  TreeModel *model = new TreeModel(this);

  TreeItem *root = model->rootItem;

  GLint i;
  gl::GetIntegerv(gl::MAJOR_VERSION, &i);
  std::string text = str(i) + ".";
  gl::GetIntegerv(gl::MINOR_VERSION, &i);
  text += str(i);

  root->appendChild(new TreeItem("API version", text, root));
  root->appendChild(new TreeItem("Renderer", (const char *)gl::GetString(gl::RENDERER), root));
  root->appendChild(new TreeItem("Vendor", (const char *)gl::GetString(gl::VENDOR), root));
  root->appendChild(new TreeItem("Version", (const char *)gl::GetString(gl::VERSION), root));

  TreeItem *bit_depths = new TreeItem("Bit depths", std::string(), root);
  root->appendChild(bit_depths);

  bit_depths->appendChild(new TreeItem("red", str(format.redBufferSize()), bit_depths));
  bit_depths->appendChild(new TreeItem("green", str(format.greenBufferSize()), bit_depths));
  bit_depths->appendChild(new TreeItem("blue", str(format.blueBufferSize()), bit_depths));
  bit_depths->appendChild(new TreeItem("alpha", str(format.alphaBufferSize()), bit_depths));
  bit_depths->appendChild(new TreeItem("depth", str(format.depthBufferSize()), bit_depths));
  bit_depths->appendChild(new TreeItem("stencil", str(format.stencilBufferSize()), bit_depths));

  root->appendChild(new TreeItem("Buffering",
                                 format.swapBehavior() == QSurfaceFormat::SingleBuffer
                                     ? "single"
                                     : (format.swapBehavior() == QSurfaceFormat::DoubleBuffer ? "double" : "triple"),
                                 root));
  root->appendChild(new TreeItem("VSync", format.swapInterval() ? "on" : "off", root));
  root->appendChild(
      new TreeItem("Multisample anti-aliasing", format.samples() ? str(format.samples()).c_str() : "off", root));

  gl::GetIntegerv(gl::MAX_TEXTURE_SIZE, &i);
  root->appendChild(new TreeItem("Maximum 2D texture size", str(i), root));

  gl::GetIntegerv(gl::MAX_3D_TEXTURE_SIZE, &i);
  root->appendChild(new TreeItem("Maximum 3D texture size", str(i), root));

  QTreeView *view = new QTreeView;
  view->setModel(model);
  view->resizeColumnToContents(0);
  view->resizeColumnToContents(1);
  view->setMinimumSize(500, 200);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->addWidget(view);
  layout->addWidget(buttonBox);
  setLayout(layout);

  setWindowTitle(tr("OpenGL information"));
  setSizeGripEnabled(true);
  adjustSize();
}

} // namespace MR::GUI::Dialog
