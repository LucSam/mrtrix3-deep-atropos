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

#include "algo/loop.h"
#include "app.h"
#include "fixel/fixel.h"
#include "formats/mrtrix_utils.h"
#include "image.h"
#include "image_diff.h"
#include "image_helpers.h"

namespace MR {
class InvalidFixelDirectoryException : public Exception {
public:
  InvalidFixelDirectoryException(const std::string &msg) : Exception(msg) {}
  InvalidFixelDirectoryException(const Exception &previous_exception, const std::string &msg)
      : Exception(previous_exception, msg) {}
};

namespace Peaks {
FORCE_INLINE void check(const Header &in) {
  if (!in.datatype().is_floating_point())
    throw Exception("Image \"" + in.name() + "\" is not a valid peaks image: Does not contain floating-point data");
  try {
    check_effective_dimensionality(in, 4);
  } catch (Exception &e) {
    throw Exception(e, "Image \"" + in.name() + "\" is not a valid peaks image: Expect 4 dimensions");
  }
  if (in.size(3) % 3)
    throw Exception("Image \"" + in.name() +
                    "\" is not a valid peaks image: Number of volumes must be a multiple of 3");
}
} // namespace Peaks

namespace Fixel {
FORCE_INLINE bool is_index_filename(const std::string &path) {
  for (std::initializer_list<const std::string>::iterator it = supported_sparse_formats.begin();
       it != supported_sparse_formats.end();
       ++it) {
    if (Path::basename(path) == "index" + *it)
      return true;
  }
  return false;
}

template <class HeaderType> FORCE_INLINE bool is_index_image(const HeaderType &in) {
  return is_index_filename(in.name()) && in.ndim() == 4 && in.size(3) == 2;
}

template <class HeaderType> FORCE_INLINE void check_index_image(const HeaderType &index) {
  if (!is_index_image(index))
    throw InvalidImageException(
        index.name() + " is not a valid fixel index image. Image must be 4D with 2 volumes in the 4th dimension");
}

template <class HeaderType> FORCE_INLINE bool is_data_file(const HeaderType &in) {
  return in.ndim() == 3 && in.size(2) == 1;
}

FORCE_INLINE bool is_directions_filename(const std::string &path) {
  for (std::initializer_list<const std::string>::iterator it = supported_sparse_formats.begin();
       it != supported_sparse_formats.end();
       ++it) {
    if (Path::basename(path) == "directions" + *it)
      return true;
  }
  return false;
}

template <class HeaderType> FORCE_INLINE bool is_directions_file(const HeaderType &in) {
  return is_directions_filename(in.name()) && in.ndim() == 3 && in.size(1) == 3 && in.size(2) == 1;
}

template <class HeaderType> FORCE_INLINE void check_data_file(const HeaderType &in) {
  if (!is_data_file(in))
    throw InvalidImageException(in.name() +
                                " is not a valid fixel data file. Expected a 3-dimensional image of size n x m x 1");
}

FORCE_INLINE std::string get_fixel_directory(const std::string &fixel_file) {
  std::string fixel_directory = Path::dirname(fixel_file);
  // assume the user is running the command from within the fixel directory
  if (fixel_directory.empty())
    fixel_directory = Path::cwd();
  return fixel_directory;
}

template <class IndexHeaderType> FORCE_INLINE index_type get_number_of_fixels(IndexHeaderType &index_header) {
  check_index_image(index_header);
  if (index_header.keyval().count(n_fixels_key)) {
    return std::stoul(index_header.keyval().at(n_fixels_key));
  } else {
    auto index_image = Image<index_type>::open(index_header.name());
    index_image.index(3) = 1;
    index_type num_fixels = 0;
    index_type max_offset = 0;
    for (auto i = MR::Loop(index_image, 0, 3)(index_image); i; ++i) {
      if (index_image.value() > max_offset) {
        max_offset = index_image.value();
        index_image.index(3) = 0;
        num_fixels = index_image.value();
        index_image.index(3) = 1;
      }
    }
    return (max_offset + num_fixels);
  }
}

template <class IndexHeaderType, class DataHeaderType>
FORCE_INLINE bool fixels_match(const IndexHeaderType &index_header, const DataHeaderType &data_header) {
  bool fixels_match(false);

  if (is_index_image(index_header)) {
    if (index_header.keyval().count(n_fixels_key)) {
      fixels_match = std::stoul(index_header.keyval().at(n_fixels_key)) == (index_type)data_header.size(0);
    } else {
      auto index_image = Image<index_type>::open(index_header.name());
      index_image.index(3) = 1;
      index_type num_fixels = 0;
      index_type max_offset = 0;
      for (auto i = MR::Loop(index_image, 0, 3)(index_image); i; ++i) {
        if (index_image.value() > max_offset) {
          max_offset = index_image.value();
          index_image.index(3) = 0;
          num_fixels = index_image.value();
          index_image.index(3) = 1;
        }
      }
      fixels_match = (max_offset + num_fixels) == (index_type)data_header.size(0);
    }
  }

  return fixels_match;
}

FORCE_INLINE void check_fixel_size(const Header &index_h, const Header &data_h) {
  check_index_image(index_h);
  check_data_file(data_h);

  if (!fixels_match(index_h, data_h))
    throw InvalidImageException("Fixel number mismatch between index image " + index_h.name() + " and data image " +
                                data_h.name());
}

FORCE_INLINE void
check_fixel_directory(const std::string &path, bool create_if_missing = false, bool check_if_empty = false) {
  std::string path_temp = path;
  // handle the use case when a fixel command is run from inside a fixel directory
  if (path.empty())
    path_temp = Path::cwd();

  bool exists(true);

  if (!(exists = Path::exists(path_temp))) {
    if (create_if_missing)
      File::mkdir(path_temp);
    else
      throw Exception("Fixel directory (" + str(path_temp) + ") does not exist");
  } else if (!Path::is_dir(path_temp))
    throw Exception(str(path_temp) + " is not a directory");

  if (check_if_empty && !Path::Dir(path_temp).read_name().empty())
    throw Exception("Output fixel directory \"" + path_temp + "\" is not empty" +
                    (App::overwrite_files
                         ? " (-force option cannot safely be applied on directories; please erase manually instead)"
                         : ""));
}

FORCE_INLINE Header find_index_header(const std::string &fixel_directory_path) {
  Header header;
  check_fixel_directory(fixel_directory_path);

  for (std::initializer_list<const std::string>::iterator it = supported_sparse_formats.begin();
       it != supported_sparse_formats.end();
       ++it) {
    std::string full_path = Path::join(fixel_directory_path, "index" + *it);
    if (Path::exists(full_path)) {
      if (header.valid())
        throw InvalidFixelDirectoryException("Multiple index images found in directory " + fixel_directory_path);
      header = Header::open(full_path);
    }
  }
  if (!header.valid())
    throw InvalidFixelDirectoryException("Could not find index image in directory " + fixel_directory_path);

  check_index_image(header);
  return header;
}

FORCE_INLINE std::vector<Header> find_data_headers(const std::string &fixel_directory_path,
                                                   const Header &index_header,
                                                   const bool include_directions = false) {
  check_index_image(index_header);
  auto dir_walker = Path::Dir(fixel_directory_path);
  std::vector<std::string> file_names;
  {
    std::string temp;
    while (!(temp = dir_walker.read_name()).empty())
      file_names.push_back(temp);
  }
  std::sort(file_names.begin(), file_names.end());

  std::vector<Header> data_headers;
  for (auto fname : file_names) {
    if (Path::has_suffix(fname, supported_sparse_formats)) {
      try {
        auto H = Header::open(Path::join(fixel_directory_path, fname));
        if (is_data_file(H)) {
          if (fixels_match(index_header, H)) {
            if (!is_directions_file(H) || include_directions)
              data_headers.emplace_back(std::move(H));
          } else {
            WARN("fixel data file (" + fname +
                 ") does not contain the same number of elements as fixels in the index file");
          }
        }
      } catch (...) {
        WARN("unable to open file \"" + fname + "\" as potential fixel data file");
      }
    }
  }

  return data_headers;
}

FORCE_INLINE Header find_directions_header(const std::string fixel_directory_path) {
  bool directions_found(false);
  Header header;
  check_fixel_directory(fixel_directory_path);
  Header index_header = Fixel::find_index_header(fixel_directory_path);

  auto dir_walker = Path::Dir(fixel_directory_path);
  std::string fname;
  while (!(fname = dir_walker.read_name()).empty()) {
    if (is_directions_filename(fname)) {
      Header tmp_header = Header::open(Path::join(fixel_directory_path, fname));
      if (is_directions_file(tmp_header)) {
        if (fixels_match(index_header, tmp_header)) {
          if (directions_found == true)
            throw Exception("multiple directions files found in fixel image directory: " + fixel_directory_path);
          directions_found = true;
          header = std::move(tmp_header);
        } else {
          WARN("fixel directions file (" + fname +
               ") does not contain the same number of elements as fixels in the index file");
        }
      }
    }
  }

  if (!directions_found)
    throw InvalidFixelDirectoryException("Could not find directions image in directory " + fixel_directory_path);

  return header;
}

//! Generate a header for a sparse data file (Nx1x1)
FORCE_INLINE Header data_header_from_nfixels(const size_t nfixels) {
  Header header;
  header.ndim() = 3;
  header.size(0) = nfixels;
  header.size(1) = 1;
  header.size(2) = 1;
  header.spacing(0) = header.spacing(1) = header.spacing(2) = 1.0;
  header.stride(0) = 1;
  header.stride(1) = 2;
  header.stride(2) = 3;
  header.spacing(0) = header.spacing(1) = header.spacing(2) = 1.0;
  header.transform().setIdentity();
  header.datatype() = DataType::native(DataType::Float32);
  return header;
}

//! Generate a header for a sparse data file (Nx1x1) using an index image as a template
template <class IndexHeaderType> FORCE_INLINE Header data_header_from_index(IndexHeaderType &index) {
  Header header(data_header_from_nfixels(get_number_of_fixels(index)));
  for (size_t axis = 0; axis != 3; ++axis)
    header.spacing(axis) = index.spacing(axis);
  header.keyval() = index.keyval();
  return header;
}

//! Generate a header for a fixel directions data file (Nx3x1) based on knowledge of the number of fixels
FORCE_INLINE Header directions_header_from_nfixels(const size_t nfixels) {
  Header header = data_header_from_nfixels(nfixels);
  header.size(1) = 3;
  header.stride(0) = 2;
  header.stride(1) = 1;
  return header;
}

//! Generate a header for a fixel directions data file (Nx3x1) using an index image as a template
template <class IndexHeaderType> FORCE_INLINE Header directions_header_from_index(IndexHeaderType &index) {
  Header header = data_header_from_index(index);
  for (size_t axis = 0; axis != 3; ++axis)
    header.spacing(axis) = index.spacing(axis);
  header.size(1) = 3;
  header.stride(0) = 2;
  header.stride(1) = 1;
  return header;
}

//! Copy a file from one fixel directory into another.
FORCE_INLINE void copy_fixel_file(const std::string &input_file_path, const std::string &output_directory) {
  check_fixel_directory(output_directory, true);
  std::string output_path = Path::join(output_directory, Path::basename(input_file_path));
  Header input_header = Header::open(input_file_path);
  auto input_image = input_header.get_image<float>();
  auto output_image = Image<float>::create(output_path, input_header);
  threaded_copy(input_image, output_image);
}

//! Copy the index file from one fixel directory into another
FORCE_INLINE void copy_index_file(const std::string &input_directory, const std::string &output_directory) {
  Header input_header = Fixel::find_index_header(input_directory);
  check_fixel_directory(output_directory, true);

  std::string output_path = Path::join(output_directory, Path::basename(input_header.name()));

  // If the index file already exists check it is the same as the input index file
  if (Path::exists(output_path)) {
    auto input_image = input_header.get_image<index_type>();
    auto output_image = Image<index_type>::open(output_path);
    if (!images_match_abs(input_image, output_image))
      throw Exception("output fixel directory \"" + output_directory + "\" already contains index file, " +
                      "which is not the same as the expected output" +
                      (App::overwrite_files
                           ? " (-force option cannot safely be applied on directories; please erase manually instead)"
                           : ""));
  } else {
    auto output_image =
        Image<index_type>::create(Path::join(output_directory, Path::basename(input_header.name())), input_header);
    auto input_image = input_header.get_image<index_type>();
    threaded_copy(input_image, output_image);
  }
}

//! Copy the directions file from one fixel directory into another.
FORCE_INLINE void copy_directions_file(const std::string &input_directory, const std::string &output_directory) {
  Header input_header = Fixel::find_directions_header(input_directory);
  std::string output_path = Path::join(output_directory, Path::basename(input_header.name()));

  // If the directions file already exists check it is the same as the input directions file
  if (Path::exists(output_path)) {
    auto input_image = input_header.get_image<float>();
    auto output_image = Image<float>::open(output_path);
    if (!images_match_abs(input_image, output_image))
      throw Exception("output fixel directory \"" + output_directory + "\" already contains directions file, " +
                      "which is not the same as the expected output" +
                      (App::overwrite_files
                           ? " (-force option cannot safely be applied on directories; please erase manually instead)"
                           : ""));
  } else {
    auto output_image =
        Image<float>::create(Path::join(output_directory, Path::basename(input_header.name())), input_header);
    auto input_image = input_header.get_image<float>();
    threaded_copy(input_image, output_image);
  }
}

FORCE_INLINE void copy_index_and_directions_file(const std::string &input_directory,
                                                 const std::string &output_directory) {
  copy_index_file(input_directory, output_directory);
  copy_directions_file(input_directory, output_directory);
}

//! Copy all data files in a fixel directory into another directory. Data files do not include the index or directions
//! file.
FORCE_INLINE void copy_all_data_files(const std::string &input_directory, const std::string &output_directory) {
  for (auto &input_header : Fixel::find_data_headers(input_directory, Fixel::find_index_header(input_directory)))
    copy_fixel_file(input_header.name(), output_directory);
}

//! open a data file. checks that a user has not input a fixel directory or index image
template <class ValueType> Image<ValueType> open_fixel_data_file(const std::string &input_file) {
  if (Path::is_dir(input_file))
    throw Exception("please input the specific fixel data file to be converted (not the fixel directory)");

  Header in_data_header = Header::open(input_file);
  Fixel::check_data_file(in_data_header);
  auto in_data_image = in_data_header.get_image<ValueType>();

  Header in_index_header = Fixel::find_index_header(Fixel::get_fixel_directory(input_file));
  if (input_file == in_index_header.name())
    throw Exception("input fixel data file cannot be the index file");

  return in_data_image;
}
} // namespace Fixel
} // namespace MR
