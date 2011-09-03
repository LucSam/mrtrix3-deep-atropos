/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef __mrtrix_exception_h__
#define __mrtrix_exception_h__

#include <cerrno>
#include <string>
#include <vector>

#include "types.h"

namespace MR
{

  extern void (*print) (const std::string& msg);
  extern void (*error) (const std::string& msg);
  extern void (*info) (const std::string& msg);
  extern void (*debug) (const std::string& msg);

  class Exception
  {
    public:
      Exception (const std::string& msg) {
        description.push_back (msg);
      }
      Exception (const Exception& previous_exception, const std::string& msg) :
        description (previous_exception.description) {
        description.push_back (msg);
      }

      void display (int log_level = 1) const {
        display_func (*this, log_level);
      }

      size_t num () const {
        return (description.size());
      }
      const std::string& operator[] (size_t n) const {
        return (description[n]);
      }

      static void (*display_func) (const Exception& E, int log_level);

      std::vector<std::string> description;
  };


  void display_exception_cmdline (const Exception& E, int log_level);
  void cmdline_print (const std::string& msg);
  void cmdline_error (const std::string& msg);
  void cmdline_info (const std::string& msg);
  void cmdline_debug (const std::string& msg);
}

#endif

