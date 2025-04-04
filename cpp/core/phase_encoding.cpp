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

#include "phase_encoding.h"

namespace MR::PhaseEncoding {

// clang-format off
using namespace App;
const OptionGroup ImportOptions =
    OptionGroup("Options for importing phase-encode tables")
    + Option("import_pe_table", "import a phase-encoding table from file")
      + Argument("file").type_file_in()
    + Option("import_pe_eddy", "import phase-encoding information from an EDDY-style config / index file pair")
      + Argument("config").type_file_in()
      + Argument("indices").type_file_in();

const OptionGroup SelectOptions =
    OptionGroup("Options for selecting volumes based on phase-encoding")
    + Option("pe",
             "select volumes with a particular phase encoding;"
             " this can be three comma-separated values"
             " (for i,j,k components of vector direction)"
             " or four (direction & total readout time)")
      + Argument("desc").type_sequence_float();

const OptionGroup ExportOptions =
    OptionGroup("Options for exporting phase-encode tables")
    + Option("export_pe_table", "export phase-encoding table to file")
      + Argument("file").type_file_out()
    + Option("export_pe_eddy", "export phase-encoding information to an EDDY-style config / index file pair")
      + Argument("config").type_file_out()
      + Argument("indices").type_file_out();
// clang-format on

void clear_scheme(Header &header) {
  auto erase = [&](const std::string &s) {
    auto it = header.keyval().find(s);
    if (it != header.keyval().end())
      header.keyval().erase(it);
  };
  erase("pe_scheme");
  erase("PhaseEncodingDirection");
  erase("TotalReadoutTime");
}

Eigen::MatrixXd parse_scheme(const Header &header) {
  Eigen::MatrixXd PE;
  const auto it = header.keyval().find("pe_scheme");
  if (it != header.keyval().end()) {
    try {
      PE = MR::parse_matrix(it->second);
    } catch (Exception &e) {
      throw Exception(e, "malformed PE scheme in image \"" + header.name() + "\"");
    }
    if (ssize_t(PE.rows()) != ((header.ndim() > 3) ? header.size(3) : 1))
      throw Exception("malformed PE scheme in image \"" + header.name() + "\":" + //
                      " number of rows does not equal number of volumes");
  } else {
    const auto it_dir = header.keyval().find("PhaseEncodingDirection");
    if (it_dir != header.keyval().end()) {
      const auto it_time = header.keyval().find("TotalReadoutTime");
      const size_t cols = it_time == header.keyval().end() ? 3 : 4;
      Eigen::Matrix<default_type, Eigen::Dynamic, 1> row(cols);
      row.head(3) = Axes::id2dir(it_dir->second);
      if (it_time != header.keyval().end())
        row[3] = to<default_type>(it_time->second);
      PE.resize((header.ndim() > 3) ? header.size(3) : 1, cols);
      PE.rowwise() = row.transpose();
    }
  }
  return PE;
}

Eigen::MatrixXd get_scheme(const Header &header) {
  DEBUG("searching for suitable phase encoding data...");
  using namespace App;
  Eigen::MatrixXd result;

  try {
    const auto opt_table = get_options("import_pe_table");
    if (!opt_table.empty())
      result = load(opt_table[0][0], header);
    const auto opt_eddy = get_options("import_pe_eddy");
    if (!opt_eddy.empty()) {
      if (!opt_table.empty())
        throw Exception("Phase encoding table can be provided"
                        " using either -import_pe_table or -import_pe_eddy option,"
                        " but NOT both");
      result = load_eddy(opt_eddy[0][0], opt_eddy[0][1], header);
    }
    if (opt_table.empty() && opt_eddy.empty())
      result = parse_scheme(header);
  } catch (Exception &e) {
    throw Exception(e, "error importing phase encoding table for image \"" + header.name() + "\"");
  }

  if (!result.rows())
    return result;

  if (result.cols() < 3)
    throw Exception("unexpected phase encoding table matrix dimensions");

  INFO("found " + str(result.rows()) + "x" + str(result.cols()) + " phase encoding table");

  return result;
}

Eigen::MatrixXd eddy2scheme(const Eigen::MatrixXd &config, const Eigen::Array<int, Eigen::Dynamic, 1> &indices) {
  if (config.cols() != 4)
    throw Exception("Expected 4 columns in EDDY-format phase-encoding config file");
  Eigen::MatrixXd result(indices.size(), 4);
  for (ssize_t row = 0; row != indices.size(); ++row) {
    if (indices[row] > config.rows())
      throw Exception("Malformed EDDY-style phase-encoding information:"
                      " index exceeds number of config entries");
    result.row(row) = config.row(indices[row] - 1);
  }
  return result;
}

void export_commandline(const Header &header) {
  auto check = [&](const Eigen::MatrixXd &m) -> const Eigen::MatrixXd & {
    if (!m.rows())
      throw Exception("no phase-encoding information found within image \"" + header.name() + "\"");
    return m;
  };

  auto scheme = parse_scheme(header);

  auto opt = get_options("export_pe_table");
  if (!opt.empty())
    save(check(scheme), header, opt[0][0]);

  opt = get_options("export_pe_eddy");
  if (!opt.empty())
    save_eddy(check(scheme), header, opt[0][0], opt[0][1]);
}

} // namespace MR::PhaseEncoding
