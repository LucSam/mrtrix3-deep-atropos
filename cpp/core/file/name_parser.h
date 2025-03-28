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

#include "file/path.h"
#include "memory.h"
#include "mrtrix.h"

namespace MR::File {

//! a class to interpret numbered filenames
class NameParser {
public:
  class Item {
  public:
    Item() : seq_length(0) {}

    void set_str(const std::string &s) {
      clear();
      str = s;
    }

    void set_seq(const std::string &s) {
      clear();
      if (!s.empty())
        seq = parse_ints<uint32_t>(s);
      seq_length = 1;
    }

    void clear() {
      str.clear();
      seq.clear();
      seq_length = 0;
    }

    std::string string() const { return (str); }

    const std::vector<uint32_t> &sequence() const { return (seq); }

    std::vector<uint32_t> &sequence() { return (seq); }

    bool is_string() const { return (seq_length == 0); }

    bool is_sequence() const { return (seq_length != 0); }

    size_t size() const { return (seq_length ? seq_length : str.size()); }

    void calc_padding(size_t maxval = 0);

    friend std::ostream &operator<<(std::ostream &stream, const Item &item);

  protected:
    size_t seq_length;
    std::string str;
    std::vector<uint32_t> seq;
  };

  void parse(const std::string &imagename, size_t max_num_sequences = std::numeric_limits<size_t>::max());

  size_t num() const { return (array.size()); }

  std::string spec() const { return (specification); }

  const Item &operator[](size_t i) const { return (array[i]); }

  const std::vector<uint32_t> &sequence(size_t index) const { return (array[seq_index[index]].sequence()); }

  size_t ndim() const { return (seq_index.size()); }

  size_t index_of_sequence(size_t number = 0) const { return (seq_index[number]); }

  bool match(const std::string &file_name, std::vector<uint32_t> &indices) const;
  void calculate_padding(const std::vector<uint32_t> &maxvals);
  std::string name(const std::vector<uint32_t> &indices);
  std::string get_next_match(std::vector<uint32_t> &indices, bool return_seq_index = false);

  friend std::ostream &operator<<(std::ostream &stream, const NameParser &parser);

private:
  std::vector<Item> array;
  std::vector<size_t> seq_index;
  std::string folder_name, specification, current_name;
  std::unique_ptr<Path::Dir> folder;

  void insert_str(const std::string &str) {
    Item item;
    item.set_str(str);
    array.insert(array.begin(), item);
  }

  void insert_seq(const std::string &str) {
    Item item;
    item.set_seq(str);
    array.insert(array.begin(), item);
    seq_index.push_back(array.size() - 1);
  }
};

//! a class to hold a parsed image filename
class ParsedName {
public:
  ParsedName(const std::string &name, const std::vector<uint32_t> &index) : indices(index), filename(name) {}

  //! a class to hold a set of parsed image filenames
  class List {
  public:
    std::vector<uint32_t> parse_scan_check(const std::string &specifier,
                                           size_t max_num_sequences = std::numeric_limits<size_t>::max());

    void scan(NameParser &parser);

    std::vector<uint32_t> count() const;

    size_t biggest_filename_size() const { return max_name_size; }

    size_t size() const { return list.size(); }

    const ParsedName &operator[](size_t index) const { return *(list[index]); }

    friend std::ostream &operator<<(std::ostream &stream, const List &list);

  protected:
    std::vector<std::shared_ptr<ParsedName>> list;
    void count_dim(std::vector<uint32_t> &dim, size_t &current_entry, size_t current_dim) const;
    size_t max_name_size;
  };

  std::string name() const { return filename; }
  size_t ndim() const { return indices.size(); }
  uint32_t index(size_t num) const { return indices[num]; }

  bool operator<(const ParsedName &pn) const;
  friend std::ostream &operator<<(std::ostream &stream, const ParsedName &pin);

protected:
  std::vector<uint32_t> indices;
  std::string filename;
};

} // namespace MR::File
