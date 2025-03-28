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

#include "mrview/colourmap_button.h"
#include "opengl/glutils.h"

namespace MR::GUI::MRView::Tool {

class Connectome;

// Classes to receive input from the colourmap buttons and act accordingly
class NodeColourObserver : public ColourMapButtonObserver {
public:
  NodeColourObserver(Connectome &connectome) : master(connectome) {}
  void selected_colourmap(size_t, const ColourMapButton &) override;
  void selected_custom_colour(const QColor &, const ColourMapButton &) override;
  void toggle_show_colour_bar(bool, const ColourMapButton &) override;
  void toggle_invert_colourmap(bool, const ColourMapButton &) override;
  void reset_colourmap(const ColourMapButton &) override;

private:
  Connectome &master;
};
class EdgeColourObserver : public ColourMapButtonObserver {
public:
  EdgeColourObserver(Connectome &connectome) : master(connectome) {}
  void selected_colourmap(size_t, const ColourMapButton &) override;
  void selected_custom_colour(const QColor &, const ColourMapButton &) override;
  void toggle_show_colour_bar(bool, const ColourMapButton &) override;
  void toggle_invert_colourmap(bool, const ColourMapButton &) override;
  void reset_colourmap(const ColourMapButton &) override;

private:
  Connectome &master;
};

} // namespace MR::GUI::MRView::Tool
