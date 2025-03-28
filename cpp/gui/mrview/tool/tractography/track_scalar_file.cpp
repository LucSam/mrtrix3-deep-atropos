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

#include "mrview/tool/tractography/track_scalar_file.h"

#include "dialog/file.h"
#include "mrview/tool/tractography/tractogram.h"

namespace MR::GUI::MRView::Tool {

TrackScalarFileOptions::TrackScalarFileOptions(Tractography *parent)
    : QGroupBox("Scalar file options", parent), tool(parent), tractogram(nullptr) {
  main_box = new Tool::Base::VBoxLayout(this);

  colour_groupbox = new QGroupBox("Colour map and scaling");
  Tool::Base::VBoxLayout *vlayout = new Tool::Base::VBoxLayout;
  vlayout->setContentsMargins(0, 0, 0, 0);
  vlayout->setSpacing(0);
  colour_groupbox->setLayout(vlayout);

  Tool::Base::HBoxLayout *hlayout = new Tool::Base::HBoxLayout;
  hlayout->setContentsMargins(0, 0, 0, 0);
  hlayout->setSpacing(0);

  intensity_file_button = new QPushButton(this);
  intensity_file_button->setToolTip(tr("Open (track) scalar file for colouring streamlines"));
  connect(intensity_file_button, SIGNAL(clicked()), this, SLOT(open_intensity_track_scalar_file_slot()));
  hlayout->addWidget(intensity_file_button);

  colourmap_button = new ColourMapButton(this, *this, false, false, true);
  hlayout->addWidget(colourmap_button);

  vlayout->addLayout(hlayout);

  hlayout = new Tool::Base::HBoxLayout;
  hlayout->setContentsMargins(0, 0, 0, 0);
  hlayout->setSpacing(0);

  min_entry = new AdjustButton(this);
  connect(min_entry, SIGNAL(valueChanged()), this, SLOT(on_set_scaling_slot()));
  hlayout->addWidget(min_entry);

  max_entry = new AdjustButton(this);
  connect(max_entry, SIGNAL(valueChanged()), this, SLOT(on_set_scaling_slot()));
  hlayout->addWidget(max_entry);

  vlayout->addLayout(hlayout);

  main_box->addWidget(colour_groupbox);

  QGroupBox *threshold_box = new QGroupBox("Thresholds");
  vlayout = new Tool::Base::VBoxLayout;
  vlayout->setContentsMargins(0, 0, 0, 0);
  vlayout->setSpacing(0);
  threshold_box->setLayout(vlayout);

  threshold_file_combobox = new QComboBox(this);
  threshold_file_combobox->addItem("None");
  threshold_file_combobox->addItem("Use colour scalar file");
  threshold_file_combobox->addItem("Separate scalar file");
  connect(threshold_file_combobox, SIGNAL(activated(int)), this, SLOT(threshold_scalar_file_slot(int)));
  vlayout->addWidget(threshold_file_combobox);

  hlayout = new Tool::Base::HBoxLayout;
  hlayout->setContentsMargins(0, 0, 0, 0);
  hlayout->setSpacing(0);

  threshold_lower_box = new QCheckBox(this);
  connect(threshold_lower_box, SIGNAL(stateChanged(int)), this, SLOT(threshold_lower_changed(int)));
  hlayout->addWidget(threshold_lower_box);
  threshold_lower = new AdjustButton(this, 0.1);
  connect(threshold_lower, SIGNAL(valueChanged()), this, SLOT(threshold_lower_value_changed()));
  hlayout->addWidget(threshold_lower);

  threshold_upper_box = new QCheckBox(this);
  hlayout->addWidget(threshold_upper_box);
  threshold_upper = new AdjustButton(this, 0.1);
  connect(threshold_upper_box, SIGNAL(stateChanged(int)), this, SLOT(threshold_upper_changed(int)));
  connect(threshold_upper, SIGNAL(valueChanged()), this, SLOT(threshold_upper_value_changed()));
  hlayout->addWidget(threshold_upper);

  vlayout->addLayout(hlayout);

  main_box->addWidget(threshold_box);

  update_UI();
}

void TrackScalarFileOptions::set_tractogram(Tractogram *selected_tractogram) { tractogram = selected_tractogram; }

void TrackScalarFileOptions::render_tractogram_colourbar(const Tractogram &tractogram) {

  float min_value =
      (tractogram.get_threshold_type() == TrackThresholdType::UseColourFile && tractogram.use_discard_lower())
          ? tractogram.scaling_min_thresholded()
          : tractogram.scaling_min();

  float max_value =
      (tractogram.get_threshold_type() == TrackThresholdType::UseColourFile && tractogram.use_discard_upper())
          ? tractogram.scaling_max_thresholded()
          : tractogram.scaling_max();

  window().colourbar_renderer.render(
      tractogram.colourmap,
      tractogram.scale_inverted(),
      min_value,
      max_value,
      tractogram.scaling_min(),
      tractogram.display_range,
      {tractogram.colour[0] / 255.0f, tractogram.colour[1] / 255.0f, tractogram.colour[2] / 255.0f});
}

void TrackScalarFileOptions::update_UI() {

  if (!tractogram) {
    setVisible(false);
    return;
  }
  setVisible(true);

  if (tractogram->get_color_type() == TrackColourType::ScalarFile) {

    colour_groupbox->setVisible(true);
    min_entry->setRate(tractogram->scaling_rate());
    max_entry->setRate(tractogram->scaling_rate());
    min_entry->setValue(tractogram->scaling_min());
    max_entry->setValue(tractogram->scaling_max());

    colourmap_button->setEnabled(true);
    colourmap_button->set_colourmap_index(tractogram->colourmap);
    colourmap_button->set_scale_inverted(tractogram->scale_inverted());
    colourmap_button->set_show_colourbar(tractogram->show_colour_bar);

    assert(tractogram->intensity_scalar_filename.length());
    intensity_file_button->setText(qstr(shorten(Path::basename(tractogram->intensity_scalar_filename), 35, 0)));
    intensity_file_button->setToolTip(qstr(tractogram->intensity_scalar_filename));

  } else {
    colour_groupbox->setVisible(false);
    intensity_file_button->setToolTip(tr("Open (track) scalar file for colouring streamlines"));
  }

  threshold_file_combobox->removeItem(3);
  threshold_file_combobox->blockSignals(true);
  threshold_file_combobox->setToolTip(QString());
  switch (tractogram->get_threshold_type()) {
  case TrackThresholdType::None:
    threshold_file_combobox->setCurrentIndex(0);
    break;
  case TrackThresholdType::UseColourFile:
    threshold_file_combobox->setCurrentIndex(1);
    break;
  case TrackThresholdType::SeparateFile:
    assert(tractogram->threshold_scalar_filename.length());
    threshold_file_combobox->addItem(qstr(shorten(Path::basename(tractogram->threshold_scalar_filename), 35, 0)));
    threshold_file_combobox->setToolTip(qstr(tractogram->threshold_scalar_filename));
    threshold_file_combobox->setCurrentIndex(3);
    break;
  }
  threshold_file_combobox->blockSignals(false);

  const bool show_threshold_controls = (tractogram->get_threshold_type() != TrackThresholdType::None);
  threshold_lower_box->setVisible(show_threshold_controls);
  threshold_lower->setVisible(show_threshold_controls);
  threshold_upper_box->setVisible(show_threshold_controls);
  threshold_upper->setVisible(show_threshold_controls);

  if (show_threshold_controls) {
    threshold_lower_box->setChecked(tractogram->use_discard_lower());
    threshold_lower->setEnabled(tractogram->use_discard_lower());
    threshold_upper_box->setChecked(tractogram->use_discard_upper());
    threshold_upper->setEnabled(tractogram->use_discard_upper());
    threshold_lower->setRate(tractogram->get_threshold_rate());
    threshold_lower->setValue(tractogram->lessthan);
    threshold_upper->setRate(tractogram->get_threshold_rate());
    threshold_upper->setValue(tractogram->greaterthan);
  }
}

bool TrackScalarFileOptions::open_intensity_track_scalar_file_slot() {
  std::string scalar_file = Dialog::File::get_file(
      this, "Select scalar text file or Track Scalar file (.tsf) to open", "", &tool->current_folder);
  return open_intensity_track_scalar_file_slot(scalar_file);
}

bool TrackScalarFileOptions::open_intensity_track_scalar_file_slot(std::string scalar_file) {
  if (!scalar_file.empty()) {
    try {
      tractogram->load_intensity_track_scalars(scalar_file);
      tractogram->set_color_type(TrackColourType::ScalarFile);
    } catch (Exception &E) {
      E.display();
      scalar_file.clear();
    }
  }
  update_UI();
  window().updateGL();
  return !scalar_file.empty();
}

void TrackScalarFileOptions::toggle_show_colour_bar(bool show_colour_bar, const ColourMapButton &) {
  if (tractogram) {
    tractogram->show_colour_bar = show_colour_bar;
    window().updateGL();
  }
}

void TrackScalarFileOptions::selected_colourmap(size_t cmap, const ColourMapButton &) {
  if (tractogram) {
    tractogram->colourmap = cmap;
    window().updateGL();
  }
}

void TrackScalarFileOptions::selected_custom_colour(const QColor &c, const ColourMapButton &) {
  if (tractogram) {
    tractogram->set_colour(c);
    window().updateGL();
  }
}

void TrackScalarFileOptions::set_threshold(GUI::MRView::Tool::TrackThresholdType dataSource,
                                           default_type min,
                                           default_type max) // TrackThresholdType dataSource
{
  if (tractogram) {
    // Source
    tractogram->set_threshold_type(dataSource);
    // Range
    if (dataSource != TrackThresholdType::None) {
      tractogram->lessthan = min;
      tractogram->greaterthan = max;
      threshold_lower_box->setChecked(true);
      threshold_upper_box->setChecked(true);
    }

    update_UI();
    window().updateGL();
  }
}

void TrackScalarFileOptions::set_scaling(default_type min, default_type max) {
  if (tractogram) {
    tractogram->set_windowing(min, max);
    update_UI();
    window().updateGL();
  }
}

void TrackScalarFileOptions::on_set_scaling_slot() {
  if (tractogram) {
    tractogram->set_windowing(min_entry->value(), max_entry->value());
    window().updateGL();
  }
}

bool TrackScalarFileOptions::threshold_scalar_file_slot(int /*unused*/) {

  std::string file_path;
  switch (threshold_file_combobox->currentIndex()) {
  case 0:
    tractogram->set_threshold_type(TrackThresholdType::None);
    tractogram->erase_threshold_scalar_data();
    tractogram->set_use_discard_lower(false);
    tractogram->set_use_discard_upper(false);
    break;
  case 1:
    if (tractogram->get_color_type() == TrackColourType::ScalarFile) {
      tractogram->set_threshold_type(TrackThresholdType::UseColourFile);
      tractogram->erase_threshold_scalar_data();
    } else {
      QMessageBox::warning(
          QApplication::activeWindow(),
          tr("Tractogram threshold error"),
          tr("Can only threshold based on scalar file used for streamline colouring if that colour mode is active"),
          QMessageBox::Ok,
          QMessageBox::Ok);
      threshold_file_combobox->blockSignals(true);
      switch (tractogram->get_threshold_type()) {
      case TrackThresholdType::None:
        threshold_file_combobox->setCurrentIndex(0);
        break;
      case TrackThresholdType::UseColourFile:
        assert(0);
      case TrackThresholdType::SeparateFile:
        threshold_file_combobox->setCurrentIndex(3);
        break;
      }
      threshold_file_combobox->blockSignals(false);
      return false;
    }
    break;
  case 2:
    file_path = Dialog::File::get_file(
        this, "Select scalar text file or Track Scalar file (.tsf) to open", "", &tool->current_folder);
    if (!file_path.empty()) {
      try {
        tractogram->load_threshold_track_scalars(file_path);
        tractogram->set_threshold_type(TrackThresholdType::SeparateFile);
      } catch (Exception &E) {
        E.display();
        file_path.clear();
      }
    }
    if (file_path.empty()) {
      threshold_file_combobox->blockSignals(true);
      switch (tractogram->get_threshold_type()) {
      case TrackThresholdType::None:
        threshold_file_combobox->setCurrentIndex(0);
        break;
      case TrackThresholdType::UseColourFile:
        threshold_file_combobox->setCurrentIndex(1);
        break;
      case TrackThresholdType::SeparateFile:
        // Should still be an entry in the combobox corresponding to the old file
        threshold_file_combobox->setCurrentIndex(3);
        break;
      }
      threshold_file_combobox->blockSignals(false);
      return false;
    }
    break;
  case 3: // Re-selected the same file as used previously; do nothing
    assert(tractogram->get_threshold_type() == TrackThresholdType::SeparateFile);
    break;
  default:
    assert(0);
  }
  update_UI();
  window().updateGL();
  return true;
}

void TrackScalarFileOptions::threshold_lower_changed(int) {
  if (tractogram) {
    threshold_lower->setEnabled(threshold_lower_box->isChecked());
    tractogram->set_use_discard_lower(threshold_lower_box->isChecked());
    window().updateGL();
  }
}

void TrackScalarFileOptions::threshold_upper_changed(int) {
  if (tractogram) {
    threshold_upper->setEnabled(threshold_upper_box->isChecked());
    tractogram->set_use_discard_upper(threshold_upper_box->isChecked());
    window().updateGL();
  }
}

void TrackScalarFileOptions::threshold_lower_value_changed() {
  if (tractogram && threshold_lower_box->isChecked()) {
    tractogram->lessthan = threshold_lower->value();
    window().updateGL();
  }
}

void TrackScalarFileOptions::threshold_upper_value_changed() {
  if (tractogram && threshold_upper_box->isChecked()) {
    tractogram->greaterthan = threshold_upper->value();
    window().updateGL();
  }
}

void TrackScalarFileOptions::reset_colourmap(const ColourMapButton &) {
  if (tractogram) {
    tractogram->reset_windowing();
    update_UI();
    window().updateGL();
  }
}

void TrackScalarFileOptions::toggle_invert_colourmap(bool invert, const ColourMapButton &) {
  if (tractogram) {
    tractogram->set_invert_scale(invert);
    window().updateGL();
  }
}

} // namespace MR::GUI::MRView::Tool
